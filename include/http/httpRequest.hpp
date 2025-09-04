#ifndef __ZHAO_HTTPREQUEST_HPP_
#define __ZHAO_HTTPREQUEST_HPP_

#include <memory>
#include <stdint.h>
#include <stddef.h>
#include <map>
#include <string>
#include "http.hpp"
namespace zhao
{
    

    class HttpRequest
    {
    public:
        typedef std::map<std::string, std::string, CaseIgnoreCompare> MapType;

        typedef std::shared_ptr<HttpRequest> ptr;

        /**
         * @brief 默认是非长连接的
         */
        HttpRequest(uint8_t version = 0x11, bool close = true);

        /**获取的还是字符串类型，如果没有返回空字符串 */
        HttpMethod getMethod() const { return m_method; }
        uint8_t getVersion() const { return m_version; }
        std::string getPath() const { return m_path; }
        std::string getQuery() const { return m_query; }
        std::string getFragment() const { return m_fragment; }
        std::string getBody() const { return m_body; }
        MapType getHeaders() const { return m_headers; }
        MapType getParams() const { return m_params; }
        MapType getCookies() const { return m_cookies; }
        std::string getHeader(const std::string &key) ;
        std::string getParam(const std::string &key) ;
        std::string getCookie(const std::string &key) ;
        bool isClose() const { return m_close; }
        /**直接获取值，通过lexical_cast转换 */
        template <class T>
        bool checkGetHeaderAs(const std::string &key, T &val, const T &def = T())
        {
            return checkGetAs(m_headers, key, val, def);
        }
        template <class T>
        T getHeaderAs(const std::string &key, const T &def = T())
        {
            return getAs(m_headers, key, def);
        }
        template <class T>
        bool checkGetParamAs(const std::string &key, T &val, const T &def = T())
        {
            initQueryParam();
            initBodyParam();
            return checkGetAs(m_params, key, val, def);
        }
        template <class T>
        T getParamAs(const std::string &key, const T &def = T())
        {
            initQueryParam();
            initBodyParam();
            return getAs(m_params, key, def);
        }
        template <class T>
        bool checkGetCookieAs(const std::string &key, T &val, const T &def = T())
        {
            initCookies();
            return checkGetAs(m_cookies, key, val, def);
        }
        template <class T>
        T getCookieAs(const std::string &key, const T &def = T())
        {
            initCookies();
            return getAs(m_cookies, key, def);
        }

        void setMethod(HttpMethod method) { m_method = method; }
        void setVersion(uint8_t version) { m_version = version; }
        void setClose(bool close) { m_close = close; }
        void setPath(const std::string &path) { m_path = path; }
        void setQuery(const std::string &query) { m_query = query; }
        void setFragment(const std::string &fragment) { m_fragment = fragment; }
        void setBody(const std::string &body) { m_body = body; }
        void setHeader(const std::string &key, const std::string &val);
        void setParam(const std::string &key, const std::string &val);
        void setCookie(const std::string &key, const std::string &val);

        void delHeader(const std::string &key);
        void delParam(const std::string &key);
        void delCookie(const std::string &key);

        bool hasHeader(const std::string &key);
        bool hasParam(const std::string &key);
        bool hasCookie(const std::string &key);

        std::string toString() const;

        void init();
        void initParam();
        void initQueryParam();
        void initBodyParam();
        void initCookies();
    private:
        /**
         * @brief 检查并获取Map中的值，若不存在则返回false
         */
        template <class T>
        bool checkGetAs(const MapType &m, const std::string &key, T &val, const T &def = T())
        {
            auto it = m.find(key);
            if (it == m.end())
            {
                val = def;
                return false;
            }
            try
            {
                val = boost::lexical_cast<T>(it->second);
                return true;
            }
            catch (...)
            {
                val = def;
            }
            return false;
        }
        /**
         * @brief 获取Map中的值，若不存在则返回默认值
         */
        template <class T>
        T getAs(const MapType &m, const std::string &key, const T &def = T())
        {
            auto it = m.find(key);
            if (it == m.end())
            {
                return def;
            }
            try
            {
                return boost::lexical_cast<T>(it->second); // 基础类型字符串转值
            }
            catch (...)
            {
            }
            return def;
        }

        HttpMethod m_method;
        uint8_t m_version;

        /*http://www.baidu.com/page/hello.html?a=1&b=2#fr*/
        /// 请求路径 /page/hello.html
        std::string m_path;
        /// 请求参数,a=1 b=2
        std::string m_query;
        /// 请求fragment fr  锚点，告诉前端滚到fr元素位置
        std::string m_fragment;
        /// 请求消息体
        std::string m_body;
        /// 请求头部MAP
        MapType m_headers; // 键值对形式
        /// 请求参数MAP
        MapType m_params; // 键值对形式
        /// 请求Cookie MAP  例如Cookie: session_id=abc123
        // 客户端发请求带上session_id，用于身份识别
        MapType m_cookies;
        /// 是否自动关闭
        bool m_close;

        uint8_t m_parserParamFlag;
    };
}
#endif