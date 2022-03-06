#include <SmartNetwork/DeviceMap.hpp>

int main() {
    DeviceMap map;
    map.add("room/place/temperature", {}, {});
    map.add("room/place/light", {}, {});
    map.add("space/light", {}, {});
    map.add("space/light/temperature", {}, {});

    std::cout << "\nAll: " << std::endl;
    for(auto d : map.find("*"))
    {
        std::cout << map.getPath(d) << std::endl;
    }

    std::cout << "\nTemperature: " << std::endl;
    for(auto d : map.find("*/temperature"))
    {
        std::cout << map.getPath(d) << std::endl;
    }

    std::cout << "\nPlace: " << std::endl;
    for(auto d : map.find("room/place/*"))
    {
        std::cout << map.getPath(d) << std::endl;
    }
}
