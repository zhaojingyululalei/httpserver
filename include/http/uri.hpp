#ifndef __ZHAO_URI_HPP_
#define __ZHAO_URI_HPP_
#include <string>
namespace zhao
{
    class URI
    {
    public:
        static std::string UrlDecode(const std::string &str, bool space_as_plus=true);
    };
}

#endif