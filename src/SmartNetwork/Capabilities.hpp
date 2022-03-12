#pragma once

#include <string>
#include <vector>
#include <optional>

enum class DataType {
    Int,
    Float,
    Bool,
};

using DeviceType = unsigned;

using WorkMode = unsigned;

using Parameter = unsigned;

using Indicator = unsigned;

class Capabilities {
public:
    DeviceType addDeviceType(std::string_view name);

    WorkMode addWorkMode(DeviceType deviceType, std::string_view name);

    Parameter addParameter(WorkMode workMode, std::string_view name, DataType type);

    Indicator addIndicator(WorkMode workMode, std::string_view name, DataType type);

    std::optional<DeviceType> findDeviceType(std::string_view name);

    std::optional<WorkMode> findWorkMode(DeviceType deviceType, std::string_view name);

    std::vector<WorkMode> enumerateWorkModes(DeviceType deviceType)
    {
        return deviceTypes[deviceType].workModes;
    }

    std::string_view workModeName(WorkMode workMode)
    {
        return workModes[workMode].name;
    }

    std::vector<Indicator> enumerateIndicators(WorkMode workMode)
    {
        return workModes[workMode].indicators;
    }

    std::vector<Parameter> enumerateParameters(WorkMode workMode)
    {
        return workModes[workMode].parameters;
    }

    std::optional<Indicator> findIndicator(WorkMode workMode, std::string_view name);

    std::optional<Parameter> findParameter(WorkMode workMode, std::string_view name);

    DataType indicatorType(Indicator indicator)
    {
        return indicators[indicator].type;
    }

    std::string_view indicatorName(Indicator indicator)
    {
        return indicators[indicator].name;
    }

    DataType parameterType(Parameter parameter)
    {
        return parameters[parameter].type;
    }

    std::string_view parameterName(Parameter parameter)
    {
        return parameters[parameter].name;
    }

private:
    struct DeviceTypeData {
        std::string name;
        std::vector<WorkMode> workModes;
    };

    struct WorkModeData {
        std::string name;
        std::vector<Parameter> parameters;
        std::vector<Indicator> indicators;
    };

    struct ParameterData {
        std::string name;
        DataType type;
    };

    struct IndicatorData {
        std::string name;
        DataType type;
    };

    std::vector<DeviceTypeData> deviceTypes;
    std::vector<WorkModeData> workModes;
    std::vector<ParameterData> parameters;
    std::vector<IndicatorData> indicators;
};

