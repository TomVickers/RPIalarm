// Config.h - config items and definition of config class

#pragma once

static const int      CRITICAL_TIMEOUT_S  = 10;      // sound fast beep when arm/disarm timeout below this
static const int      MAX_PIN_DIGITS      = 16;      // max length of digits in pin code
static const int      MIN_PIN_DIGITS      = 4;       // pins must have at least this digit count
static const int      MAX_KEYPADS         = 8;       // max number of keypads (starting at kp16)
static const uint32_t DEFAULT_TIMEOUT_MS  = 45000;   // default timeout for arm/disarm
static const uint32_t BACKLIGHT_ON_TIME   = 10000;   // keep backlight on for 10 secs after last key press
static const uint32_t TEMP_MSG_TIMEOUT    = 3000;    // ms to display temp mesg
static const uint32_t PIN_TIMEOUT         = 5000;    // timeout pin after no key presses
static const uint32_t CHIME_TIMEOUT       = 2000;    // time between CHIME tone and disable

static const uint32_t ALERT_MSG_REPEAT_TIMEOUT = 60*1000; // time between send msg repeats

static const int READ_BUF_SIZE       = 256;
static const int SOCK_BUF_SIZE       = 256;

static const int MAX_PIN_LENGTH       = 16;
static const int MAX_PIN_NAME_LENGTH  = 32;
static const int MAX_PIN_CODES        =  8;
static const int MAX_SENSE_LOOPS      = 16;
static const int MAX_LOOP_NAME_LENGTH = 32;
static const int MAX_PARM_LENGTH      = 64;
static const int MAX_FUNC_NAME_LENGTH = 16;
static const int MAX_GPIO_OUTPUTS     = 4;

#define ALARM_CONFIG_FILE "/etc/alarm_config"  // path to config file

// tone codes for alarm status struct
enum {
    TONE_NONE      = 0x0,
    TONE_CHIME_1   = 0x1,
    TONE_CHIME_2   = 0x2,
    TONE_CHIME_3   = 0x3,
    TONE_FAST_RATE = 0x4,
    TONE_SLOW_RATE = 0x5,
    TONE_ALARM     = 0x7
};

class Pin
{
public:
    bool init(const char * line);

    char name[MAX_PIN_NAME_LENGTH];
    char pin[MAX_PIN_LENGTH];
};

class Loop
{
public:
    bool init(const char * line);

    int     gpio;
    char    name[MAX_LOOP_NAME_LENGTH];
    uint8_t chimeTone;
    bool    entryAllowed;
};

class GpioOutput
{
public:
    bool init(const char * line);

    int  gpio;
    char funcName[MAX_FUNC_NAME_LENGTH];
};

class Config
{
public:
    Config(void)
    {
        pinCount = 0;
        loopCount = 0;
        outputCount = 0;
        baudRate = 0;
        chimeDefault = false;
        serialPort[0] = '\0';
        sendEmailAccnt[0] = '\0';
        sendEmailPasswd[0] = '\0';
        alertEmail[0] = '\0';
    }

    bool readFile(const char * pathAndFilename);

    Pin * getPin(void)
    {
        return AlarmPin;
    }
    int getPinCount(void)
    {
        return pinCount;
    }

    Loop * getLoop(void)
    {
        return SenseLoop;
    }
    int getLoopCount(void)
    {
        return loopCount;
    }
    GpioOutput * getOutputs(void)
    {
        return Output;
    }
    int getOutputCount(void)
    {
        return outputCount;
    }

    const char * getSerial(void)
    {
        return serialPort;
    }
    const char * getSendEmailAccnt(void)
    {
        return sendEmailAccnt;
    }
    const char * getSendEmailPasswd(void)
    {
        return sendEmailPasswd;
    }
    const char * getAlertEmail(void)
    {
        return alertEmail;
    }
    int getBaudRate(void)
    {
        return baudRate;
    }
    bool otherConfig(const char * line);

    bool getChimeDefault(void)
    {
        return chimeDefault;
    }
    int getOutput(const char * func);

    static const char * nextParm(const char * buf);

    static int getTextParm(const char * buf, char * parm, int parmSize);
    static int getNumParm(const char * buf, char * parm, int parmSize);

private:
    static const int LINE_BUF_SIZE = 256;
    char lineBuf[LINE_BUF_SIZE];

    char serialPort[MAX_PARM_LENGTH];
    char sendEmailAccnt[MAX_PARM_LENGTH];
    char sendEmailPasswd[MAX_PARM_LENGTH];
    char alertEmail[MAX_PARM_LENGTH];
    int  baudRate;
    bool chimeDefault;

    Pin        AlarmPin[MAX_PIN_CODES];     // max number of alarm pins in config file
    Loop       SenseLoop[MAX_SENSE_LOOPS];  // max number of sense loops in config file
    GpioOutput Output[MAX_GPIO_OUTPUTS];    // max number of gpio outputs in config file

    int pinCount;
    int loopCount;
    int outputCount;
};

// end of Config.h
