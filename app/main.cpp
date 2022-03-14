#include <SmartNetwork/WebsocketServer.hpp>
#include <SmartNetwork/ServerCommands.hpp>
#include <SmartNetwork/DeviceRelations.hpp>

int main() {
    WebsocketServer server;
    Capabilities capabilities;
    DeviceMap map(&capabilities);

    DeviceRelations relations(&map, &capabilities);
    ServerCommands commands(&server, &map, &capabilities, &relations);

    server.receive([&commands](auto &j) { return commands.callback(j); });

    server.run(9002);
}
