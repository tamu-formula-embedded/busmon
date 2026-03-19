#include "map_parser.h"

bool is_whitespace(const char ch)
{
    return (ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\r');
}

bool between(const char ch, const char min, const char max)
{
    return ch >= min && ch <= max;
}

std::string FrameMapping::Str()
{
    return Utils::StrFmt("FrameMapping{ range=%d:%d identifier=%s coef=%d unit=%s }", start, end, identifier, coef, unit);
}

bool FrameMapParser::ExpectID(fileiter& it, fileiter end, uint32_t& id)
{
    id = 0;
    if (*it == '0')
    {
        it++;
        if (*it == 'x')
        {
            IterWS(it);
            while (it != end)
            {
                if (between(*it, '0', '9'))
                {
                    id <<= 4;
                    id |= (uint8_t)(*it - '0');
                }
                else if (between(*it, 'A', 'F'))
                {
                    id <<= 4;
                    id |= (uint8_t)((*it - 'A') + 10);
                }
                else if (between(*it, 'a', 'f'))
                {
                    id <<= 4;
                    id |= (uint8_t)((*it - 'a') + 10);
                }
                else
                {
                    if (*it == '{') return true;
                    return Unexpected("0-9 or a-f or A-F", *it);
                }
                IterWS(it);
            }
            return BadEOF();
        }
        else if (*it == 'b')
        {
            IterWS(it);
            while (it != end)
            {
                if (*it == '0')
                {
                    id <<= 1;
                }
                else if (*it == '1')
                {
                    id <<= 1;
                    id |= 1;
                }
                else
                {
                    if (*it == '{') return true;
                    return Unexpected("0 or 1", *it);
                }
                IterWS(it);
            }
            return BadEOF();
        }
        else
        {
            return Unexpected("x or b", *it);
        }
    }
    else
    {
        return Unexpected("0", *it);
    }
}

bool FrameMapParser::ExpectRange(fileiter& it, fileiter end, uint8_t& first, uint8_t& last)
{
    while (it != end)
    {
        if (!between(*it, '1', '8')) return Unexpected("1-8", *it);
        first = *it - '1';
        IterWS(it);
        if (*it != ':') return Unexpected("-", *it);
        IterWS(it);
        if (!between(*it, '1', '8')) return Unexpected("1-8", *it);
        last = *it - '1';
        IterWS(it);
        return true;
    }
    return BadEOF();
}

bool FrameMapParser::ExpectFlag(fileiter& it, fileiter end, bool& is_negable, bool& is_little_endian, bool& is_float)
{
    while (it != end && between(*it, 'a', 'z'))
    {
        std::string kw;
        while (it != end && between(*it, 'a', 'z')) kw += *it++;

        if (is_whitespace(*it)) IterWS(it);

        if (kw == "signed") is_negable = true;
        else if (kw == "little") is_little_endian = true;
        else if (kw == "float") is_float = true;
        else
        {
            printf("uknnown keyword bro bro: %s\n", kw.c_str());
            return false;
        }
    }
    return true;
}

bool FrameMapParser::ExpectIdentifier(fileiter& it, fileiter end, std::string& identifier)
{
    if (it == end) return BadEOF();
    if (!between(*it, 'a', 'z') && !between(*it, 'A', 'Z')) return Unexpected("a-z or A-Z", *it);
    identifier += *it++;
    while (it != end)
    {
        if (between(*it, 'a', 'z') || between(*it, 'A', 'Z') || *it == '_' ||
            between(*it, '0', '9'))
            identifier += *it++;
        else
        {
            if (is_whitespace(*it)) IterWS(it);
            return true;
        }
    }
    return BadEOF();
}

bool FrameMapParser::ExpectCoef(fileiter& it, fileiter end, uint64_t& coef)
{
    coef = 0;
    while (it != end)
    {
        if (between(*it, '0', '9'))
        {
            coef *= 10;
            coef += (uint8_t)(*it - '0');
            it++;
        }
        else
        {
            if (is_whitespace(*it)) IterWS(it);
            break;
        }
    }
    return coef != 0; /* coef must be non-zero; 0 also indicates parse failure */
}

bool FrameMapParser::LoadMappings(const std::string& path)
{
    std::ifstream file(path);
    if (!file)
    {
        printf("Failed to open config file: %s\n", path.c_str());
        return false;
    }

    fileiter it  = std::istreambuf_iterator<char>(file);
    fileiter end = std::istreambuf_iterator<char>();

    while (it != end)
    {

        uint32_t can_id;
        if (!ExpectID(it, end, can_id)) return false;

        if (*it != '{') return Unexpected("{", *it);
        else IterWS(it);

        std::vector<FrameMapping> mappings;
        while (it != end)
        {
            std::string identifier;
            if (!ExpectIdentifier(it, end, identifier)) return false;

            if (*it != '=') return Unexpected("=", *it);
            else IterWS(it);

            uint8_t first, last;
            if (!ExpectRange(it, end, first, last)) return false;

            bool is_negable = false, is_little_endian = false, is_float = false;
            if (!ExpectFlag(it, end, is_negable, is_little_endian, is_float)) return false;

            uint64_t coef = 1;
            if (*it == '/')
            {
                IterWS(it);
                if (!ExpectCoef(it, end, coef))
                {
                    printf("[ParsingError] Failed to parse coef or encountered invalid coef.\n");
                    return false;
                }
            }

            std::string unit = "";
            if (*it == '(')
            {
                IterWS(it);
                while (it != end)
                {
                    if (*it == ')')
                    {
                        IterWS(it);
                        break;
                    }
                    unit += *it++;
                }
            }

            FrameMapping mapping{first, last, identifier, coef, unit};
            mapping.negable          = is_negable;
            mapping.is_little_endian = is_little_endian;
            mapping.is_float         = is_float;
            mappings.push_back(mapping);

            if (*it == '}')
            {
                IterWS(it);
                break;
            }
            else if (*it == ',')
            {
                IterWS(it);
                continue;
            }
            else
            {
                return Unexpected(", or }", *it);
            }
        }
        this->mappings[can_id] = mappings;
    }
    return true;
}
