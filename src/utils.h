/**
 * cpputil
 *
 * A collection of utilities that I have been copying
 * from project-to-project for past ~5 years. Finally,
 * it is time to serialize in a "library".
 *
 * This "library" owes much of its origin to my good
 * friend Paul Bailey. He and I worked significantly
 * with this code while working at NASA JSC.
 *
 * Justus Languell     https://www.linkedin.com/in/justusl/
 * Paul Ryan Bailey    https://www.linkedin.com/in/paul-ryan-bailey/
 */
#ifndef __UTILCPP_H__
#define __UTILCPP_H__

#include <stdint.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#define _USE_MATH_DEFINES
#include <sys/time.h>
#include <cmath>

#ifdef __GNUC__
#define vsprintf_s vsnprintf
#define sprintf_s snprintf
#define _strdup strdup
#define _snprintf std::snprintf
#endif

namespace Utils
{
    // typedef int64_t msec_t;

    void BufAppendInt16(uint8_t *buffer, int16_t number, int32_t *index);
    void BufAppendInt32(uint8_t *buffer, int32_t number, int32_t *index);
    void BufAppendFloat16(uint8_t *buffer, float number, float scale, int32_t *index);
    void BufAppendFloat32(uint8_t *buffer, float number, float scale, int32_t *index);

    // Returns a YYYY-MM-DD HH:MM:SS format date for the current day.
    std::string CurrentDateTimeStr(const char *fmt = "%Y-%m-%d %H:%M:%S");

    // A function to extend the functionality of the Java
    // Map interface's getOrDefault method to the C++ map.
    // template<typename K, typename V>
    template <typename K, typename V>
    V MapGetOrDefault(const std::map<K, V> map, const K key, const V default_val)
    {
        auto it = map.find(key);
        return it == map.end() ? default_val : it->second;
    }

    // A function to clamp a given value between an upper and lower bound
    template <typename T>
    T Clamp(T val, T upperBound, T lowerBound)
    {
        return std::min(std::max(val, lowerBound), upperBound);
    }

    // A constant expression function that converts std::string
    // types to char* C strings in variatic argument lists. The
    // function works by evaulating if a given type is of the
    // std::string type. If so, it returns the result of .c_str(),
    // otherwise, it just returns the param. This is designed to
    // be used in variatic argument comprehention.
    template <typename T>
    auto _convert(T &&t)
    {
        if constexpr (std::is_same<std::remove_cv_t<std::remove_reference_t<T>>,
                                   std::string>::value)
            return std::forward<T>(t).c_str();
        else
            return std::forward<T>(t);
    }

    // Internal function to perform a C snprintf style format
    // string. The problem with this function, the reason it's
    // the internal function, is that it cannot take std::string
    // types as arguments, only char* C strings. Wrapping this
    // function solves this problem.
    template <typename... A>
    std::string _strfmt(const std::string &fmt, A &&...args)
    {
        const auto size =
            _snprintf(nullptr, 0, fmt.c_str(), std::forward<A>(args)...) + 1;
        if (size <= 0)
            return "<StrFmt error>";
        std::unique_ptr<char[]> buf(new char[size]);
        _snprintf(buf.get(), size, fmt.c_str(), args...);
        return std::string(buf.get(), buf.get() + size - 1);
    }

    // Formats a string the same way C snprintf does it,
    // returning the formatted string. This method can take
    // std::string objects as arguments (for the %s format).
    template <typename... A>
    std::string StrFmt(const std::string &fmt, A &&...args)
    {
        return _strfmt(fmt, _convert(std::forward<A>(args))...);
    }

    // Prints a string the same way C printf does it, however
    // this method is modernized for C++, and can use std::string
    // objects as arguments (for the %s format).
    template <typename... A>
    void PrintFmt(const std::string &fmt, A &&...args)
    {
        std::cout << _strfmt(fmt, _convert(std::forward<A>(args))...);
    }

    // The same as PrintFmt but has a newline appended to the end.
    template <typename... A>
    void PrintLnFmt(const std::string &fmt, A &&...args)
    {
        std::cout << _strfmt(fmt + "\n", _convert(std::forward<A>(args))...);
    }

    // This one also prints the time at the beginning b/c its a log
    template <typename... A>
    void LogFmt(const std::string &fmt, A &&...args)
    {
        std::cout << _strfmt("[" + CurrentDateTimeStr() + "] " + fmt + "\n",
                             _convert(std::forward<A>(args))...);
    }

    // Handle errors.
    template <typename... A>
    void ErrFmt(const std::string &fmt, A &&...args)
    {
        std::cerr << _strfmt("[" + CurrentDateTimeStr() + "] " + fmt + "\n",
                             _convert(std::forward<A>(args))...);
        std::exit(1);
    }

    // Returns true if element x is present inside of the vector v.
    template <typename T>
    inline bool VecContains(const std::vector<T> v, const T x)
    {
        return std::find(v.begin(), v.end(), x) != v.end();
    }

    // Returns the index of element x if it is present inside of the
    // vector v, otherwise returns -1.
    template <typename T>
    inline long VecIndexOf(const std::vector<T> v, const T x)
    {
        ptrdiff_t pos = find(v.begin(), v.end(), x) - v.begin();
        if (pos >= v.size())
            return -1;
        return pos;
    }

    double ScheduleRate(int rate, std::chrono::high_resolution_clock::time_point start_time);

    double NormalizeAnglePositive(double angle);
    double NormalizeAngle(double angle);
    double ShortestAngularDistance(double from, double to);

    using t_us = std::chrono::microseconds;
    using t_ms = std::chrono::milliseconds;

    template <typename A, typename T>
    inline A PreciseTime()
    {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
        return static_cast<A>(std::chrono::duration_cast<T>(now).count());
    }

}; // namespace Utils

#endif // __UTILCPP_H__