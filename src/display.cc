#include "display.h"

#include <chrono>

void RenderTable(const DisplayState &state, const PacketMapper &mapper, uint64_t uptime_ms)
{
    Term::Home();

    // Header
    Term::Bold();
    Term::Cyan();
    std::printf("  busmon");
    Term::Reset();
    Term::Dim();
    std::printf("  —  CAN bus monitor\n");
    Term::Reset();

    // Status bar
    if (state.connected)
    {
        Term::Green();
        std::printf("  ● CONNECTED");
    }
    else
    {
        Term::Red();
        std::printf("  ● DISCONNECTED");
    }
    Term::Reset();
    Term::Dim();
    std::printf("    rx: %-8lu   uptime: %.1fs",
                (unsigned long)state.rx_count, uptime_ms / 1000.0);
    Term::Reset();
    std::printf("\n\n");

    // Table header
    Term::Bold();
    std::printf("  %-24s %14s  %-8s  %10s\n",
                "IDENTIFIER", "VALUE", "UNIT", "AGE (ms)");
    Term::Reset();
    Term::Dim();
    std::printf("  ────────────────────────────────────────────────────────────\n");
    Term::Reset();

    uint32_t now_ms = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());

    for (const auto &[can_id, submappings] : mapper.GetMappings())
    {
        for (const auto &mapping : submappings)
        {
            auto it = state.values.find(mapping.identifier);
            if (it != state.values.end())
            {
                const MappedPacket &mp = it->second;
                uint32_t age = now_ms - mp.timestamp;

                std::printf("  %-24s ", mapping.identifier.c_str());
                Term::Bold();
                std::printf("%14.4f", mp.value);
                Term::Reset();
                std::printf("  %-8s  ", mapping.unit.c_str());

                if (age < 500)
                    Term::Green();
                else if (age < 2000)
                    Term::Yellow();
                else
                    Term::Red();
                std::printf("%8u ms", age);
                Term::Reset();
                std::printf("\n");
            }
            else
            {
                Term::Dim();
                std::printf("  %-24s %14s  %-8s  %10s\n",
                            mapping.identifier.c_str(), "---",
                            mapping.unit.c_str(), "---");
                Term::Reset();
            }
        }
    }

    for (int i = 0; i < 4; i++)
        std::printf("%-72s\n", "");
    std::fflush(stdout);
}