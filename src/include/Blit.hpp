#pragma once

#include <hyprutils/math/Box.hpp>
#include <array>
#include <cmath>
#include <optional>

namespace Aquamarine {
    // A copy never scales. Validate before importing buffers or changing GL state.
    inline std::optional<std::array<float, 8>> blitTextureCoordinates(const Hyprutils::Math::Vector2D& from, const Hyprutils::Math::Vector2D& to,
                                                                      const std::optional<Hyprutils::Math::CBox>& source = std::nullopt) {
        const auto rect = source.value_or(Hyprutils::Math::CBox{{}, from});
        for (const double value : {from.x, from.y, to.x, to.y, rect.x, rect.y, rect.w, rect.h}) {
            if (!std::isfinite(value) || std::floor(value) != value)
                return std::nullopt;
        }
        if (from.x <= 0 || from.y <= 0 || to.x <= 0 || to.y <= 0 || rect.x < 0 || rect.y < 0 || rect.w != to.x || rect.h != to.y || rect.x + rect.w > from.x ||
            rect.y + rect.h > from.y)
            return std::nullopt;
        const float left = rect.x / from.x, right = (rect.x + rect.w) / from.x;
        const float top = rect.y / from.y, bottom = (rect.y + rect.h) / from.y;
        return std::array<float, 8>{right, top, left, top, right, bottom, left, bottom};
    }
}
