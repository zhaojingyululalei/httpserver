#include "address/edian.hpp"

#include "config/configvar.hpp"
#include "edian.hpp"

namespace zhao
{
    Endian::Type g_endian = Endian::getSystemEndian();
    Endian::Type Endian::getSystemEndian()
    {
        uint16_t test = 0x0001;
        if (*reinterpret_cast<uint8_t *>(&test) == 0x01)
        {
            return Endian::_LITTLE_ENDIAN;
        }
        return Endian::_BIG_ENDIAN;
    }
    uint32_t Endian::autoSwap32(uint32_t value)
    {
        // 如果系统是小段，则转成网络序
        if (g_endian == Endian::_LITTLE_ENDIAN)
        {
            return ((value & 0x000000FF) << 24) |
                   ((value & 0x0000FF00) << 8) |
                   ((value & 0x00FF0000) >> 8) |
                   ((value & 0xFF000000) >> 24);
        }
        return value;
    }
    uint16_t Endian::autoSwap16(uint16_t value)
    {
        // 如果系统是小段，则转成网络序
        if (g_endian == Endian::_LITTLE_ENDIAN)
        {
            return (value << 8) | (value >> 8);
        }
        return value;
    }

    uint64_t zhao::Endian::autoSwap64(uint64_t value)
    {
        // 如果系统是小段，则转成网络序
        if (g_endian == Endian::_LITTLE_ENDIAN)
        {
            return ((value & 0x00000000000000FF) << 56) |
                   ((value & 0x000000000000FF00) << 40) |
                   ((value & 0x00000000FF000000) << 24) |
                   ((value & 0x00FF000000000000) << 8) |
                   ((value & 0xFF00000000000000) >> 56) |
                   ((value & 0x00000000FF000000) >> 40) |
                   ((value & 0x0000FF0000000000) >> 24) |
                   ((value & 0x00FF000000000000) >> 8);
        }
        return value;
    }
}
