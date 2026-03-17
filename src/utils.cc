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
#include "utils.h"

#include <thread>

/**
 * @brief Appends a 16-bit integer to a buffer at the specified index.
 *
 * This function takes a 16-bit integer, splits it into two 8-bit values, and
 * appends them to a buffer at the specified index. The index is then
 * incremented by 2.
 *
 * @param buffer The buffer to which the integer will be appended.
 * @param number The 16-bit integer to be appended.
 * @param index A pointer to the index at which the integer will be appended.
 * The index is updated after appending the integer.
 */
void Utils::BufAppendInt16(uint8_t *buffer, int16_t number, int32_t *index)
{
    buffer[(*index)++] = number >> 8;
    buffer[(*index)++] = number;
}

/**
 * @brief Appends a 32-bit integer to a buffer at the specified index.
 *
 * This function takes a 32-bit integer, splits it into four 8-bit values, and
 * appends them to a buffer at the specified index. The index is then
 * incremented by 4.
 *
 * @param buffer The buffer to which the integer will be appended. This buffer
 * must be large enough to accommodate the appended values.
 * @param number The 32-bit integer to be appended.
 * @param index A pointer to the index at which the integer will be appended.
 * The index is updated after appending the integer. This pointer must be valid
 * and point to a valid memory location.
 *
 * @return void.
 */
void Utils::BufAppendInt32(uint8_t *buffer, int32_t number, int32_t *index)
{
    buffer[(*index)++] = number >> 24;
    buffer[(*index)++] = number >> 16;
    buffer[(*index)++] = number >> 8;
    buffer[(*index)++] = number;
}

/**
 * @brief Appends a 16-bit floating-point number to a buffer at the specified
 * index, scaled by a given factor.
 *
 * This function takes a floating-point number, scales it by a given factor,
 * converts it to a 16-bit integer, and appends the resulting integer to a
 * buffer at the specified index. The index is then incremented by 2.
 *
 * @param buffer The buffer to which the scaled floating-point number will be
 * appended.
 * @param number The floating-point number to be appended.
 * @param scale The factor by which the floating-point number will be scaled
 * before being converted to a 16-bit integer.
 * @param index A pointer to the index at which the scaled floating-point number
 * will be appended. The index is updated after appending the number.
 *
 * @return void.
 */
void Utils::BufAppendFloat16(uint8_t *buffer, float number, float scale, int32_t *index)
{
    BufAppendInt16(buffer, (int16_t)(number * scale), index);
}

/**
 * @brief Appends a 32-bit floating-point number to a buffer at the specified
 * index, scaled by a given factor.
 *
 * This function takes a floating-point number, scales it by a given factor,
 * converts it to a 32-bit integer, and appends the resulting integer to a
 * buffer at the specified index. The index is then incremented by 4.
 *
 * @param buffer The buffer to which the scaled floating-point number will be
 * appended. This buffer must be large enough to accommodate the appended
 * values.
 * @param number The floating-point number to be appended. This number will be
 * multiplied by the scale factor before being converted to a 32-bit integer.
 * @param scale The factor by which the floating-point number will be scaled
 * before being converted to a 32-bit integer.
 * @param index A pointer to the index at which the scaled floating-point number
 * will be appended. The index is updated after appending the number. This
 * pointer must be valid and point to a valid memory location.
 *
 * @return void.
 */
void Utils::BufAppendFloat32(uint8_t *buffer, float number, float scale, int32_t *index)
{
    BufAppendInt32(buffer, (int32_t)(number * scale), index);
}

std::string Utils::CurrentDateTimeStr(const char *fmt)
{
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), fmt, &tstruct);
    return buf;
}

/**
 * @brief Enforces a rate (Hz) on a loop and returns the time elapsed since the
 * last call.
 *
 * This function calculates the time elapsed since the last call and sleeps if
 * necessary to enforce the specified rate. If the elapsed time is greater than
 * the specified rate, it returns the elapsed time in seconds.
 *
 * @param rate The desired rate in Hz.
 * @param start_time The time point from which to calculate the elapsed time.
 *
 * @return The time elapsed since the last call in seconds.
 */
double Utils::ScheduleRate(
    int rate, std::chrono::high_resolution_clock::time_point start_time)
{
    int dt = std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::high_resolution_clock::now() - start_time)
                 .count();
    if (dt < 1000 / rate)
    {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(int(1000.0 / rate - dt - 2)));
    }
    else
    {
        return dt / 1000.0;
    }
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::high_resolution_clock::now() - start_time)
               .count() /
           1000.0;
}

/*
 *
 *
 */
double Utils::NormalizeAnglePositive(double angle)
{
    return fmod(fmod(angle, (2.0 * M_PI)) + (2.0 * M_PI), (2.0 * M_PI));
}

double Utils::NormalizeAngle(double angle)
{
    double a = NormalizeAnglePositive(angle);
    if (a > M_PI)
        a -= (2.0 * M_PI);
    return a;
}

double Utils::ShortestAngularDistance(double from, double to)
{
    double result = Utils::NormalizeAnglePositive(Utils::NormalizeAnglePositive(to) - Utils::NormalizeAnglePositive(from));
    if (result > M_PI)
        result = -((2.0 * M_PI) - result);
    return Utils::NormalizeAngle(result);
}