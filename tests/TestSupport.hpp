#pragma once

#include <cstdlib>
#include <iostream>

#define EMBER_CHECK(expression) do { \
    if (!(expression)) { \
        std::cerr << "[FAIL] " #expression " at " << __FILE__ << ':' << __LINE__ << '\n'; \
        std::exit(EXIT_FAILURE); \
    } \
} while (false)

#ifdef _MSC_VER
#include <crtdbg.h>

// Keep CRT reports visible in CI output without opening modal dialogs.
namespace ember::test
{
struct CrtReportConfiguration
{
    CrtReportConfiguration() noexcept
    {
        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    }
};
inline CrtReportConfiguration crtReportConfiguration{};
}
#endif
