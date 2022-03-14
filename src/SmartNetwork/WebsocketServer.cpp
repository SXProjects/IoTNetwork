#include "WebsocketServer.hpp"

void WebsocketServer::run(int port) {
    try {
        // Set logging settings
        server.set_access_channels(websocketpp::log::alevel::all);
        server.clear_access_channels(websocketpp::log::alevel::frame_payload);

        // Initialize Asio
        server.init_asio();

        server.set_open_handler([this](auto hdl) {
            std::cout << "Connected to server" << std::endl;
            connection = hdl;
        });

        // Register our message handler
        server.set_message_handler(
                [this](auto &&hdl, auto &&msg) {
                    Json json;
                    Json result;

                    auto formResponse = [&result](Json const &r) {
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

                    send(result);
                });

        server.listen(port);

        // Start the server accept loop
        server.start_accept();

        // Start the ASIO io_service run loop
        server.run();
    } catch (std::exception const &e) {
        std::cout << e.what() << std::endl;
    }
}

Json WebsocketServer::sendError(const std::string &message, const std::string &what) {
    Json json;
    json["error"] = message + ": " + what;
    return json;
}
