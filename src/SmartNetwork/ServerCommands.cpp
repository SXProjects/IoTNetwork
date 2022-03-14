#include "ServerCommands.hpp"
#include "DeviceRelations.hpp"

std::string ServerCommands::timeAndDate(hclock::time_point now) {
    auto in_time_t = hclock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%b-%dT%H:%M:%S");
    return ss.str();
}

hclock::time_point ServerCommands::timePoint(const std::string &date) {
    std::tm tm = {};
    std::stringstream ss(date);
    ss >> std::get_time(&tm, "%Y-%b-%dT%H:%M:%S");
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

void ServerCommands::callback(Json json) {
    auto command = json["command_name"].get<std::string>();
    json.erase("command_name");
    auto id = json["device_id"].get<int>();
    json.erase("device_id");

    if (command == "transmit_data") {
        for (auto j: json) {
            auto indicator = j[0].get<Indicator>();
            auto const &val = j[1];
            auto iType = capabilities->indicatorType(indicator);

            if (iType == DataType::Float) {
                relations->transmit(id, indicator, val.get<float>());
            }
            if (iType == DataType::Int) {
                relations->transmit(id, indicator, val.get<int>());
            }
            if (iType == DataType::Bool) {
                relations->transmit(id, indicator, val.get<bool>());
            }
        }
    } else if (command == "history") {
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
                auto param = p.get<Parameter>();
                relations->parameterHistory(id, param, startDate, endDate, interval, approx);
            }
        }

        if (json.contains("indicator")) {
            for (auto const &i: json["indicator"]) {
                auto indicator = i.get<Indicator>();
                relations->indicatorHistory(id, indicator, startDate, endDate, interval, approx);
            }
        }

        Json result;
        result["device_id"] = id;
        auto history = Json::array();
        for (auto &p: currentHistory) {
            p.second["time"] = timeAndDate(p.first);
            history.push_back(p.second);
        }
        json["history"] = history;
    } else if (command == "add_device_type") {
        auto type = capabilities->addDeviceType(json["name"].get<std::string>());
        for (auto const &wm: json["workModes"]) {
            auto workMode = capabilities->addWorkMode(type, wm["name"].get<std::string>());
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
    } else if (command == "system_info") {
        Json res;
        res["command"] = "system_info";

        if (json.contains("wm_name")) {
            res["wm_name"] = capabilities->workModeName(json["get_wm_name"].get<WorkMode>());
        }
        if (json.contains("parameter_name")) {
            res["parameter_name"] = capabilities->parameterName(
                    json["parameter_name"].get<WorkMode>());
        }
        if (json.contains("indicator_name")) {
            res["parameter_name"] = capabilities->parameterName(
                    json["indicator_name"].get<Parameter>());
        }
        if(json.contains("enumerate_parameters"))
        {
            capabilities->enumerateParameters(json["enumerate_parameters"])
        }
    } else if (command == "add_device") {
        map->add(
                json["location"].get<std::string>(),
                json["type"].get<DeviceType>(),
                json["wm"].get<WorkMode>());
    }

    server->success(command);
}
