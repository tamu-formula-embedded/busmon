#ifndef _MAP_PARSER_
#define _MAP_PARSER_

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>

#include "utils.h"

struct CANFrame;

static std::vector<std::string> expected_types({"uint", "int", "float", "double"});

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
using FrameMappings = std::map<uint32_t, std::vector<FrameMapping>>;

class FrameMapParser
{
    /* treemap is intentional -- bounded to human-comprehensible size n */
    FrameMappings mappings{};

    /** Advance past current char and any trailing whitespace */
    inline char IterWS(const char*& it)
    {
        char n = *it++;
        while (is_whitespace(*it)) it++;
        return n;
    }

    /** Report an unexpected character during parsing */
    inline bool Unexpected(const std::string& expected, char unexpected)
    {
        printf("Parsing error: Expected %s, found %c\n", expected.c_str(), unexpected);
        return false;
    }

    /** Report an unexpected EOF during parsing */
    inline bool BadEOF()
    {
        printf("Parsing error: Reached EOF before expected\n");
        return false;
    }

    /** Skip whitespace and comments (line and block) */
    void SkipWSAndComments(const char*& it, const char* end);

    /**
     * Parses the CAN ID at the beginning of a rule.
     * Accepts hex (0x...) or binary (0b...) with optional
     * whitespace breaks for readability.
     */
    bool ExpectID(const char*& it, const char* end, uint32_t& id);

    /**
     * Parses a byte range N:M where N,M are digits in [1,8].
     * Result is stored 0-indexed in first and last.
     */
    bool ExpectRange(const char*& it, const char* end, uint8_t& first, uint8_t& last);

    /**
     * Parses an identifier token.
     * First char must be a-z or A-Z, subsequent chars may
     * also include 0-9 and underscore.
     */
    bool ExpectIdentifier(const char*& it, const char* end, std::string& identifier);

    /** Parses type keyword: uint, int, float, double */
    bool ExpectType(const char*& it, const char* end, std::string& type);

    /** Parses a decimal integer coefficient (must be non-zero) */
    bool ExpectCoef(const char*& it, const char* end, uint64_t& coef);

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
