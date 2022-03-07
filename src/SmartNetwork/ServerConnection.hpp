#pragma once

#include <iostream>
#include "DeviceMap.hpp"

template<typename T>
struct Timestamp {
    T val;
    time_point time;
};

class ServerConnection {
public:
    ServerConnection(DeviceMap *map, DeviceCapabilities *capabilities) :
            map(map), capabilities(capabilities), initTime(hclock::now()) {}

    template<typename T>
    void send(Device receiver, Parameter parameter, Timestamp<T> val) {
        std::cout << map->getName(receiver) << " " << capabilities->parameterName(parameter) << " "
                << std::chrono::duration_cast<seconds>(val.time - initTime).count() << " " << val.val << std::endl;
    }

    template<typename T>
    void send(Device receiver, Parameter parameter, std::vector<Timestamp<T>> const &data) {
        for (auto v: data) {
            send(receiver, parameter, v);
        }
    }

private:
    DeviceMap *map;
    DeviceCapabilities *capabilities;
    time_point initTime;
};

