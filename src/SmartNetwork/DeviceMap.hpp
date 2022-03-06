#pragma once

#include <string>
#include "DeviceCapabilities.hpp"
#include <unordered_map>
#include <memory>
#include <optional>
#include <iostream>
#include <utility>

using Device = unsigned;

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

private:
    struct DeviceData {
        bool active;
        std::string path;
        DeviceType deviceType;
        WorkMode workMode;
    };

    LocationTree locations;
    std::vector<DeviceData> devices;
};

