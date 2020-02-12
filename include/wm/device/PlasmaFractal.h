#pragma once

#include "wm/Device.h"

#include <unirender/Texture.h>

namespace wm
{
namespace device
{

class PlasmaFractal : public Device
{
public:
    PlasmaFractal()
    {
        m_exports = {
            {{ DeviceVarType::Heightfield, "out" }},
        };
    }

    virtual void Execute() override;

private:
    void MakePlasma(size_t width, size_t height);

    RTTR_ENABLE(Device)

#define PARM_FILEPATH "wm/device/PlasmaFractal.parm.h"
#include <dag/node_parms_gen.h>
#undef PARM_FILEPATH

}; // PlasmaFractal

}
}