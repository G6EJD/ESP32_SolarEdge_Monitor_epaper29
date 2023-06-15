/* ESP32 SolarPV Eenergy Display using an EPD 4.2" Display, obtains data from SolarEdge, decodes and then displays it.
  ####################################################################################################################################
  This software, the ideas and concepts is Copyright (c) David Bird 2018. All rights to this software are reserved.

  Any redistribution or reproduction of any part or all of the contents in any form is prohibited other than the following:
  1. You may print or download to a local hard disk extracts for your personal and non-commercial use only.
  2. You may copy the content to individual third parties for their personal use, but only if you acknowledge the author David Bird as the source of the material.
  3. You may not, except with my express written permission, distribute or commercially exploit the content.
  4. You may not transmit it or store it in any other website or other form of electronic retrieval system for commercial purposes.

  The above copyright ('as annotated') notice and this permission notice shall be included in all copies or substantial portions of the Software and where the
  software use is visible to an end-user.

  THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT. FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY
  OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
  See more at http://www.dsbird.org.uk
*/
#include "credentials.h"       // See 'credentials' tab and enter your OWM API key and set the Wifi SSID and PASSWORD
#include <ArduinoJson.h>       // https://github.com/bblanchon/ArduinoJson NOTE: *** MUST BE Version-6 or above ***
#include <WiFi.h>              // Built-in
#include <WiFiClientSecure.h>  // Built-in
#include "time.h"              // Built-in
#include <SPI.h>               // Built-in 
#include "EPD_WaveShare.h"     // Copyright (c) 2017 by Daniel Eichhorn https://github.com/ThingPulse/minigrafx
#include "MiniGrafx.h"         // Copyright (c) 2017 by Daniel Eichhorn https://github.com/ThingPulse/minigrafx
#include "DisplayDriver.h"     // Copyright (c) 2017 by Daniel Eichhorn https://github.com/ThingPulse/minigrafx
#include "ArialRounded.h"      // Copyright (c) 2017 by Daniel Eichhorn https://github.com/ThingPulse/minigrafx

#define SCREEN_HEIGHT  128
#define SCREEN_WIDTH   296
#define BITS_PER_PIXEL 1
#define EPD_BLACK 0
#define EPD_WHITE 1
uint16_t palette[] = { 0, 1 };

// pins_arduino.h, e.g. LOLIN32 LITE
static const uint8_t EPD_BUSY = 4;
static const uint8_t EPD_SS   = 5;
static const uint8_t EPD_RST  = 16;
static const uint8_t EPD_DC   = 17;
static const uint8_t EPD_SCK  = 18;
static const uint8_t EPD_MISO = 19; // Master-In Slave-Out not used, as no data from display
static const uint8_t EPD_MOSI = 23;

EPD_WaveShare epd(EPD2_9, EPD_SS, EPD_RST, EPD_DC, EPD_BUSY); // //EPD_WaveShare epd(EPD2_9, CS, RST, DC, BUSY);

MiniGrafx gfx = MiniGrafx(&epd, BITS_PER_PIXEL, palette);

//################  VERSION  ##########################
String version = "v10";      // Version of this program
//################ VARIABLES ###########################

int    SleepDuration = 30;

String time_str, date_time_str; // strings to hold time and received time data
String query_day, query_month, query_year, query_start_date, query_end_date;
int    query_year_start, query_year_end, current_year, current_month, current_hour, rssi, 
       CurrentHour, CurrentMin, CurrentSec, StartTime;
float  total_system_revenue;
String YearlyRequest   = "";
String MonthlyRequest  = "";
String OverviewRequest = "";

//################ PROGRAM VARIABLES and OBJECTS ################

float Production[11][12]      = {0};
float Consumption[11][12]     = {0};
float SelfConsumption[11][12] = {0};
float Purchased[11][12]       = {0};
float FeedIn[11][12]          = {0};

float mProduction[1]      = {0};
float mConsumption[1]     = {0};
float mSelfConsumption[1] = {0};
float mPurchased[1]       = {0};
float mFeedIn[1]          = {0};

float LifeTimeEnergy      = 0;
float Revenue             = 0;
float LastYearEnergy      = 0;
float LastMonthEnergy     = 0;
float LastDayEnergy       = 0;

#define autoscale_on  true
#define autoscale_off false
#define barchart_on   true
#define barchart_off  false

WiFiClientSecure client; // wifi client object

//#########################################################################################
void setup() {
  StartTime = millis();
  Serial.begin(115200);
  StartWiFi();
  SetupTime(); 
  bool Received_EnergyData_OK = false;
  // Overview: https://monitoringapi.solaredge.com/ site/{siteId}/overview?api_key=I5CLG1OK6HN7P3OJ8OM58B7761N0JJLD
  OverviewRequest  = "GET https://monitoringapi.solaredge.com/site/799222/overview?api_key=I5CLG1OK6HN7P3OJ8OM58B7761N0JJLD";
  query_year_start = current_year;
  query_year_end   = current_year; //Only ask for the last year of data as the maximum, otherwise the API returns 0, can't look forward!
  query_start_date = String(query_year_start) + "-01-01" + "%2000:00:00";
  query_end_date   = String(query_year_end) + "-12-31" + "%2023:59:59";
  YearlyRequest    = "GET https://monitoringapi.solaredge.com/site/799222/energyDetails.json?meters=&timeUnit=MONTH&startTime=" +
                     query_start_date + "&endTime=" +
                     query_end_date   + "&api_key=I5CLG1OK6HN7P3OJ8OM58B7761N0JJLD  HTTP/1.0";
  query_year_start = current_year-1;
  query_year_end   = current_year-1; //Only ask for the last year of data as the maximum, otherwise the API returns 0, can't look forward!
  query_start_date = String(query_year_start) + "-01-01" + "%2000:00:00";
  query_end_date   = String(query_year_end) + "-12-31" + "%2023:59:59";
  String last_years_request = "GET https://monitoringapi.solaredge.com/site/799222/energyDetails.json?meters=&timeUnit=MONTH&startTime=" +
                     query_start_date + "&endTime=" +
                     query_end_date   + "&api_key=I5CLG1OK6HN7P3OJ8OM58B7761N0JJLD  HTTP/1.0";
  bool Active = true;
  if (CurrentHour >= 22 || CurrentHour <= 7) Active = false;
  if (Active) Received_EnergyData_OK = (Obtain_Energy_Reading("Overview", OverviewRequest, current_year) &&
                                        Obtain_Energy_Reading("Yearly", YearlyRequest, current_year)&&
                                        Obtain_Energy_Reading("Yearly", last_years_request, 2018));

  // Now only refresh the screen if all the data was received OK, otherwise wait until the next timed check
  if (Received_EnergyData_OK) {
    StopWiFi(); // Reduces power consumption whilst updating the screen and before entering sleep
    gfx.init();
    gfx.setRotation(3);
    gfx.setFont(ArialMT_Plain_10);
    gfx.setColor(EPD_BLACK);
    gfx.fillBuffer(EPD_WHITE);
    gfx.setTextAlignment(TEXT_ALIGN_LEFT);
    Display_Energy();
    gfx.commit();
    delay(2000);
    Serial.println("total time to update = " + String(millis() - StartTime));
  }
  begin_sleep();
}
//#########################################################################################
void loop() { // this will never run!
}
//#########################################################################################
void begin_sleep() {
  long SleepTimer = SleepDuration * 60 - ((CurrentMin % SleepDuration) * 60 + CurrentSec - (millis()-StartTime)/1000+1);
  Serial.println("Entering "+String(SleepTimer)+"-secs of sleep time");
  esp_sleep_enable_timer_wakeup(SleepTimer * 1000000LL);
#ifdef BUILTIN_LED
  pinMode(BUILTIN_LED, INPUT);     // In case it's on, turn output off, sometimes PIN-5 on some boards is used for SPI-SS
  digitalWrite(BUILTIN_LED, HIGH); // In case it's on, turn LED off, as sometimes PIN-5 on some boards is used for SPI-SS
#endif
  Serial.println("Starting sleep period...");
  delay(1000);
  esp_deep_sleep_start(); // Sleep for the time specified e.g. long or short
}
//#########################################################################################
void Display_Energy() {                          // 4.2" e-paper display is 400x300 resolution
  Display_Heading_Section();                     // Top line of the display
  Display_EnergyOverview(current_year);
  Display_YearlyEnergySummary(current_year);
  total_system_revenue += Display_MonthlyEnergySummary(2018); 
  total_system_revenue += Display_MonthlyEnergySummary(2019); 
  total_system_revenue += Display_MonthlyEnergySummary(2020); 
  total_system_revenue += Display_MonthlyEnergySummary(2021); 
  total_system_revenue += Display_MonthlyEnergySummary(2022);
  total_system_revenue += Display_MonthlyEnergySummary(2023);
  gfx.setFont(ArialRoundedMTBold_14);
  gfx.drawRect(30,51,245,16);
  gfx.drawString(38,50,"Total System Revenue = £"+String(total_system_revenue));
  Display_Status_Section(100, 0); // Wi-Fi signal strength and Battery voltage
}
//#########################################################################################
void Display_Heading_Section() {
  gfx.drawLine(0, 15, SCREEN_WIDTH, 15);
  gfx.setFont(ArialMT_Plain_10);
  gfx.setTextAlignment(TEXT_ALIGN_RIGHT);
  gfx.drawString(SCREEN_WIDTH - 3, 0, date_time_str);
  gfx.setTextAlignment(TEXT_ALIGN_LEFT);
  gfx.drawString(5, 0, time_str);
}
//#########################################################################################
void Display_EnergyOverview(int sample_year) {
  float total_consumption      = 0;
  float total_selfconsumption  = 0;
  float total_feedin           = 0;
  float total_production       = 0;
  float total_purchased        = 0;
  for (int r = 0; r <= 11; r++) {
    total_consumption     += Consumption[sample_year-2018][r];
    total_selfconsumption += SelfConsumption[sample_year-2018][r];
    total_feedin          += FeedIn[sample_year-2018][r];
    total_production      += Production[sample_year-2018][r];
    total_purchased       += Purchased[sample_year-2018][r];
  }
  float selfconsumption_saving = 0;
  if (sample_year >= 2019 && query_month.toInt() > 3) selfconsumption_saving = total_selfconsumption * 0.16443; // Tarif as of April 2019 for Electricity
  else selfconsumption_saving = total_selfconsumption * 0.133245; // Tarif as of Sept 2018 for Electricity
  gfx.setFont(ArialRoundedMTBold_14);
  gfx.setTextAlignment(TEXT_ALIGN_LEFT);
  gfx.drawString(10, 15, "Total Production: " + Display_Power(LifeTimeEnergy)+ "  £" + String(Revenue) );
  gfx.drawString(10, 30, "Monthly: " + Display_Power(LastMonthEnergy)+ " " + "Today: " + Display_Power(LastDayEnergy / 1000)); // Daily consumption is usually in the range 0-30KWhr
}
//#########################################################################################
float Display_MonthlyEnergySummary(int sample_year) {
  if (sample_year > current_year) return 0;
  float Total_Production = 0;
  float Total_SelfConsumption = 0;
  float feed_in_tariff        = 0.0655;
  float current_supply_tariff = 0.133245;
  for (int m = 0; m < 12; m++) {
    Total_Production      += Production[sample_year-2018][m]*feed_in_tariff;
    if ((sample_year == 2019 && m > 03)) current_supply_tariff = 0.16443;
    Total_SelfConsumption += SelfConsumption[sample_year-2018][m]*current_supply_tariff;
  }
  gfx.setFont(ArialMT_Plain_10);
  gfx.setTextAlignment(TEXT_ALIGN_LEFT);
  gfx.drawString(10,70+(sample_year-2018)*10,String(sample_year)+
                        " ~  £"+String(Total_Production,2)+
                        " + £"+String(Total_SelfConsumption,2));
  gfx.drawString(150,70+(sample_year-2018)*11,"An annual total of  £"+String(Total_Production+Total_SelfConsumption,2));
  return Total_Production+Total_SelfConsumption;
}
//#########################################################################################
float total_revenue(){
  float Total_Production = 0;
  float Total_SelfConsumption = 0;
  float feed_in_tariff = 0.0655;
  float current_supply_tariff = 0.133245;
  for (int y = 2018; y <= 2022; y++){
    for (int m = 0; m < 12; m++) {
      if ((y == 2019 && m > 03) || 2019 > 1) current_supply_tariff = 0.16443;
      Total_Production      += isnan(Production[y-2018][m])?0:Production[y-2018][m] * feed_in_tariff;
      Total_SelfConsumption += isnan(SelfConsumption[y-2018][m])?0:SelfConsumption[y-2018][m] * current_supply_tariff;
    }
  }
  return (Total_Production + Total_SelfConsumption);
}
//#########################################################################################
void Display_YearlyEnergySummary(int sample_year) {
  gfx.setFont(ArialRoundedMTBold_14);
  gfx.setTextAlignment(TEXT_ALIGN_CENTER);
  //gfx.drawString(SCREEN_WIDTH / 2, 170, "Yearly Summary");
  gfx.setFont(ArialMT_Plain_10);
  gfx.setTextAlignment(TEXT_ALIGN_LEFT);
}
//#########################################################################################
String Display_Power(float reading) {
  String units = " KWhr";
  if (reading / 1000 >= 1) {
    units    = " KWhr";
  }
  if (reading / 1000000 >= 1) {
    units    = " MWhr";
  }
  if (reading / 1000000 >= 1) return String(reading / 1000000, 4) + units;
  if (reading / 1000    >= 1) return String(reading / 1000, 2) + units;
  return String(reading, 2) + units;
}
//#########################################################################################
bool Obtain_Energy_Reading(String RequestType, String Request, int sample_year) {
  String rxtext = "";
  if (sample_year > current_year) return true; // Can't sample ahead of time so return true to signify success
  Serial.println("Connecting to server for " + RequestType + " data");
  client.stop(); // close connection before sending a new request
  if (client.connect(server, 443)) { // if the connection succeeds
    Serial.println("Connecting...");
    // send the HTTP PUT request:
    client.println(Request);
    client.println("Host: monitoringapi.solaredge.com");
    client.println("User-Agent: ESP Energy Receiver/1.1");
    client.println("Connection: close");
    client.println();
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 10000) {  // Server can be slow so give it time
        Serial.println(">>> Client Timeout !");
        client.stop();
        return false;
      }
    }
    char c = 0;
    bool startJson = false;
    int jsonend = 0;
    while (client.available()) {
      c = client.read();
      Serial.print(c);
      // JSON formats contain an equal number of open and close curly brackets, so check that JSON is received correctly by counting open and close brackets
      if (c == '{') {
        startJson = true; // set true to indicate JSON message has started
        jsonend++;
      }
      if (c == '}') {
        jsonend--;
      }
      if (startJson == true) {
        rxtext += c;  // Add in the received character
      }
      // if jsonend = 0 then we have have received equal number of curly braces
      if (jsonend == 0 && startJson == true) {
        Serial.println("Received OK...");
        //Serial.println(rxtext);
        if (!DecodeEnergyData(rxtext, RequestType, sample_year)) return false;
        client.stop();
        return true;
      }
    }
  }
  else {
    // if no connection was made:
    Serial.println("connection failed");
    return false;
  }
  rxtext = "";
  return true;
}
//#########################################################################################
// Problems with stucturing JSON decodes, see here: https://arduinojson.org/assistant/
bool DecodeEnergyData(String json, String Type, int sample_year) {
  Serial.print(F("Creating object...and "));
  DynamicJsonDocument doc(40 * 1024);
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, json);
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return false;
  }
  // convert it to a JsonObject
  JsonObject root = doc.as<JsonObject>();
  Serial.println(" Decoding " + Type + " data");
  if (Type == "Overview") {
    // All Serial.println statements are for diagnostic purposes and not required, remove if not needed
    JsonObject  overview = root["overview"];
    const char* overview_lastUpdateTime       = overview["lastUpdateTime"];          // "2018-09-20 10:48:56"
    long        overview_lifeTimeData_energy  = overview["lifeTimeData"]["energy"];  // 350820
    float       overview_lifeTimeData_revenue = overview["lifeTimeData"]["revenue"]; // 22.95513
    long        overview_lastYearData_energy  = overview["lastYearData"]["energy"];  // 350460
    long        overview_lastMonthData_energy = overview["lastMonthData"]["energy"]; // 225261
    int         overview_lastDayData_energy   = overview["lastDayData"]["energy"];   // 680
    int         overview_currentPower_power   = overview["currentPower"]["power"];   // 264
    const char* overview_measuredBy           = overview["measuredBy"];              // "METER"
    LifeTimeEnergy  = overview_lifeTimeData_energy;
    Revenue         = overview_lifeTimeData_revenue;
    LastYearEnergy  = overview_lastYearData_energy;
    LastMonthEnergy = overview_lastMonthData_energy;
    LastDayEnergy   = overview_lastDayData_energy;
  }
  JsonObject energyDetails = root["energyDetails"];
  const char* energyDetails_timeUnit = energyDetails["timeUnit"]; // "MONTH"
  const char* energyDetails_unit = energyDetails["unit"]; // "Wh"
  JsonArray energyDetails_meters = energyDetails["meters"];
  for (int type = 0; type < 5; type++) {
    String energyDetails_meters_type = energyDetails_meters[type]["type"]; // "Consumption","Purchased","FeedIn","Production" or "SelfConsumption" the order supplied varies!
    Serial.println(energyDetails_meters_type);
    if (energyDetails_meters_type == "Consumption") {
      JsonArray energyDetails_meters0_values = energyDetails_meters[type]["values"];
      for (int index = 0; index < 1; index++) {
        mConsumption[index] = int(energyDetails_meters0_values[index]["value"]) / 1000;
      }
    }
    if (energyDetails_meters_type == "SelfConsumption") {
      JsonArray energyDetails_meters1_values = energyDetails_meters[type]["values"];
      for (int index = 0; index < 1; index++) {
        mSelfConsumption[index] = int(energyDetails_meters1_values[index]["value"]) / 1000;
      }
    }
    if (energyDetails_meters_type == "FeedIn") {
      JsonArray energyDetails_meters2_values = energyDetails_meters[type]["values"];
      for (int index = 0; index < 1; index++) {
        mFeedIn[index] = int(energyDetails_meters2_values[index]["value"]) / 1000;
      }
    }
    if (energyDetails_meters_type == "Production") {
      JsonArray energyDetails_meters3_values = energyDetails_meters[type]["values"];
      for (int index = 0; index < 1; index++) {
        mProduction[index] = int(energyDetails_meters3_values[index]["value"]) / 1000;
      }
    }
    if (energyDetails_meters_type == "Purchased") {
      JsonArray energyDetails_meters4_values = energyDetails_meters[type]["values"];
      for (int index = 0; index < 1; index++) {
        mPurchased[index] = int(energyDetails_meters4_values[index]["value"]) / 1000;
      }
    }
  }
  if (Type == "Yearly") {
    JsonObject energyDetails = root["energyDetails"];
    const char* energyDetails_timeUnit = energyDetails["timeUnit"]; // "MONTH"
    const char* energyDetails_unit = energyDetails["unit"]; // "Wh"
    JsonArray energyDetails_meters = energyDetails["meters"];
    for (int type = 0; type < 5; type++) {
      String energyDetails_meters_type = energyDetails_meters[type]["type"]; // "Consumption","Purchased","FeedIn","Production" or "SelfConsumption" the order supplied varies!
      Serial.println(energyDetails_meters_type);
      if (energyDetails_meters_type == "Consumption") {
        JsonArray energyDetails_meters0_values = energyDetails_meters[type]["values"];
        for (int index = 0; index < 12; index++) {
          Consumption[sample_year-2018][index] = int(energyDetails_meters0_values[index]["value"]) / 1000;
        }
      }
      if (energyDetails_meters_type == "SelfConsumption") {
        JsonArray energyDetails_meters1_values = energyDetails_meters[type]["values"];
        for (int index = 0; index < 12; index++) {
          SelfConsumption[sample_year-2018][index] = int(energyDetails_meters1_values[index]["value"]) / 1000;
        }
      }
      if (energyDetails_meters_type == "FeedIn") {
        JsonArray energyDetails_meters2_values = energyDetails_meters[type]["values"];
        for (int index = 0; index < 12; index++) {
          FeedIn[sample_year-2018][index] = int(energyDetails_meters2_values[index]["value"]) / 1000;
        }
      }
      if (energyDetails_meters_type == "Production") {
        JsonArray energyDetails_meters3_values = energyDetails_meters[type]["values"];
        for (int index = 0; index < 12; index++) {
          Production[sample_year-2018][index] = int(energyDetails_meters3_values[index]["value"]) / 1000;
        }
      }
      if (energyDetails_meters_type == "Purchased") {
        JsonArray energyDetails_meters4_values = energyDetails_meters[type]["values"];
        for (int index = 0; index < 12; index++) {
          Purchased[sample_year-2018][index] = int(energyDetails_meters4_values[index]["value"]) / 1000;
        }
      }
    }
  }
  return true;
}
//#########################################################################################
int StartWiFi() {
  int connAttempts = 0;
  Serial.print(F("\r\nConnecting to: ")); Serial.println(String(ssid1));
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid1, password1);
  while (WiFi.status() != WL_CONNECTED ) {
    delay(500); Serial.print(".");
    if (connAttempts > 20) {
      WiFi.disconnect();
      begin_sleep();
    }
    connAttempts++;
  }
  Serial.println("WiFi connected at: " + String(WiFi.localIP()));
  rssi = WiFi.RSSI(); // Get rssi before Wifi is turned off!
  return 1;
}
//#########################################################################################
void StopWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}
//#########################################################################################
void Display_Status_Section(int x, int y) {
  gfx.setFont(ArialMT_Plain_10);
  gfx.drawString(x-35,y+1,version);
  Draw_RSSI(x - 10, y + 11);
  DrawBattery(x + 75, y - 3);
}
//#########################################################################################
void Draw_RSSI(int x, int y) {
  int WIFIsignal = 0;
  int xpos = 1;
  for (int _rssi = -100; _rssi <= rssi; _rssi = _rssi + 20) {
    if (_rssi <= -20)  WIFIsignal = 10; //           <20dbm displays 5-bars
    if (_rssi <= -40)  WIFIsignal = 8; //  -40dbm to -21dbm displays 4-bars
    if (_rssi <= -60)  WIFIsignal = 6; //  -60dbm to -41dbm displays 3-bars
    if (_rssi <= -80)  WIFIsignal = 4;  // -80dbm to -61dbm displays 2-bars
    if (_rssi <= -100) WIFIsignal = 2;  // -100dbm to -81dbm displays 1-bar
    gfx.fillRect(x + xpos * 5, y - WIFIsignal, 4, WIFIsignal);
    xpos++;
  }
  gfx.fillRect(x, y - 1, 4, 1);
}
//#########################################################################################
void DrawBattery(int x, int y) {
  uint8_t percentage = 100;
  float voltage = analogRead(35) / 4096.0 * 7.2;
  if (voltage >= 30 ) { // Only display if there is a valid reading
    Serial.println("Battery voltage = " + String(voltage, 2));
    if (voltage >= 3.8) percentage = 100;
    else if (voltage < 3.60) percentage = 0;
    gfx.setColor(EPD_BLACK);
    gfx.drawRect(x - 22, y + 5, 19, 10);
    gfx.fillRect(x - 2,  y + 8, 2, 5);
    gfx.fillRect(x - 20, y + 7, 16 * percentage / 100.0, 7);
    gfx.setFont(ArialMT_Plain_10);
    gfx.drawString(x - 43, y + 3, String(percentage) + "%");
    gfx.drawString(x + 7,  y + 3, String(voltage, 2) + "v");
  }
}
//#########################################################################################
void SetupTime() {
  configTime(0, 0, "0.uk.pool.ntp.org", "time.nist.gov");
  setenv("TZ", Timezone, 1);
  delay(500);
  UpdateLocalTime();
}
//#########################################################################################
void UpdateLocalTime() {
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo,5000)) {
    Serial.println(F("Failed to obtain time"));
  }
  //See http://www.cplusplus.com/reference/ctime/strftime/
  //Serial.println(&timeinfo, "%a %b %d %Y   %H:%M:%S");     // Displays: Saturday, June 24 2017 14:05:49
  Serial.println(&timeinfo, "%H:%M:%S");                     // Displays: 14:05:49
  char output[30], day_output[30], queryday[30], querymonth[30], queryyear[30];
  strftime(output, 30, "%H", &timeinfo);
  CurrentHour = String(output).toInt();
  strftime(output, 30, "%M", &timeinfo);
  CurrentMin  = String(output).toInt();
  strftime(output, 30, "%S", &timeinfo);
  CurrentSec  = String(output).toInt();
  strftime(queryday ,  30, "%d", &timeinfo);                 // Displays: 24
  strftime(querymonth, 30, "%m", &timeinfo);                 // Displays: 06
  strftime(queryyear,  30, "%Y", &timeinfo);                 // Displays: 2017
  strftime(day_output, 30, "%a  %d-%b-%y", &timeinfo);       // Displays: Sat 24-Jun-17
  strftime(output, 30, "(%H:%M:%S)", &timeinfo);             // Creates: '@ 14:05:49'
  date_time_str = day_output;
  time_str      = output;
  query_day     = queryday;
  query_month   = querymonth;
  query_year    = queryyear;
  current_year  = String(queryyear).toInt();
  current_month = String(querymonth).toInt(); // months range 0-11
}
