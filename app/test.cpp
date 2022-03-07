#include <SmartNetwork/DeviceMap.hpp>
#include <SmartNetwork/DeviceRelations.hpp>

int main() {
    DeviceCapabilities capabilities;
    auto tt = capabilities.addDeviceType("thermometer");
    auto twm = capabilities.addWorkMode(tt, "time");
    capabilities.addIndicator(twm, "temperature", DataType::Float);

    auto lt = capabilities.addDeviceType("switch");
    auto lwm = capabilities.addWorkMode(lt, "command");
    capabilities.addWorkMode(lt, "time");
    capabilities.addParameter(lwm, "enable", DataType::Float);

    std::cout << "\nThermometer modes: " << std::endl;
    for (auto d: capabilities.enumerateWorkModes(*capabilities.findDeviceType("thermometer"))) {
        std::cout << capabilities.workModeName(d) << std::endl;

        for (auto i: capabilities.enumerateIndicators(d)) {
            std::cout << "    " << capabilities.indicatorName(i) << std::endl;
        }
    }

    std::cout << "\nLight modes: " << std::endl;
    for (auto d: capabilities.enumerateWorkModes(lt)) {
        std::cout << capabilities.workModeName(d) << std::endl;

        for (auto i: capabilities.enumerateParameters(d)) {
            std::cout << "    " << capabilities.parameterName(i) << std::endl;
        }
    }

    DeviceMap map;
    ServerConnection connection(&map, &capabilities);
    auto t1 = map.add("room/place/thermometer", tt, twm);
    auto l1 = map.add("room/place/light", lt, lwm);
    auto l2 = map.add("space/light", *capabilities.findDeviceType("switch"), lwm);
    map.setChangeInfo(l1, ChangeInfo{hclock::now(), 2, seconds(10)});
    auto t2 = map.add("space/light/thermometer", tt, *capabilities.findWorkMode(tt, "time"));

    std::cout << "\nAll: " << std::endl;
    for (auto d: map.find("*")) {
        std::cout << map.getPath(d) << std::endl;
    }

    std::cout << "\nTemperature: " << std::endl;
    for (auto d: map.find("*/thermometer")) {
        std::cout << map.getPath(d) << std::endl;
    }

    std::cout << "\nPlace: " << std::endl;
    for (auto d: map.find("room/place/*")) {
        std::cout << map.getPath(d) << std::endl;
    }


    DeviceRelations links(&map, &capabilities, &connection);

    links.link(t1, *capabilities.findIndicator(twm, "temperature"),
            l2, *capabilities.findParameter(lwm, "enable"));

    links.link(t1, *capabilities.findIndicator(twm, "temperature"),
            l1, *capabilities.findParameter(lwm, "enable"));


    links.transmit(t1, *capabilities.findIndicator(twm, "temperature"), 10.0f);
    links.transmit(t1, *capabilities.findIndicator(twm, "temperature"), 15.0f);
    links.transmit(t1, *capabilities.findIndicator(twm, "temperature"), 20.0f);
    links.awake(l1);
}
