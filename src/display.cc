#include "display.h"

#include <chrono>

void RenderTable(const DisplayState &state, const PacketMapper &mapper, uint64_t uptime_ms)
{
    Term::Home();

    Term::Bold();
    Term::Cyan();
    printf("  busmon");
    Term::Reset();
    Term::Dim();
    printf("  —  CAN bus monitor\n");
    Term::Reset();

    /* Connection status bar */
    if (state.connected)
    {
        Term::Green();
        printf("  ● CONNECTED");
    }
    else
    {
        Term::Red();
        printf("  ● DISCONNECTED");
    }
    Term::Reset();
    Term::Dim();
    printf("    rx: %-8lu   uptime: %.1fs",
           (unsigned long)state.rx_count, uptime_ms / 1000.0);
    Term::Reset();
    printf("\n\n");

    /* Table header */
    Term::Bold();
    printf("  %-24s %14s  %-8s  %10s\n", "IDENTIFIER", "VALUE", "UNIT", "AGE (ms)");
    Term::Reset();
    Term::Dim();
    printf("  ────────────────────────────────────────────────────────────\n");
    Term::Reset();

    uint32_t now_ms = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());

    /* One row per mapped field, ordered by CAN ID */
    for (const auto &[can_id, submappings] : mapper.GetMappings())
    {
        for (const auto &mapping : submappings)
        {
            auto it = state.values.find(mapping.identifier);
            if (it != state.values.end())
            {
                const MappedPacket &mp = it->second;
                uint32_t age = now_ms - mp.timestamp;

                printf("  %-24s ", mapping.identifier.c_str());
                Term::Bold();
                printf("%14.4f", mp.value);
                Term::Reset();
                printf("  %-8s  ", mapping.unit.c_str());

                if (age < 500)
                    Term::Green();
                else if (age < 2000)
                    Term::Yellow();
                else
                    Term::Red();
                printf("%8u ms", age);
                Term::Reset();
                printf("\n");
            }
            else
            {
                Term::Dim();
                printf("  %-24s %14s  %-8s  %10s\n",
                       mapping.identifier.c_str(), "---", mapping.unit.c_str(), "---");
                Term::Reset();
            }
        }
    }

    /* Padding to clear ghost rows from previous renders */
    for (int i = 0; i < 4; i++)
        printf("%-72s\n", "");
    fflush(stdout);
}