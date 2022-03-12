#include "DeviceRelations.hpp"
#include <numeric>

void DeviceRelations::link(Device transmitter, Indicator indicator,
        Device receiver, Parameter parameter) {

    impl::TransmitData transmitData = {transmitter, indicator, map->getWorkMode(transmitter)};
    impl::ReceiveData receiveData = {receiver, parameter, map->getWorkMode(receiver)};

    auto tit = storage.find(transmitData);
    impl::Storage *pS;
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
        } else {
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

                rit->second.erase(
                        std::find(rit->second.begin(), rit->second.end(), &tit->first.data));

                if (tit->second.empty()) {
                    storage.erase(tit);
                }
                return;
            }
        }
    }
}

template<typename T>
auto approximate(T begin, T end, ApproxMode approxMode) {
    switch (approxMode) {
        case ApproxMode::Min:
            if constexpr(std::is_same_v<decltype(begin->val), bool>) {
                return std::find_if(begin, end, [](auto &&v) { return v.val == true; }) == end;
            } else {
                return std::min_element(begin, end,
                        [](auto &&lhs, auto &&rhs) { return lhs.val < rhs.val; })->val;
            }
        case ApproxMode::Max:
            if constexpr(std::is_same_v<decltype(begin->val), bool>) {
                return std::find_if(begin, end, [](auto &&v) { return v.val == true; }) != end;
            } else {
                return std::max_element(begin, end,
                        [](auto &&lhs, auto &&rhs) { return lhs.val < rhs.val; })->val;
            }
        case ApproxMode::First:
            return begin->val;
        case ApproxMode::Last:
            return (--end)->val;
        case ApproxMode::Average: {
            if constexpr(std::is_same_v<decltype(begin->val), bool>) {
                return std::find_if(begin, end, [](auto v) { return v.val == true; }) != end;
            } else {
                float size = end - begin;
                decltype(begin->val) res{};
                for (; begin != end; ++begin) {
                    res += begin->val;
                }
                return decltype(begin->val)(res / size);
            }
        }
        default:
            return decltype(begin->val){};
    }
}

auto timeCmp = [](auto &&lhs, auto &&rhs) { return lhs.time < rhs.time; };

template<typename T, typename R>
void history(T &result, R const &data, time_point from, time_point to,
        seconds discreteInterval, ApproxMode approxMode) {
    // находим пограничные значения в заданном промежутке времени
    auto begin = std::lower_bound(data.begin(), data.end(),
            Timestamp<T>{T{}, from}, timeCmp);
    auto end = std::upper_bound(data.begin(), data.end(),
            Timestamp<T>{T{}, to}, timeCmp);

    // разбиваем промежуток времени на диапазоны, вычисляя приблизительное значение
    // на каждом из них, чтобы не передавать огромное количество данных просто так
    if (discreteInterval != seconds(0)) {
        auto lastBegin = begin;
        for (seconds t(0); (t <= from - to) && begin != end; ++begin) {
            if (begin->time - from > t + discreteInterval) {
                t += discreteInterval;
                result.push_back({approximate(lastBegin, begin, approxMode), from + t});
                lastBegin = begin;
            }
        }
    } else {
        std::copy(begin, end, std::back_inserter(result));
    }
}

void DeviceRelations::parameterHistory(Device receiver, Parameter parameter, time_point from,
        time_point to,
        seconds discreteInterval, ApproxMode approxMode) {
    auto wm = map->getWorkMode(receiver);
    impl::ReceiveData receiveData{receiver, parameter, wm};
    auto it = receiveDependencies.find(receiveData);
    // если параметр не связан ни с какими показателями, пропускаем
    if (it == receiveDependencies.end() || it->second.empty()) {
        return;
    }

    impl::Storage resultStorage;
    std::visit([&resultStorage](auto const &s) {
        resultStorage = std::decay_t<decltype(s)>{};
    }, *it->second.front());

    std::visit([&](auto &result) {
        using T = std::decay_t<decltype(result)>;
        // каждый параметр может зависеть от нескольких передающих устройств
        for (auto &i: it->second) {
            auto &data = std::get<T>(*i);
            history(result, data, from, to, discreteInterval, approxMode);
        }
        std::sort(result.begin(), result.end(), timeCmp);
        connection->history(receiver, parameter, result);
    }, resultStorage);
}

void DeviceRelations::parameterHistory(Device device, Parameter parameter, time_point from,
        seconds discreteInterval, ApproxMode approxMode) {
    parameterHistory(device, parameter, from, hclock::now(), discreteInterval, approxMode);
}

void DeviceRelations::parameterHistory(Device device, Parameter parameter, time_point from,
        time_point to) {
    parameterHistory(device, parameter, from, to, seconds(0), ApproxMode::First);
}

void DeviceRelations::parameterHistory(Device device, Parameter parameter, time_point from) {
    parameterHistory(device, parameter, from, hclock::now());
}

void DeviceRelations::changes(Device device, Parameter parameter) {
    parameterHistory(device, parameter, map->getLastAwakeTime(device, parameter));
}

void DeviceRelations::changes(Device device, Parameter parameter, seconds discreteInterval,
        ApproxMode approxMode) {
    parameterHistory(device, parameter, map->getLastAwakeTime(device, parameter),
            discreteInterval, approxMode);
}

void DeviceRelations::awake(Device device, Parameter parameter) {
    map->setLastAwakeTime(device, parameter, hclock::now());
}

void DeviceRelations::indicatorHistory(Device device, Indicator indicator, time_point from,
        time_point to, seconds discreteInterval, ApproxMode approxMode) {
    auto wm = map->getWorkMode(device);
    impl::TransmitData transmitData{device, indicator, wm};
    auto it = storage.find(transmitData);
    if (it == storage.end()) {
        return;
    }

    std::visit([&](auto const &data) {
        using T = std::decay_t<decltype(data)>;
        T result{};
        history(result, data, from, to, discreteInterval, approxMode);
        connection->history(device, indicator, result);
    }, it->first.data);
}

void DeviceRelations::indicatorHistory(Device device, Parameter parameter, time_point from,
        seconds discreteInterval, ApproxMode approxMode) {
    indicatorHistory(device, parameter, from, hclock::now(), discreteInterval, approxMode);
}

void DeviceRelations::indicatorHistory(Device device, Parameter parameter,
        time_point from, time_point to) {
    parameterHistory(device, parameter, from, to, seconds(0), ApproxMode::First);
}

void DeviceRelations::indicatorHistory(Device device, Parameter parameter, time_point from) {
    parameterHistory(device, parameter, from, hclock::now());
}
