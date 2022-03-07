#pragma once

#include <string>
#include "DeviceCapabilities.hpp"
#include <unordered_map>
#include <memory>
#include <optional>
#include <iostream>
#include <utility>
#include <chrono>

using Device = unsigned;
using hclock = std::chrono::high_resolution_clock;
using time_point = std::chrono::high_resolution_clock::time_point;
using seconds = std::chrono::seconds;

struct ChangeInfo {
    time_point lastTransmitTime;
    unsigned maxTransmitCount;
    seconds maxTransmitTime;
};

struct HistoryInfo {
    unsigned maxTransmitCount;
    seconds maxTransmitTime;
};

class LocationTree {
public:
    explicit LocationTree(std::string_view base) : base(base) {}

    void addDevice(std::string_view location, Device newDevice);

    void removeDevice(std::string_view location);

    void find(std::string_view location, std::vector<Device> &res, bool match = true) {
        findImpl(location, res, "/", match);
    }

private:
    void addSubLocation(std::string_view location, Device newDevice);

    void findImpl(std::string_view location, std::vector<Device> &res, std::string_view root,
            bool match);

    std::string_view base;
    Device device = -1;
    std::vector<LocationTree> sub;
};

class DeviceMap {
public:
    DeviceMap() : locations("House") {}

    Device add(std::string_view location, DeviceType deviceType, WorkMode initMode);

    std::vector<Device> find(std::string_view location, bool match = false);

    void remove(Device device);

    std::string_view getPath(Device device) {
        return devices[device].path;
    }

    std::string_view getName(Device device);

    std::string_view getLocation(Device device);

    void setName(Device device, std::string_view name);

    void setLocation(Device device, std::string_view location);

    void setWorkMode(Device device, WorkMode workMode) {
        devices[device].workMode = workMode;
    }

    WorkMode getWorkMode(Device device) {
        return devices[device].workMode;
    }

    DeviceType deviceType(Device device) {
        return devices[device].deviceType;
    }

    void setHistoryInfo(Device receiver, std::optional<HistoryInfo> info) {
        devices[receiver].history = info;
    }

    void setChangeInfo(Device receiver, std::optional<ChangeInfo> info) {
        devices[receiver].change = info;
    }

    std::optional<HistoryInfo> getHistoryInfo(Device receiver)
    {
        return devices[receiver].history;
    }

    std::optional<ChangeInfo> getChangeInfo(Device receiver)
    {
        return devices[receiver].change;
    }

private:
    struct DeviceData {
        bool active;
        std::string path;
        DeviceType deviceType;
        WorkMode workMode;
        std::optional<ChangeInfo> change;
        std::optional<HistoryInfo> history;
    };

    LocationTree locations;
    std::vector<DeviceData> devices;
};

