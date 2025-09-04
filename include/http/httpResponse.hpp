#ifndef __ZHAO_HTTPRESPONSE_HPP_
#define __ZHAO_HTTPRESPONSE_HPP_

#include <memory>
#include <string>
#include <map>
#include <sstream>
#include "http.hpp"

namespace zhao
{
    class HttpResponse {
    public:
        typedef std::shared_ptr<HttpResponse> ptr;
        typedef std::map<std::string, std::string, CaseIgnoreCompare> MapType;

        HttpResponse(uint8_t version = 0x11, bool close = true);

        HttpStatus getStatus() const { return m_status; }
        uint8_t getVersion() const { return m_version; }
        const MapType& getHeaders() const { return m_headers; }
        const std::string& getBody() const { return m_body; }
        const std::string& getReason() const { return m_reason; }
        bool isClose() const { return m_close; }

        void setStatus(HttpStatus status) { m_status = status; }
        void setVersion(uint8_t version) { m_version = version; }
        void setBody(const std::string& body) { m_body = body; }
        void setReason(const std::string& reason) { m_reason = reason; }
        void setClose(bool close) { m_close = close; }

        void setHeader(const std::string& key, const std::string& val);
        void delHeader(const std::string& key);
        std::string getHeader(const std::string& key, const std::string& def = "") const;

        template<class T>
        bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T()) {
            return checkGetAs(m_headers, key, val, def);
        }

        template<class T>
        T getHeaderAs(const std::string& key, const T& def = T()) {
            return getAs(m_headers, key, def);
        }

        std::string toString() const;

    private:
        template<class T>
        bool checkGetAs(const MapType& m, const std::string& key, T& val, const T& def = T()) {
            auto it = m.find(key);
            if(it == m.end()) {
                val = def;
                return false;
            }
            try {
                val = boost::lexical_cast<T>(it->second);
                return true;
            } catch (...) {
                val = def;
            }
            return false;
        }

        template<class T>
        T getAs(const MapType& m, const std::string& key, const T& def = T()) {
            auto it = m.find(key);
            if(it == m.end()) {
                return def;
            }
            try {
                return boost::lexical_cast<T>(it->second);
            } catch (...) {
            }
            return def;
        }

    private:
        /**HTTP/1.1 200 OK */
        HttpStatus m_status;           // 响应状态
        uint8_t m_version;            // 版本
        bool m_close;                 // 是否自动关闭
        std::string m_body;           // 响应体
        std::string m_reason;         // 状态描述
        MapType m_headers;            // 响应头
    };
}

#endif