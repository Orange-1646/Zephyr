#pragma once
#include <memory.h>
#include <spdlog/spdlog.h>

namespace Zephyr
{

    class Logger final
    {
    public:
        enum class Level : uint8_t
        {
            Trace = 0,
            Info,
            Warning,
            Error,
            Fatal
        };

        static void        Init();
        static void        Shutdown();
        static inline void SetLogLevel(Level level)
        {
            sLevel = level;
            spdlog::set_level(spdlog::level::trace);
        };

        static std::shared_ptr<spdlog::logger>& GetLogger() { return sLogger; }

        template<typename... Args>
        static void Log(Logger::Level level, Args&&... args)
        {
            if (sLevel > level)
            {
                return;
            }

            switch (level)
            {
                case (Level::Trace):
                    spdlog::trace("[Cosmo]Trace: {0}", args...);
                    break;
                case (Level::Info):
                    spdlog::info("[Cosmo]Info: {0}", args...);
                    break;
                case (Level::Warning):
                    spdlog::warn("[Cosmo]Warning: {0}", args...);
                    break;
                case (Level::Error):
                    spdlog::error("[Cosmo]Error: {0}", args...);
                    break;
                case (Level::Fatal):
                    spdlog::critical("[Cosmo]Fatal: {0}", args...);
                    break;
                default:
                    break;
            }
        }

    private:
        static std::shared_ptr<spdlog::logger> sLogger;
        static Level                           sLevel;
    };
} // namespace Cosmo

#define COSMO_TRACE(...) Cosmo::Logger::Log(Cosmo::Logger::Level::Trace, __VA_ARGS__)
#define COSMO_INFO(...) Cosmo::Logger::Log(Cosmo::Logger::Level::Info, __VA_ARGS__)
#define COSMO_WARN(...) Cosmo::Logger::Log(Cosmo::Logger::Level::Warning, __VA_ARGS__)
#define COSMO_ERROR(...) Cosmo::Logger::Log(Cosmo::Logger::Level::Error, __VA_ARGS__)
#define COSMO_FATAL(...) Cosmo::Logger::Log(Cosmo::Logger::Level::Fatal, __VA_ARGS__)