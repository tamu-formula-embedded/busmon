#include "packet_mapper.h"
#include "can_interface.h"

bool is_whitespace(const char ch)
{
    return (ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\r');
}

bool between(const char ch, const char min, const char max)
{
    return ch >= min && ch <= max;
}

std::string PacketMapping::Str()
{
    return Utils::StrFmt("PacketMapping{ range=%d:%d identifier=%s coef=%d unit=%s }", start, end, identifier, coef,
                         unit);
}

bool PacketMapper::ExpectID(fileiter& it, fileiter end, uint32_t& id)
{
    id = 0;
    if (*it == '0') {
        it++;
        if (*it == 'x') {
            IterWS(it);
            while (it != end) {
                if (between(*it, '0', '9')) {
                    id <<= 4;
                    id |= (uint8_t)(*it - '0');
                }
                else if (between(*it, 'A', 'F')) {
                    id <<= 4;
                    id |= (uint8_t)((*it - 'A') + 10);
                }
                else if (between(*it, 'a', 'f')) {
                    id <<= 4;
                    id |= (uint8_t)((*it - 'a') + 10);
                }
                else {
                    // look for {
                    if (*it == '{') return true;
                    return Unexpected("0-9 or a-f or A-F", *it);
                }
                IterWS(it);
            }
            return BadEOF(); // ? did correctly read but hit end,
                             // shouldnt really ever happen but
                             // thats not in the scope of this
                             // functions responsibilites
        }
        else if (*it == 'b') {
            IterWS(it);
            while (it != end) {
                if (*it == '0') { id <<= 1; }
                else if (*it == '1') {
                    id <<= 1;
                    id |= 1;
                }
                else {
                    if (*it == '{') return true;
                    return Unexpected("0 or 1", *it);
                }
                IterWS(it);
            }
            return BadEOF();
        }
        else {
            return Unexpected("x or b", *it);
        }
    }
    else {
        return Unexpected("0", *it);
    }
}

bool PacketMapper::ExpectRange(fileiter& it, fileiter end, uint8_t& first, uint8_t& last)
{
    while (it != end) {
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

bool PacketMapper::ExpectIdentifier(fileiter& it, fileiter end, std::string& identifier)
{
    if (it == end) return BadEOF();
    if (!between(*it, 'a', 'z') && !between(*it, 'A', 'Z')) return Unexpected("a-z or A-Z", *it);
    identifier += *it++;
    while (it != end) {
        if (between(*it, 'a', 'z') || between(*it, 'A', 'Z') || *it == '_' 
			|| between(*it, '0', '9')) identifier += *it++;
        else {
            if (is_whitespace(*it)) IterWS(it);
            return true;
        }
    }
    return BadEOF();
}

bool PacketMapper::ExpectCoef(fileiter& it, fileiter end, uint64_t& coef)
{
    coef = 0;
    while (it != end) {
        if (between(*it, '0', '9')) {
            coef *= 10;
            coef += (uint8_t)(*it - '0');
            it++;
        }
        else {
            if (is_whitespace(*it)) IterWS(it);
            break;
        }
    }
    return coef != 0; // coef must be non-zero, otherwise its not a valid coef.
                      // coef = 0 can also indicate a parsing failure
}

bool PacketMapper::LoadMappings(const std::string& path)
{
    std::ifstream file(path);
    if (!file) {
        std::cout << "Failed to open config file" << std::endl;
        return false;
    }

    fileiter it = std::istreambuf_iterator<char>(file);
    fileiter end = std::istreambuf_iterator<char>();

    while (it != end) {

        uint32_t can_id;
        if (!ExpectID(it, end, can_id)) return false;

        if (*it != '{') return Unexpected("{", *it);
        else IterWS(it);

        std::vector<PacketMapping> mappings;
        while (it != end) {
            std::string identifier;
            if (!ExpectIdentifier(it, end, identifier)) return false;

            if (*it != '=') return Unexpected("=", *it);
            else IterWS(it);

            uint8_t first, last;
            if (!ExpectRange(it, end, first, last)) return false;

            uint64_t coef = 1;
            if (*it == '/') {
                IterWS(it);
                if (!ExpectCoef(it, end, coef)) {
                    std::cout << "[ParsingError] Failed to parse coef or encountered invalid coef.\n";
                    return false;
                }
            }

            std::string unit = "";
            if (*it == '(') {
                IterWS(it);
                while (it != end) {
                    if (*it == ')') {
                        IterWS(it);
                        break;
                    }
                    unit += *it++;
                }
            }

            PacketMapping mapping{first, last, identifier, coef, unit};
            mappings.push_back(mapping);

            if (*it == '}') {
                IterWS(it);
                break;
            }
            else if (*it == ',') {
                IterWS(it);
                continue;
            }
            else {
                return Unexpected(", or }", *it);
            }
        }
        this->mappings[can_id] = mappings; // oh god confusing :scared_emoji:
    }
    return true;
}

void PacketMapper::MapPacket(const CANFrame& packet, std::vector<MappedPacket>& vec)
{
    auto it = mappings.find(packet.id);
    if (it == mappings.end()) return;
    for (const PacketMapping& mapping : it->second) {
        int64_t extracted = 0;
        int width = mapping.end - mapping.start;
        for (int i = mapping.start, j = 0; i <= mapping.end; i++) {
            //extracted |= packet.data[i] << (8 * (j++)); // some endian configuration
            extracted |= packet.data[i] << (8 * (width - (j++))); // other endian configuration
        }
	if (mapping.negable && ((int8_t)packet.data[mapping.start] < 0)) extracted = -extracted;
        double adjusted = extracted / (double)mapping.coef;
	//if (!std::isfinite(adjusted)) adjusted = 0;
	//std::cout << packet.id << " " << adjusted << " " << mapping.identifier << "\n";
	auto mp = MappedPacket(mapping.identifier, adjusted, packet.timestamp);
	// https://stackoverflow.com/a/73372990/11337553
	values.insert_or_assign(mapping.identifier, mp);
	vec.push_back(mp);
    }
}

std::string PacketMapper::Str()
{
    std::string d = "[\n";
    for (const auto& [can_id, submappings] : mappings) {
        d += Utils::StrFmt("  Mapping{ can_id=%04x submappings={\n", can_id);
        for (auto submapping : submappings) { d += "    " + submapping.Str() + "\n"; }
        d += "  } }\n";
    }
    d += "]\n";
    return d;
}

void PacketMapper::LogState(std::ofstream& file, uint64_t global_start_time)
{
    file << "@" << Utils::PreciseTime<uint32_t, Utils::t_ms>() - global_start_time << "\n";
    for (const auto& [key, value] : values) { file << key << "=" << value.value << "\n"; }
}

void TestPacketMapper(PacketMapper& mapper)
{
    /* Designed to test the following config file:

    0xf5ae {
        velocity = 1:4,
        acceleration = 5:8
    }

    0b 1111 1111 {
        voltage = 1:8 / 1000
    }

    */
    uint8_t a[8] = {0};
    int32_t velo = 75080350;
    memcpy(a, &velo, 4);
    int32_t accel = -650290;
    memcpy(a + 4, &accel, 4);
    CANFrame packet1(0xF5AE, a);

    uint8_t b[8] = {0};
    double voltage = 48.312;
    uint64_t ivoltage = (uint64_t)(voltage * 1000);
    memcpy(b, &ivoltage, 8);
    CANFrame packet2(0b11111111, b);

    std::cout << "\nTesting parsing of packets: \n" << packet1.Str() << "\n" << packet2.Str() << "\n";

    std::vector<MappedPacket> updated{};
    mapper.MapPacket(packet1, updated);
    mapper.MapPacket(packet2, updated);
    std::cout << "\n\nValues after parsing: \n";
    mapper.PrintState();
}