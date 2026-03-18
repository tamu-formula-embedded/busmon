#ifndef _PACKET_MAPPER_H_
#define _PACKET_MAPPER_H_

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "utils.h"

struct CANFrame; // forward declaration, defined in can_interface.h

bool is_whitespace(const char ch);

/** between(x, 'a', 'z') ~ 'a' <= x <= 'z' */
bool between(const char ch, const char min, const char max);

/**
 * Encapsulates a single field mapping within a CAN frame.
 * Maps a byte range [start, end] to a named identifier,
 * with an optional divisor (coef) and unit string.
 */
struct PacketMapping
{
    size_t      start;
    size_t      end;
    std::string identifier;
    uint64_t    coef;
    std::string unit;
    bool        negable{false};
    bool        is_little_endian{false};
    bool        is_float{false};

    PacketMapping(size_t start, size_t end, const std::string& identifier, uint64_t coef,
                  const std::string& unit)
        : start(start), end(end), identifier(identifier), coef(coef), unit(unit)
    {
    }

    std::string Str();
};

/**
 * The result of mapping a CANFrame to a human-readable value.
 * Produced by PacketMapper::MapPacket and used for display
 * and network transmission.
 */
struct MappedPacket
{
    std::string identifier;
    double      value;
    uint32_t    timestamp;

    MappedPacket(std::string identifier, double value, uint32_t timestamp)
        : identifier(identifier), value(value), timestamp(timestamp)
    {
    }
};

/**
 * Parses a mapping config file and uses it to decode raw
 * CAN frames into named, human-readable values.
 *
 * Config file format:
 *   0xID {
 *       name = start:end [/ divisor] [(unit)],
 *       ...
 *   }
 */
class PacketMapper
{
    using fileiter = std::istreambuf_iterator<char>;

    /* treemap is intentional -- bounded to human-comprehensible size n */
    std::map<uint32_t, std::vector<PacketMapping>> mappings{};

    /** Advance iterator past the current char and any trailing whitespace */
    inline char IterWS(fileiter& it)
    {
        char n = *it++;
        while (is_whitespace(*it)) it++;
        return n;
    }

    /** Report an unexpected character during parsing */
    inline bool Unexpected(const std::string& expected, char unexpected)
    {
        printf("[ParsingError] Expected %s, found %c\n", expected.c_str(), unexpected);
        return false;
    }

    /** Report an unexpected EOF during parsing */
    inline bool BadEOF()
    {
        printf("[ParsingError] Reached EOF before expected\n");
        return false;
    }

    /**
     * Parses the CAN ID at the beginning of a rule.
     * Accepts hex (0x...) or binary (0b...) with optional
     * whitespace breaks for readability.
     */
    bool ExpectID(fileiter& it, fileiter end, uint32_t& id);

    /**
     * Parses a byte range N:M where N,M are digits in [1,8].
     * Result is stored 0-indexed in first and last.
     */
    bool ExpectRange(fileiter& it, fileiter end, uint8_t& first, uint8_t& last);

    bool ExpectFlag(fileiter& it, fileiter end, bool& is_negable, bool& is_little_endian,
                    bool& is_float);

    /**
     * Parses an identifier token.
     * First char must be a-z or A-Z, subsequent chars may
     * also include 0-9 and underscore.
     */
    bool ExpectIdentifier(fileiter& it, fileiter end, std::string& identifier);

    /** Parses a decimal integer coefficient (must be non-zero) */
    bool ExpectCoef(fileiter& it, fileiter end, uint64_t& coef);

public:
    /** Most recent value for each identifier, updated by MapPacket */
    std::map<std::string, MappedPacket> values{};

    /** Parse the mapping config from a file */
    bool LoadMappings(const std::string& path);

    /**
     * Decode a CAN frame using the loaded mappings.
     * Writes results to both the internal values map
     * and the provided output vector.
     */
    void MapPacket(const CANFrame& packet, std::vector<MappedPacket>& vec);

    /** Returns the mapping tree (for iteration by display code) */
    inline const std::map<uint32_t, std::vector<PacketMapping>>& GetMappings() const
    {
        return mappings;
    }

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

    PacketMapper() {};
};

#endif