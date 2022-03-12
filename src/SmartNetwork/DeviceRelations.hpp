#pragma once

#include "DeviceMap.hpp"
#include "Capabilities.hpp"
#include "ServerCommands.hpp"
#include <variant>
#include <algorithm>

namespace impl {
    template<typename T>
    using TypeStorage = std::vector<Timestamp<T>>;

    using Storage = std::variant<TypeStorage<int>, TypeStorage<float>, TypeStorage<bool>>;

    struct TransmitData {
        Device transmitter;
        Indicator indicator;
        WorkMode workMode;
        mutable impl::Storage data;

        friend bool operator==(const TransmitData &lhs, const TransmitData &rhs) {
            return lhs.transmitter == rhs.transmitter &&
                    lhs.indicator == rhs.indicator &&
                    lhs.workMode == rhs.workMode;
        }
    };

    struct ReceiveData {
        Device receiver;
        Parameter parameter;
        WorkMode workMode;

        friend bool operator==(const ReceiveData &lhs, const ReceiveData &rhs) {
            return lhs.receiver == rhs.receiver &&
                    lhs.parameter == rhs.parameter &&
                    lhs.workMode == rhs.workMode;
        }
    };
}


template<>
struct std::hash<impl::TransmitData> {
    std::size_t operator()(impl::TransmitData const &s) const noexcept {
        std::size_t h1 = std::hash<Device>{}(s.transmitter);
        std::size_t h2 = std::hash<Indicator>{}(s.indicator);
        std::size_t h3 = std::hash<WorkMode>{}(s.workMode);
        return (h1 ^ (h2 << 1)) ^ (h3 << 1);
    }
};

template<>
struct std::hash<impl::ReceiveData> {
    std::size_t operator()(impl::ReceiveData const &s) const noexcept {
        std::size_t h1 = std::hash<Device>{}(s.receiver);
        std::size_t h2 = std::hash<Parameter>{}(s.parameter);
        std::size_t h3 = std::hash<WorkMode>{}(s.workMode);
        return (h1 ^ (h2 << 1)) ^ (h3 << 1);
    }
};

enum class ApproxMode {
    Min,
    Max,
    First,
    Last,
    Average,
};

class DeviceRelations {
public:
    explicit DeviceRelations(DeviceMap *map, Capabilities *capabilities,
            ServerCommands *connection) :
            map(map), capabilities(capabilities), connection(connection) {}

    void link(Device transmitter, Indicator indicator, Device receiver, Parameter parameter);

    void unlink(Device transmitter, Indicator indicator, Device receiver, Parameter parameter);

    void parameterHistory(Device device, Parameter parameter, time_point from, time_point to,
            seconds discreteInterval, ApproxMode approxMode);

    void parameterHistory(Device device, Parameter parameter, time_point from,
            seconds discreteInterval, ApproxMode approxMode);

    void parameterHistory(Device device, Parameter parameter, time_point from, time_point to);

    void parameterHistory(Device device, Parameter parameter, time_point from);

    void indicatorHistory(Device device, Indicator indicator, time_point from, time_point to,
            seconds discreteInterval, ApproxMode approxMode);

    void indicatorHistory(Device device, Indicator indicator, time_point from,
            seconds discreteInterval, ApproxMode approxMode);

    void indicatorHistory(Device device, Indicator indicator, time_point from, time_point to);

    void indicatorHistory(Device device, Indicator indicator, time_point from);

    void changes(Device device, Parameter parameter,
            seconds discreteInterval, ApproxMode approxMode);

    void changes(Device device, Parameter parameter);

    void awake(Device device, Parameter parameter);

    template<typename T>
    void transmit(Device transmitter, Indicator indicator, T data) {
        impl::TransmitData transmitData = {transmitter, indicator, map->getWorkMode(transmitter)};

        auto it = storage.find(transmitData);

        if (it != storage.end()) {
            Timestamp<T> timestamp{data, hclock::now()};
            std::get<impl::TypeStorage<T>>(it->first.data).push_back(timestamp);

            // идём через все устройства, получающие команды немедленно
            for (auto r: it->second) {
                if (map->getReceiveInstantly(r.receiver) &&
                        map->getWorkMode(r.receiver) == r.workMode) {
                    connection->transmit(r.receiver, r.parameter, timestamp);
                }
            }
        }
    }

private:
    DeviceMap *map;
    Capabilities *capabilities;
    ServerCommands *connection;
    std::unordered_map<impl::TransmitData, std::vector<impl::ReceiveData>> storage;
    std::unordered_map<impl::ReceiveData, std::vector<impl::Storage *>> receiveDependencies;
};