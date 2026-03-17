#ifndef _PACKET_MAPPER_H_
#define _PACKET_MAPPER_H_

#include <cstdint>
#include <iostream>
#include <map>
#include <vector>

#include "utils.h"

bool is_whitespace(const char ch);

// between(x, 'a', 'z') ~ 'a' <= x <= 'z'
bool between(const char ch, const char min, const char max);

struct PacketMapping
{ // encapsulates the concept: "identifier": [start, end]
    size_t start;
    size_t end;
    std::string identifier;
    uint64_t coef;
    std::string unit;
    bool negable{true};

    PacketMapping(size_t start, size_t end, const std::string &identifier, uint64_t coef, const std::string &unit)
        : start(start), end(end), identifier(identifier), coef(coef), unit(unit)
    {
    }

    std::string Str();
};

// MappedPacket is the result of mapping a CANFrame to a human
// readable format. This is the result of the mapping process
// and is used to send the data over the network. This is not
// the same as the PacketMapping, which is used to map the
// CANFrame to a human readable format.
struct MappedPacket
{
    std::string identifier;
    double value;
    uint32_t timestamp;

    MappedPacket(std::string identifier, double value, uint32_t timestamp)
        : identifier(identifier), value(value), timestamp(timestamp)
    {
    }

    inline void Stream(std::ostream &os) const
    {
        // os << Utils::StrFmt("{\"identifier\": \"%s\", \"value\": %f, \"timestamp\": %u}", identifier.c_str(), value,
        // timestamp);
        os << timestamp << ":" << identifier << ":" << value;
    }
};

class PacketMapper
{
    using fileiter = std::istreambuf_iterator<char>;
    // treemap is intentional, these are bounded to human
    // ~comprehensable~ size n
    std::map<uint32_t, std::vector<PacketMapping>> mappings{};

    inline char IterWS(fileiter &it)
    { // next and skip whitespace
        char n = *it++;
        while (is_whitespace(*it))
            it++;
        return n;
    }

    // to throw when encountering a character that you dont expect
    inline bool Unexpected(const std::string &expected, char unexpected)
    {
        std::cout << "[ParsingError] Expected " << expected << ", found " << unexpected << "\n";
        return false;
    }

    // to throw when encountering EOF before you would expect
    inline bool BadEOF()
    {
        std::cout << "[ParsingError] Reached EOF before expected\n";
        return false;
    }

    // Parses through the can_id at the beginning of a rule
    // wants hex or binary -- valid forms:
    // - 0xf5ae
    // - 0xE1EC
    // - 0b110011001100
    // *note* whitespace breaks are allowed, usually for
    // making binary formatting easier to read, ex:
    // - 0b 111 101110 110010
    bool ExpectID(fileiter &it, fileiter end, uint32_t &id);

    // Parses through the range #-# where # is a digit in range [1,8]
    // Specifies the start and end byte for a mapping
    // The result is emplaced in first and last as 0 indexed!
    bool ExpectRange(fileiter &it, fileiter end, uint8_t &first, uint8_t &last);

    // Expect an identifier -- rules:
    // - first character: a-z or A-Z
    // - subsequent characters: a-z, A-Z, 0-9, or _ (underscore)
    // - no whitespace breaks
    //
    bool ExpectIdentifier(fileiter &it, fileiter end, std::string &identifier);

    bool ExpectCoef(fileiter &it, fileiter end, uint64_t &coeg);

public:
    inline std::map<uint32_t, std::vector<PacketMapping>> &GetMappings()
    {
        return mappings;
    }

    std::map<std::string, MappedPacket> values{};

    // Parse the mapping tree from the config file at path
    bool LoadMappings(const std::string &path);

    // Take a CANFrame that was parsed from the network and map it
    // based on the config file. This writes the mapped packets to the
    // (a) the values map in this class scope
    // (b) the vector passed in as reference
    void MapPacket(const CANFrame &packet, std::vector<MappedPacket> &vec);

    std::string Str();

    void LogState(std::ofstream &file, uint64_t global_start_time);

    inline void PrintState()
    {
        for (const auto &[key, value] : values)
        {
            value.Stream(std::cout); // when printing we stream each packet
                                     // to stdout
            std::cout << "\n";
        }
    }

    PacketMapper() {};
};

void TestPacketMapper(PacketMapper &mapper);

#endif