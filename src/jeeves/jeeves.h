// jeeves.h --------------------------------------------------------------------------------------------------------
// Jeeves — Xeom's lightweight, self-registering unit test framework.
//
// Usage:
//   #include "jeeves/jeeves.h"
//
//   JEEVES_TEST( "MyModule: something works")
//   {
//       JEEVES_CHECK( 1 + 1 == 2);
//       JEEVES_CHECK_MSG( foo() == 42, "foo must return 42");
//   }
//
// Test files are placed alongside the code they cover, e.g. "src/silo/tests/arr_tests.cpp",
// and are only compiled/linked in when the CMake option XEOM_TESTS is ON (default: ON).
// At runtime, tests run when the executable is invoked with "--test [filter]"; if a filter
// string is given, only tests whose name contains that substring are executed.
#pragma once

#include "cove/includes.h"
#include "jeeves/logger.h"

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::jeeves {

using TestFn = void ( *)( void);

struct TestCase
{
    std::string_view name;
    TestFn            fn;
};

class Registry
{
public:
    static Registry &instance( void)
    {
        static Registry reg;
        return reg;
    }

    void add( std::string_view name, TestFn fn)
    {
        m_tests.push_back( { name, fn});
    }

    const std::vector< TestCase> &tests( void) const
    {
        return m_tests;
    }

private:
    std::vector< TestCase> m_tests;
};

struct Registrar
{
    Registrar( std::string_view name, TestFn fn)
    {
        Registry::instance().add( name, fn);
    }
};

// Assertion bookkeeping for the test currently in flight.
inline size_t g_passed_asserts = 0;
inline size_t g_failed_asserts = 0;

inline void record_assert( bool cond, std::string_view expr, const char *file, int line)
{
    if ( cond) {
        ++g_passed_asserts;
    } else {
        ++g_failed_asserts;
        xeom::Logger::error( "  ASSERT FAILED: {} ({}:{})", expr, file, line);
    }
}

// Runs every registered test whose name contains `filter` (empty filter runs everything).
// Returns the number of failed assertions (0 == success), suitable for use as a process exit code.
inline int run( std::string_view filter = {})
{
    g_passed_asserts = 0;
    g_failed_asserts = 0;

    const auto &tests = Registry::instance().tests();

    xeom::Logger::info( "==================== Jeeves Test Framework ====================");
    if ( !filter.empty()) {
        xeom::Logger::info( "Filter: \"{}\"", filter);
    }
    xeom::Logger::info( "Registered tests: {}", tests.size());

    size_t ran = 0;
    for ( const auto &t : tests) {
        if ( !filter.empty() && t.name.find( filter) == std::string_view::npos) {
            continue;
        }
        ++ran;
        xeom::Logger::info( "--- RUN: {} ---", t.name);
        t.fn();
    }

    xeom::Logger::info( "===============================================================");
    xeom::Logger::info( "Tests run: {} / {}", ran, tests.size());
    if ( g_failed_asserts == 0) {
        xeom::Logger::success( "All {} assertions passed!", g_passed_asserts);
    } else {
        xeom::Logger::error( "{} assertion(s) failed ({} passed)", g_failed_asserts, g_passed_asserts);
    }
    return static_cast< int>( g_failed_asserts);
}

} // namespace xeom::jeeves

//-----------------------------------------------------------------------------------------------------------------

#define JEEVES_CONCAT_( a, b) a##b
#define JEEVES_CONCAT( a, b) JEEVES_CONCAT_( a, b)

// Defines and self-registers a test case under `name`. Body follows like a function.
#define JEEVES_TEST( name)                                                                          \
    static void JEEVES_CONCAT( jeeves_test_fn_, __LINE__)( void);                                   \
    namespace {                                                                                     \
    static const ::xeom::jeeves::Registrar JEEVES_CONCAT( jeeves_test_reg_, __LINE__)(              \
        name, &JEEVES_CONCAT( jeeves_test_fn_, __LINE__));                                          \
    }                                                                                                \
    static void JEEVES_CONCAT( jeeves_test_fn_, __LINE__)( void)

#define JEEVES_CHECK( cond) ::xeom::jeeves::record_assert( static_cast< bool>( cond), #cond, __FILE__, __LINE__)

#define JEEVES_CHECK_MSG( cond, msg) \
    ::xeom::jeeves::record_assert( static_cast< bool>( cond), msg, __FILE__, __LINE__)
