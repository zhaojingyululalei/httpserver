#include "http/httpResponse.hpp"

namespace zhao {

    HttpResponse::HttpResponse(uint8_t version, bool close)
        :m_status(HttpStatus::OK)
        ,m_version(version)
        ,m_close(close)
        ,m_reason("OK") {
    }

    std::string HttpResponse::getHeader(const std::string& key, const std::string& def) const {
        auto it = m_headers.find(key);
        return it == m_headers.end() ? def : it->second;
    }

    void HttpResponse::setHeader(const std::string& key, const std::string& val) {
        m_headers[key] = val;
    }

    void HttpResponse::delHeader(const std::string& key) {
        m_headers.erase(key);
    }

    std::string HttpResponse::toString() const {
        std::stringstream ss;
        ss << "HTTP/"
           << ((uint32_t)(m_version >> 4))
           << "."
           << ((uint32_t)(m_version & 0x0F))
           << " "
           << (uint32_t)m_status
           << " "
           << (m_reason.empty() ? httpStatusToString(m_status) : m_reason)
           << "\r\n";

        for(auto& i : m_headers) {
            ss << i.first << ": " << i.second << "\r\n";
        }

        ss << "\r\n";
        
        if(!m_body.empty()) {
            ss << m_body;
        }
        
        return ss.str();
    }

}

