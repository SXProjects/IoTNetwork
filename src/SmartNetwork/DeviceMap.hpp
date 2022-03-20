#pragma once

#include "Capabilities.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <iostream>
#include <utility>
#include <chrono>

using Device = unsigned;
using hclock = std::chrono::system_clock;
using time_point = hclock::time_point;
using seconds = std::chrono::seconds;


class LocationTree {
public:
    explicit LocationTree(std::string_view base) : base(base) {}

    void addDevice(std::string_view location, Device newDevice);

    void removeDevice(std::string_view location);

    void find(std::string_view location, std::vector<Device> &res, bool match = true) {
        findImpl(location, res, "/", match);
    }

    void listLocations(std::string_view location, std::vector<std::string_view> &res,
            bool match = true);

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
    DeviceMap(Capabilities *capabilities) : locations("House"), capabilities(capabilities) {}

    Device add(std::string_view location, DeviceType deviceType, WorkMode initMode);

    std::vector<Device> find(std::string_view location, bool match = false);

    Device removeDeviceType(DeviceType type);

    void remove(Device device);

    std::vector<std::string_view> listLocations(std::string_view location, bool match);

    std::string_view getPath(Device device) {
        return devices[device].path;
    }

    void setPath(Device device, std::string_view path);

    void setWorkMode(Device device, WorkMode workMode) {
        devices[device].workMode = workMode;
    }

    WorkMode getWorkMode(Device device) {
        return devices[device].workMode;
    }

    DeviceType deviceType(Device device) {
        return devices[device].deviceType;
    }

    void setLastAwakeTime(Device receiver, Parameter parameter, time_point time) {
        devices[receiver].lastAwake[parameter] = time;
    }

    time_point getLastAwakeTime(Device receiver, Parameter parameter) {
        return devices[receiver].lastAwake[parameter];
    }

    void setReceiveInstantly(Device receiver, bool val) {
        devices[receiver].instantly = val;
    }

    bool getReceiveInstantly(Device receiver) {
        return devices[receiver].instantly;
    }

private:
    struct DeviceData {
        bool instantly;
        std::string path;
        DeviceType deviceType;
        WorkMode workMode;
        std::vector<time_point> lastAwake;
        bool active;
    };

    LocationTree locations;
    std::vector<DeviceData> devices;
    Capabilities *capabilities;

    void listLocations(std::string_view location, std::vector<std::string_view> &res, bool match);
};

