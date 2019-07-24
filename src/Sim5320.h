#ifndef _SIM5320_HEADER_H_
#define _SIM5320_HEADER_H_

#include <Arduino.h>
#include <Stream.h>
#include <stdint.h>

#define SIM5320_DEFAULT_TIMEOUT_MS 500

typedef const __FlashStringHelper *FlashStringPtr;
#define prog_char char PROGMEM
#define prog_char_strcmp(a, b) strcmp_P((a), (b))
#define prog_char_strstr(a, b) strstr_P((a), (b))
#define prog_char_strlen(a) strlen_P((a))
#define prog_char_strcpy(to, fromprogmem) strcpy_P((to), (fromprogmem))

class Sim5320 : public Stream
{

public:
    Sim5320(uint8_t rst, Stream &serial);
    ~Sim5320(void);

    bool begin();
    bool connect(char *apn, char *user = NULL, char *pwd = NULL);
    bool disconnect(void);
    bool connected();
    int16_t status(void);
    void reset(void);

    // Stream
    int available(void);
    size_t write(uint8_t a);
    int read(void);
    int peek(void);
    void flush(void);

    // RTC
    bool enableRTC(uint8_t i);
    bool readRTC(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *hr, uint8_t *min, uint8_t *sec);

    // Battery and ADC
    bool getADCVoltage(uint16_t *v);
    bool getBattPercent(uint16_t *p);
    bool getBattVoltage(uint16_t *v);

    // SIM query
    uint8_t unlockSIM(char *pin);
    uint8_t getSIMCCID(char *ccid);
    uint8_t getNetworkStatus(void);
    uint8_t getRSSI(void);

    // GPRS handling
    bool enableGPRS(bool onoff);
    uint8_t GPRSstate(void);
    bool getGSMLoc(uint16_t *replycode, char *buff, uint16_t maxlen);
    bool getGSMLoc(float *lat, float *lon);
    void setGPRSNetworkSettings(char *apn, char *username = 0, char *password = 0);

    // TCP raw connections
    bool TCPconnect(const char *server, uint16_t port);
    bool TCPclose(void);
    bool TCPconnected(void);
    bool TCPsend(const uint8_t *packet, uint8_t len);
    uint16_t TCPpeek(void);
    uint16_t TCPavailable(void);
    uint16_t TCPread(uint8_t *buff, uint8_t len);

    // GPS Handling
    bool enableGPS(bool onoff);
    int8_t GPSstatus(void);
    uint8_t getGPS(uint8_t arg, char *buffer, uint8_t maxbuff);
    bool getGPS(float *lat, float *lon, float *speed_kph = 0, float *heading = 0, float *altitude = 0);
    bool enableGPSNMEA(uint8_t nmea);

    // Helper functions to verify responses.
    bool expectReply(FlashStringPtr reply, uint16_t timeout = 10000);
    bool sendCheckReply(char *send, char *reply, uint16_t timeout = SIM5320_DEFAULT_TIMEOUT_MS);
    bool sendCheckReply(FlashStringPtr send, FlashStringPtr reply, uint16_t timeout = SIM5320_DEFAULT_TIMEOUT_MS);
    bool sendCheckReply(char *send, FlashStringPtr reply, uint16_t timeout = SIM5320_DEFAULT_TIMEOUT_MS);

protected:
    void flushInput();
    uint16_t readRaw(uint16_t b);
    uint8_t readline(uint16_t timeout = SIM5320_DEFAULT_TIMEOUT_MS, bool multiline = false);
    uint8_t getReply(char *send, uint16_t timeout = SIM5320_DEFAULT_TIMEOUT_MS);
    uint8_t getReply(FlashStringPtr send, uint16_t timeout = SIM5320_DEFAULT_TIMEOUT_MS);
    uint8_t getReply(FlashStringPtr prefix, char *suffix, uint16_t timeout = SIM5320_DEFAULT_TIMEOUT_MS);
    uint8_t getReply(FlashStringPtr prefix, int32_t suffix, uint16_t timeout = SIM5320_DEFAULT_TIMEOUT_MS);
    uint8_t getReply(FlashStringPtr prefix, int32_t suffix1, int32_t suffix2, uint16_t timeout); // Don't set default value or else function call is ambiguous.
    uint8_t getReplyQuoted(FlashStringPtr prefix, FlashStringPtr suffix, uint16_t timeout = SIM5320_DEFAULT_TIMEOUT_MS);

    bool sendCheckReply(FlashStringPtr prefix, char *suffix, FlashStringPtr reply, uint16_t timeout = SIM5320_DEFAULT_TIMEOUT_MS);
    bool sendCheckReply(FlashStringPtr prefix, int32_t suffix, FlashStringPtr reply, uint16_t timeout = SIM5320_DEFAULT_TIMEOUT_MS);
    bool sendCheckReply(FlashStringPtr prefix, int32_t suffix, int32_t suffix2, FlashStringPtr reply, uint16_t timeout = SIM5320_DEFAULT_TIMEOUT_MS);
    bool sendCheckReplyQuoted(FlashStringPtr prefix, FlashStringPtr suffix, FlashStringPtr reply, uint16_t timeout = SIM5320_DEFAULT_TIMEOUT_MS);

    template <class T> bool parseReply(FlashStringPtr toreply, T *v  , char divider = ',', uint8_t index = 0);
    bool parseReply(FlashStringPtr toreply, char *v     , char divider = ',', uint8_t index = 0);
    bool parseReply(FlashStringPtr toreply, float *f    , char divider = ',', uint8_t index = 0);
    bool parseReplyQuoted(FlashStringPtr toreply, char *v, int maxlen, char divider = ',', uint8_t index = 0);

    template <class T> bool sendParseReply(FlashStringPtr tosend, FlashStringPtr toreply, T *v   , char divider = ',', uint8_t index = 0);
    bool sendParseReply(FlashStringPtr tosend, FlashStringPtr toreply, float *f     , char divider = ',', uint8_t index = 0);


private:
    uint8_t rstpin;
    Stream *serial;

    char replybuffer[255];
    FlashStringPtr apn;
    FlashStringPtr apnusername;
    FlashStringPtr apnpassword;
    FlashStringPtr ok_reply;
};
#endif //_SIM5320_HEADER_H_