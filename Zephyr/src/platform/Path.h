#include "pch.h"

namespace Zephyr
{
    struct Path
    {
        static std::string rootDir;

        static bool SetRootDir(const std::string& path);
        static std::string GetFilePath(const std::string& pathFromRoot);

        static const std::filesystem::path GetRelativePath(const std::filesystem::path& directory,
                                                           const std::filesystem::path& path);

        static const std::vector<std::string> GetPathSegments(const std::filesystem::path& path);

        static const std::tuple<std::string, std::string, std::string>
        GetFileExtensions(const std::filesystem::path& path);

        static const std::string GetFilePureName(const std::string& path);
    };
}