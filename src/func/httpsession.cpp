#include "http/httpsession.hpp"
#include "http/httpParser.hpp"

namespace zhao
{
    HttpSession::HttpSession(Socket::ptr sock)
        : m_socket(sock), m_stream(sock)
    {
    }

    HttpRequest::ptr HttpSession::recvRequest()
    {
        HttpRequestParser parser;
        std::string buff;
        // 限定缓存长度，防止恶意攻击(把请求报文搞得特别大)。
        uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
        buff.resize(buff_size);
        m_stream >> buff;
        size_t s = parser.execute(&buff[0], buff.size());
        int offset = buff.size() - s;
        std::string body = buff.substr(s, offset);
        parser.getData()->setBody(body);
        parser.getData()->init();
        return parser.getData();
    }

    int HttpSession::sendResponse(HttpResponse::ptr rsp)
    {
        std::string data = rsp->toString();
        m_stream << data;
        return data.size();
    }
}