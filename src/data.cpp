#include "data.h"
#include <imnodes.h>

const unsigned int Data::COLORS[] = {
    IM_COL32(136 * .7, 194 * .7, 115 * .7, 255), // Int
    IM_COL32(136 * .7, 194 * .7, 115 * .7, 255), // IVec2
    IM_COL32(136 * .7, 194 * .7, 115 * .7, 255), // IVec3
    IM_COL32(136 * .7, 194 * .7, 115 * .7, 255), // IVec4
    IM_COL32(162 * .7, 210 * .7, 223 * .7, 255), // Float
    IM_COL32(162 * .7, 210 * .7, 223 * .7, 255), // Vec2
    IM_COL32(162 * .7, 210 * .7, 223 * .7, 255), // Vec3
    IM_COL32(162 * .7, 210 * .7, 223 * .7, 255), // Vec4
    IM_COL32(246 * .7, 239 * .7, 189 * .7, 255), // Texture2D
};
const unsigned int Data::COLORS_HOVER[] = {
    IM_COL32(136, 194, 115, 255), // Int
    IM_COL32(136, 194, 115, 255), // IVec2
    IM_COL32(136, 194, 115, 255), // IVec3
    IM_COL32(136, 194, 115, 255), // IVec4
    IM_COL32(162, 210, 223, 255), // Float
    IM_COL32(162, 210, 223, 255), // Vec2
    IM_COL32(162, 210, 223, 255), // Vec3
    IM_COL32(162, 210, 223, 255), // Vec4
    IM_COL32(246, 239, 189, 255), // Texture2D
};
