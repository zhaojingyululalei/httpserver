#include "http/httpRequest.hpp"
#include "http/uri.hpp"
#include "utils/utils.hpp"
namespace zhao
{

    HttpRequest::HttpRequest(uint8_t version, bool close)
        : m_method(HttpMethod::GET), m_version(version), m_close(close), m_path("/"),m_parserParamFlag(0)
    {
    }
    std::string HttpRequest::getHeader(const std::string &key) 
    {
        auto it = m_headers.find(key);
        return it == m_headers.end() ? "" : it->second;
    }

    std::string HttpRequest::getParam(const std::string &key) 
    {
        initQueryParam();
        initBodyParam();
        auto it = m_params.find(key);
        return it == m_params.end() ? "" : it->second;
    }

    std::string HttpRequest::getCookie(const std::string &key) 
    {
        initCookies();
        auto it = m_cookies.find(key);
        return it == m_cookies.end() ? "" : it->second;
    }

    void HttpRequest::setHeader(const std::string &key, const std::string &val)
    {
        m_headers[key] = val;
    }

    void HttpRequest::setParam(const std::string &key, const std::string &val)
    {
        
        m_params[key] = val;
    }

    void HttpRequest::setCookie(const std::string &key, const std::string &val)
    {
        m_cookies[key] = val;
    }

    void HttpRequest::delHeader(const std::string &key)
    {
        m_headers.erase(key);
    }

    void HttpRequest::delParam(const std::string &key)
    {
        m_params.erase(key);
    }

    void HttpRequest::delCookie(const std::string &key)
    {
        m_cookies.erase(key);
    }

    bool HttpRequest::hasHeader(const std::string &key)
    {
        return m_headers.find(key) != m_headers.end();
    }

    bool HttpRequest::hasParam(const std::string &key)
    {
        initQueryParam();
        initBodyParam();
        return m_params.find(key) != m_params.end();
    }

    bool HttpRequest::hasCookie(const std::string &key)
    {
        initCookies();
        return m_cookies.find(key) != m_cookies.end();
    }

    std::string HttpRequest::toString() const
    {
        std::stringstream ss;
        // 添加请求行
        ss << httpMethodToString(m_method) << " "
           << m_path
           << (m_query.empty() ? "" : "?" + m_query)
           << (m_fragment.empty() ? "" : "#" + m_fragment)
           << " HTTP/"
           << ((uint32_t)(m_version >> 4))
           << "."
           << ((uint32_t)(m_version & 0x0F))
           << "\r\n";
           
           // 添加请求头
           for (auto &i : m_headers)
           {
               ss << i.first << ": " << i.second << "\r\n";
            }
            
            // 添加cookies
            if (!m_cookies.empty())
            {
                ss << "Cookie: ";
                bool first = true;
                for (auto &i : m_cookies)
                {
                    if (!first)
                    {
                        ss << "; ";
                    }
                    first = false;
                    ss << i.first << "=" << i.second;
                }
                ss << "\r\n";
            }
            
            // 空行分隔头部和消息体
            ss << "\r\n";
            
            // 添加消息体
            if (!m_body.empty())
            {
                ss << m_body;
            }
            
            //std::cout <<ss.str()<<std::endl;
            return ss.str();
    }

    void HttpRequest::init()
    {
        std::string conn = getHeader("connection");
        if (!conn.empty())
        {
            if (strcasecmp(conn.c_str(), "keep-alive") == 0)
            {
                m_close = false;
            }
            else
            {
                m_close = true;
            }
        }
    }

    void HttpRequest::initParam()
    {
        initQueryParam();
        initBodyParam();
        initCookies();
    }

    void HttpRequest::initQueryParam()
    {
        if (m_parserParamFlag & 0x1)
        {
            return;
        }

#define PARSE_PARAM(str, m, flag, trim)                                                             \
    size_t pos = 0;                                                                                 \
    do                                                                                              \
    {                                                                                               \
        size_t last = pos;                                                                          \
        pos = str.find('=', pos);                                                                   \
        if (pos == std::string::npos)                                                               \
        {                                                                                           \
            break;                                                                                  \
        }                                                                                           \
        size_t key = pos;                                                                           \
        pos = str.find(flag, pos);                                                                  \
        m.insert(std::make_pair(trim(str.substr(last, key - last)),                                 \
                                zhao::URI::UrlDecode(str.substr(key + 1, pos - key - 1)))); \
        if (pos == std::string::npos)                                                               \
        {                                                                                           \
            break;                                                                                  \
        }                                                                                           \
        ++pos;                                                                                      \
    } while (true);

        PARSE_PARAM(m_query, m_params, '&', );
        m_parserParamFlag |= 0x1;
    }

    void HttpRequest::initBodyParam()
    {
        if (m_parserParamFlag & 0x2)
        {
            return;
        }
        std::string content_type = getHeader("content-type");
        if (strcasestr(content_type.c_str(), "application/x-www-form-urlencoded") == nullptr)
        {
            m_parserParamFlag |= 0x2;
            return;
        }
        PARSE_PARAM(m_body, m_params, '&', );
        m_parserParamFlag |= 0x2;
    }

    void HttpRequest::initCookies()
    {
        if (m_parserParamFlag & 0x4)
        {
            return;
        }
        std::string cookie = getHeader("cookie");
        if (cookie.empty())
        {
            m_parserParamFlag |= 0x4;
            return;
        }
        PARSE_PARAM(cookie, m_cookies, ';', zhao::Utils::Str::Trim);
        m_parserParamFlag |= 0x4;
    }
}