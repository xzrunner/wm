#pragma once

#include "terr/typedef.h"

namespace terr
{

class HeightField;

class DeviceHelper
{
public:
    static DevicePtr GetInputDevice(const Device& dev, size_t idx);
    static std::shared_ptr<HeightField>
        GetInputHeight(const Device& dev, size_t idx);

}; // DeviceHelper

}