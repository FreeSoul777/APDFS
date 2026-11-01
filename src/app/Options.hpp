#pragma once

#include "core/Types.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace apdfs
{

struct Options
{
    std::filesystem::path graphFile;
    VertexIndex source = 0;
    VertexIndex sink = 0;
    size_t threads = 1;
    std::filesystem::path outputDir;
    bool showProgress = false;
    uint64_t maxCuts = 0;
    uint64_t maxExamined = 0;
    size_t maxCutSize = 0;
    bool help = false;
    bool version = false;
};

struct OptionsResult
{
    Options options;
    std::string error;

    bool ok() const noexcept
    {
        return error.empty();
    }
};

OptionsResult parseOptions(int argc, const char* const* argv);

std::string usageText();

} // namespace apdfs
