#include "DeviceRelations.hpp"

void
DeviceRelations::link(Device transmitter, Indicator indicator, Device receiver,
        Parameter parameter) {

    impl::TransmitData transmitData = {transmitter, indicator, map->getWorkMode(transmitter)};
    impl::ReceiveData receiveData = {receiver, parameter, map->getWorkMode(receiver)};

    auto tit = storage.find(transmitData);
    impl::Storage* pS;
    if (tit == storage.end()) {
        auto type = capabilities->indicatorType(indicator);
        impl::Storage s;
        switch (type) {
            case DataType::Int:
                s = impl::TypeStorage<int>{};
                break;
            case DataType::Float:
                s = impl::TypeStorage<float>{};
                break;
            case DataType::Bool:
                s = impl::TypeStorage<bool>{};
                break;
        }

        tit = storage.insert({transmitData, {receiveData}}).first;
        tit->first.data = std::move(s);
    } else {
        if (std::find(tit->second.begin(), tit->second.end(), receiveData) == tit->second.end()) {
            tit->second.push_back(receiveData);
        } else
        {
            return;
        }
    }

    pS = &tit->first.data;

    auto rit = receiveDependencies.find(receiveData);
    if (rit == receiveDependencies.end()) {
        receiveDependencies.insert({receiveData, {pS}});
    } else {
        rit->second.push_back(pS);
    }
}

void DeviceRelations::unlink(Device transmitter, Indicator indicator,
        Device receiver, Parameter parameter) {
    impl::TransmitData transmitData = {transmitter, indicator, map->getWorkMode(transmitter)};
    impl::ReceiveData receiveData = {receiver, parameter, map->getWorkMode(receiver)};
    auto tit = storage.find(transmitData);
    auto rit = receiveDependencies.find(receiveData);

    if (tit != storage.end() && rit != receiveDependencies.end()) {
        for (int i = 0; i < tit->second.size(); ++i) {
            if (tit->second[i] == receiveData) {
                tit->second.erase(tit->second.begin() + i);

                rit->second.erase(std::find(rit->second.begin(), rit->second.end(), &tit->first.data));

                if (tit->second.empty()) {
                    storage.erase(tit);
                }
                return;
            }
        }
    }
}

void DeviceRelations::historyImpl(Device receiver, ChangeInfo changeInfo) {
    auto wm = map->getWorkMode(receiver);

    auto now = hclock::now();;

    for (Parameter p: capabilities->enumerateParameters(wm)) {
        impl::ReceiveData data{receiver, p, wm};

        auto it = receiveDependencies.find(data);

        if (it != receiveDependencies.end()) {
            impl::Storage s;
            std::visit([&s](auto && t) {s=std::decay_t<decltype(t)>();}, *it->second.front());

            // каждый параметр может зависеть от нескольких передающих устройств
            for (auto & i : it->second) {
                std::visit([&s, changeInfo, &now](auto const &data) {
                    using T = std::decay_t<decltype(data)>;
                    auto &paramStorage = std::get<T>(s);
                    T res(paramStorage.size() + data.size());

                    // фильтруем все показания, подходящие под запрашиваемое время
                    auto timestamp = data.rbegin();
                    for (; timestamp != data.rend(); ++timestamp) {
                        if (timestamp->time > changeInfo.lastTransmitTime &&
                                now - timestamp->time > changeInfo.maxTransmitTime) {
                            break;
                        }
                    }

                    // собираем показания от разных источников в один массив,
                    // сохраняя порядок сортировки по времени
                    std::merge(timestamp.base(), data.end(), paramStorage.begin(),
                            paramStorage.end(), res.begin(),
                            [](auto &&lhs, auto &&rhs) { return lhs.time < rhs.time; });

                    // отрезаем старые значения, если их больше, чем требует запрос
                    if (res.size() > changeInfo.maxTransmitCount) {
                        paramStorage.resize(changeInfo.maxTransmitCount);
                        std::copy(res.end() - changeInfo.maxTransmitCount, res.end(),
                                paramStorage.begin());
                    } else {
                        paramStorage = res;
                    }
                }, *i);
            }

            std::visit([this, receiver, p](auto const &data) {
                connection->send(receiver, p, data);
            }, s);
        }
    }
    changeInfo.lastTransmitTime = now;
    map->setChangeInfo(receiver, changeInfo);
}

void DeviceRelations::awake(Device receiver) {
    ChangeInfo changeInfo;
    if (map->getChangeInfo(receiver)) {
        changeInfo = *map->getChangeInfo(receiver);
    } else {
        changeInfo.maxTransmitCount = 1;
        changeInfo.lastTransmitTime = time_point::min();
        changeInfo.maxTransmitTime = seconds(100000);
    }
    historyImpl(receiver, changeInfo);
}

void DeviceRelations::power(Device receiver) {
    if (!map->getHistoryInfo(receiver)) {
        return;
    }
    auto historyInfo = *map->getHistoryInfo(receiver);

    ChangeInfo changeInfo{
            time_point::min(), historyInfo.maxTransmitCount,
            historyInfo.maxTransmitTime};
    historyImpl(receiver, changeInfo);
}
