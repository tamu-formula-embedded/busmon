#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <cstdint>
#include <cstdio>
#include <map>
#include <mutex>
#include <string>

#include "packet_mapper.h"

namespace Term {
    inline void Clear()      { std::fputs("\033[2J", stdout); }
    inline void Home()       { std::fputs("\033[H",  stdout); }
    inline void HideCursor() { std::fputs("\033[?25l", stdout); }
    inline void ShowCursor() { std::fputs("\033[?25h", stdout); }
    inline void Bold()       { std::fputs("\033[1m", stdout); }
    inline void Dim()        { std::fputs("\033[2m", stdout); }
    inline void Reset()      { std::fputs("\033[0m", stdout); }
    inline void Green()      { std::fputs("\033[32m", stdout); }
    inline void Yellow()     { std::fputs("\033[33m", stdout); }
    inline void Cyan()       { std::fputs("\033[36m", stdout); }
    inline void Red()        { std::fputs("\033[31m", stdout); }
}

struct DisplayState {
    std::mutex                          mu;
    std::map<std::string, MappedPacket> values;
    uint64_t                            rx_count{0};
    bool                                connected{false};
};

void RenderTable(const DisplayState& state, const PacketMapper& mapper, uint64_t uptime_ms);

#endif