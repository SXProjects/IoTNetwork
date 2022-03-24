#pragma once

#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_

#include <websocketpp/server.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <nlohmann/json.hpp>

using Json = nlohmann::json;

template<typename M, typename F, typename S>
void messageHandler(S &&send, F &&callback, websocketpp::connection_hdl hdl, M msg) {

    auto sendError = [](const std::string &message, const std::string &what) {
        Json json;
        json["error"] = message + ": " + what;
        return json;
    };

    Json json;
    Json result;

    auto formResponse = [&result](Json const &r) {
        if (r.empty()) {
            return;
        }

        if (r.is_array()) {
            for (auto const &e: r) {
                result.push_back(e);
            }
        } else {

            result.push_back(r);
        }
    };

    try {
        json = Json::parse(msg->get_payload());
    } catch (std::exception &e) {
        return send(sendError("parse error", e.what()));
    }

    if (json.contains("command_name")) {
        if (json["command_name"] == "stop")
        {
            throw std::runtime_error("saving and stopping...");
        }
    }

    try {
        if (json.is_array()) {
            for (auto &j: json) {
                formResponse(callback(j));
            }
        } else {
            formResponse(callback(json));
        }
    } catch (std::exception &e) {
        result.push_back(sendError("undefined request error", e.what()));
    }

    if (!result.empty()) {
        send(result);
    }
}

template<typename S, typename F, typename C>
void setup(S &s, C &&cntCall, F &&msgCall, websocketpp::connection_hdl &connection) {
    s.set_access_channels(websocketpp::log::alevel::all);
    s.clear_access_channels(websocketpp::log::alevel::frame_payload);

    s.init_asio();

    s.set_open_handler([&connection, &cntCall, &s](auto hdl) {
        std::cout << "Connected to server" << std::endl;
        connection = hdl;
        auto msg = cntCall();
        if (!msg.empty()) {
            s.send(connection, cntCall().dump(), websocketpp::frame::opcode::text);
        }
    });

    s.set_message_handler(
            [&](auto &&hdl, auto &&msg) {
                messageHandler([&](Json const &json) {
                    if (json.empty()) {
                        return;
                    }
                    s.send(connection, json.dump(), websocketpp::frame::opcode::text);
                }, msgCall, hdl, msg);
            }
    );
}

template<typename F, typename C>
void runServer(C &&cntCall, F &&msgCall, int port) {
    websocketpp::connection_hdl connection;
    using Server = websocketpp::server<websocketpp::config::asio>;
    Server server;
    setup(server, cntCall, msgCall, connection);
    server.listen(port);
    server.start_accept();
    server.run();
}

template<typename F, typename C>
void runClient(C &&cntCall, F &&msgCall, std::string const &uri) {
    websocketpp::connection_hdl connection;
    using Client = websocketpp::client<websocketpp::config::asio_client>;
    Client client;
    setup(client, cntCall, msgCall, connection);

    websocketpp::lib::error_code ec;
    Client::connection_ptr con = client.get_connection(uri, ec);
    if (ec) {
        std::cout << "could not create connection because: " << ec.message() << std::endl;
        return;
    }

    client.connect(con);
    client.run();
}
