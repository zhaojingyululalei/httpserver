#include "stream/bytestream.hpp"
#include "log/logger.hpp"
#include "utils/algrithm.hpp"
#include "bytestream.hpp"

#define MODULE_NAME "bytestream"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_GET("system"))
#define warn MODULE_WARN(MODULE_NAME, LOGGER_GET("system"))
#define error MODULE_ERROR(MODULE_NAME, LOGGER_GET("system"))

namespace zhao
{
    ByteStream::ByteStream(size_t nodeSize)
        : m_nodeSize(nodeSize), m_capacity(nodeSize), m_remainSize(nodeSize), m_nodeCnt(0)
    {
        m_head = createNode();
        if (!m_head)
        {
            error << "Failed to create initial node in ByteStream";
            throw std::runtime_error("ByteStream initialization failed");
        }
    }

    ByteStream::~ByteStream()
    {
        while (m_head)
        {
            Node *tmp = m_head;
            m_head = m_head->next;
            delete[] tmp->data;
            pool._delete(tmp); // 使用对象池释放节点
        }
    }

    void ByteStream::write(const char *data, size_t size)
    {
        if (!data || size == 0)
            return;

        
        if(!m_head)
        {
            m_head = createNode();
            if (!m_head)
            {
                error << "Failed to create initial node in ByteStream";
                throw std::runtime_error("ByteStream initialization failed");
            }
            m_capacity = m_nodeSize;
            m_remainSize = m_nodeSize;
            m_size = 0;
            m_position = 0;
        }
        Node* current = m_head;
        size_t remainingSize = size;
        size_t dataOffset = 0;

        // 定位到最后一个节点
        while (current->next)
        {
            current = current->next;
        }

        // 计算当前节点的已使用空间
        size_t usedSpace = m_nodeSize - m_remainSize;

        while (remainingSize > 0)
        {
            // 当前节点还有空间
            if (m_remainSize > 0)
            {
                size_t copySize = std::min(remainingSize, m_remainSize);
                memcpy(current->data + usedSpace, data + dataOffset, copySize);

                m_remainSize -= copySize;
                remainingSize -= copySize;
                dataOffset += copySize;
                m_size += copySize;

                if (remainingSize == 0)
                    break;
            }

            // 需要新节点
            if (m_remainSize == 0)
            {
                Node *newNode = createNode();
                current->next = newNode;
                current = newNode;

                m_capacity += m_nodeSize;
                m_remainSize = m_nodeSize;
                usedSpace = 0;
                
            }
        }
    }

    size_t ByteStream::read(char *buffer, size_t size)
    {
        if (!buffer || size == 0 || m_position >= m_size)
            return 0;

        size_t totalRead = 0;
        size_t remainToRead = std::min(size, m_size - m_position);

        // 找到起始节点和节点内的偏移
        Node *current = m_head;
        size_t nodeOffset = m_position;
        while (nodeOffset >= m_nodeSize)
        {
            current = current->next;
            nodeOffset -= m_nodeSize;
        }

        while (remainToRead > 0 && current)
        {
            size_t available = std::min(remainToRead, m_nodeSize - nodeOffset);
            memcpy(buffer + totalRead, current->data + nodeOffset, available);

            totalRead += available;
            remainToRead -= available;
            nodeOffset = 0; // 后续节点从0开始
            current = current->next;
        }

        m_position += totalRead;
        return totalRead;
    }

    void ByteStream::lseek(int offset)
    {
        if (offset == 0)
            return;

        // 计算新位置
        int64_t newPosition = static_cast<int64_t>(m_position) + offset;

        // 确保新位置不小于0
        if (newPosition < 0)
        {
            m_position = 0;
            return;
        }

        // 如果新位置超过当前大小，扩展流
        if (static_cast<size_t>(newPosition) > m_size)
        {
            size_t extraSize = newPosition - m_size;
            char *zeros = new char[extraSize](); // 初始化为0
            write(zeros, extraSize);
            delete[] zeros;
        }

        m_position = static_cast<size_t>(newPosition);
    }
    void ByteStream::clear()
    {
        while (m_head)
        {
            Node *tmp = m_head;
            m_head = m_head->next;
            delete[] tmp->data;
            pool._delete(tmp); // 使用对象池释放节点
        }
        m_size = 0;
        m_position = 0;
        m_remainSize = 0;
        m_nodeCnt = 0;
        m_capacity = 0;
    }
    void ByteStream::writeInt8(int8_t value)
    {
        write((char *)&value, sizeof(value));
    }
    void ByteStream::writeUint8(uint8_t value)
    {
        write((char *)&value, sizeof(value));
    }
    #define XX(tmpsize) \
    { \
        uint8_t tmp[tmpsize]; \
        uint8_t i = 0; \
        /* Varint编码，尤其能压缩小数据，减少无谓的0 */ \
        /* 只要够1个字节的，就一直循环 */ \
        while(value >= 0x80) { \
            tmp[i++] = (value & 0x7F) | 0x80; /* 取低7位，并将高位置1 */ \
            value >>= 7; \
        } \
        tmp[i++] = value; \
        write((const char*)tmp, i); \
    }
    
    void ByteStream::writeInt16(int16_t value)
    {
        // 先zigzag编码转无符号
        int16_t zigzag = Encode::EncodeZigZag16(value);
        // 然后varint编码后写入
        writeUint16(zigzag);
    }
    void ByteStream::writeUint16(uint16_t value)
    {
        // varint编码后写入
        XX(3);
    }
    void ByteStream::writeInt32(int32_t value)
    {
        // 先zigzag编码转无符号
        int32_t zigzag = Encode::EncodeZigZag32(value);
        // 然后varint编码后写入
        writeUint32(zigzag);
    }
    void ByteStream::writeUint32(uint32_t value)
    {
        // varint编码后写入
        XX(5);
    }
    void ByteStream::writeInt64(int64_t value)
    {
        // 先zigzag编码转无符号
        int64_t zigzag = Encode::EncodeZigZag64(value);
        // 然后varint编码后写入
        writeUint64(zigzag);
    }
    void ByteStream::writeUint64(uint64_t value)
    {
        // varint编码后写入
        XX(10);
    }
    //浮点数不适合用varint编码，高位0并不明显，太随机，直接写入
    void ByteStream::writeFloat(float value)
    {
        write((char *)&value, sizeof(value));
    }
    void ByteStream::writeDouble(double value)
    {
        write((char *)&value, sizeof(value));
    }
    void ByteStream::writeString(const char* str,size_t len)
    {
        write(str,len);
    }
    void ByteStream::writeStringWithSize(const char* str,size_t len)
    {
        writeUint64(len);
        write(str,len);
    }
    #undef XX
    ByteStream::Node *ByteStream::createNode()
    {
        ByteStream::Node *node = static_cast<Node *>(pool._new());
        node->data = new char[m_nodeSize];
        node->size = m_nodeSize;
        node->next = nullptr;
        m_nodeCnt++;
        return node;
    }
    void ByteStream::destroyNode(ByteStream::Node *node)
    {
        if (node){
            delete[] node->data;
            pool._delete(node);
            m_nodeCnt--;
        }
    }

    int8_t ByteStream::readInt8() {
        int8_t value;
        read((char*)&value, sizeof(value));
        return value;
    }

    uint8_t ByteStream::readUint8() {
        uint8_t value;
        read((char*)&value, sizeof(value));
        return value;
    }

    #define XX(type, size) { \
        type result = 0; \
        uint8_t tmp; \
        uint8_t i = 0; \
        do { \
            read((char*)&tmp, 1); \
            result |= (type)((tmp & 0x7F) << (7 * i)); \
            ++i; \
            if(!(tmp & 0x80)) break; \
        } while(i < size); \
        return result; \
    }

    int16_t ByteStream::readInt16() {
        uint16_t zigzag = readUint16();
        return Decode::DecodeZigZag16(zigzag);
    }

    uint16_t ByteStream::readUint16() {
        XX(uint16_t, 3)
    }

    int32_t ByteStream::readInt32() {
        uint32_t zigzag = readUint32();
        return Decode::DecodeZigZag32(zigzag);
    }

    uint32_t ByteStream::readUint32() {
        XX(uint32_t, 5)
    }

    int64_t ByteStream::readInt64() {
        uint64_t zigzag = readUint64();
        return Decode::DecodeZigZag64(zigzag);
    }

    uint64_t ByteStream::readUint64() {
        XX(uint64_t, 10)
    }

    float ByteStream::readFloat() {
        float value;
        read((char*)&value, sizeof(value));
        return value;
    }

    double ByteStream::readDouble() {
        double value;
        read((char*)&value, sizeof(value));
        return value;
    }
    std::string ByteStream::readString()
    {
        std::string result;
        char c;
        while (read((char*)&c, 1) == 1 && c != '\0') {
            result.push_back(c);
        }
        return result;
    }
    std::string ByteStream::readStringWithSize()
    {
        uint64_t len = readUint64();
        std::string result(len, '\0');
        read(&result[0], len);
        return result;
    }
    #undef XX

    uint64_t ByteStream::getReadBuffers(std::vector<iovec>& buffers, uint64_t len) const {
        return getReadBuffers(buffers, len, m_position);
    }

    uint64_t ByteStream::getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const {
        buffers.clear();
        if (position >= m_size) {
            return 0;
        }
        uint64_t readSize = m_size - position;
        if (len > readSize) {
            len = readSize;
        }
        if (len == 0) {
            return 0;
        }
        Node* current = m_head;
        uint64_t nodeOffset = position;
        // 定位到起始节点和节点内偏移
        while (nodeOffset >= m_nodeSize && current) {
            current = current->next;
            nodeOffset -= m_nodeSize;
        }
        uint64_t remain = len;
        while (current && remain > 0) {
            size_t available = m_nodeSize - nodeOffset;
            size_t toRead = std::min<uint64_t>(remain, available);
            iovec iov;
            iov.iov_base = current->data + nodeOffset;
            iov.iov_len = toRead;
            buffers.push_back(iov);
            remain -= toRead;
            current = current->next;
            nodeOffset = 0;
        }
        return len;
    }

    uint64_t ByteStream::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len) {
        buffers.clear();
        if (len == 0) {
            return 0;
        }
        // 扩容
        size_t need = m_position + len;
        while (need > m_capacity) {
            Node* current = m_head;
            while (current->next) {
                current = current->next;
            }
            Node* newNode = createNode();
            current->next = newNode;
            m_capacity += m_nodeSize;
            m_remainSize += m_nodeSize;
        }
        Node* current = m_head;
        size_t nodeOffset = m_position;
        // 定位到起始节点和节点内偏移
        while (nodeOffset >= m_nodeSize && current) {
            current = current->next;
            nodeOffset -= m_nodeSize;
        }
        uint64_t remain = len;
        size_t pos = nodeOffset;
        while (current && remain > 0) {
            size_t available = m_nodeSize - pos;
            size_t toWrite = std::min<uint64_t>(remain, available);
            iovec iov;
            iov.iov_base = current->data + pos;
            iov.iov_len = toWrite;
            buffers.push_back(iov);
            remain -= toWrite;
            current = current->next;
            pos = 0;
        }
        return len;
    }
}
