#include "ServerCommands.hpp"

Json ServerCommands::historyJson() {
    auto history = Json::array();
    for (auto &p: currentHistory) {
        p.second["time"] = timeAndDate(p.first);
        history.push_back(p.second);
    }
    return history;
}

std::string timeAndDate(hclock::time_point now) {
    auto in_time_t = hclock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

hclock::time_point timePoint(const std::string &date) {
    std::tm tm = {};
    std::stringstream ss(date);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return hclock::from_time_t(std::mktime(&tm));
}

DataType parseDataType(const std::string &data) {
    if (data == "int") {
        return DataType::Int;
    } else if (data == "float") {
        return DataType::Float;
    } else if (data == "bool") {
        return DataType::Bool;
    }
    throw std::runtime_error("invalid data type");
}

ApproxMode parseApprox(std::string const &mode) {
    if (mode == "max") {
        return ApproxMode::Max;
    } else if (mode == "min") {
        return ApproxMode::Min;
    } else if (mode == "first") {
        return ApproxMode::First;
    } else if (mode == "last") {
        return ApproxMode::Last;
    } else if (mode == "avg") {
        return ApproxMode::Average;
    }
    throw std::runtime_error("invalid approximation mode");
}

seconds parseInterval(std::string const &mode) {
    if (mode == "hours") {
        return seconds(3600);
    } else if (mode == "days") {
        return seconds(3600 * 24);
    } else if (mode == "minutes") {
        return seconds(60);
    }
    throw std::runtime_error("invalid approximation interval");
}

std::string typeString(DataType type) {
    switch (type) {
        case DataType::Int:
            return "int";
        case DataType::Float:
            return "float";
        case DataType::Bool:
            return "bool";
        default:
            throw std::runtime_error("invalid data type");
    }
}

// Команды -----------------------------------------------------------------------------------------

Json ServerCommands::transmitData(Json const &json) {
    auto id = json["device_id"].get<Device>();
    auto time = timePoint(json["time"].get<std::string>());

    transmitJson.clear();
    if (!json.contains("data")) {
        return errorJson(map->getPath(id).data(), "transmit_data",
                "'data' is required json parameter");
    }

    for (auto j: json["data"]) {
        Indicator indicator = findIndicator(id, j["name"]);

        auto const &val = j["value"];

        auto iType = capabilities->indicatorType(indicator);

        if (iType == DataType::Float) {
            relations->transmit([this](auto &&...args) { transmit(args...); },
                    id, indicator, val.get<float>(), time);
        }
        if (iType == DataType::Int) {
            relations->transmit([this](auto &&...args) { transmit(args...); },
                    id, indicator, val.get<int>(), time);
        }
        if (iType == DataType::Bool) {
            relations->transmit([this](auto &&...args) { transmit(args...); },
                    id, indicator, val.get<bool>(), time);
        }
    }
    return transmitJson;
}

Json ServerCommands::history(Json const &json) {
    auto id = json["device_id"].get<Device>();
    time_point startDate = timePoint(json["start_date"].get<std::string>());

    time_point endDate;
    if (json.contains("end_date")) {
        endDate = timePoint(json["end_date"].get<std::string>());
    } else {
        endDate = hclock::now();
    }

    seconds interval(0);
    if (json.contains("interval")) {
        interval = parseInterval(json["interval"].get<std::string>());
    }

    ApproxMode approx = ApproxMode::Average;
    if (json.contains("approx")) {
        approx = parseApprox(json["approx"].get<std::string>());
    }

    currentHistory.clear();
    if (json.contains("parameter")) {
        for (auto const &p: json["parameter"]) {
            Parameter param = findParameter(id, p["name"]);
            relations->parameterHistory(
                    [this](auto &&...args) { prepareHistory(args...); }, id,
                    param, startDate, endDate, interval, approx);
        }
    }

    if (json.contains("indicator")) {
        for (auto const &i: json["indicator"]) {
            Indicator indicator = findIndicator(id, i);
            relations->indicatorHistory(
                    [this](auto &&...args) { prepareHistory(args...); }, id,
                    indicator, startDate, endDate, interval, approx);
        }
    }

    Json result;
    result["data"] = historyJson();
    result["device_id"] = id;
    return result;
}

Json ServerCommands::addDeviceType(Json const &json) {
    auto typeName = json["name"].get<std::string>();
    auto type = capabilities->addDeviceType(typeName);

    if (!json.contains("work_modes")) {
        return errorJson(typeName, "add_device_type",
                "'work_modes' is required json parameter");
    }

    for (auto const &wm: json["work_modes"]) {
        auto wmName = wm["name"].get<std::string>();
        auto workMode = capabilities->addWorkMode(type, wmName);

        if (wm.contains("parameters")) {
            for (auto p: wm["parameters"]) {
                capabilities->addParameter(workMode,
                        p["name"].get<std::string>(),
                        parseDataType(p["type"].get<std::string>()));
            }
        }

        if (wm.contains("indicators")) {
            for (auto p: wm["indicators"]) {
                capabilities->addIndicator(workMode,
                        p["name"].get<std::string>(),
                        parseDataType(p["type"].get<std::string>()));
            }
        }
    }
    return {};
}

Json ServerCommands::removeDeviceType(Json const &json) {
    Json res;
    auto typeStr = json["device_type"].get<std::string>();
    auto deviceType = capabilities->findDeviceType(typeStr);
    if (!deviceType.has_value()) {
        throw std::runtime_error("device type '" + typeStr + "' is not exist");
    }
    auto usedDevice = map->removeDeviceType(*deviceType);
    if (usedDevice != -1) {
        throw std::runtime_error(
                "device type '" + typeStr + "' is used by device '" +
                        std::string(map->getPath(usedDevice)) + "'");
    }
    return res;
}

Json ServerCommands::deviceTypeInfo(const Json &) {
    Json res;
    for (DeviceType type: capabilities->enumerateDeviceTypes()) {
        Json deviceType;
        deviceType["name"] = capabilities->deviceTypeName(type).data();

        auto workModes = capabilities->enumerateWorkModes(type);
        for (WorkMode wm: workModes) {
            Json workMode;
            workMode["name"] = capabilities->workModeName(wm).data();

            for (Indicator i: capabilities->enumerateIndicators(wm)) {
                Json indicator;
                indicator["name"] = capabilities->indicatorName(i).data();
                indicator["type"] = typeString(capabilities->indicatorType(i));
                workMode["indicators"].push_back(indicator);
            }

            for (Indicator i: capabilities->enumerateParameters(wm)) {
                Json parameter;
                parameter["name"] = capabilities->parameterName(i).data();
                std::cout << capabilities->parameterName(i).data() << std::endl;
                parameter["type"] = typeString(capabilities->parameterType(i));
                workMode["parameters"].push_back(parameter);
            }

            deviceType["work_modes"].push_back(workMode);
        }

        res["device_types"].push_back(deviceType);
    }

    return res;
}

Json ServerCommands::addDevice(Json const &json) {
    auto location = json["location"].get<std::string>();

    auto typeStr = json["device_type"].get<std::string>();
    auto deviceType = capabilities->findDeviceType(typeStr);
    if (!deviceType.has_value()) {
        throw std::runtime_error("device type '" + typeStr + "' is not exist");
    }


    auto wmName = json["work_mode"].get<std::string>();
    auto workMode = capabilities->findWorkMode(*deviceType, wmName);
    if (!workMode.has_value()) {
        throw std::runtime_error("work mode '" + wmName + "' is not exist");
    }

    auto id = map->add(location, *deviceType, *workMode);

    Json res;
    res["device_id"] = id;
    return res;
}

Json ServerCommands::deviceInfo(const Json &) {
    Json res;
    for (Device d: map->find("*")) {
        Json device;
        device["device_id"] = d;
        device["location"] = map->getPath(d).data();
        device["device_type"] = capabilities->deviceTypeName(map->deviceType(d)).data();
        device["work_mode"] = capabilities->workModeName(map->deviceType(d)).data();
        res["devices"].push_back(device);
    }
    return res;
}

Json ServerCommands::findDevice(Json const &json) {
    auto match = json["match"].get<bool>();
    auto location = json["location"].get<std::string>();
    auto devices = map->find(location, match);

    Json res;
    res["device_id"] = devices;
    return res;
}

Json ServerCommands::removeDevice(Json const &json) {
    auto id = json["device_id"].get<Device>();
    map->remove(id);
    return {};
}

Json ServerCommands::setWorkMode(Json const &json) {
    auto id = json["device_id"].get<Device>();
    auto wm = capabilities->findWorkMode(map->deviceType(id), json["work_mode"].get<std::string>());
    map->setWorkMode(id, *wm);
    return {};
}

Json ServerCommands::setLocation(Json const &json) {
    auto id = json["device_id"].get<Device>();
    auto location = json["location"].get<std::string>();
    map->setPath(id, location);
    return {};
}

Json ServerCommands::link(Json const &json, bool unlink) {
    auto transmitter = json["transmitter"].get<Device>();
    Indicator indicator = findIndicator(transmitter, json["indicator"]);
    auto receiver = json["receiver"].get<Device>();
    Parameter parameter = findParameter(receiver, json["parameter"]);

    if (unlink) {
        relations->unlink(transmitter, indicator, receiver, parameter);
    } else {
        relations->link(transmitter, indicator, receiver, parameter);
    }

    return {};
}

Json ServerCommands::callback(Json const &json) {
    if (!json.contains("command_name")) {
        return errorJson(
                "undefined command", "request",
                "all commands must specify 'command_name'");
    }

    auto command = json["command_name"].get<std::string>();
    std::cout << "Accepted command: " << command << std::endl;

    Json result;
    try {
        if (command == "transmit_data") {
            result = transmitData(json);
        } else if (command == "history") {
            result = history(json);
        } else if (command == "add_device_type") {
            result = addDeviceType(json);
        } else if (command == "remove_device_type") {
            result = removeDeviceType(json);
        } else if (command == "device_type_info") {
            result = deviceTypeInfo(json);
        } else if (command == "add_device") {
            result = addDevice(json);
        } else if (command == "device_info") {
            result = deviceInfo(json);
        } else if (command == "find_device") {
            result = findDevice(json);
        } else if (command == "remove_device") {
            result = removeDevice(json);
        } else if (command == "set_work_mode") {
            result = setWorkMode(json);
        } else if (command == "set_location") {
            result = setLocation(json);
        } else if (command == "link") {
            result = link(json, false);
        } else if (command == "unlink") {
            result = link(json, true);
        }
    } catch (std::exception const &e) {
        result = errorJson("", command, e.what());
    }

    if (result.is_array()) {
        for (auto &r: result) {
            r["command-name"] = command;
        }
    } else {
        result["command_name"] = command;
    }
    return result;
}

Json ServerCommands::errorJson(std::string const &from, std::string const &stage,
        const std::string &msg) {
    Json j;
    if (from.empty()) {
        j["error"] = "at '" + stage + "': " + msg;
    } else {
        j["error"] = "from '" + from + "' at '" + stage + "': " + msg;
    }
    return j;
}

Indicator ServerCommands::findIndicator(Device id, std::string const &name) {
    auto indicator = capabilities->findIndicator(map->getWorkMode(id), name);
    if (!indicator.has_value()) {
        throw std::runtime_error("indicator '" + name + "' is not exist");
    }
    return *indicator;
}

Parameter ServerCommands::findParameter(Device id, std::string const &name) {
    auto indicator = capabilities->findParameter(map->getWorkMode(id), name);
    if (!indicator.has_value()) {
        throw std::runtime_error("parameter '" + name + "' is not exist");
    }
    return *indicator;
}
