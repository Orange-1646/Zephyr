#pragma once
#include "pch.h"

namespace Zephyr
{
    struct FileSystem
    {
        std::vector<std::filesystem::path> GetFiles(const std::filesystem::path& directory);
    };
}