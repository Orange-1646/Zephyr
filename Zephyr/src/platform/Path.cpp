#include "Path.h"

namespace Zephyr
{
    std::string Path::rootDir;

    bool Path::SetRootDir(const std::string& path)
    {
        Path::rootDir = path;
        return true;
    }

    std::string Path::GetFilePath(const std::string& pathFromRoot) { return rootDir + pathFromRoot; }

    const std::filesystem::path Path::GetRelativePath(const std::filesystem::path& directory,
                                                      const std::filesystem::path& path)
    {
        return path.lexically_relative(directory);
    }

    const std::vector<std::string> Path::GetPathSegments(const std::filesystem::path& path)
    {
        std::vector<std::string> segments;
        for (auto iter = path.begin(); iter != path.end(); ++iter)
        {
            segments.emplace_back(iter->generic_string());
        }
        return segments;
    }

    const std::tuple<std::string, std::string, std::string> Path::GetFileExtensions(const std::filesystem::path& path)
    {
        return std::make_tuple(path.extension().generic_string(),
                               path.stem().extension().generic_string(),
                               path.stem().stem().extension().generic_string());
    }

    const std::string Path::GetFilePureName(const std::string& fullName)
    {
        std::string name = fullName;
        auto        pos  = fullName.find_first_of('.');
        if (pos != std::string::npos)
        {
            name = fullName.substr(0, pos);
        }

        return name;
    }
} // namespace Zephyr