#include "frame_mapper.h"
#include "can_interface.h"

void FrameMapper::MapFrame(const CANFrame& packet)
{
    auto it = mappings.find(packet.id);
    if (it == mappings.end()) return;

    for (const FrameMapping& mapping : it->second)
    {
        int64_t extracted = 0;
        int     width     = mapping.end - mapping.start;

        // handle endianness flag
        if (mapping.is_little_endian)
        {
            for (int i = mapping.start, j = 0; i <= (int)mapping.end; i++)
            {
                extracted |= (int64_t)packet.data[i] << (8 * (j++));
            }
        }
        else
        {
            for (int i = mapping.start, j = 0; i <= (int)mapping.end; i++)
            {
                extracted |= (int64_t)packet.data[i] << (8 * (width - (j++)));
            }
        }

        // handle float flag
        double adjusted_num;
        if (mapping.is_float)
        {
            if (width + 1 == 4)
            {
                float f;
                std::memcpy(&f, &extracted, sizeof(float));
                adjusted_num = f / (double)mapping.coef;
            }
            else if (width + 1 == 8)
            {
                double d;
                std::memcpy(&d, &extracted, sizeof(double));
                adjusted_num = d / (double)mapping.coef;
            }
            else
            {
                // invalid float width they shouldn't have put float
                continue;
            }
        }
        else
        {
            if (mapping.negable && ((int8_t)packet.data[mapping.start] < 0)) extracted = -extracted;
            adjusted_num = extracted / (double)mapping.coef;
        }

        auto mp = FrameEntry(mapping.identifier, adjusted_num, packet.timestamp);
        values.insert_or_assign(mapping.identifier, mp);
    }
}

std::string FrameMapper::Str()
{
    std::string d = "[\n";
    for (const auto& [can_id, submappings] : mappings)
    {
        d += Utils::StrFmt("  Mapping{ can_id=%04x submappings={\n", can_id);
        for (auto submapping : submappings) d += "    " + submapping.Str() + "\n";
        d += "  } }\n";
    }
    d += "]\n";
    return d;
}

void FrameMapper::LogState(std::ofstream& file, uint64_t global_start_time)
{
    file << "@" << Utils::PreciseTime<uint32_t, Utils::t_ms>() - global_start_time << "\n";
    for (const auto& [key, mp] : values) file << key << "=" << mp.value << "\n";
}