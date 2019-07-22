#include "Sim5320Client.h"
#include "Sim5320.h"


Sim5320Client::Sim5320Client()
{

}


 Sim5320Client::Sim5320Client(Sim5320& sim)
{
    _sim = &sim;
}

Sim5320Client::~Sim5320Client()
{
    stop();
}

int Sim5320Client::connect(IPAddress ip, uint16_t port)
{
    return _sim->TCPconnect(String(ip).c_str(), port);
}

int Sim5320Client::connect(const char *host, uint16_t port)
{
    return _sim->TCPconnect(host, port);
}

size_t Sim5320Client::write(uint8_t a)
{
    return _sim->TCPsend(&a, 1);
    // AT+CIPSEND=1,1
    // >a
    // Expect
    // OK
    // +CIPSEND: 1,1,1
}

size_t Sim5320Client::write(const uint8_t *buf, size_t size)
{
    return _sim->TCPsend(buf, size);
    // AT+CIPSEND=1,size
    // >buf
    // Expect
    // OK
    // +CIPSEND: 1,size,size
}

int Sim5320Client::available()
{
    return _sim->TCPavailable();
    // AT+CIPRXGET=4
    // Expect
    // +CIPRXGET: 4,size
    return true;
}

int Sim5320Client::read()
{
    uint8_t a;
    _sim->TCPread(&a,1);
    // AT+CIPRXGET=1  // Get data manually and not auto.
    // AT+CIPRXGET=2,1
    // Expect
    // +CIPRXGET: 2,size
    // Data
    return a;
}

int Sim5320Client::read(uint8_t *buf, size_t size)
{
    _sim->TCPread(buf,size);
    // AT+CIPRXGET=1  // Get data manually and not auto.
    // AT+CIPRXGET=2,1
    // Expect
    // +CIPRXGET: 2,size
    // Data
    return 1;
}

int Sim5320Client::peek()
{
    return 0; //todo
}

void Sim5320Client::flush()
{
    while (available())
    {
        uint8_t buffer[500];
        read(buffer, 500);
    }
}

void Sim5320Client::stop()
{
    _sim->TCPclose();
}

uint8_t Sim5320Client::connected()
{
    return _sim->TCPconnected();
}


Sim5320Client::operator bool()
{
    return connected();
}