#ifndef _PACKET_MAPPER_H_
#define _PACKET_MAPPER_H_

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "map_parser.h"
#include "utils.h"

struct CANFrame;

/**
 * The result of mapping a CANFrame to a human-readable value.
 * Produced by FrameMapper::MapPacket and used for display
 * and network transmission.
 */
struct FrameEntry
{
    std::string identifier;
    double      value;
    uint32_t    timestamp;

    FrameEntry(std::string identifier, double value, uint32_t timestamp)
        : identifier(identifier), value(value), timestamp(timestamp)
    {
    }
};

/**
 * Parses a mapping config file and uses it to decode raw
 * CAN frames into named, human-readable values.
 *
 */
class FrameMapper
{
    FrameMappings mappings;

public:
    /** Most recent value for each identifier, updated by MapPacket */
    std::map<std::string, FrameEntry> values{};

    /**
     * Decode a CAN frame using the loaded mappings.
     * Writes results to both the internal values map
     * and the provided output vector.
     */
    void MapFrame(const CANFrame& frame);

    /** Debug string representation of all loaded mappings */
    std::string Str();

    /** Write a timestamped snapshot of all current values to a log file */
    void LogState(std::ofstream& file, uint64_t global_start_time);

    /** Print all current values to stdout */
    inline void PrintState()
    {
        for (const auto& [key, mp] : values)
            printf("%u:%s:%f\n", mp.timestamp, mp.identifier.c_str(), mp.value);
    }

    FrameMapper() {};
};

#endif