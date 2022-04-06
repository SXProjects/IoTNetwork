#define _WEBSOCKETPP_CPP11_THREAD_

#include <fstream>
#include <SmartNetwork/Websockets.hpp>

int main() {
    try {
        auto cntCallback = []() {
            std::fstream f("test.json");
            if (!f) {
                std::cout
                        << "CALL THIS EXE FILE FROM WORKING DIRECTORY THAT CONTAINS VALID TEST.JSON"
                        << std::endl;
                throw std::runtime_error("cannot read file");
            }
            std::stringstream buffer;
            buffer << f.rdbuf();
            return Json::parse(buffer.str());
        };

        auto msgCallback = [](Json const &j) {
            std::cout << j << std::endl;
            return Json();
        };

        std::ifstream i("config.json");
        if (!i) {
            std::cout << "CALL THIS EXE FILE FROM WORKING DIRECTORY THAT CONTAINS VALID CONFIG.JSON"
                    << std::endl;
            return 0;
        }
        Json j;
        i >> j;

        if (j["mode"] == "client") {
            std::cout << "RUNNING TEST SERVER" << std::endl;
            std::string uri = j["client"].get<std::string>();
            std::string sPort = uri.substr(uri.rfind(':') + 1);
            int port = std::stoi(sPort);
            std::cout << "port: " << port;
            runServer(cntCallback, msgCallback, port, [](){});
            return 0;
        }

        if (j["mode"] == "server") {
            std::cout << "RUNNING TEST CLIENT" << std::endl;
            auto addr = "ws://127.0.0.1:" + std::to_string(j["server"].get<int>());
            std::cout << "Connecting to " << addr << std::endl;
            runClient(cntCallback, msgCallback, addr, [](){});
            return 0;
        }
    }
    catch (std::bad_alloc const &e) {
        std::cout << "UNDEFINED ERROR: " << e.what() << std::endl;
    }
}
