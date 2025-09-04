#ifndef __UNIX_ADDRESS_HPP_
#define __UNIX_ADDRESS_HPP_
#include "address.hpp"
namespace zhao
{
    class UnixAddress;
    extern std::ostream &operator<<(std::ostream &os, const UnixAddress &addr);
    class UnixAddress : public Address
    {
    public:
        typedef std::shared_ptr<UnixAddress> ptr;
        UnixAddress();
        ~UnixAddress();
        UnixAddress(const std::string& path);

        sockaddr *getSocketAddress()  override
        {
            return (sockaddr*)&m_addr;
        }
        const sockaddr* getSocketAddress()  const override;
        socklen_t getAddrLen() const override;
        void setAddrLen(socklen_t len) 
        {
            m_length = len;
        }
        std::string getPath() const;
        std::string toString() const override;

        friend std::ostream &operator<<(std::ostream &os, const UnixAddress &addr);
    private:
        sockaddr_un m_addr;
        socklen_t m_length;
    };
    
}
#endif