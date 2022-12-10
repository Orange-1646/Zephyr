#include "Logger.h"

namespace Zephyr
{
    Logger::Level                   Logger::sLevel  = Logger::Level::Trace;
    std::shared_ptr<spdlog::logger> Logger::sLogger = nullptr;

    void Logger::Init()
    {
        sLogger = std::make_shared<spdlog::logger>("Cosmo");
        SetLogLevel(Level::Info);
    }

    void Logger::Shutdown() { sLogger.reset(); }

} // namespace Cosmo