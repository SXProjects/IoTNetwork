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

class ServerCommands {
public:
    ServerCommands(WebsocketServer* server, DeviceMap *map, Capabilities *capabilities) :
            map(map), capabilities(capabilities), initTime(hclock::now()), server(server) {
    }

    template<typename T>
    void transmit(Device receiver, Parameter parameter, Timestamp<T> val) {
        Json json;
        json["device_id"] = receiver;
        json["command_name"] = "transmit_data";
        json["time"] = time_and_date(val.time);
        json[std::to_string(parameter)] = val.val;
        server->send(json);
    }

    template<typename T>
    void history(Device receiver, Parameter parameter, std::vector<Timestamp<T>> const &data) {

    }


private:
    static std::string time_and_date(hclock::time_point now) {
        auto in_time_t = hclock::to_time_t(now);

        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
        return ss.str();
    }

    void beginHistory(Device receiver, std::string_view command)
    {

    }

    void endHistory(Device receiver);

    WebsocketServer* server;
    DeviceMap *map;
    Capabilities *capabilities;
    time_point initTime;
    Json currentJson;
};

