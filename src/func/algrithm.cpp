#include "utils/algrithm.hpp"
#include "algrithm.hpp"
#include <cmath>
namespace zhao
{

    bool DynamicTokenBucket::try_consume()
    {
        Mutex::MutexLockGuardType lock(m_mutex);
        uint64_t now = Utils::Time::getCurrentSec();
        uint64_t elapsed = now - m_last_request_time;

        // 更新令牌数
        if (elapsed <= m_a_seconds)
        {
            // 在a秒内再次请求，增加令牌
            m_tokens = std::min(m_tokens + 1, m_max_tokens);
        }
        else if (elapsed > m_b_seconds)
        {
            // 超过b秒没有请求，减少令牌直到1
            uint64_t decrease_intervals = (elapsed ) / m_b_seconds;
            int decrease_amount = static_cast<int>(decrease_intervals);
            m_tokens = std::max(1, m_tokens - decrease_amount);
        }

        m_last_request_time = now;

        if (m_tokens > 0)
        {
            //m_tokens--;
            return true;
        }
        return false;
    }

    size_t DynamicTokenBucket::calculateRelease(size_t token_count,size_t max_tokens, size_t free_blocks, size_t used_blocks, size_t min_retain)
    {
        // 基础参数计算
        //申请频率权重 越高表示最近越频繁申请 → 应少归还
        float token_ratio = static_cast<float>(token_count) / max_tokens;
        //空闲资源权重  越高表示空闲越多 → 可多归还
        float free_ratio = static_cast<float>(free_blocks) / (free_blocks + used_blocks);

        // 动态权重计算（核心逻辑）
        float return_factor = (1.0f - token_ratio) * free_ratio; // 范围[0,1]

        // 计算建议归还量（至少保留min_retain个）
        size_t suggested = static_cast<size_t>(std::round(free_blocks * return_factor));
        size_t max_return = free_blocks - min_retain;
        return std::min(suggested, max_return);
    }
}