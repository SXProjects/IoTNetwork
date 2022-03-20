#pragma once

#include <iostream>
#include "DeviceMap.hpp"
#include <nlohmann/json.hpp>
#include "Relations.hpp"

using Json = nlohmann::json;

std::string timeAndDate(hclock::time_point now);

hclock::time_point timePoint(std::string const &date);

class Commands {
public:
    Commands(DeviceMap *map, Capabilities *capabilities,
            Relations *relations) : map(map), capabilities(capabilities),
            initTime(hclock::now()), relations(relations) {
    }

    Json callback(Json const &json);

    // вызывает Relations

    template<typename T>
    void transmit(Device receiver, Parameter parameter, Timestamp<T> val) {
        Json json;
        try {
            json["device_id"] = receiver;
            json["time"] = timeAndDate(val.time);
            Json data;
            data["name"] = capabilities->parameterName(parameter).data();
            data["value"] = val.val;
            json["data"].push_back(data);
        }
        catch (std::exception const &e) {
            std::cout << "transmit error: " << e.what() << std::endl;
            json = errorJson(map->getPath(receiver).data(), "transmit", e.what());
        }
        transmitJson.push_back(json);
    }

    template<typename T>
    void prepareHistory(std::string name, std::vector<Timestamp<T>> const &data) {
        for (auto t: data) {
            currentHistory[t.time][name] = t.val;
        }
    }

    // Вызываются в ответ н команды с сервера

    Json transmitData(Json const &json);

    Json history(Json const &json);

    Json addDeviceType(Json const &json);

    Json removeDeviceType(Json const &json);

    Json deviceTypeInfo(Json const &json);

    Json addDevice(Json const &json);

    Json deviceInfo(Json const &json);

    Json findDevice(Json const &json);

    Json listLocations(Json const &json);

    Json removeDevice(Json const &json);

    Json setWorkMode(Json const &json);

    Json setLocation(Json const &json);

    Json link(Json const &json, bool unlink);

private:
    Json historyJson();

    static Json errorJson(std::string const &from, std::string const &stage,
            std::string const &msg);

    Indicator findIndicator(Device id, std::string const& name);

    Parameter findParameter(Device id, std::string const& name);

    DeviceMap *map;
    Capabilities *capabilities;
    Relations *relations;
    time_point initTime;
    std::map<time_point, Json> currentHistory;
    Json transmitJson;
};
