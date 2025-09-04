#ifndef __ZHAO_BYTESTREAM_HPP_
#define __ZHAO_BYTESTREAM_HPP_
#include <stddef.h>
#include <stdint.h>
#include <memory>
#include "mem/objectpool.hpp"
namespace zhao
{
    /* Structure for scatter/gather I/O.  */
    struct iovec
    {
        void *iov_base;	/* Pointer to data.  */
        size_t iov_len;	/* Length of data.  */
    };
    class ByteStream
    {
    public:
        typedef std::shared_ptr<ByteStream> ptr;
        ByteStream(size_t nodeSize = 4096);
        ~ByteStream();
        /**
         * @brief 写入数据到ByteStream
         * @details 如果容量不够自动扩容
         */
        void write(const char *data, size_t size);
        /**
         * @brief 从position开始读取数据
         */
        size_t read(char *buffer, size_t size);
        /**
         * @brief 移动position，如果超出当前size，自动扩容并更新position
         */
        void lseek(int offset);
        void clear();
        size_t getSize() const { return m_size; };
        size_t getCapacity() const { return m_capacity; };
        size_t getRemainSize() const { return m_remainSize; };
        size_t getPosition() const { return m_position; };
        size_t getNodeSize() const { return m_nodeSize; };
        size_t getNodeCount() const { return m_nodeCnt; }
        void writeInt8(int8_t value);
        void writeUint8(uint8_t value);
        void writeInt16(int16_t value);
        void writeUint16(uint16_t value);
        void writeInt32(int32_t value);
        void writeUint32(uint32_t value);
        void writeInt64(int64_t value);
        void writeUint64(uint64_t value);
        void writeFloat(float value);
        void writeDouble(double value);
        void writeString(const char* str,size_t len);
        void writeStringWithSize(const char* str,size_t len);
        template <class T>
        void writeArray(const T *array, size_t size)
        {
            for (size_t i = 0; i < size; ++i)
            {
                if constexpr (std::is_same_v<T, int8_t>)
                {
                    writeInt8(array[i]);
                }
                else if constexpr (std::is_same_v<T, uint8_t>)
                {
                    writeUint8(array[i]);
                }
                else if constexpr (std::is_same_v<T, int16_t>)
                {
                    writeInt16(array[i]);
                }
                else if constexpr (std::is_same_v<T, uint16_t>)
                {
                    writeUint16(array[i]);
                }
                else if constexpr (std::is_same_v<T, int32_t>)
                {
                    writeInt32(array[i]);
                }
                else if constexpr (std::is_same_v<T, uint32_t>)
                {
                    writeUint32(array[i]);
                }
                else if constexpr (std::is_same_v<T, int64_t>)
                {
                    writeInt64(array[i]);
                }
                else if constexpr (std::is_same_v<T, uint64_t>)
                {
                    writeUint64(array[i]);
                }
                else if constexpr (std::is_same_v<T, float>)
                {
                    writeFloat(array[i]);
                }
                else if constexpr (std::is_same_v<T, double>)
                {
                    writeDouble(array[i]);
                }
                else
                {
                    static_assert(sizeof(T) == 0, "Unsupported type in writeArray");
                }
            }
        }
        int8_t readInt8();
        uint8_t readUint8();
        int16_t readInt16();
        uint16_t readUint16();
        int32_t readInt32();
        uint32_t readUint32();
        int64_t readInt64();
        uint64_t readUint64();
        float readFloat();
        double readDouble();
        std::string readString();
        std::string readStringWithSize();
        template <class T>
        void readArray(T *array, size_t size)
        {
            for (size_t i = 0; i < size; ++i)
            {
                if constexpr (std::is_same_v<T, int8_t>)
                {
                    array[i] = readInt8();
                }
                else if constexpr (std::is_same_v<T, uint8_t>)
                {
                    array[i] = readUint8();
                }
                else if constexpr (std::is_same_v<T, int16_t>)
                {
                    array[i] = readInt16();
                }
                else if constexpr (std::is_same_v<T, uint16_t>)
                {
                    array[i] = readUint16();
                }
                else if constexpr (std::is_same_v<T, int32_t>)
                {
                    array[i] = readInt32();
                }
                else if constexpr (std::is_same_v<T, uint32_t>)
                {
                    array[i] = readUint32();
                }
                else if constexpr (std::is_same_v<T, int64_t>)
                {
                    array[i] = readInt64();
                }
                else if constexpr (std::is_same_v<T, uint64_t>)
                {
                    array[i] = readUint64();
                }
                else if constexpr (std::is_same_v<T, float>)
                {
                    array[i] = readFloat();
                }
                else if constexpr (std::is_same_v<T, double>)
                {
                    array[i] = readDouble();
                }
                else
                {
                    static_assert(sizeof(T) == 0, "Unsupported type in readArray");
                }
            }
        }
    /**
     * @brief 返回可读取数据大小
     */
    size_t getReadSize() const { return m_size - m_position;}
        /**
     * @brief 获取可读取的缓存,保存成iovec数组
     * @param[out] buffers 保存可读取数据的iovec数组
     * @param[in] len 读取数据的长度,如果len > getReadSize() 则 len = getReadSize()
     * @return 返回实际数据的长度
     */
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const;

    /**
     * @brief 获取可读取的缓存,保存成iovec数组,从position位置开始
     * @param[out] buffers 保存可读取数据的iovec数组
     * @param[in] len 读取数据的长度,如果len > getReadSize() 则 len = getReadSize()
     * @param[in] position 读取数据的位置
     * @return 返回实际数据的长度
     */
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const;

    /**
     * @brief 获取可写入的缓存,保存成iovec数组
     * @param[out] buffers 保存可写入的内存的iovec数组
     * @param[in] len 写入的长度
     * @return 返回实际的长度
     * @post 如果(m_position + len) > m_capacity 则 m_capacity扩容N个节点以容纳len长度
     */
    uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);

    private:
        size_t m_nodeSize;
        size_t m_capacity;
        size_t m_remainSize;
        size_t m_size = 0;
        size_t m_position = 0;
        struct Node
        {
            char *data;
            size_t size;
            Node *next;
        };
        Node *m_head = nullptr;
        size_t m_nodeCnt;
        Node *createNode();
        void destroyNode(Node *node);
        ObjectPool<Node> pool;
    };
} // namespace zhao

#endif