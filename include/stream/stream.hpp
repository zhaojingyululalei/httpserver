#ifndef __ZHAO_STREAM_HPP_
#define __ZHAO_STREAM_HPP_
#include <memory>
#include "bytestream.hpp"
#include <stddef.h>
namespace zhao
{
    class Stream
    {
    public:
        typedef std::shared_ptr<Stream> ptr;
        virtual ~Stream() {}
        /**
         * @brief 能读多少算多少
         */
        virtual int read(size_t length) = 0;
        /**
         * @brief 必须读够length才能返回
         */
        virtual int readFixSize(size_t length) = 0;
        virtual int write(const char *data, size_t length) = 0;
        virtual int writeFixSize(const void* data, size_t length) = 0;
        virtual void flush(void)  = 0;
    protected:
        ByteStream m_instream;
        ByteStream m_outstream;

    };
} // namespace zhao

#endif