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

std::string timeAndDate(hclock::time_point now);

hclock::time_point timePoint(std::string const &date);

class ServerCommands {
public:
    ServerCommands(WebsocketServer *server, DeviceMap *map, Capabilities *capabilities,
            DeviceRelations *relations) : map(map), capabilities(capabilities),
            initTime(hclock::now()), server(server), relations(relations) {
    }

    void callback(Json json);

    template<typename T>
    void transmit(Device receiver, Parameter parameter, Timestamp<T> val) {
        Json json;
        json["device_id"] = receiver;
        json["command_name"] = "transmit_data";
        json["time"] = timeAndDate(val.time);
        std::string param = capabilities->parameterName(parameter).data();
        json[param] = val.val;
        server->send(json);
    }

    void transmitData(Json json);

    template<typename T>
    void history(std::string name, std::vector<Timestamp<T>> const &data)
    {
        for (auto t: data) {
            currentHistory[t.time][name] = t.val;
        }
    }


private:
    WebsocketServer *server;
    DeviceMap *map;
    Capabilities *capabilities;
    DeviceRelations *relations;
    time_point initTime;
    std::map<time_point, Json> currentHistory;
};

