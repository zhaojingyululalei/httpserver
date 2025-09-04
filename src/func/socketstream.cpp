#include "stream/socketstream.hpp"
#include "socket.hpp"
#include <string.h>

namespace zhao
{
    

    int SocketStream::read(size_t length) {
        if(!isConnected()) {
            return -1;
        }
        std::vector<iovec> buffers;
        m_instream.getWriteBuffers(buffers, length);
        if(buffers.empty()) {
            return -1;
        }
        int rt = m_socket->recv(&buffers[0], buffers.size());
        if(rt > 0) {
            m_instream.lseek(rt);
        }
        return rt;
    }

    int SocketStream::readFixSize(size_t length) {
        size_t offset = 0;
        while(offset < length) {
            int rt = read(length - offset);
            if(rt <= 0) {
                return rt;
            }
            offset += rt;
        }
        return length;
    }

    int SocketStream::write(const char* data, size_t length) {
        if(!isConnected()) {
            return -1;
        }
        return m_socket->send(data, length);
    }

    int SocketStream::writeFixSize(const void* data, size_t length) {
        if(!isConnected()) {
            return -1;
        }
        size_t offset = 0;
        while(offset < length) {
            int rt = write(static_cast<const char*>(data) + offset, length - offset);
            if(rt <= 0) {
                return rt;
            }
            offset += rt;
        }
        return length;
    }

    void SocketStream::flush() {
        if(!isConnected()) {
            return;
        }
        std::vector<iovec> buffers;
        m_outstream.getReadBuffers(buffers, m_outstream.getSize());
        if(buffers.empty()) {
            return;
        }
        m_socket->send(&buffers[0], buffers.size());
        m_outstream.clear();
    }

    // 字符串特化实现
    SocketStream& SocketStream::operator>>(std::string& value) {
        value = m_instream.readStringWithSize();
        return *this;
    }

    SocketStream& SocketStream::operator>>(char* value) {
        std::string str = m_instream.readString();
        strcpy(value, str.c_str());
        return *this;
    }

    SocketStream& SocketStream::operator<<(const std::string& value) {
        m_outstream.writeStringWithSize(value.c_str(), value.size());
        if(value.back() == '\n' || m_outstream.getSize() >= 4096) {
            flush();
        }
        return *this;
    }

    SocketStream& SocketStream::operator<<(const char* value) {
        size_t len = strlen(value);
        m_outstream.writeStringWithSize(value, len);
        if(value[len-1] == '\n' || m_outstream.getSize() >= 4096) {
            flush();
        }
        return *this;
    }
} // namespace zhao