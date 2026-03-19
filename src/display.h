#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <cstdint>
#include <cstdio>
#include <map>
#include <mutex>
#include <string>

#include "packet_mapper.h"

/** ANSI terminal escape helpers */
namespace Term
{
inline void Clear()
{
    fputs("\033[2J", stdout);
}
inline void Home()
{
    fputs("\033[H", stdout);
}
inline void HideCursor()
{
    fputs("\033[?25l", stdout);
}
inline void ShowCursor()
{
    fputs("\033[?25h", stdout);
}
inline void Bold()
{
    fputs("\033[1m", stdout);
}
inline void Dim()
{
    fputs("\033[2m", stdout);
}
inline void Reset()
{
    fputs("\033[0m", stdout);
}
inline void Green()
{
    fputs("\033[32m", stdout);
}
inline void Yellow()
{
    fputs("\033[33m", stdout);
}
inline void Cyan()
{
    fputs("\033[36m", stdout);
}
inline void Red()
{
    fputs("\033[31m", stdout);
}
} // namespace Term

/**
 * Shared state between the rx callback thread and the
 * render loop. Protected by mu.
 */
struct DisplayState
{
    std::mutex                          mu;
    std::map<std::string, MappedPacket> values;
    uint64_t                            rx_count{0};
    bool                                connected{false};
};

/**
 * Redraws the live terminal table showing all mapped
 * CAN values, their units, and age since last update.
 * Age is color-coded: green < 500ms, yellow < 2s, red otherwise.
 */
void RenderTable(const DisplayState& state, const PacketMapper& mapper, uint64_t uptime_ms);

#endif