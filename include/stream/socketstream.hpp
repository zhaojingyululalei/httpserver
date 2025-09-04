/**
 * 
 */
#ifndef __ZHAO_SOCKETSTREAM_HPP_
#define __ZHAO_SOCKETSTREAM_HPP_

#include "stream.hpp"
#include "socket.hpp"
#include <memory>

namespace zhao
{
    class SocketStream : public Stream
    {
    public:
        typedef std::shared_ptr<SocketStream> ptr;
        SocketStream()
        {
        }
        SocketStream(Socket::ptr sock)
            : m_socket(sock)
        {
        }
        virtual ~SocketStream() {}

        virtual int read(size_t length) override;
        virtual int readFixSize(size_t length) override;
        virtual int write(const char *data, size_t length) override;
        virtual int writeFixSize(const void *data, size_t length) override;
        virtual void flush(void) override;

        Socket::ptr getSocket() const { return m_socket; }
        bool isConnected() const { return m_socket && m_socket->isConnected(); }

        // 通用类型如int之类的，全部用通用模板即可匹配
        template<typename T>
        SocketStream& operator>>(T& value) {
            m_instream.read((char*)&value, sizeof(T));
            return *this;
        }
        
        /**
         * @note 字符串遇到换行符结尾会刷新;缓冲区满会刷新;手动调用flush也可以刷新;调用>>输入也会自动刷新。
         */
        template<typename T>
        SocketStream& operator<<(const T& value) {
            m_outstream.write((const char*)&value, sizeof(T));
            if(m_outstream.getSize() >= 4096) { // 缓冲区超过4K就刷新
                flush();
            }
            return *this;
        }

        // 字符串特化，特殊处理
        SocketStream& operator>>(std::string& value);
        SocketStream& operator>>(char* value);
        SocketStream& operator<<(const std::string& value);
        SocketStream& operator<<(const char* value);

    protected:
        Socket::ptr m_socket;
    };

} // namespace zhao

#endif