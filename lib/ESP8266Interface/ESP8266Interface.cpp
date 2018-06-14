#include "ESP8266Interface.h"

ESP8266Interface::ESP8266Interface(PinName tx, PinName rx, PinName rst)
        : esp(tx, rx), reset(rst)
{
    esp.baud(115200); // the default baud rate of the ESP8266
    esp.attach(this, &ESP8266Interface::callBack, Serial::RxIrq);
}


void ESP8266Interface::softReset()
{
    reset=0;
    wait(1);
    reset=1; // send esp8266 the reset signal
    wait(3); // avoids data sent back after a reset to output in between other commands
}

bool ESP8266Interface::init()
{
    // basic hardware check and turn off echo
    // return false if "OK" not found in response
    if (sendCmdGetResponse("ATE0\r\n").find("OK\r\n") == std::string::npos)
        return false;
    // set device to client mode
    // return false if "OK" not found in response
    else if (sendCmdGetResponse("AT+CWMODE=1\r\n").find("OK\r\n") == std::string::npos)
        return false;

    return true;
}

bool ESP8266Interface::connectToAccessPoint(const char* ssid, const char* password)
{
    char command[100];
    sprintf(command, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    // in the event of an error the device takes a long time to respond so a
    // long delay is used
    std::string response = sendCmdGetResponse(command, 5);

    // return false if any of the substrings are not found in the response
    if (response.find("WIFI CONNECTED") == std::string::npos &&
        response.find("WIFI GOT IP") == std::string::npos &&
        response.find("OK\r\n") == std::string::npos)
        return false;

    return true;
}

bool ESP8266Interface::isConnected()
{
    std::string response = sendCmdGetResponse("AT+CIPSTATUS\r\n");
    if (response[7] != '2') // a status of "2" means the device is connected
        return false;
    return true;
}

std::string ESP8266Interface::getPage(const char* host, const char* page)
{
    char connectCommand[200];
    char sendCommand[50];
    char getRequest[200];
    int requestLength = 25 + strlen(host) + strlen(page); // calculate length of GET request in bytes
    std::string pageData = "";

    sprintf(connectCommand, "AT+CIPSTART=\"TCP\",\"%s\",80\r\n", host);
    sprintf(sendCommand, "AT+CIPSEND=%d\r\n", requestLength);
    sprintf(getRequest, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", page, host);

    // establish TCP connection to host
    // return empty string if command fails
    if (sendCmdGetResponse(connectCommand, 2).find("OK\r\n") != std::string::npos)
    {
        // send get request
        // receiving "> " from the device means it is ready to receive the GET request
        // return empty string if this string is not found in the response
        if (sendCmdGetResponse(sendCommand).find("> ") != std::string::npos)
        {
            pageData = sendCmdGetResponse(getRequest, 5);
            cleanPage(pageData);
        }
    }

    return pageData;
}

std::string ESP8266Interface::sendCmdGetResponse(const char* cmd, float delay)
{
    buf = "";
    std::string response = "";

    esp.puts(cmd);
    wait(delay);
    response = buf;
    wait(0.3);
    return response;
}

void ESP8266Interface::cleanPage(std::string& page)
{
    size_t currentPos = 0;
    size_t start;
    size_t length;

    do
    {
        length = 0;
        start = page.find("\r\n+IPD", currentPos);
        if (start != std::string::npos)
        {
            while (page[start + length] != ':')
            {
                length++;
            }
            length++;
            page.erase(start, length);
            currentPos = start;
        }
    } while(start != std::string::npos);
}

void ESP8266Interface::callBack()
{
    while (esp.readable())
        buf += esp.getc();
}
