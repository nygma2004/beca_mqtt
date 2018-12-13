/*
 ************** HEADER DECLARATION *************
#include <WiFiUdp.h>                // Added for NTP functionality
#include <Time.h>
#include <TimeLib.h>
#include <sunMoon.h>                // Sunrise, sunset calculation

 ************** Add to Setup *******************
  // Start UDP for NTP function
  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  requestNTPUpdate();
  // Initialize sunrise and sunset calculator
  sm.init(GMTOffset, sm_latitude, sm_longtitude);

 ************** Add to Loop ********************
  // Check if NTP update is due
  if ((millis() - NTPUpdateMillis >= 60*60*1000) && (!NTPRequested)) {  
    requestNTPUpdate();
  }    

  // Check for NTP response
  int cb = udp.parsePacket();
  if (cb!=0) {
    NTPUpdateMillis = millis();
    NTPRequested = false;
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    const unsigned long seventyYears = 2208988800UL;
    epoch = secsSince1900 - seventyYears;
    if (summerTime(epoch)) {
      epoch += 3600; // add 1 hour DST
    }
    epoch += GMTOffset * 60;
    Serial.print("NTP Update (epoch): ");
    Serial.println(epoch);
    Serial.print("NTP Update (time): ");
    printDate(epoch);  Serial.println("");

    // Sunrise and Sunset calculations
    time_t sRise = sm.sunRise(epoch);
    time_t sSet  = sm.sunSet(epoch);
    sRise += epoch / 86400L * 86400L;
    sSet += epoch / 86400L * 86400L;
    if (summerTime(epoch)) {
      sRise += 3600; // add 1 hour DST
      sSet += 3600; // add 1 hour DST
    }
    Serial.print("Today sunrise and sunset: ");
    printDate(sRise); Serial.print("; ");
    printDate(sSet);  Serial.println("");
  }  

 */

// Update the below global variables for your project
const char* ntpServerName = "hu.pool.ntp.org";
int GMTOffset = 60;                      // Offset to GMT time in minutes, CET: GMT+1 hour = 60 minutes
long sm_latitude = 47.44;                // Your coordinates, get it from: http://www.mapseasy.com/adress-to-gps-coordinates.php
long sm_longtitude = 19.02;

unsigned long NTPUpdateMillis = 0;       // Last time NTP update was received
bool NTPRequested = false;               // flag that NTP update requested
unsigned long epoch = 0;                 // Unix time received from NTP
unsigned int localPort = 2390;           // local port to listen for UDP packets
const int NTP_PACKET_SIZE = 48;         
byte packetBuffer[ NTP_PACKET_SIZE];

IPAddress timeServerIP;
WiFiUDP udp;
sunMoon  sm;

static const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
#define LEAP_YEAR(Y) ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )


struct  strDateTime
{
  byte hour;
  byte minute;
  byte second;
  int year;
  byte month;
  byte day;
  byte wday;
} ;

strDateTime DateTime;                      // Global DateTime structure, will be refreshed every Second

void printDate(time_t date) {
  char buff[20];
  sprintf(buff, "%02d-%02d-%4d %02d:%02d:%02d",
  day(date), month(date), year(date), hour(date), minute(date), second(date));
  Serial.print(buff);
}

void printTime(time_t date) {
  char buff[20];
  sprintf(buff, "%02d:%02d:%02d", hour(date), minute(date), second(date));
  Serial.print(buff);
}

byte getHour(time_t date) {
  return hour(date);
}

byte getMinute(time_t date) {
  return minute(date);
}

byte calcDayOfWeek (time_t date) {
  int y;
  int m;
  int d;
  m = month(date);
  y = year(date)-2000;
  d = day(date);  
  // Old mental arithmetic method for calculating day of week
  // adapted for Arduino, for years 2000~2099
  // returns 1 for Sunday, 2 for Monday, etc., up to 7 for Saturday
  // for "bad" dates (like Feb. 30), it returns 0
  // Note: input year (y) should be a number from 0~99
  if (y > 99) return 0; // we don't accept years after 2099
  // we take care of bad months later
  if (d < 1) return 0; // because there is no day 0
  byte w = 6; // this is a magic number (y2k fix for this method)
  // one ordinary year is 52 weeks + 1 day left over
  // a leap year has one more day than that
  // we add in these "leftover" days
  w += (y + (y >> 2));
  // correction for Jan. and Feb. of leap year
  if (((y & 3) == 0) && (m <= 2)) w--;
  // add in "magic number" for month
  switch (m) {
    case 1:  if (d > 31) return 0; w += 1; break;
    case 2:  if (d > ((y & 3) ? 28 : 29)) return 0; w += 4; break;
    case 3:  if (d > 31) return 0; w += 4; break;
    case 4:  if (d > 30) return 0; break;
    case 5:  if (d > 31) return 0; w += 2; break;
    case 6:  if (d > 30) return 0; w += 5; break;
    case 7:  if (d > 31) return 0; break;
    case 8:  if (d > 31) return 0; w += 3; break;
    case 9:  if (d > 30) return 0; w += 6; break;
    case 10: if (d > 31) return 0; w += 1; break;
    case 11: if (d > 30) return 0; w += 4; break;
    case 12: if (d > 31) return 0; w += 6; break;
    default: return 0;
  }
  // then add day of month
  w += d;
  // there are only 7 days in a week, so we "cast out" sevens
  while (w > 7) w = (w >> 3) + (w & 7);
  w--;
  if (w==0) {
    w = 7; // Beca expects 7 for Sunday
  }
  
  return w;
}

unsigned long getDayOfWeek(time_t date) {
// returns the day of week, 1 Monday, 7 Sunday
/*
  unsigned long dn;
  int y;
  int m;
  int d;
  byte dow;
  m = month(date);
  y = year(date);
  d = day(date);
  m = (m + 9) % 12;
  y -= m / 10;
  dn = 365*y + y/4 - y/100 + y/400 + (m*306 + 5)/10 + (d - 1);

  dow = (dn - 2) % 7;
  if (dow==0) {
    dow = 7; // Beca expects 7 for Sunday
  }
  return dow;
*/

  unsigned long y;
  unsigned long  m;
  unsigned long  d;
  unsigned long f;
  unsigned long g;
  unsigned long n;
  m = month(date);
  y = year(date);
  d = day(date);
  
 if ( m <= 2)
  f = y - 1;
 else
  f = y;
  
 if ( m <= 2)
  g = m +13;
 else
  g = m +1;

   n = 1461 * f / 4 + 153 * g / 5 + d;
    n = n - 621049 % 7;
      if (n==0) {
    n = 7; // Beca expects 7 for Sunday
  }
  return n;
}

strDateTime ConvertUnixTimeStamp( unsigned long _tempTimeStamp) {
  strDateTime _tempDateTime;
  uint8_t year;
  uint8_t month, monthLength;
  uint32_t time;
  unsigned long days;

  time = (uint32_t)_tempTimeStamp;
  _tempDateTime.second = time % 60;
  time /= 60; // now it is minutes
  _tempDateTime.minute = time % 60;
  time /= 60; // now it is hours
  _tempDateTime.hour = time % 24;
  time /= 24; // now it is days
  _tempDateTime.wday = ((time + 4) % 7) + 1;  // Sunday is day 1

  year = 0;
  days = 0;
  while ((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time) {
    year++;
  }
  _tempDateTime.year = year; // year is offset from 1970

  days -= LEAP_YEAR(year) ? 366 : 365;
  time  -= days; // now it is days in this year, starting at 0

  days = 0;
  month = 0;
  monthLength = 0;
  for (month = 0; month < 12; month++) {
    if (month == 1) { // february
      if (LEAP_YEAR(year)) {
        monthLength = 29;
      } else {
        monthLength = 28;
      }
    } else {
      monthLength = monthDays[month];
    }

    if (time >= monthLength) {
      time -= monthLength;
    } else {
      break;
    }
  }
  _tempDateTime.month = month + 1;  // jan is month 1
  _tempDateTime.day = time + 1;     // day of month
  _tempDateTime.year += 1970;

  return _tempDateTime;
}

//
// Summertime calculates the daylight saving time for middle Europe. Input: Unixtime in UTC
//
boolean summerTime(unsigned long _timeStamp ) {
  strDateTime  _tempDateTime = ConvertUnixTimeStamp(_timeStamp);
  // printTime("Innerhalb ", _tempDateTime);

  if (_tempDateTime.month < 3 || _tempDateTime.month > 10) return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
  if (_tempDateTime.month > 3 && _tempDateTime.month < 10) return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
  if (_tempDateTime.month == 3 && (_tempDateTime.hour + 24 * _tempDateTime.day) >= (3 +  24 * (31 - (5 * _tempDateTime.year / 4 + 4) % 7)) || _tempDateTime.month == 10 && (_tempDateTime.hour + 24 * _tempDateTime.day) < (3 +  24 * (31 - (5 * _tempDateTime.year / 4 + 1) % 7)))
    return true;
  else
    return false;
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("Sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  NTPRequested = true;
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void requestNTPUpdate() {
  WiFi.hostByName(ntpServerName, timeServerIP); 
  Serial.print("Requesting NTP update from ");
  Serial.println(timeServerIP);
  sendNTPpacket(timeServerIP);  
}

