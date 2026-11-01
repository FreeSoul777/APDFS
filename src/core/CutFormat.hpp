#pragma once

#include <cstddef>
#include <cstdint>

namespace apdfs
{

inline constexpr uint32_t CUT_FILE_MAGIC = 0x4544484D;
inline constexpr uint32_t CUT_FILE_VERSION = 1;
inline constexpr size_t CUT_FILE_HEADER_WORDS = 4;

} // namespace apdfs
