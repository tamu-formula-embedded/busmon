#ifndef _MAP_PARSER_
#define _MAP_PARSER_

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "utils.h"

struct CANFrame;

bool is_whitespace(const char ch);

/** between(x, 'a', 'z') ~ 'a' <= x <= 'z' */
bool between(const char ch, const char min, const char max);

/**
 * Encapsulates a single field mapping within a CAN frame.
 * Maps a byte range [start, end] to a named identifier,
 * with an optional divisor (coef) and unit string.
 */
struct FrameMapping
{
    size_t      start;
    size_t      end;
    std::string identifier;
    uint64_t    coef;
    std::string unit;
    bool        negable{false};
    bool        is_little_endian{false};
    bool        is_float{false};

    FrameMapping(size_t start, size_t end, const std::string& identifier, uint64_t coef,
                 const std::string& unit)
        : start(start), end(end), identifier(identifier), coef(coef), unit(unit)
    {
    }

    std::string Str();
};

/**
 *
 */
class FrameMapParser
{
    using fileiter = std::istreambuf_iterator<char>;

    using FrameMappings = std::map<uint32_t, std::vector<FrameMapping>>;

    /* treemap is intentional -- bounded to human-comprehensible size n */
    FrameMappings mappings{};

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
    /** Parse the mapping config from a file */
    bool LoadMappings(const std::string& path);

    /** Returns the mapping tree */
    inline const FrameMappings GetMappings() const
    {
        return mappings;
    }
};

#endif
