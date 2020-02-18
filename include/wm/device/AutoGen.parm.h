#ifndef PARAM_INFO
#error "You must define PARAM_INFO macro before include this file"
#endif

PARAM_INFO(Width,  Int,   width,  m_width,  (512))
PARAM_INFO(Height, Int,   height, m_height, (512))

PARAM_INFO(GroupCap, Float, group_cap, m_group_cap, (0.1f))
