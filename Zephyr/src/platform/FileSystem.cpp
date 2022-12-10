#include "FileSystem.h"

namespace Zephyr
{
    std::vector<std::filesystem::path> FileSystem::GetFiles(const std::filesystem::path& directory)
    {
        std::vector<std::filesystem::path> files;
        for (auto const& entry : std::filesystem::recursive_directory_iterator {directory})
        {
            if (entry.is_regular_file())
            {
                files.push_back(entry);
            }
        }
        return files;
    }
}