#include "wm/device/LayoutGen.h"
#include "wm/DeviceHelper.h"
#include "wm/HeightField.h"
#include "wm/Mask.h"

namespace
{

// order
// 3 4 5
// 2   6
// 1 0 7
static const int QUERY_COUNT = 8;
static const sm::ivec2 QUERY_OFFSET[QUERY_COUNT] = {
	sm::ivec2( 0, -1),
	sm::ivec2(-1, -1),
	sm::ivec2(-1,  0),
	sm::ivec2(-1,  1),
	sm::ivec2( 0,  1),
	sm::ivec2( 1,  1),
	sm::ivec2( 1,  0),
	sm::ivec2( 1, -1),
};

}

namespace wm
{
namespace device
{

void LayoutGen::Execute(const Context& ctx)
{
    auto mask = DeviceHelper::GetInputMask(*this, 0);
    if (!mask) {
        return;
    }

    m_hf = MaskToHeightField(*mask);
}

std::shared_ptr<HeightField>
LayoutGen::MaskToHeightField(const Mask& mask) const
{
    std::vector<float> dists(m_width * m_height, 0);

    auto hf = std::make_shared<HeightField>(m_width, m_height);

    auto mw = mask.Width();
    auto mh = mask.Height();
    auto mdist = CalcMaskDistMap(mask);
    std::vector<float> heights(m_width * m_height, 0.0f);
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            float mx = static_cast<float>(x) / m_width * mw;
            float my = static_cast<float>(y) / m_height * mh;
            size_t ix = static_cast<size_t>(std::floor(mx));
            size_t iy = static_cast<size_t>(std::floor(my));
            float fx = mx - ix;
            float fy = my - iy;
            float h;
            if (ix == mw - 1 && iy == mh - 1)
            {
                h = mdist[mw * iy + ix];
            }
            else if (ix == mw - 1)
            {
                h = mdist[mw * iy + ix] * (1 - fy) +
                    mdist[mw * (iy + 1) + ix] * fy;
            }
            else if (iy == mh - 1)
            {
                h = mdist[mw * iy + ix] * (1 - fx) +
                    mdist[mw * iy + ix + 1] * fx;
            }
            else
            {
                h = mdist[mw * iy + ix] * (1 - fx) * (1 - fy) +
                    mdist[mw * iy + ix + 1] * fx * (1 - fy) +
                    mdist[mw * (iy + 1) + ix] * (1 - fx) * fy +
                    mdist[mw * (iy + 1) + ix + 1] * fx * fy;
            }
            heights[m_width * y + x] = h;
        }
    }

    hf->Fill(heights);
    hf->Normalize();

    return hf;
}

std::vector<float>
LayoutGen::CalcMaskDistMap(const Mask& mask)
{
    auto w = mask.Width();
    auto h = mask.Height();
    auto& vals = mask.GetValues();

    std::vector<float> dist_map;

    std::set<size_t> buf;

    dist_map.resize(w * h);
    for (size_t i = 0, n = w * h; i < n; ++i)
    {
        if (vals[i]) {
            dist_map[i] = std::numeric_limits<float>::max();
        } else {
            dist_map[i] = -1;
            continue;
        }
        if (mask.IsPixelBorder(i % w, i / w)) {
            dist_map[i] = 1.0f;
            buf.insert(i);
        }
    }

    while (!buf.empty())
    {
        std::set<size_t> next_buf;
        for (auto& i : buf)
        {
            sm::ivec2 curr_pos(i % w, i / w);
            float next_dist = dist_map[i] + 1.0f;
            for (int j = 0; j < QUERY_COUNT; ++j)
            {
                auto nearby = curr_pos + QUERY_OFFSET[j];
                if (nearby.x < 0 || nearby.x >= static_cast<int>(w) ||
                    nearby.y < 0 || nearby.y >= static_cast<int>(h)) {
                    continue;
                }

                auto next = w * nearby.y + nearby.x;
                if (!vals[next]) {
                    continue;
                }

                if (next_dist < dist_map[next]) {
                    dist_map[next] = next_dist;
                    next_buf.insert(next);
                }
            }
        }
        buf = next_buf;
    }

    return dist_map;
}

}
}