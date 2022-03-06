#pragma once

#include "DeviceMap.hpp"

struct ReceiveData
{
    Device receiver;
    Parameter parameter;
};

class DeviceData {
public:
    void link(Device transmitter, Indicator indicator, Device receiver, Parameter parameter) {

    }

    void unlink(Device transmitter, Indicator indicator, Device receiver, Parameter parameter);

    std::vector<ReceiveData> dependencies(Device transmitter, Indicator indicator);

private:
    struct TransmitData {
        Device transmitter;
        Indicator indicator;
    };

    std::unordered_map<TransmitData, std::vector<ReceiveData>> data;
};
