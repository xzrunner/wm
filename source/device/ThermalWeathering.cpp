#include "terraingraph/device/ThermalWeathering.h"
#include "terraingraph/DeviceHelper.h"
#include "terraingraph/EvalGPU.h"
#include "terraingraph/Context.h"

#include <heightfield/HeightField.h>
#include <painting0/ShaderUniforms.h>

namespace
{

std::shared_ptr<terraingraph::EvalGPU> EVAL = nullptr;

const char* cs = R"(

#version 430

layout(binding = 0, std430) coherent buffer HeightfieldDataFloat
{
	float height_buf[];
};

layout(std140) uniform UBO
{
	int   grid_sizex;
	int   grid_sizey;
	float amplitude;
	float cell_size;
	float tan_threshold_angle;
} ubo;

bool Inside(int x, int y)
{
	if (x < 0 || x >= ubo.grid_sizex ||
        y < 0 || y >= ubo.grid_sizey) {
		return false;
    }
	return true;
}

int ToIndex1D(int x, int y)
{
	return y * ubo.grid_sizex + x;
}

layout(local_size_x = 1024) in;
void main()
{
	uint id = gl_GlobalInvocationID.x;
	if (id >= height_buf.length()) {
        return;
    }

	int i_amplitude = int(ubo.amplitude);
	float max_y_diff = 0;
	int   max_idx = -1;
	int y = int(id) / ubo.grid_sizex;
	int x = int(id) % ubo.grid_sizex;
	for (int k = -1; k <= 1; k += 2)
	{
		for (int l = -1; l <= 1; l += 2)
		{
			if (Inside(x + k, y + l) == false)
				continue;
			int index = ToIndex1D(x + k, y + l);
			float h = float(height_buf[index]);
			float diff = float(height_buf[id] - h);
			if (diff > max_y_diff)
			{
				max_y_diff = diff;
				max_idx = index;
			}
		}
	}
	if (max_idx != -1 && max_y_diff / ubo.cell_size > ubo.tan_threshold_angle)
	{
		height_buf[id] = height_buf[id] - ubo.amplitude;
		height_buf[max_idx] = height_buf[max_idx] + ubo.amplitude;
	}
}

)";

}

namespace terraingraph
{
namespace device
{

void ThermalWeathering::Execute(const std::shared_ptr<dag::Context>& ctx)
{
    auto prev_hf = DeviceHelper::GetInputHeight(*this, 0);
    if (!prev_hf) {
        return;
    }

    m_hf = std::make_shared<hf::HeightField>(*prev_hf);

    auto& dev = *std::static_pointer_cast<Context>(ctx)->ur_dev;
#ifdef THERMAL_WEATHERING_GPU
    if (!EVAL) {
        EVAL = std::make_shared<EvalGPU>(dev, cs);
    }

    auto num = m_hf->Width() * m_hf->Height();
    auto group_sz = EVAL->GetComputeWorkGroupSize();
    int thread_group_count = num / group_sz;
    if (num % group_sz > 0) {
        thread_group_count++;
    }

    for (int i = 0; i < m_iter; ++i) {
        StepGPU(dev, thread_group_count);
    }
#else
    for (int i = 0; i < m_iter; ++i) {
        StepCPU(dev);
    }
#endif // THERMAL_WEATHERING_GPU
}

// from Outerrain
/*
\brief Perform a thermal erosion step with maximum amplitude defined by user. Based on http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.27.8939&rep=rep1&type=pdf.
\param amplitude maximum amount of matter moved from one point to another. Something between [0.05, 0.1] gives plausible results.
*/
void ThermalWeathering::StepCPU(const ur::Device& dev)
{
    size_t w = m_hf->Width();
    size_t h = m_hf->Height();

    // todo: 50.0f from wmv renderer
    const float h_scale = 50.0f;
	float cell_dist_x = 1.0f / h_scale;
	for (size_t y = 0; y < h; y++)
	{
		for (size_t x = 0; x < w; x++)
		{
			float max_y_diff = 0.0f;
			int nei_x = -1;
			int nei_y = -1;
			for (int k = -1; k <= 1; k++)
			{
				for (int l = -1; l <= 1; l++)
				{
                    if ((k == 0 && l == 0) ||
                        m_hf->Inside(x + l, y + k) == false) {
                        continue;
                    }
					float h = static_cast<float>(m_hf->Get(dev, x, y) - m_hf->Get(dev, x + l, y + k));
					if (h > max_y_diff)
					{
						max_y_diff = h;
						nei_x = x + l;
						nei_y = y + k;
					}
				}
			}

			if (nei_x != -1 && max_y_diff / cell_dist_x > m_tan_threshold_angle)
			{
                m_hf->Add(x, y, -m_amplitude);
                m_hf->Add(nei_x, nei_y, m_amplitude);
			}
		}
	}
}

void ThermalWeathering::StepGPU(const ur::Device& dev, int thread_group_count)
{
    pt0::ShaderUniforms vals;
    vals.AddVar("ubo.grid_sizex",          pt0::RenderVariant(static_cast<int>(m_hf->Width())));
    vals.AddVar("ubo.grid_sizey",          pt0::RenderVariant(static_cast<int>(m_hf->Height())));
    vals.AddVar("ubo.amplitude",           pt0::RenderVariant(m_amplitude));
    vals.AddVar("ubo.tan_threshold_angle", pt0::RenderVariant(m_tan_threshold_angle));
    vals.AddVar("ubo.cell_size",           pt0::RenderVariant(1.0f / 50.0f));

    EVAL->RunCS(dev, vals, thread_group_count, *m_hf);
}

}
}