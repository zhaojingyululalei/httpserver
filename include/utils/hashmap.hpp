#ifndef __ZHAO_HASHMAP_HPP__
#define __ZHAO_HASHMAP_HPP__
#include "mem/objectpool.hpp"
namespace zhao
{
    template <typename Key, typename Value>
    struct HashMapNode
    {
        Key m_key;
        Value m_value;
        HashMapNode *m_next; // 用于解决哈希冲突的链表
        HashMapNode() = default;  // 添加默认构造函数
        HashMapNode(const Key &key, const Value &value)
            : m_key(key), m_value(value), m_next(nullptr) {}
    };

    template <class Key, class Value, class Hash = std::hash<Key>>
    class HashMap
    {
    private:
        // 使用ObjectPool分配节点
        ObjectPool<HashMapNode<Key, Value>> m_nodePool;

        // 哈希桶数组
        HashMapNode<Key, Value> **m_buckets;

        // 哈希表容量
        size_t m_capacity;

        // 元素数量
        size_t m_size;

        // 哈希函数
        Hash m_hashFunc;

    public:
        // 构造函数
        explicit HashMap(size_t initialCapacity = 16)
            : m_capacity(initialCapacity), m_size(0)
        {
            // 使用ObjectPool分配桶数组
            m_buckets = reinterpret_cast<HashMapNode<Key, Value> **>(
                m_nodePool._new_array(initialCapacity));

            // 初始化桶为nullptr
            for (size_t i = 0; i < m_capacity; ++i)
            {
                m_buckets[i] = nullptr;
            }
        }

        // 析构函数
        ~HashMap()
        {
            clear();
            // 不需要手动释放m_buckets，ObjectPool会管理
        }

        // 插入键值对
        void insert(const Key &key, const Value &value)
        {
            // 检查是否需要扩容
            if (m_size >= m_capacity * 0.7)
            {
                rehash(m_capacity * 2);
            }

            size_t index = m_hashFunc(key) % m_capacity;
            HashMapNode<Key, Value> *node = m_buckets[index];

            // 检查是否已存在相同key
            while (node != nullptr)
            {
                if (node->m_key == key)
                {
                    node->m_value = value; // 更新值
                    return;
                }
                node = node->m_next;
            }

            // 创建新节点
            HashMapNode<Key, Value> *newNode = m_nodePool._new();
            new (newNode) HashMapNode<Key, Value>(key, value);

            // 插入到链表头部
            newNode->m_next = m_buckets[index];
            m_buckets[index] = newNode;

            ++m_size;
        }

        // 查找
        Value *find(const Key &key)
        {
            size_t index = m_hashFunc(key) % m_capacity;
            HashMapNode<Key, Value> *node = m_buckets[index];

            while (node != nullptr)
            {
                if (node->m_key == key)
                {
                    return &node->m_value;
                }
                node = node->m_next;
            }

            return nullptr;
        }

        // 删除
        bool erase(const Key &key)
        {
            size_t index = m_hashFunc(key) % m_capacity;
            HashMapNode<Key, Value> *node = m_buckets[index];
            HashMapNode<Key, Value> *prev = nullptr;

            while (node != nullptr)
            {
                if (node->m_key == key)
                {
                    if (prev == nullptr)
                    {
                        m_buckets[index] = node->m_next;
                    }
                    else
                    {
                        prev->m_next = node->m_next;
                    }

                    m_nodePool._delete(node);
                    --m_size;
                    return true;
                }

                prev = node;
                node = node->m_next;
            }

            return false;
        }

        // 清空哈希表
        void clear()
        {
            for (size_t i = 0; i < m_capacity; ++i)
            {
                HashMapNode<Key, Value> *node = m_buckets[i];
                while (node != nullptr)
                {
                    HashMapNode<Key, Value> *next = node->m_next;
                    m_nodePool._delete(node);
                    node = next;
                }
                m_buckets[i] = nullptr;
            }
            m_size = 0;
        }
         // operator[] 重载 - 支持 map[key] 访问和赋值
    Value& operator[](const Key& key)
    {
        // 先查找是否已存在该key
        Value* value = find(key);
        if (value != nullptr) {
            // 如果存在，直接返回对应的值
            return *value;
        }
        
        // 如果不存在，插入一个默认构造的值
        // 检查是否需要扩容
        if (m_size >= m_capacity * 0.7) {
            rehash(m_capacity * 2);
        }
        
        size_t index = m_hashFunc(key) % m_capacity;
        
        // 创建新节点，使用默认构造的Value
        HashMapNode<Key, Value>* newNode = m_nodePool._new();
        new (newNode) HashMapNode<Key, Value>(key, Value{});
        
        // 插入到链表头部
        newNode->m_next = m_buckets[index];
        m_buckets[index] = newNode;
        
        ++m_size;
        
        return newNode->m_value;
    }
    private:
        // 扩容哈希表
        void rehash(size_t newCapacity)
        {
            // 正确获取数组指针的方式
            HashMapNode<Key, Value> *newArray = m_nodePool._new_array(newCapacity);
            HashMapNode<Key, Value> **newBuckets = reinterpret_cast<HashMapNode<Key, Value> **>(newArray);

            for (size_t i = 0; i < newCapacity; ++i)
            {
                newBuckets[i] = nullptr;
            }

            // 重新哈希所有元素
            for (size_t i = 0; i < m_capacity; ++i)
            {
                HashMapNode<Key, Value> *node = m_buckets[i];
                while (node != nullptr)
                {
                    HashMapNode<Key, Value> *next = node->m_next;

                    size_t newIndex = m_hashFunc(node->m_key) % newCapacity;
                    node->m_next = newBuckets[newIndex];
                    newBuckets[newIndex] = node;

                    node = next;
                }
            }

            m_capacity = newCapacity;
            m_buckets = newBuckets;
        }
    };
}
#endif