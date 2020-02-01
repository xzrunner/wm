#pragma once

#include "terr/Device.h"

#include <unirender/Texture.h>

namespace terr
{
namespace device
{

class Combiner : public Device
{
public:
    enum class Method
    {
        Average,
        Add,
        Subtract,
        Multiply,
        Divide,
        Max,
        Min,
    };

public:
    Combiner()
    {
        m_imports = {
            {{ DeviceVarType::Heightfield, "in0" }},
            {{ DeviceVarType::Heightfield, "in1" }},
        };
        m_exports = {
            {{ DeviceVarType::Heightfield, "out" }},
        };
    }

    virtual void Execute(const Context& ctx) override;

    RTTR_ENABLE(Device)

#define PARM_FILEPATH "terr/device/Combiner.parm.h"
#include <dag/node_parms_gen.h>
#undef PARM_FILEPATH

}; // Combiner

}
}