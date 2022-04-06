#pragma once

#include "DeviceMap.hpp"
#include <variant>
#include <algorithm>

template<typename T>
struct Timestamp {
    T val;
    time_point time;

    template<class Archive>
    void serialize(Archive &ar) {
        ar(val, time);
    }
};

enum class ApproxMode {
    Min,
    Max,
    First,
    Last,
    Average,
};

namespace impl {
    template<typename T>
    using TypeStorage = std::vector<Timestamp<T>>;

    using Storage = std::variant<TypeStorage<int>, TypeStorage<float>, TypeStorage<bool>>;

    struct TransmitData {
        Device transmitter;
        Indicator indicator;
        WorkMode workMode;
        mutable std::shared_ptr<impl::Storage> data;

        template<class Archive>
        void serialize(Archive &ar) {
            ar(transmitter, indicator, workMode, data);
        }

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

        template<class Archive>
        void serialize(Archive &ar) {
            ar(receiver, parameter, workMode);
        }

        friend bool operator==(const ReceiveData &lhs, const ReceiveData &rhs) {
            return lhs.receiver == rhs.receiver &&
                    lhs.parameter == rhs.parameter &&
                    lhs.workMode == rhs.workMode;
        }
    };

    auto const timeCmp = [](auto &&lhs, auto &&rhs) { return lhs.time < rhs.time; };

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

    template<typename T, typename R>
    void history(T &result, R const &data, time_point from, time_point to,
            seconds discreteInterval, ApproxMode approxMode) {
        if (to <= from) {
            throw std::runtime_error("'to' time must be greater than 'from' time");
        }

        // находим пограничные значения в заданном промежутке времени
        auto begin = std::lower_bound(data.begin(), data.end(),
                Timestamp<T>{T{}, from}, impl::timeCmp);
        auto end = std::upper_bound(data.begin(), data.end(),
                Timestamp<T>{T{}, to}, impl::timeCmp);

        // разбиваем промежуток времени на диапазоны, вычисляя приблизительное значение
        // на каждом из них, чтобы не передавать огромное количество данных просто так
        if (discreteInterval != seconds(0)) {
            auto lastBegin = begin;
            for (seconds t(0); (t <= from - to) && begin != end; ++begin) {
                if (begin->time - from > t + discreteInterval) {
                    t += discreteInterval;
                    result.push_back({impl::approximate(lastBegin, begin, approxMode), from + t});
                    lastBegin = begin;
                }
            }
        } else {
            std::copy(begin, end, std::back_inserter(result));
        }
    }


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

class Relations {
public:
    explicit Relations(DeviceMap *map, Capabilities *capabilities) :
            map(map), capabilities(capabilities) {}

    template<class Archive>
    void save(Archive &ar) const {
        ar(storage, receiveDependencies);
    }

    template<class Archive>
    void load(Archive &ar) {
        ar(storage, receiveDependencies);
    }

    void link(Device transmitter, Indicator indicator, Device receiver, Parameter parameter);

    void unlink(Device transmitter, Indicator indicator, Device receiver, Parameter parameter);

    template<typename F>
    void parameterHistory(F &&prepareHistory, Device receiver, Parameter parameter,
            time_point from, time_point to, seconds discreteInterval,
            ApproxMode approxMode) {
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
                impl::history(result, data, from, to, discreteInterval, approxMode);
            }
            std::sort(result.begin(), result.end(), impl::timeCmp);
            prepareHistory(capabilities->parameterName(parameter).data(), result);
        }, resultStorage);
    }

    template<typename F>
    void indicatorHistory(F &&prepareHistory, Device device, Indicator indicator, time_point from,
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
            impl::history(result, data, from, to, discreteInterval, approxMode);
            prepareHistory(capabilities->indicatorName(indicator).data(), result);
        }, *it->first.data);
    }

    template<typename F>
    void changes(F &&prepareHistory, Device device, Parameter parameter,
            seconds discreteInterval, ApproxMode approxMode) {
        parameterHistory(prepareHistory, device, parameter,
                map->getLastAwakeTime(device, parameter),
                hclock::now(), discreteInterval, approxMode);
    }

    void awake(Device device, Parameter parameter);

    template<typename F, typename T>
    void transmit(F &&transmit, Device transmitter, Indicator indicator, T data, time_point time) {
        impl::TransmitData transmitData = {transmitter, indicator, map->getWorkMode(transmitter)};

        auto it = storage.find(transmitData);

        if (it != storage.end()) {
            Timestamp<T> timestamp{data, time};
            std::get<impl::TypeStorage<T>>(*it->first.data).push_back(timestamp);

            // идём через все устройства, получающие команды немедленно
            for (auto r: it->second) {
                if (map->getReceiveInstantly(r.receiver) &&
                        map->getWorkMode(r.receiver) == r.workMode) {
                    transmit(r.receiver, r.parameter, timestamp);
                }
            }
        }
    }

private:
    DeviceMap *map;
    Capabilities *capabilities;
    std::unordered_map<impl::TransmitData, std::vector<impl::ReceiveData>> storage;
    std::unordered_map<impl::ReceiveData, std::vector<std::shared_ptr<impl::Storage>>> receiveDependencies;
};