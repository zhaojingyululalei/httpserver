#include "workqueue.hpp"

namespace zhao
{

    WorkQueue::WorkQueue() {}

    WorkQueue::~WorkQueue() {}

    void WorkQueue::push(Work::ptr work) {
        m_works.push_back(work);
    }

    Work::ptr WorkQueue::pop() {
        if (m_works.empty()) {
            return nullptr;
        }
        Work::ptr work = m_works.front();
        m_works.pop_front();
        return work;
    }

    
    void WorkQueue::push_range(std::list<Work::ptr> &works) {
        m_works.splice(m_works.end(), works);
    }

    
    std::list<Work::ptr> WorkQueue::pop_range(int start, int len) {
        if (start < 0 || len <= 0) {
            return {};
        }
        
        auto first = m_works.begin();
        auto last = m_works.begin();
        
        // 移动到起始位置
        try {
            std::advance(first, start);
        } catch (...) {
            return {}; // 起始位置超出范围
        }
        
        // 移动到结束位置
        try {
            std::advance(last, std::min(start + len, static_cast<int>(m_works.size())));
        } catch (...) {
            last = m_works.end(); // 长度超出范围则取到末尾
        }
        
        std::list<Work::ptr> subList;
        subList.splice(subList.end(), m_works, first, last);
        
        return subList;
    }

    std::list<Work::ptr>  WorkQueue::pop_range()
    {
        pop_range(0, size());
    }
    bool WorkQueue::empty() const {
        return m_works.empty();
    }

    size_t WorkQueue::size() const {
        return m_works.size();
    }


}