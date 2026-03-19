/**
 * cpputil
 *
 * Justus Languell     https://www.linkedin.com/in/justusl/
 * Paul Ryan Bailey    https://www.linkedin.com/in/paul-ryan-bailey/
 */
#ifndef __UTILCPP_H__
#define __UTILCPP_H__

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>

#ifdef __GNUC__
#define _snprintf std::snprintf
#endif

namespace Utils
{

/**
 * Converts std::string arguments to C strings for use
 * in variadic printf-style formatting. Non-string types
 * are passed through unchanged.
 */
template<typename T> auto _convert(T&& t)
{
    if constexpr (std::is_same<std::remove_cv_t<std::remove_reference_t<T>>, std::string>::value)
        return std::forward<T>(t).c_str();
    else return std::forward<T>(t);
}

/**
 * Internal snprintf wrapper. Cannot accept std::string
 * arguments directly -- use StrFmt instead.
 */
template<typename... A> std::string _strfmt(const std::string& fmt, A&&... args)
{
    const auto size = _snprintf(nullptr, 0, fmt.c_str(), std::forward<A>(args)...) + 1;
    if (size <= 0) return "<StrFmt error>";
    std::unique_ptr<char[]> buf(new char[size]);
    _snprintf(buf.get(), size, fmt.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1);
}

/**
 * Formats a string like snprintf, but returns a std::string
 * and accepts std::string arguments for %s.
 */
template<typename... A> std::string StrFmt(const std::string& fmt, A&&... args)
{
    return _strfmt(fmt, _convert(std::forward<A>(args))...);
}

using t_ms = std::chrono::milliseconds;

/**
 * Returns the current time since epoch, cast to type A,
 * in the precision specified by chrono duration type T.
 *
 * Example: PreciseTime<uint32_t, t_ms>() -> ms since epoch
 */
template<typename A, typename T> inline A PreciseTime()
{
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    return static_cast<A>(std::chrono::duration_cast<T>(now).count());
}

}; // namespace Utils

#endif // __UTILCPP_H__