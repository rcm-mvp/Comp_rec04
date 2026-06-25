#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace im110 {

using page_id_t = std::int32_t;
using frame_id_t = std::int32_t;

inline constexpr std::size_t PAGE_SIZE = 8192;
inline constexpr page_id_t INVALID_PAGE_ID = -1;
inline constexpr frame_id_t INVALID_FRAME_ID = -1;

using PageBytes = std::array<std::byte, PAGE_SIZE>;

}  // namespace im110
