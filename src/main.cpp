#include "mbed.h"
#include "ESP8266Interface.h"
#include "tinyxml.h"
#include "TextLCD.h"
#include <string>
#include <regex>

#define SSID "daBoizWiFly"
#define PASSWORD "b6owX@wvcdAjl3"
#define HOST "rss.theweathernetwork.com"
#define PAGE "/weather/caqc0363" // montreal weather
#define DELAY 600 // delay between weather updates in seconds, maximum 30 minutes

typedef struct DayData
{
    std::string name;
    std::string high;
    std::string low;
    std::string description;
} DayData;

// global variables for callback functions
bool nextDayRequested = false;
bool timeToUpdate = true;

// called every DELAY seconds
void updateData()
{
    timeToUpdate = true;
}

// called when button pressed
void buttonPressed()
{
    nextDayRequested = true;
}

bool parseWeatherNetworkXml(std::string xmlData, DayData daysArr[]);
void printDay(TextLCD* lcd, DayData day);

int main(void)
{
    ESP8266Interface wifi(D8, D2, D7);
    TextLCD lcd(D9, D10, D12, D13, D14, D15);
    DigitalOut connectionStatus(D4);
    DigitalOut updateStatus(D6);
    InterruptIn button(D5);
    button.rise(&buttonPressed);

    bool connected = false;
    std::string response;
    DayData days[4];
    int dayToDisplay = 0;

    updateStatus = 0;
    connectionStatus = 1;
    lcd.printf("Initializing...");
    wifi.softReset();
    wifi.init();

    Ticker t;
    t.attach(&updateData, DELAY);

    if (wifi.connectToAccessPoint(SSID, PASSWORD))
    {
        connected = true;
        connectionStatus = 0;
    }
    else
    {
        lcd.cls();
        lcd.printf("connection error");
    }

    while (true)
    {
        if (!wifi.isConnected())
        {
            connected = false;
            connectionStatus = 1;
        }

        if (!connected)
        {
            printf("connecting...\n");
            if (wifi.connectToAccessPoint(SSID, PASSWORD))
            {
                connected = true;
                connectionStatus = 0;
                printf("connected to access point\n");
            }
        }

        if (connected && timeToUpdate)
        {
            button.rise(NULL); // disable button while updating
            updateStatus = 1;
            response = wifi.getPage(HOST, PAGE);
            size_t pos = response.find("<");
            response.erase(0, pos); // remove everything up to the first xml tag

            // if failed to parse, try again next loop
            if (parseWeatherNetworkXml(response, days))
            {
                dayToDisplay = 0;
                timeToUpdate = false;
                printDay(&lcd, days[dayToDisplay]);
                updateStatus = 0;
            }
            button.rise(&buttonPressed); // enable button
        }

        if (nextDayRequested)
        {
            dayToDisplay = (dayToDisplay + 1) % 4;
            printDay(&lcd, days[dayToDisplay]);
            nextDayRequested = false;
        }
    }
}


void printDay(TextLCD* lcd, DayData day)
{
    lcd->cls();

    if (day.name == "Current Weather")
    {
        lcd->printf("Current temp:%s", day.high.c_str());
        lcd->locate(0,1);
        lcd->printf("%s", day.description.substr(0,16).c_str());
    }
    else
    {
        lcd->printf("%s hi:%s lo:%s", day.name.substr(0, 3).c_str(), day.high.c_str(), day.low.c_str());
        lcd->locate(0,1);
        lcd->printf("%s", day.description.substr(0,16).c_str());
    }
}

// returns true if data was properly parsed, returns false otherwise which can
// occur when data is corrupted
bool parseWeatherNetworkXml(std::string xmlData, DayData daysArr[])
{
    int day = 0;
    bool parsed = true;
    TiXmlDocument doc;
    doc.Parse(xmlData.c_str());

    TiXmlElement* currentRoot = doc.RootElement();
    TiXmlElement* currentElement; // used to traverse currentRoots children
    std::string currentText;

    std::regex r1("([A-Za-z\\s]+), (-?[0-9]+)"); // matches the format for the first day
    std::regex r2("([A-Za-z\\s]+),[A-Za-z\\s]+(-?[0-9]+)&nbsp;&deg;C,[A-Za-z\\s]+(-?[0-9]+)"); // matches the format for the other days
    std::smatch sm;

    currentRoot = currentRoot->FirstChildElement();
    currentRoot = currentRoot->FirstChildElement("item"); // each day is rooted at an item tag

    while (currentRoot != NULL)
    {
        currentElement = currentRoot->FirstChildElement("title");
        currentText = currentElement->FirstChild()->ToText()->Value();

        // current weather's description tag has a different format than the rest
        if (currentText == "Current Weather")
        {
            daysArr[day].name = currentText;
            currentText = currentRoot->FirstChildElement("description")->FirstChild()->ToText()->Value();

            if (std::regex_search(currentText, sm, r1))
            {
                daysArr[day].description = sm[1].str();
                daysArr[day].high = sm[2].str();
            }
            else
                parsed = false;
        }
        else
        {
            size_t splitPos = currentText.find(",");
            daysArr[day].name = currentText.substr(0, splitPos);
            currentText = currentRoot->FirstChildElement("description")->FirstChild()->ToText()->Value();

            if (std::regex_search(currentText, sm, r2))
            {
                daysArr[day].description = sm[1].str();
                daysArr[day].high = sm[2].str();
                daysArr[day].low = sm[3].str();
            }
            else
                parsed = false;
        }

        currentRoot = currentRoot->NextSiblingElement("item");
        day++;
    }

    return parsed;
}
