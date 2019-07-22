#include "Sim5320.h"
#define SIM5320_DEBUG 0
#if SIM5320_DEBUG
#define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define DEBUG_PRINT(...) 
#define DEBUG_PRINTLN(...) 
#endif

Sim5320::Sim5320(uint8_t _rst, Stream &_serial)
{
    rstpin = _rst;
    serial = &_serial;
    ok_reply = F("OK");

    pinMode(rstpin, OUTPUT);
    digitalWrite(rstpin, HIGH);
}

Sim5320::~Sim5320()
{
}

void Sim5320::reset()
{
    /*DEBUG_PRINTLN(F("Rebooting device  (4s)"));
    digitalWrite(rstpin, LOW);
    delay(2000);
    digitalWrite(rstpin, HIGH);
    delay(2000);*/
}

bool Sim5320::begin()
{
    int16_t timeout = 7000;

    while (timeout > 0)
    {
        while (serial->available())
            serial->read();
        if (sendCheckReply(F("AT"), ok_reply))
            break;

        while (serial->available())
            serial->read();
        if (sendCheckReply(F("AT"), F("AT")))
            sendCheckReply(F("ATE0"), ok_reply);

        delay(500);
        timeout -= 500;
    }

    if (timeout <= 0)
    {
        DEBUG_PRINTLN(F("Could not communicate with device."));
        return false;
    }

    // turn off Echo!
    sendCheckReply(F("ATE0"), ok_reply);
    delay(100);

    if (!sendCheckReply(F("ATE0"), ok_reply))
    {
        return false;
    }

    // turn on hangupitude
    sendCheckReply(F("AT+CVHU=0"), ok_reply);
    delay(100);
    flushInput();

    return true;
}

bool Sim5320::connect(char *_apn, char *_user, char *_pwd)
{
    if (_apn != NULL)
    {
        apn = F(_apn);
    }
    else
    {
        return false;
    }

    if (_user)
        apnusername = F(_user);
    if (_pwd)
        apnpassword = F(_pwd);

    uint16_t retry = 0;
    delay(100);
    while (!sendCheckReply(F("AT+CGATT=1"), ok_reply, 10000) && ++retry < 10)
    {
        delay(500);
    }

    // set bearer profile access point name
    if (apn)
    {
        // Send command AT+CGSOCKCONT=1,"IP","<apn value>" where <apn value> is the configured APN name.
        if (!sendCheckReplyQuoted(F("AT+CGSOCKCONT=1,\"IP\","), apn, ok_reply, 10000))
            return false;

        // set username/password
        if (apnusername)
        {
            char authstring[100] = "AT+CGAUTH=1,1,\"";
            char *strp = authstring + strlen(authstring);
            prog_char_strcpy(strp, (prog_char *)apnusername);
            strp += prog_char_strlen((prog_char *)apnusername);
            strp[0] = '\"';
            strp++;
            strp[0] = 0;

            if (apnpassword)
            {
                strp[0] = ',';
                strp++;
                strp[0] = '\"';
                strp++;
                prog_char_strcpy(strp, (prog_char *)apnpassword);
                strp += prog_char_strlen((prog_char *)apnpassword);
                strp[0] = '\"';
                strp++;
                strp[0] = 0;
            }

            if (!sendCheckReply(authstring, ok_reply, 10000))
                return false;
        }
    }

    // connect in transparent
    uint16_t cipmode = 0;
    /*sendParseReply(F("AT+CIPMODE?"), F("+CIPMODE: "), &cipmode);

    if (!cipmode)
    {
        if (!sendCheckReply(F("AT+CIPMODE=1"), ok_reply, 10000))
            return false;
    }*/

    // open network (?)
    if (!sendCheckReply(F("AT+CGACT=1"), ok_reply, 10000))
        return false;

    DEBUG_PRINTLN("\t--->AT+NETOPEN");
    serial->println("AT+NETOPEN");
    if (!expectReply(ok_reply))
        return false;
    if (!expectReply(F("+NETOPEN: 0")))
        return false;

    /*sendCheckReply(F("AT+NETOPEN"), F("OK"), 1000);
    readline();
    if(!sendCheckReply(F(""),F("+NETOPEN: 0"),10000))
    {
        return false;
    }*/

    readline(); // eat 'OK'

    return true;
}

bool Sim5320::disconnect(void)
{
    sendCheckReply(F("AT+NETCLOSE"), F("OK"), 1000);
    readline();
    if (!sendCheckReply(F(""), F("+NETCLOSE: 0"), 10000))
    {
        return false;
    }

    return true;
}

int16_t Sim5320::status(void)
{
    uint16_t state;

    if (!sendParseReply(F("AT+CGATT?"), F("+CGATT: "), &state))
        return -1;

    return state;
}

// TCP raw connections

bool Sim5320::TCPconnect(const char *server, uint16_t port)
{
    flushInput();

    // close all old connections
    // single connection at a time
    if (TCPconnected())
    {
        TCPclose();
    }

    // manually read data
    if (!sendCheckReply(F("AT+CIPRXGET=1"), ok_reply))
        return false;

    DEBUG_PRINT(F("\tAT+CIPOPEN=1,\"TCP\",\""));
    DEBUG_PRINT(server);
    DEBUG_PRINT(F("\","));
    DEBUG_PRINT(port);
    DEBUG_PRINTLN(F(""));

    serial->print(F("AT+CIPOPEN=1,\"TCP\",\""));
    serial->print(server);
    serial->print(F("\","));
    serial->print(port);
    serial->println(F(""));

    if (!expectReply(ok_reply))
        return false;
    if (!expectReply(F("+CIPOPEN: 1,0")))
        return false;

    // looks like it was a success (?)
    return true;
}

bool Sim5320::TCPclose(void)
{
    DEBUG_PRINTLN(F("AT+CIPCLOSE=1"));
    serial->println(F("AT+CIPCLOSE=1"));
    if (!expectReply(ok_reply))
        return false;
    if (!expectReply(F("+CIPCLOSE: 1,0")))
        return false;

    return true;
}

bool Sim5320::TCPconnected(void)
{
    uint16_t connected = 0;
    sendParseReply(F("AT+CIPCLOSE?"), F("+CIPCLOSE: 0,"), &connected);

    return connected && true;
}

bool Sim5320::TCPsend(const uint8_t *packet, uint8_t len)
{
    serial->flush();

    DEBUG_PRINT(F("AT+CIPSEND=1,"));
    DEBUG_PRINTLN(len);
#ifdef Sim5320_DEBUG
    for (uint16_t i = 0; i < len; i++)
    {
        DEBUG_PRINT(F(" 0x"));
        DEBUG_PRINT(packet[i], HEX);
    }
#endif
    DEBUG_PRINTLN();

    serial->print(F("AT+CIPSEND=1,"));
    serial->println(len);
    readline();

    DEBUG_PRINT(F("\t<--- "));
    DEBUG_PRINTLN(replybuffer);

    if (replybuffer[0] != '>')
        return false;

    serial->write((const uint8_t *)packet, len);
    readline(1000); // wait up to 3 seconds to send the data
    DEBUG_PRINT(F("\t<--- "));
    DEBUG_PRINTLN(replybuffer);
    readline(1000); // wait up to 3 seconds to send the data
    DEBUG_PRINT(F("\t<--- "));
    DEBUG_PRINTLN(replybuffer);

    return 1;
}

uint16_t Sim5320::TCPavailable(void)
{
    uint16_t avail;

    if (!sendParseReply(F("AT+CIPRXGET=4,1"), F("+CIPRXGET: 4,1,"), &avail, ',', 0))
        return false;

    DEBUG_PRINT(avail);
    DEBUG_PRINTLN(F(" bytes available"));

    return avail;
}

uint16_t Sim5320::TCPread(uint8_t *buff, uint8_t len)
{
    uint16_t avail;

    DEBUG_PRINT("AT+CIPRXGET=2,1,");
    DEBUG_PRINTLN(len);
    serial->print(F("AT+CIPRXGET=2,1,"));
    serial->println(len);
    readline();
    if (!parseReply(F("+CIPRXGET: 2,1,"), &avail, ',', 0))
        return false;

    readRaw(avail);

#ifdef Sim5320_DEBUG
    DEBUG_PRINT(avail);
    DEBUG_PRINTLN(F(" bytes read"));
    for (uint8_t i = 0; i < avail; i++)
    {
        DEBUG_PRINT(F(" 0x"));
        DEBUG_PRINT(replybuffer[i], HEX);
    }
    DEBUG_PRINTLN();
#endif

    memcpy(buff, replybuffer, avail);

    return avail;
}

uint16_t Sim5320::TCPpeek()
{

    return 0;
}

int Sim5320::available(void)
{
    return serial->available();
}

size_t Sim5320::write(uint8_t a)
{
    return serial->write(a);
}

int Sim5320::read(void)
{
    return serial->read();
}

int Sim5320::peek(void)
{
    return serial->peek();
}

void Sim5320::flush(void)
{
    serial->flush();
}


//********* RTC *********
bool Sim5320::readRTC(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *hr, uint8_t *min, uint8_t *sec)
{
    sendParseReply(F("AT+CCLK?"),F("+CCLK: "),year,'/',0);
}

//********* BATTERY & ADC *********

/* returns value in mV (uint16_t) */
bool Sim5320::getBattVoltage(uint16_t *v)
{
    float f;
    bool b = sendParseReply(F("AT+CBC"), F("+CBC: "), &f, ',', 2);
    *v = f * 1000;
    return b;
}

/* returns the percentage charge of battery as reported by sim800 */
bool Sim5320::getBattPercent(uint16_t *p)
{
    return sendParseReply(F("AT+CBC"), F("+CBC: "), p, ',', 1);
}

bool Sim5320::getADCVoltage(uint16_t *v)
{
    return sendParseReply(F("AT+CADC?"), F("+CADC: 1,"), v);
}

// GPS

bool Sim5320::enableGPS(bool onoff)
{
    uint16_t state;

    // first check if its already on or off
    if (!sendParseReply(F("AT+CGPS?"), F("+CGPS: "), &state))
        return false;

    if (onoff && !state)
    {
        if (!sendCheckReply(F("AT+CGPS=1"), ok_reply))
            return false;
    }
    else if (!onoff && state)
    {
        if (!sendCheckReply(F("AT+CGPS=0"), ok_reply))
            return false;
        // this takes a little time
        readline(2000); // eat '+CGPS: 0'
    }
    return true;
}

int8_t Sim5320::GPSstatus(void)
{
    getReply(F("AT+CGPSINFO"));
    char *p = prog_char_strstr(replybuffer, (prog_char *)F("+CGPSINFO:"));
    if (p == 0)
        return -1;
    if (p[10] != ',')
        return 3;
    return 0;
}

uint8_t Sim5320::getGPS(uint8_t arg, char *buffer, uint8_t maxbuff)
{
    int32_t x = arg;

    getReply(F("AT+CGPSINFO"));

    char *p = prog_char_strstr(replybuffer, (prog_char *)F("SINF"));
    if (p == 0)
    {
        buffer[0] = 0;
        return 0;
    }

    p += 6;

    uint8_t len = max(maxbuff - 1, (int)strlen(p));
    strncpy(buffer, p, len);
    buffer[len] = 0;

    readline(); // eat 'OK'
    return len;
}

bool Sim5320::getGPS(float *lat, float *lon, float *speed_kph, float *heading, float *altitude)
{

    char gpsbuffer[120];

    // we need at least a 2D fix
    if (GPSstatus() < 2)
        return false;

    // grab the mode 2^5 gps csv from the sim808
    uint8_t res_len = getGPS(32, gpsbuffer, 120);

    // make sure we have a response
    if (res_len == 0)
        return false;

    // Parse 3G respose
    // +CGPSINFO:4043.000000,N,07400.000000,W,151015,203802.1,-12.0,0.0,0
    // skip beginning

    // grab the latitude
    char *latp = strtok(gpsbuffer, ",");
    if (!latp)
        return false;

    // grab latitude direction
    char *latdir = strtok(NULL, ",");
    if (!latdir)
        return false;

    // grab longitude
    char *longp = strtok(NULL, ",");
    if (!longp)
        return false;

    // grab longitude direction
    char *longdir = strtok(NULL, ",");
    if (!longdir)
        return false;

    // skip date & time
    char *tok;
    tok = strtok(NULL, ",");
    tok = strtok(NULL, ",");

    // only grab altitude if needed
    if (altitude != NULL)
    {
        // grab altitude
        char *altp = strtok(NULL, ",");
        if (!altp)
            return false;
        *altitude = atof(altp);
    }

    // only grab speed if needed
    if (speed_kph != NULL)
    {
        // grab the speed in km/h
        char *speedp = strtok(NULL, ",");
        if (!speedp)
            return false;

        *speed_kph = atof(speedp);
    }

    // only grab heading if needed
    if (heading != NULL)
    {

        // grab the speed in knots
        char *coursep = strtok(NULL, ",");
        if (!coursep)
            return false;

        *heading = atof(coursep);
    }

    double latitude = atof(latp);
    double longitude = atof(longp);

    // convert latitude from minutes to decimal
    float degrees = floor(latitude / 100);
    double minutes = latitude - (100 * degrees);
    minutes /= 60;
    degrees += minutes;

    // turn direction into + or -
    if (latdir[0] == 'S')
        degrees *= -1;

    *lat = degrees;

    // convert longitude from minutes to decimal
    degrees = floor(longitude / 100);
    minutes = longitude - (100 * degrees);
    minutes /= 60;
    degrees += minutes;

    // turn direction into + or -
    if (longdir[0] == 'W')
        degrees *= -1;

    *lon = degrees;

    return true;
}

bool Sim5320::enableGPSNMEA(uint8_t i)
{

    char sendbuff[15] = "AT+CGPSOUT=000";
    sendbuff[11] = (i / 100) + '0';
    i %= 100;
    sendbuff[12] = (i / 10) + '0';
    i %= 10;
    sendbuff[13] = i + '0';

    return sendCheckReply(sendbuff, ok_reply, 2000);
}



/********* HELPERS *********************************************/

bool Sim5320::expectReply(FlashStringPtr reply,
                          uint16_t timeout)
{
    readline(timeout);

    DEBUG_PRINT(F("\t<--- "));
    DEBUG_PRINTLN(replybuffer);

    return (prog_char_strcmp(replybuffer, (prog_char *)reply) == 0);
}

void Sim5320::flushInput()
{
    // Read all available serial input to flush pending data.
    uint16_t timeoutloop = 0;
    while (timeoutloop++ < 40)
    {
        while (available())
        {
            read();
            timeoutloop = 0; // If char was received reset the timer
        }
        delay(1);
    }
}

uint16_t Sim5320::readRaw(uint16_t b)
{
    uint16_t idx = 0;

    while (b && (idx < sizeof(replybuffer) - 1))
    {
        if (serial->available())
        {
            replybuffer[idx] = serial->read();
            idx++;
            b--;
        }
    }
    replybuffer[idx] = 0;

    return idx;
}

uint8_t Sim5320::readline(uint16_t timeout, bool multiline)
{
    uint16_t replyidx = 0;

    while (timeout--)
    {
        if (replyidx >= 254)
        {
            //DEBUG_PRINTLN(F("SPACE"));
            break;
        }

        while (serial->available())
        {
            char c = serial->read();
            if (c == '\r')
                continue;
            if (c == 0xA)
            {
                if (replyidx == 0) // the first 0x0A is ignored
                    continue;

                if (!multiline)
                {
                    timeout = 0; // the second 0x0A is the end of the line
                    break;
                }
            }
            replybuffer[replyidx] = c;
            //DEBUG_PRINT(c, HEX); DEBUG_PRINT("#"); DEBUG_PRINTLN(c);
            replyidx++;
        }

        if (timeout == 0)
        {
            //DEBUG_PRINTLN(F("TIMEOUT"));
            break;
        }
        delay(1);
    }
    replybuffer[replyidx] = 0; // null term
    return replyidx;
}

uint8_t Sim5320::getReply(char *send, uint16_t timeout)
{
    flushInput();

    DEBUG_PRINT(F("\t---> "));
    DEBUG_PRINTLN(send);

    serial->println(send);

    uint8_t l = readline(timeout);

    DEBUG_PRINT(F("\t<--- "));
    DEBUG_PRINTLN(replybuffer);

    return l;
}

uint8_t Sim5320::getReply(FlashStringPtr send, uint16_t timeout)
{
    flushInput();

    DEBUG_PRINT(F("\t---> "));
    DEBUG_PRINTLN(send);

    serial->println(send);

    uint8_t l = readline(timeout);

    DEBUG_PRINT(F("\t<--- "));
    DEBUG_PRINTLN(replybuffer);

    return l;
}

// Send prefix, suffix, and newline. Return response (and also set replybuffer with response).
uint8_t Sim5320::getReply(FlashStringPtr prefix, char *suffix, uint16_t timeout)
{
    flushInput();

    DEBUG_PRINT(F("\t---> "));
    DEBUG_PRINT(prefix);
    DEBUG_PRINTLN(suffix);

    serial->print(prefix);
    serial->println(suffix);

    uint8_t l = readline(timeout);

    DEBUG_PRINT(F("\t<--- "));
    DEBUG_PRINTLN(replybuffer);

    return l;
}

// Send prefix, suffix, and newline. Return response (and also set replybuffer with response).
uint8_t Sim5320::getReply(FlashStringPtr prefix, int32_t suffix, uint16_t timeout)
{
    flushInput();

    DEBUG_PRINT(F("\t---> "));
    DEBUG_PRINT(prefix);
    DEBUG_PRINTLN(suffix, DEC);

    serial->print(prefix);
    serial->println(suffix, DEC);

    uint8_t l = readline(timeout);

    DEBUG_PRINT(F("\t<--- "));
    DEBUG_PRINTLN(replybuffer);

    return l;
}

// Send prefix, suffix, suffix2, and newline. Return response (and also set replybuffer with response).
uint8_t Sim5320::getReply(FlashStringPtr prefix, int32_t suffix1, int32_t suffix2, uint16_t timeout)
{
    flushInput();

    DEBUG_PRINT(F("\t---> "));
    DEBUG_PRINT(prefix);
    DEBUG_PRINT(suffix1, DEC);
    DEBUG_PRINT(',');
    DEBUG_PRINTLN(suffix2, DEC);

    serial->print(prefix);
    serial->print(suffix1);
    serial->print(',');
    serial->println(suffix2, DEC);

    uint8_t l = readline(timeout);

    DEBUG_PRINT(F("\t<--- "));
    DEBUG_PRINTLN(replybuffer);

    return l;
}

// Send prefix, ", suffix, ", and newline. Return response (and also set replybuffer with response).
uint8_t Sim5320::getReplyQuoted(FlashStringPtr prefix, FlashStringPtr suffix, uint16_t timeout)
{
    flushInput();

    DEBUG_PRINT(F("\t---> "));
    DEBUG_PRINT(prefix);
    DEBUG_PRINT('"');
    DEBUG_PRINT(suffix);
    DEBUG_PRINTLN('"');

    serial->print(prefix);
    serial->print('"');
    serial->print(suffix);
    serial->println('"');

    uint8_t l = readline(timeout);

    DEBUG_PRINT(F("\t<--- "));
    DEBUG_PRINTLN(replybuffer);

    return l;
}

bool Sim5320::sendCheckReply(char *send, char *reply, uint16_t timeout)
{
    if (!getReply(send, timeout))
        return false;

    return (strcmp(replybuffer, reply) == 0);
}

bool Sim5320::sendCheckReply(FlashStringPtr send, FlashStringPtr reply, uint16_t timeout)
{
    if (!getReply(send, timeout))
        return false;

    return (prog_char_strcmp(replybuffer, (prog_char *)reply) == 0);
}

bool Sim5320::sendCheckReply(char *send, FlashStringPtr reply, uint16_t timeout)
{
    if (!getReply(send, timeout))
        return false;
    return (prog_char_strcmp(replybuffer, (prog_char *)reply) == 0);
}

// Send prefix, suffix, and newline.  Verify FONA response matches reply parameter.
bool Sim5320::sendCheckReply(FlashStringPtr prefix, char *suffix, FlashStringPtr reply, uint16_t timeout)
{
    getReply(prefix, suffix, timeout);
    return (prog_char_strcmp(replybuffer, (prog_char *)reply) == 0);
}

// Send prefix, suffix, and newline.  Verify FONA response matches reply parameter.
bool Sim5320::sendCheckReply(FlashStringPtr prefix, int32_t suffix, FlashStringPtr reply, uint16_t timeout)
{
    getReply(prefix, suffix, timeout);
    return (prog_char_strcmp(replybuffer, (prog_char *)reply) == 0);
}

// Send prefix, suffix, suffix2, and newline.  Verify FONA response matches reply parameter.
bool Sim5320::sendCheckReply(FlashStringPtr prefix, int32_t suffix1, int32_t suffix2, FlashStringPtr reply, uint16_t timeout)
{
    getReply(prefix, suffix1, suffix2, timeout);
    return (prog_char_strcmp(replybuffer, (prog_char *)reply) == 0);
}

// Send prefix, ", suffix, ", and newline.  Verify FONA response matches reply parameter.
bool Sim5320::sendCheckReplyQuoted(FlashStringPtr prefix, FlashStringPtr suffix, FlashStringPtr reply, uint16_t timeout)
{
    getReplyQuoted(prefix, suffix, timeout);
    return (prog_char_strcmp(replybuffer, (prog_char *)reply) == 0);
}


template <class T> bool Sim5320::parseReply(FlashStringPtr toreply, T *v  , char divider, uint8_t index)
{
        char *p = prog_char_strstr(replybuffer, (prog_char *)toreply); // get the pointer to the voltage
    if (p == 0)
        return false;
    p += prog_char_strlen((prog_char *)toreply);
    //DEBUG_PRINTLN(p);
    for (uint8_t i = 0; i < index; i++)
    {
        // increment dividers
        p = strchr(p, divider);
        if (!p)
            return false;
        p++;
        //DEBUG_PRINTLN(p);
    }
    *v = atoi(p);

    return true;
}


bool Sim5320::parseReply(FlashStringPtr toreply,
                         char *v, char divider, uint8_t index)
{
    uint8_t i = 0;
    char *p = prog_char_strstr(replybuffer, (prog_char *)toreply);
    if (p == 0)
        return false;
    p += prog_char_strlen((prog_char *)toreply);

    for (i = 0; i < index; i++)
    {
        // increment dividers
        p = strchr(p, divider);
        if (!p)
            return false;
        p++;
    }

    for (i = 0; i < strlen(p); i++)
    {
        if (p[i] == divider)
            break;
        v[i] = p[i];
    }

    v[i] = '\0';

    return true;
}

// Parse a quoted string in the response fields and copy its value (without quotes)
// to the specified character array (v).  Only up to maxlen characters are copied
// into the result buffer, so make sure to pass a large enough buffer to handle the
// response.
bool Sim5320::parseReplyQuoted(FlashStringPtr toreply,
                               char *v, int maxlen, char divider, uint8_t index)
{
    uint8_t i = 0, j;
    // Verify response starts with toreply.
    char *p = prog_char_strstr(replybuffer, (prog_char *)toreply);
    if (p == 0)
        return false;
    p += prog_char_strlen((prog_char *)toreply);

    // Find location of desired response field.
    for (i = 0; i < index; i++)
    {
        // increment dividers
        p = strchr(p, divider);
        if (!p)
            return false;
        p++;
    }

    // Copy characters from response field into result string.
    for (i = 0, j = 0; j < maxlen && i < strlen(p); ++i)
    {
        // Stop if a divier is found.
        if (p[i] == divider)
            break;
        // Skip any quotation marks.
        else if (p[i] == '"')
            continue;
        v[j++] = p[i];
    }

    // Add a null terminator if result string buffer was not filled.
    if (j < maxlen)
        v[j] = '\0';

    return true;
}

template <class T> bool Sim5320::sendParseReply(FlashStringPtr tosend, FlashStringPtr toreply, T *v   , char divider, uint8_t index)
{
    getReply(tosend);

    if (!parseReply(toreply, v, divider, index))
        return false;

    readline(); // eat 'OK'

    return true;
}


bool Sim5320::sendParseReply(FlashStringPtr tosend, FlashStringPtr toreply, float *f, char divider, uint8_t index)
{
    getReply(tosend);

    if (!parseReply(toreply, f, divider, index))
        return false;

    readline(); // eat 'OK'

    return true;
}

bool Sim5320::parseReply(FlashStringPtr toreply, float *f, char divider, uint8_t index)
{
    char *p = prog_char_strstr(replybuffer, (prog_char *)toreply); // get the pointer to the voltage
    if (p == 0)
        return false;
    p += prog_char_strlen((prog_char *)toreply);
    //DEBUG_PRINTLN(p);
    for (uint8_t i = 0; i < index; i++)
    {
        // increment dividers
        p = strchr(p, divider);
        if (!p)
            return false;
        p++;
        //DEBUG_PRINTLN(p);
    }
    *f = atof(p);

    return true;
}