#include "http/httpRequest.hpp"
#include "http/httpResponse.hpp"
#include "http/httpParser.hpp"

static void  test01()
{
    zhao::HttpRequest::ptr req = std::make_shared<zhao::HttpRequest>();
    req->setHeader("Content-Type", "application/json");
    req->setParam("id", "123");
    req->setCookie("session", "abc123");

    std::cout<<req->toString() << std::endl;

    zhao::HttpResponse::ptr res = std::make_shared<zhao::HttpResponse>();
    res->setStatus(zhao::HttpStatus::OK);
    res->setBody("{\"message\": \"Hello, World!\"}");

    std::cout << res->toString() << std::endl;
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
static void test02(void)
{
    zhao::HttpResponseParser parser;
    std::string tmp = test_response_data;
    size_t s = parser.execute(&tmp[0], tmp.size(), true);
    int offset = tmp.size() - s;
    std::string body = tmp.substr(s, offset);
    parser.getData()->setBody(body);
    // std::cout << "execute rt=" << s
    //     << " has_error=" << parser.hasError()
    //     << " is_finished=" << parser.isFinished()
    //     << " total=" << tmp.size()
    //     << " content_length=" << parser.getContentLength()
    //     << " tmp[s]=" << tmp[s]<<std::endl;

    // tmp.resize(tmp.size() - s);

    std::cout << parser.getData()->toString();
    std::cout << tmp;
}
static void test03(void)
{
    zhao::HttpRequestParser parser;
    std::string tmp = test_request_data;
    size_t s = parser.execute(&tmp[0], tmp.size());
    int offset = tmp.size()-  s;
    std::string body = tmp.substr(s, offset);
    parser.getData()->setBody(body);
    parser.getData()->init();
    // std::cout << "execute rt=" << s
    //     << "has_error=" << parser.hasError()
    //     << " is_finished=" << parser.isFinished()
    //     << " total=" << tmp.size()
    //     << " content_length=" << parser.getContentLength();
    //tmp.resize(tmp.size() - s);
    std::cout << parser.getData()->toString()<<std::endl;
    std::cout<<";;;;;;;;;"<<std::endl;
    //std::cout << tmp;
}
#include "server/httpServer.hpp"
static void test04()
{   
    zhao::HttpServer::ptr server(new zhao::HttpServer(true));
   // zhao::Address::ptr addr = zhao::Address::LookupAnyIPAddress("192.168.169.128:8020");
    auto addr = zhao::Address::LookupAny("0.0.0.0:8020",AF_INET,SOCK_STREAM,0);
    server->bind(addr);
    sleep(1);
    server->start();
}
void http_test(void)
{
    //test01();
    //test02();
    //test03();
    test04();
}