#include "address/port.hpp"
#include "address/edian.hpp"
namespace zhao
{
    Port::Port(port_t port)
    {
        setPort(port);
    }
    Port::~Port()
    {
    }
    port_t Port::getNetSeqPort() const
    {
        return m_port;
    }
    port_t Port::getHostSeqPort() const
    {
        return Endian::autoSwap(m_port);
    }
    void Port::setPort(port_t port)
    {
        m_port = Endian::autoSwap(port);
    }
    std::string Port::toString() const
    {
        return std::to_string(Endian::autoSwap(m_port));
    }
}