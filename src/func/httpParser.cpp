#include "http/httpParser.hpp"
#include "httpParser.hpp"
#include "log/logger.hpp"
#include "config/configvar.hpp"
#define MODULE_NAME "http"
#define dbg MODULE_DEBUG(MODULE_NAME, LOGGER_GET("system"))
#define warn MODULE_WARN(MODULE_NAME, LOGGER_GET("system"))
#define error MODULE_ERROR(MODULE_NAME, LOGGER_GET("system"))
namespace zhao
{
    //最多提取4KB的请求报文进行解析，超出部分将稍后解析
    static auto g_http_request_buffer_size = zhao::ConfigMgr::getInstance()->add<uint64_t>("http.request.buffer_size", 4 * 1024, "请求报文缓存大小");
    //防止用户恶意请求导致服务器内存耗尽
    static auto g_http_request_max_body_size = zhao::ConfigMgr::getInstance()->add<uint64_t>("http.request.max_body_size", 64 * 1024 * 1024, "请求报文最大体积");
    static auto g_http_response_buffer_size = zhao::ConfigMgr::getInstance()->add<uint64_t>("http.response.buffer_size", 4 * 1024, "响应报文缓存大小");
    static auto g_http_response_max_body_size = zhao::ConfigMgr::getInstance()->add<uint64_t>("http.response.max_body_size", 64 * 1024 * 1024, "响应报文最大体积");

    static uint64_t s_http_request_buffer_size = 0;
    static uint64_t s_http_request_max_body_size = 0;
    static uint64_t s_http_response_buffer_size = 0;
    static uint64_t s_http_response_max_body_size = 0;

    struct ParserIniter
    {
        ParserIniter()
        {
            s_http_request_buffer_size = g_http_request_buffer_size->getValue();
            s_http_request_max_body_size = g_http_request_max_body_size->getValue();
            s_http_response_buffer_size = g_http_response_buffer_size->getValue();
            s_http_response_max_body_size = g_http_response_max_body_size->getValue();

            g_http_request_buffer_size->addListener([](const uint64_t &ov, const uint64_t &nv)
                                                    { s_http_request_buffer_size = nv; });
            g_http_request_max_body_size->addListener([](const uint64_t &ov, const uint64_t &nv)
                                                      { s_http_request_max_body_size = nv; });
            g_http_response_buffer_size->addListener([](const uint64_t &ov, const uint64_t &nv)
                                                     { s_http_response_buffer_size = nv; });
            g_http_response_max_body_size->addListener([](const uint64_t &ov, const uint64_t &nv)
                                                       { s_http_response_max_body_size = nv; });
        }
    };

    static ParserIniter _parser;
    static void on_request_method(void *data, const char *at, size_t length)
    {
        HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
        HttpMethod m = charsToHttpMethod(at);

        if (m == HttpMethod::INVALID_METHOD)
        {
            warn << "invalid http request method: "
                 << std::string(at, length);
            parser->setError(1000);
            return;
        }
        parser->getData()->setMethod(m);
    }
    static void on_request_uri(void *data, const char *at, size_t length)
    {
        // 去on_request_path on_request_path on_request_fragment中做，这里什么都不做
        return;
    }
    static void on_request_fragment(void *data, const char *at, size_t length)
    {
        HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
        parser->getData()->setFragment(std::string(at, length));
    }
    static void on_request_path(void *data, const char *at, size_t length)
    {
        HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
        parser->getData()->setPath(std::string(at, length));
    }
    static void on_request_query(void *data, const char *at, size_t length)
    {
        HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
        parser->getData()->setQuery(std::string(at, length));
    }
    static void on_request_version(void *data, const char *at, size_t length)
    {
        HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
        uint8_t v = 0;
        if (strncmp(at, "HTTP/1.1", length) == 0)
        {
            v = 0x11;
        }
        else if (strncmp(at, "HTTP/1.0", length) == 0)
        {
            v = 0x10;
        }
        else
        {
            warn << "invalid http request version: "
                 << std::string(at, length);
            parser->setError(1001);
            return;
        }
        parser->getData()->setVersion(v);
    }
    static void on_request_header_done(void *data, const char *at, size_t length)
    {
        return;
    }
    static void on_request_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen)
    {
        HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
        if (flen == 0)
        {
            dbg << "invalid http request field length == 0";
            // parser->setError(1002);
            return;
        }
        parser->getData()->setHeader(std::string(field, flen) // key
                                     ,
                                     std::string(value, vlen)); // val
    }
    HttpRequestParser::HttpRequestParser()
        : m_error(0)
    {
        m_data.reset(new HttpRequest); // 解析完成后，把结果放在m_data里
        http_parser_init(&m_parser);   // 初始化解析器
        m_parser.request_method = on_request_method;
        m_parser.request_uri = on_request_uri;
        m_parser.fragment = on_request_fragment;
        m_parser.request_path = on_request_path;
        m_parser.query_string = on_request_query;
        m_parser.http_version = on_request_version;
        m_parser.header_done = on_request_header_done;
        m_parser.http_field = on_request_http_field;
        m_parser.data = this;
    }
    // 1: 成功
    //-1: 有错误
    //>0: 已处理的字节数，且data有效数据为len - v;
    size_t HttpRequestParser::execute(char *data, size_t len)
    {
        // 返回解析了多少字符串
        size_t offset = http_parser_execute(&m_parser, data, len, 0);
        memmove(data, data + offset, (len - offset)); // 把剩余没解析成功的再移到data开头
        return offset;                                // 返回解析了多少
    }
    int HttpRequestParser::isFinished()
    {
        return http_parser_finish(&m_parser);
    }
    int HttpRequestParser::hasError()
    {
        return m_error || http_parser_has_error(&m_parser);
    }
    uint64_t HttpRequestParser::getContentLength()
    {
        return m_data->getHeaderAs<uint64_t>("content-length", 0);
    }

    uint64_t HttpRequestParser::GetHttpRequestBufferSize()
    {
        return s_http_request_buffer_size;
    }
    uint64_t HttpRequestParser::GetHttpRequestMaxBodySize()
    {
        return s_http_request_max_body_size;
    }

    static void on_response_reason(void *data, const char *at, size_t length)
    {
        HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
        parser->getData()->setReason(std::string(at, length));
    }

    static void on_response_status(void *data, const char *at, size_t length)
    {
        HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
        HttpStatus status = (HttpStatus)(atoi(at));
        parser->getData()->setStatus(status);
    }

    static void on_response_chunk(void *data, const char *at, size_t length)
    {
    }

    static void on_response_version(void *data, const char *at, size_t length)
    {
        HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
        uint8_t v = 0;
        if (strncmp(at, "HTTP/1.1", length) == 0)
        {
            v = 0x11;
        }
        else if (strncmp(at, "HTTP/1.0", length) == 0)
        {
            v = 0x10;
        }
        else
        {
            warn << "invalid http response version: "
                 << std::string(at, length);
            parser->setError(1001);
            return;
        }

        parser->getData()->setVersion(v);
    }

    static void on_response_header_done(void *data, const char *at, size_t length)
    {
    }

    static void on_response_last_chunk(void *data, const char *at, size_t length)
    {
    }

    static void on_response_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen)
    {
        HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
        if (flen == 0)
        {
            warn << "invalid http response field length == 0";
            // parser->setError(1002);
            return;
        }
        parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
    }
    HttpResponseParser::HttpResponseParser()
        : m_error(0)
    {
        m_data.reset(new zhao::HttpResponse);
        httpclient_parser_init(&m_parser);
        m_parser.reason_phrase = on_response_reason;
        m_parser.status_code = on_response_status;
        m_parser.chunk_size = on_response_chunk;
        m_parser.http_version = on_response_version;
        m_parser.header_done = on_response_header_done;
        m_parser.last_chunk = on_response_last_chunk;
        m_parser.http_field = on_response_http_field;
        m_parser.data = this;
    }
    // chunck为true，表示上个响应都解析完了，开始一个全新的解析。
    // chunck为false，表示上次没解析完全，接着上次的解析。
    /*HTTP 协议支持一种叫做 Chunked Transfer Encoding 的方式发送响应体。
    它允许服务器将响应体分成多个“块”（chunk）发送，每个块前有大小标识。*/
    size_t HttpResponseParser::execute(char *data, size_t len, bool chunck)
    {
        if (chunck)
        {
            httpclient_parser_init(&m_parser);
        }
        size_t offset = httpclient_parser_execute(&m_parser, data, len, 0);

        memmove(data, data + offset, (len - offset));
        return offset;
    }

    int HttpResponseParser::isFinished()
    {
        return httpclient_parser_finish(&m_parser);
    }

    int HttpResponseParser::hasError()
    {
        return m_error || httpclient_parser_has_error(&m_parser);
    }

    uint64_t HttpResponseParser::getContentLength()
    {
        return m_data->getHeaderAs<uint64_t>("content-length", 0);
    }
    uint64_t HttpResponseParser::GetHttpResponseBufferSize()
    {
        return s_http_response_buffer_size;
    }

    uint64_t HttpResponseParser::GetHttpResponseMaxBodySize()
    {
        return s_http_response_max_body_size;
    }
}