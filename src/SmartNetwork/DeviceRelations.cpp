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


void DeviceRelations::awake(Device device, Parameter parameter) {
    map->setLastAwakeTime(device, parameter, hclock::now());
}
