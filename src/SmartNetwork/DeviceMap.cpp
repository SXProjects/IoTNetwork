#include "DeviceMap.hpp"

void LocationTree::addSubLocation(std::string_view location, Device newDevice) {
    // если устройство уже существовало, ничего не делаем
    for (auto &s: sub) {
        if (s.device == -1) {
            s.device = newDevice;
        }

        if (s.base == location) {
            return;
        }
    }

    // добавляем новое местоположение
    sub.emplace_back(location);
    sub.back().device = newDevice;
}

void LocationTree::findImpl(std::string_view location, std::vector<Device> &res,
        std::string_view root, bool match) {
    auto const pos = location.find('/');

    auto compare = [match](std::string_view b, std::string_view l) {
        if (match) {
            return b == l;
        } else {
            return (b.find(l) != std::string::npos);
        }
    };

    if (pos == std::string::npos) {
        // если мы нашли нужный файл или ищем любой файл
        if (compare(base, location) || (location == "*" && root == "*")) {
            if (device != -1) {
                res.push_back(device);
            }
        }
    }

    // если корень был произвольным местоположением, то мы продолжаем итерироваться дальше,
    // не переходя в конкретное местоположение
    if (root != "*") {
        root = location.substr(0, pos);
        location = location.substr(pos + 1);
    }

    // итерируемся по вложенному местоположению
    if (root == "*") {
        for (auto it = sub.begin(); it != sub.end(); ++it) {
            it->findImpl(location, res, root, match);
        }
        return;
    }

    // Ищем конкретное местоположение
    if (match) {
        for (auto it = sub.begin(); it != sub.end(); ++it) {
            if (compare(it->base, root)) {
                it->findImpl(location, res, root, match);
                return;
            }
        }
    } else {
        for (auto it = sub.begin(); it != sub.end(); ++it) {
            if (compare(it->base, root)) {
                it->findImpl(location, res, root, match);
            }
        }
    }
}

void LocationTree::removeDevice(std::string_view location) {
    auto const pos = location.find('/');
    if (pos == std::string::npos) {
        for (auto it = sub.begin(); it != sub.end(); ++it) {
            if (it->base == location) {
                sub.erase(it);
                return;
            }
        }
    } else {
        std::string_view root = location.substr(0, pos);
        std::string_view path = location.substr(pos + 1);

        // Ищем конкретное местоположение
        for (auto &s: sub) {
            if (s.base == root) {
                return s.removeDevice(path);
            }
        }
    }
}

void LocationTree::addDevice(std::string_view location, Device newDevice) {
    auto const pos = location.find('/');
    if (pos == std::string::npos) {
        addSubLocation(location, newDevice);
    } else {
        std::string_view root = location.substr(0, pos);
        std::string_view path = location.substr(pos + 1);

        // Ищем конкретное местоположение
        for (auto &s: sub) {
            if (s.base == root) {
                s.addDevice(path, newDevice);
            }
        }

        // Если не находим, создаём поддиректорию
        addSubLocation(root, -1);
        sub.back().addDevice(path, newDevice);
    }
}

void DeviceMap::setLocation(Device device, std::string_view location) {
    locations.removeDevice(devices[device].path);
    auto last = std::copy(devices[device].path.begin() + devices[device].path.rfind('/'),
            devices[device].path.end(),
            devices[device].path.begin());
    devices[device].path.resize(last - devices[device].path.begin());
    devices[device].path = location.data() + devices[device].path;
    locations.addDevice(devices[device].path, device);
}

void DeviceMap::setName(Device device, std::string_view name) {
    locations.removeDevice(devices[device].path);
    devices[device].path.resize(devices[device].path.rfind('/'));
    devices[device].path += name;
    locations.addDevice(devices[device].path, device);
}

std::vector<Device> DeviceMap::find(std::string_view location, bool match) {
    std::vector<Device> result;
    locations.find(location, result, match);
    return result;
}

Device DeviceMap::add(std::string_view location, DeviceType deviceType, WorkMode initMode) {
    devices.push_back(DeviceData{
            true,
            location.data(),
            deviceType,
            initMode,
            std::vector<time_point>(
                    capabilities->enumerateParameters(deviceType).size(), time_point::min()),
            true,
    });
    locations.addDevice(devices.back().path, devices.size() - 1);
    return devices.size() - 1;
}

void DeviceMap::remove(Device device) {
    devices[device].active = false;
    locations.removeDevice(devices[device].path);
}

std::string_view DeviceMap::getName(Device device) {
    return std::string_view(devices[device].path).substr(devices[device].path.rfind('/') + 1);
}

std::string_view DeviceMap::getLocation(Device device) {
    return std::string_view(devices[device].path).substr(0, devices[device].path.rfind('/'));
}
