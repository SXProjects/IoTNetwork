#include <SmartNetwork/Websockets.hpp>
#include <SmartNetwork/Commands.hpp>
#include <SmartNetwork/Relations.hpp>
#include <fstream>
#include <cereal/archives/binary.hpp>
#include <sstream>
#include <cereal/types/string.hpp>
#include <filesystem>

int main() {
    Capabilities capabilities;
    DeviceMap map(&capabilities);
    Relations relations(&map, &capabilities);

    try {
        if (std::filesystem::exists("data.cereal")) {
            std::ifstream is("data.cereal", std::ios::binary);
            cereal::BinaryInputArchive iarchive(is);
            iarchive(capabilities, map, relations);
            std::cout << "Loading data..." << std::endl;
        } else {
            std::cout << "Empty start..." << std::endl;
        }

        Commands commands(&map, &capabilities, &relations);
        auto callback = [&commands](auto &j) { return commands.callback(j); };

        std::ifstream i("config.json");
        if (!i) {
            std::cout << "CALL THIS EXE FILE FROM WORKING DIRECTORY THAT CONTAINS VALID CONFIG.JSON"
                    << std::endl;
            return 0;
        }
        Json j;
        i >> j;

        if (j["mode"] == "server") {
            std::cout << "RUNNING SERVER" << std::endl;
            runServer([] { return Json(); }, callback, j["server"].get<int>());
            return 0;
        }

        if (j["mode"] == "client") {
            std::cout << "RUNNING CLIENT" << std::endl;
            runClient([] { return Json(); }, callback, j["client"].get<std::string>());
            return 0;
        }
    }
    catch (std::exception const &e) {
        std::cout << "UNDEFINED ERROR: " << e.what() << std::endl;
    }

    std::ofstream os("data.cereal", std::ios::binary);
        cereal::BinaryOutputArchive oarchive(os);
        oarchive(capabilities, map, relations);
        std::cout << "Saving data..." << std::endl;
}
