#ifndef _SIM5320_CLIENT_HEADER_
#define _SIM5320_CLIENT_HEADER_

#include <Sim5320.h>
#include <Client.h>

class Sim5320Client : public Client
{

public:
    Sim5320Client();
    Sim5320Client(Sim5320& _sim);
    ~Sim5320Client();

    uint8_t status();
    virtual int connect(IPAddress ip, uint16_t port);  
    virtual int connect(const char *host, uint16_t port);
    virtual int connect(IPAddress ip, uint16_t port, int timeout=1000);  
    virtual int connect(const char *host, uint16_t port, int timeout=1000);
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *buf, size_t size);
    virtual int available();
    virtual int read();
    virtual int read(uint8_t *buf, size_t size);
    virtual int peek();
    virtual void flush();
    virtual void stop();
    virtual uint8_t connected();
    virtual operator bool();

protected:
    Sim5320*        _sim;
};

#endif //_SIM5320_CLIENT_HEADER_