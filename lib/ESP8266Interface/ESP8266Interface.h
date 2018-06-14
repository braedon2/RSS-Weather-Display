#include "mbed.h"
#include "string.h"
#include <string>

/**
*  Class used to interface with the ESP8266 device
*  Uses a serial connection to configure and control the device
*/
class ESP8266Interface
{
public:
    /**
    *  Initializes the serial connection
    *
    *  tx - transmit pin of the microcontroller
    *  rx - receive pin of the microcontroller
    *  rst - gpio that connects to the reset pin of the ESP8266
    */
    ESP8266Interface(PinName tx, PinName rx, PinName rst);

    /**
    *  Resets the device but SSID and password are remembered
    */
    void softReset();

    /**
    *  Does a hardware check and sets the device to client mode
    *
    *  returns true device succesfully checked and configured, false otherwise
    */
    bool init();

    /**
    *  connects to wireless access point
    *
    *  ssid - ssid of access point (the name of the router)
    *  password - password of access point
    *
    *  returns true if succesfully connected to an access point, false otherwise
    */
    bool connectToAccessPoint(const char* ssid, const char* password);

    /**
    *  returns true if connected to access point, false otherwise
    */
    bool isConnected();

    /**
    *  Gets the contents of a page and returns it as a string. Returns an empty
    *  string on failure
    *
    *  host - the website (e.g "www.weather.gc.ca")
    *  page - the specific page to access from the host (e.g "/index.html")
    */
    std::string getPage(const char* host, const char* page);

    /**
    *  Sends an AT command to the ESP8266 and returns the response as a string
    *
    *  cmd - the AT command
    *  delay - the time in seconds to wait between sending the command and
    *          returning the response. A longer delay is needed for get requests
    *          and connecting to access points
    */
    std::string sendCmdGetResponse(const char* cmd, float delay=0.3);

private:
    std::string buf;
    RawSerial esp;
    DigitalOut reset;

    void callBack();

    /**
    *  Removes the "+IPD" substrings that the esp8266 includes when returning
    *  webpage data
    *
    *  page - the string to clean
    */
    void cleanPage(std::string& page);
};
