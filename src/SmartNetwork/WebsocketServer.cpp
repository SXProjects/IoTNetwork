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
                    callback(Json::parse(msg->get_payload()));
                });

        server.listen(port);

        // Start the server accept loop
        server.start_accept();

        // Start the ASIO io_service run loop
        server.run();
    } catch (websocketpp::exception const &e) {
        std::cout << e.what() << std::endl;
    } catch (...) {
        std::cout << "other exception" << std::endl;
    }
}
