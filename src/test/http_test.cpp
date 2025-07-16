
#include "http/http.h"
#include "http/http_parser.h"
#include "log.h"
#define dbg ZHAO_LOG_DEBUG(GET_ROOT_LOGGER())
static void test01(void)
{
    zhao::http::HttpRequest::ptr req(new zhao::http::HttpRequest);
    req->setHeader("host" , "www.zhao.top");
    req->setBody("hello zhao");
    req->dump(std::cout) << std::endl;
}
static void test02(void)
{
    zhao::http::HttpResponse::ptr rsp(new zhao::http::HttpResponse);
    rsp->setHeader("X-X", "zhao");
    rsp->setBody("hello zhao");
    rsp->setStatus((zhao::http::HttpStatus)400);
    rsp->setClose(false);

    rsp->dump(std::cout) << std::endl;
}
const char test_request_data[] = "POST / HTTP/1.1\r\n"
                                "Host: www.baidu.com\r\n"
                                "Content-Length: 10\r\n\r\n"
                                "1234567890";
const char test_response_data[] = "HTTP/1.1 200 OK\r\n"
        "Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
        "Server: Apache\r\n"
        "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
        "ETag: \"51-47cf7e6ee8400\"\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length: 81\r\n"
        "Cache-Control: max-age=86400\r\n"
        "Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
        "Connection: Close\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html>\r\n"
        "<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
        "</html>\r\n";                                
static void test03(void)
{
    zhao::http::HttpResponseParser parser;
    std::string tmp = test_response_data;
    size_t s = parser.execute(&tmp[0], tmp.size(), true);
    dbg << "execute rt=" << s
        << " has_error=" << parser.hasError()
        << " is_finished=" << parser.isFinished()
        << " total=" << tmp.size()
        << " content_length=" << parser.getContentLength()
        << " tmp[s]=" << tmp[s];

    tmp.resize(tmp.size() - s);

    dbg << parser.getData()->toString();
    dbg << tmp;
}
static void test04(void)
{
    zhao::http::HttpRequestParser parser;
    std::string tmp = test_request_data;
    size_t s = parser.execute(&tmp[0], tmp.size());
    dbg << "execute rt=" << s
        << "has_error=" << parser.hasError()
        << " is_finished=" << parser.isFinished()
        << " total=" << tmp.size()
        << " content_length=" << parser.getContentLength();
    tmp.resize(tmp.size() - s);
    dbg << parser.getData()->toString();
    dbg << tmp;
}

void http_test(void)
{
    test01();
}