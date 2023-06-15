// Change to your WiFi credentials
const char* ssid1     = "Router SSID";
const char* password1 = "Router Password";

String apikey                = "<Your API Key";      // e.g. I6CLG1OK2HN7P1OJ7OM56B7751N0JJLD
String SiteNumber            = "<Your Site Number>"; // e.g. 123456

const char  server[]         = "monitoringapi.solaredge.com";
const char DailyRequest[]    = "GET https://monitoringapi.solaredge.com/site/<your site number>/overview?api_key=<your API key> HTTP/1.0";

// Example: const char DailyRequest[]    = "GET https://monitoringapi.solaredge.com/site/123456/overview?api_key=I6CLG1OK2HN7P1OJ7OM56B7751N0JJLD HTTP/1.0";

//const char YearlyRequest[] = "GET https://monitoringapi.solaredge.com/site/<your site number>/energyDetails?meters=&timeUnit=MONTH&startTime=2018-01-01%2000:00:00&endTime=2018-12-31%2023:59:00&api_key=<your API key>  HTTP/1.0";

//Set your location according to OWM locations
String City          = "Melksham";                      // Your home City
String Country       = "GB";                            // Your country  
const char* Timezone = "GMT0BST,M3.5.0/01,M10.5.0/02";  // Choose your time zone from: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv 
// Example time zones
//const char* Timezone = "MET-1METDST,M3.5.0/01,M10.5.0/02"; // Most of Europe
//const char* Timezone = "CET-1CEST,M3.5.0,M10.5.0/3";       // Central Europe
//const char* Timezone = "EST-1METDST,M3.5.0/01,M10.5.0/02"; // Most of Europe
//const char* Timezone = "EST5EDT,M3.2.0,M11.1.0";           // EST USA  
//const char* Timezone = "CST6CDT,M3.2.0,M11.1.0";           // CST USA
//const char* Timezone = "MST7MDT,M4.1.0,M10.5.0";           // MST USA
//const char* Timezone = "NZST-12NZDT,M9.5.0,M4.1.0/3";      // Auckland
//const char* Timezone = "EET-2EEST,M3.5.5/0,M10.5.5/0";     // Asia
//const char* Timezone = "ACST-9:30ACDT,M10.1.0,M4.1.0/3":   // Australia
