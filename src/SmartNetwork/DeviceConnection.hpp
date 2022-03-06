#pragma once
#include "DeviceData.hpp"

enum class ConnectionMethod
{

};

class TransferData
{
public:

private:
    DataType type;
    void const* data;
};

class DeviceConnection {
public:
    void connect(Device device, ConnectionMethod method);

    void disconnect(Device device);

    void transmit(Device device, Parameter parameter, TransferData data);

    void receiveHistory(Device receiver, unsigned time);

    void receivePackaged(Device receiver, unsigned time);

    void receiveOnAlive(Device receiver);

    void transmitPackaged(Device transmitter, unsigned time);
};