#include "terraingraph/device/Colorizer.h"
#include "terraingraph/DeviceHelper.h"
#include "terraingraph/HeightFieldEval.h"
#include "terraingraph/Context.h"

#include <heightfield/HeightField.h>
#include <heightfield/Utility.h>

namespace terraingraph
{
namespace device
{

void Colorizer::Execute(const std::shared_ptr<dag::Context>& ctx)
{
    auto prev_hf = DeviceHelper::GetInputHeight(*this, 0);
    if (!prev_hf) {
        return;
    }

    size_t w = prev_hf->Width();
    size_t h = prev_hf->Height();
    if (!m_bmp ||
        m_bmp->Width() != w ||
        m_bmp->Height() != h) {
        m_bmp = std::make_shared<Bitmap>(w, h);
    }

    assert(!m_gradient.empty());
    SortGradientColors();

    auto& dev = *std::static_pointer_cast<Context>(ctx)->ur_dev;
    auto& heights = prev_hf->GetValues(dev);
    auto pixels = m_bmp->GetPixels();
    for (size_t i = 0, n = heights.size(); i < n; ++i)
    {
        float h = hf::Utility::HeightShortToFloat(heights[i]);
        h = std::min(std::max(h, m_gradient.front().w), m_gradient.back().w);
        for (size_t j = 0, m = m_gradient.size(); j < m - 1; ++j)
        {
            if (h >= m_gradient[j].w && h < m_gradient[j + 1].w)
            {
                sm::vec3 col0(m_gradient[j].xyzw);
                sm::vec3 col1(m_gradient[j + 1].xyzw);
                assert(m_gradient[j].w != m_gradient[j + 1].w);
                float p = (h - m_gradient[j].w) / (m_gradient[j + 1].w - m_gradient[j].w);
                auto col = col0 + (col1 - col0) * p;
                pixels[i * 3 + 0] = static_cast<unsigned char>(col.x * 255);
                pixels[i * 3 + 1] = static_cast<unsigned char>(col.y * 255);
                pixels[i * 3 + 2] = static_cast<unsigned char>(col.z * 255);
                break;
            }
        }
    }
}

void Colorizer::SortGradientColors()
{
    std::sort(m_gradient.begin(), m_gradient.end(), [](const sm::vec4& a, const sm::vec4& b) {
        return a.w < b.w;
    });
}

}
}