#ifndef __ZHAO_EDIAN_HPP_
#define __ZHAO_EDIAN_HPP_
#include <stddef.h>
#include <type_traits>
#include <stdint.h>
namespace zhao
{
    namespace Endian
    {
        enum Type
        {
            _BIG_ENDIAN,
            _LITTLE_ENDIAN
        };
        Type getSystemEndian();
        uint32_t autoSwap32(uint32_t value);
        uint16_t autoSwap16(uint16_t value);
        uint64_t autoSwap64(uint64_t value);
        /**
         * @brief 8字节类型的字节序转化
         */
        template <class T>
        typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
        autoSwap(T value)
        {
            return (T)autoSwap64((uint64_t)value);
        }

        /**
         * @brief 4字节类型的字节序转化
         */
        template <class T>
        typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
        autoSwap(T value)
        {
            return (T)autoSwap32((uint32_t)value);
        }

        /**
         * @brief 2字节类型的字节序转化
         */
        template <class T>
        typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
        autoSwap(T value)
        {
            return (T)autoSwap16((uint16_t)value);
        }

        
    }

} // namespace zhao

#endif