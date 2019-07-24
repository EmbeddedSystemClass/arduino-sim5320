#include "Sim5320Client.h"
#include "Sim5320.h"


Sim5320Client::Sim5320Client()
{
    _sim = NULL;
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

int Sim5320Client::connect(IPAddress ip, uint16_t port, int timeout)
{

    return _sim->TCPconnect(String(ip).c_str(), port);
}

int Sim5320Client::connect(const char *host, uint16_t port, int timeout)
{
    return _sim->TCPconnect(host, port);
}

size_t Sim5320Client::write(uint8_t a)
{
    return _sim->TCPsend(&a, 1);
}

size_t Sim5320Client::write(const uint8_t *buf, size_t size)
{
    return _sim->TCPsend(buf, size);
}

int Sim5320Client::available()
{
    return _sim->TCPavailable();
}

int Sim5320Client::read()
{
    uint8_t a;
    _sim->TCPread(&a,1);
    return a;
}

int Sim5320Client::read(uint8_t *buf, size_t size)
{
    return _sim->TCPread(buf,size);
}

int Sim5320Client::peek()
{
    return _sim->TCPpeek();
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