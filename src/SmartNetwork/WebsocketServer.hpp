#pragma once

#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>

#include <nlohmann/json.hpp>

using Json = nlohmann::json;

class WebsocketServer {
public:
    void run(int port);

    void receive(std::function<Json(Json const &json)> f) {
        callback = std::move(f);
    }

private:
    void send(Json const &json) {
        server.send(connection, json.dump(), websocketpp::frame::opcode::text);
    }

    Json sendError(std::string const &message, std::string const &what);

    using Server = websocketpp::server<websocketpp::config::asio>;

    Server server;
    websocketpp::connection_hdl connection;
    std::function<Json(Json const &json)> callback;
};

