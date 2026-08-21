// logger.h -------------------------------------------------------------------------------------------------------
#pragma once

#include "cove/includes.h"

//-----------------------------------------------------------------------------------------------------------------

namespace xeom {

enum class LogLevel
{
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Success
};

//-----------------------------------------------------------------------------------------------------------------

class Logger
{
    static constexpr const char *k_prefix[] = {
        "[TRACE]", "[DEBUG]", "[INFO] ", "[WARN] ", "[ERROR]", "[OK]   "
    };
    static constexpr const char *k_color[] = {
        "\033[90m", "\033[36m", "\033[34m", "\033[33m", "\033[31m", "\033[32m"
    };

public:
template < typename... TArgs>
    static void log( LogLevel lv, std::format_string< TArgs...> fmt, TArgs &&... args)
    {
        auto i = static_cast< size_t>( lv);
        std::cout << k_color[i] << k_prefix[i] << ' '
                  << std::format( fmt, std::forward< TArgs>( args)...) << "\033[0m\n";
    }

template < typename... TArgs>
    static void info( std::format_string< TArgs...> f, TArgs &&... a)
    {
        log( LogLevel::Info, f, std::forward< TArgs>( a)...);
    }

template < typename... TArgs>
    static void debug( std::format_string< TArgs...> f, TArgs &&... a)
    {
        log( LogLevel::Debug, f, std::forward< TArgs>( a)...);
    }

template < typename... TArgs>
    static void warn( std::format_string< TArgs...> f, TArgs &&... a)
    {
        log( LogLevel::Warn, f, std::forward< TArgs>( a)...);
    }

template < typename... TArgs>
    static void error( std::format_string< TArgs...> f, TArgs &&... a)
    {
        log( LogLevel::Error, f, std::forward< TArgs>( a)...);
    }

template < typename... TArgs>
    static void success( std::format_string< TArgs...> f, TArgs &&... a)
    {
        log( LogLevel::Success, f, std::forward< TArgs>( a)...);
    }
};

} // namespace xeom
