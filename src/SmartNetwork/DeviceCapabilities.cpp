#include "DeviceCapabilities.hpp"

std::optional<DeviceType> DeviceCapabilities::findDeviceType(std::string_view name) {
    for (int i = 0; i < deviceTypes.size(); ++i) {
        if (deviceTypes[i].name == name) {
            return i;
        }
    }
    return {};
}

std::optional<WorkMode>
DeviceCapabilities::findWorkMode(DeviceType deviceType, std::string_view name) {
    for (auto workMode : deviceTypes[deviceType].workModes) {
        if (workModes[workMode].name == name) {
            return workMode;
        }
    }
    return {};
}

std::optional<Indicator> DeviceCapabilities::findIndicator(WorkMode workMode, std::string_view name) {
    for(auto i : workModes[workMode].indicators)
    {
        if(indicators[i].name == name)
        {
            return i;
        }
    }
    return {};
}

std::optional<Parameter> DeviceCapabilities::findParameter(WorkMode workMode, std::string_view name) {
    for(auto i : workModes[workMode].parameters)
    {
        if(parameters[i].name == name)
        {
            return i;
        }
    }
    return {};
}

WorkMode DeviceCapabilities::addWorkMode(DeviceType deviceType, std::string_view name) {
    workModes.push_back(WorkModeData{name.data(), {}, {}});
    deviceTypes[deviceType].workModes.push_back(workModes.size() - 1);
    return workModes.size() - 1;
}

Parameter DeviceCapabilities::addParameter(WorkMode workMode, std::string_view name, DataType type) {
    parameters.push_back(ParameterData{name.data(), type});
    workModes[workMode].parameters.push_back(parameters.size() - 1);
    return parameters.size() - 1;
}

Indicator DeviceCapabilities::addIndicator(WorkMode workMode, std::string_view name, DataType type) {
    indicators.push_back(IndicatorData{name.data(), type});
    workModes[workMode].indicators.push_back(indicators.size() - 1);
    return indicators.size() - 1;
}

DeviceType DeviceCapabilities::addDeviceType(std::string_view name) {
    deviceTypes.push_back(DeviceTypeData{name.data(), {}});
    return deviceTypes.size() - 1;
}
