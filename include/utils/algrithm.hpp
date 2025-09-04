#ifndef __ZHAO_ALGORITHM_HPP__
#define __ZHAO_ALGORITHM_HPP__

#include <iostream>

#include <algorithm>
#include "thread/ipc.hpp"
#include "utils/utils.hpp"

namespace zhao
{

    class TokenBucket
    {
    public:
        TokenBucket(int max_tokens, int tokens) : m_max_tokens(max_tokens), m_tokens(tokens)
        {
        }
        virtual bool try_consume() = 0;
        int get_tokens()
        {
            Mutex::MutexLockGuardType lock(m_mutex);
            return m_tokens;
        }
        int get_max_tokens()
        {
            Mutex::MutexLockGuardType lock(m_mutex);
            return m_max_tokens;
        }

    protected:
        Mutex m_mutex;
        int m_max_tokens; // 最大令牌数
        int m_tokens;     // 当前令牌数
    };
    class DynamicTokenBucket : public TokenBucket
    {
    public:
        DynamicTokenBucket(uint64_t a = 10, uint64_t b = 30, int max_tokens = 10)
            : m_a_seconds(a), m_b_seconds(b), TokenBucket(max_tokens, 1)
        {
            if (b < a)
            {
                throw std::invalid_argument("b must be greater than or equal to a");
            }
            m_last_request_time = Utils::Time::getCurrentSec();
        }

        bool try_consume() override;
        size_t calculateRelease(size_t token_count, size_t max_tokens, size_t free_blocks, size_t used_blocks, size_t min_retain = 1);

    private:
        uint64_t m_a_seconds; // 增加令牌的时间窗口
        uint64_t m_b_seconds; // 开始减少令牌的闲置时间
        uint64_t m_last_request_time;
    };

    class Encode
    {
    public:
        static int16_t EncodeZigZag16(int16_t n)
        {
            return (n << 1) ^ (n >> 15);
        }
        static int32_t EncodeZigZag32(int32_t n)
        {
            return (n << 1) ^ (n >> 31);
        }
        static int64_t EncodeZigZag64(int64_t n)
        {
            return (n << 1) ^ (n >> 63);
        }
    };
    class Decode
    {
    public:
        static int16_t DecodeZigZag16(int16_t n)
        {
            return (n >> 1) ^ -(n & 1);
        }
        static int32_t DecodeZigZag32(int32_t n)
        {
            return (n >> 1) ^ -(n & 1);
        }
        static int64_t DecodeZigZag64(int64_t n)
        {
            return (n >> 1) ^ -(n & 1);
        }
    };

} // namespace zhao

#endif