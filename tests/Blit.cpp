#include "Blit.hpp"
#include "shared.hpp"
#include <limits>

using namespace Aquamarine;
using namespace Hyprutils::Math;

int main() {
    int            ret = 0;
    const Vector2D full{5120, 2880}, half{2560, 2880};
    const auto     left  = blitTextureCoordinates(full, half, CBox{0, 0, 2560, 2880});
    const auto     right = blitTextureCoordinates(full, half, CBox{2560, 0, 2560, 2880});
    EXPECT(left.has_value() && right.has_value(), true);
    if (!left || !right)
        return 1;
    EXPECT((*left == std::array<float, 8>{0.5F, 0, 0, 0, 0.5F, 1, 0, 1}), true);
    EXPECT((*right == std::array<float, 8>{1, 0, 0.5F, 0, 1, 1, 0.5F, 1}), true);
    EXPECT(blitTextureCoordinates(full, half).has_value(), false);
    EXPECT(blitTextureCoordinates(full, half, CBox{-1, 0, 2560, 2880}).has_value(), false);
    EXPECT(blitTextureCoordinates(full, half, CBox{2561, 0, 2560, 2880}).has_value(), false);
    EXPECT(blitTextureCoordinates(full, half, CBox{0, 1, 2560, 2880}).has_value(), false);
    EXPECT(blitTextureCoordinates(full, half, CBox{0, 0, 2559, 2880}).has_value(), false);
    EXPECT(blitTextureCoordinates(full, half, CBox{0.5, 0, 2560, 2880}).has_value(), false);
    EXPECT(blitTextureCoordinates({0, 2880}, half).has_value(), false);
    EXPECT(blitTextureCoordinates(full, {0, 2880}).has_value(), false);
    EXPECT(blitTextureCoordinates(full, half, CBox{std::numeric_limits<double>::quiet_NaN(), 0, 2560, 2880}).has_value(), false);
    const auto uncropped = blitTextureCoordinates(full, full);
    EXPECT(uncropped.has_value(), true);
    if (uncropped) {
        EXPECT((*uncropped == std::array<float, 8>{1, 0, 0, 0, 1, 1, 0, 1}), true);
    }
    return ret;
}
