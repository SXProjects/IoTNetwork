#pragma once

#include <iostream>
#include "DeviceMap.hpp"
#include <nlohmann/json.hpp>
#include "WebsocketServer.hpp"

template<typename T>
struct Timestamp {
    T val;
    time_point time;
};

class DeviceRelations;

class ServerCommands {
public:
    ServerCommands(WebsocketServer *server, DeviceMap *map, Capabilities *capabilities,
            DeviceRelations *relations) : map(map), capabilities(capabilities),
            initTime(hclock::now()), server(server), relations(relations) {
    }

    template<typename T>
    void transmit(Device receiver, Parameter parameter, Timestamp<T> val) {
        Json json;
        json["device_id"] = receiver;
        json["command_name"] = "transmit_data";
        json["time"] = timeAndDate(val.time);
        json[std::to_string(parameter)] = val.val;
        server->send(json);
    }

    template<typename T>
    void parameterHistory(Device, Parameter parameter, std::vector<Timestamp<T>> const &data) {
        for (auto t: data) {
            currentHistory[t.time]["p" + std::to_string(parameter)] = t.val;
        }
    }

    template<typename T>
    void indicatorHistory(Device, Parameter parameter, std::vector<Timestamp<T>> const &data) {
        for (auto t: data) {
            currentHistory[t.time]["i" + std::to_string(parameter)] = t.val;
        }
    }

    void callback(Json json);

private:
    static std::string timeAndDate(hclock::time_point now);

    static hclock::time_point timePoint(std::string const &date);

    WebsocketServer *server;
    DeviceMap *map;
    Capabilities *capabilities;
    DeviceRelations *relations;
    time_point initTime;
    std::map<time_point, Json> currentHistory;
};

