#include <EnableInterrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>

#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"

#include <Time.h>

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS,
  ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
  SPI_CLOCK_DIVIDER); // you can change this clock speed

#define WLAN_SSID       "lazy" 
#define WLAN_PASS       "cd0Qqtgf"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define SERVER_IP       128, 2, 83, 208
#define SERVER_IP_S     "128.2.83.208"
#define SERVER_PORT     8001
#define USERNAME        "lazy"

const unsigned long
    dhcpTimeout     = 60L * 1000L, // Max time to wait for address from DHCP
    connectTimeout  = 15L * 1000L, // Max time to wait for server connection
    responseTimeout = 15L * 1000L; // Max time to wait for data from server

int sensorPin = A0;
int ledPin = 8;
int sensorValue = 0;
int onPin = 9;

unsigned long
  lastPolledTime  = 0L, // Last value retrieved from time server
  sketchTime      = 0L; // CPU milliseconds since last server query

time_t currentTime;

uint32_t ip;

Adafruit_CC3000_Client client;

volatile bool awake = false;
volatile bool activated = false;

void setup() {
    // put your setup code here, to run once:
    pinMode(ledPin, OUTPUT);
    pinMode(sensorPin, INPUT);

    delay(100);

    Serial.begin(115200);
    Serial.println(F("Initializing CC3000..."));
    if (!cc3000.begin()) {
        Serial.println(F("Couldn't begin()! Check your wiring?"));
        while(1);
    }

    if (!cc3000.deleteProfiles()) {
        Serial.println(F("Failed!"));
        while(1);
    }

    // Connect to the AP.
    while (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
        Serial.println(F("Failed!"));
    }
    Serial.println(F("Connected!"));

    Serial.println(F("Request DHCP"));
    int attempts = 0;
    while (!cc3000.checkDHCP()) {
        if (attempts > 5) {
            Serial.println(F("DHCP didn't finish!"));
        }
        attempts += 1;
        delay(500);
    }

    /* Display the IP address DNS, Gateway, etc. */  
    displayConnectionDetails();

    // unsigned long t  = getTime(); // Query time server
    // if(t) {                       // Success?
    //     lastPolledTime = t;         // Save time
    //     sketchTime     = millis();  // Save sketch time of last valid time query
    //     currentTime = lastPolledTime + (millis() - sketchTime) / 1000;
    // } else {
        currentTime = 1429848335;
    // }

    setTime(currentTime);
    
    ip = cc3000.IP2U32(SERVER_IP);

    // delay(1000);

    enableInterrupt(sensorPin, rise, RISING);
}
 
void loop() {
    if(awake) {
        Serial.println("Wake up!");
        int count = 0;
        digitalWrite(ledPin, HIGH);

        //String startTime = "2015-04-03T03:45";
        String startTime = getCurrentTime();
        Serial.println(startTime);

        while(activated) {
            sensorValue = analogRead(sensorPin);
        
            Serial.println(sensorValue, DEC);
            if(sensorValue < 200) {
                if(count++ >= 3) {
                    activated = false;
                    String endTime = getCurrentTime();
                    digitalWrite(ledPin, LOW);
                    sendData(startTime, endTime);
                    break;
                }
            } else {
                count = 0;
            }
            Serial.println(count, DEC);
            delay(1000);
        }
    }
    //digitalWrite(ledPin, LOW);
    sleep();
}

// Put the Arduino to sleep.
void sleep()
{
    awake = false;

    // Set sleep to full power down.  Only external interrupts or 
    // the watchdog timer can wake the CPU!
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    // Turn off the ADC while asleep.
    power_adc_disable();

    // Enable sleep and enter sleep mode.
    sleep_mode();

    // CPU is now asleep and program execution completely halts!
    // Once awake, execution will resume at this point.

    // When awake, disable sleep mode and turn on all devices.
    sleep_disable();
    power_all_enable();
}

void rise() {
    awake = true;
    activated = true;
}

void sendData(String beginTime, String endTime) {
    Serial.print(F("\n\rPinging ")); cc3000.printIPdotsRev(ip); Serial.print("...");  
    uint8_t replies = cc3000.ping(ip, 5);
    Serial.print(replies); Serial.println(F(" replies"));
    if (replies)
        Serial.println(F("Ping successful!"));

    Serial.print(F("Connecting to server..."));
    Adafruit_CC3000_Client client = cc3000.connectTCP(ip, SERVER_PORT);
    if(client.connected()) {
        Serial.print(F("Connected!"));

        String data0 = "user_name=" + String(USERNAME);
        String data1 = "&time_recorded=" + String(beginTime);
        String data2 = "&start=" + String(beginTime);
        String data3 = "&end=" + String(endTime);
        int dataLength = data0.length() + data1.length() + data2.length() + data3.length();

        // Serial.println(F("\nSending:"));
        // Serial.println(data0);
        // Serial.println(data1);
        // Serial.println(data2);
        // Serial.println(data3);

        //String data = "start=2015-02-03T00:00&user_name=lazy&time_recorded=2015-02-03T00:00&end=2015-03-02T00:10";
        //dataLength = data.length();

        client.fastrprint(F("POST /api/v1/washroom/ HTTP/1.1\r\n"));
        //client.fastrprint(F("Host: 128.2.83.208:8001\r\n"));
        
        client.fastrprint(F("Host: ")); client.print(SERVER_IP_S);
        client.fastrprint(F(":")); client.print(SERVER_PORT);
        client.fastrprint(F("\r\n"));
        client.fastrprint(F("Accept:*/*\r\n"));
        client.fastrprint(F("Accept-Encoding:gzip, deflate\r\n"));
        client.fastrprint(F("User-Agent: runscope/0.1\r\n"));
        client.fastrprint(F("Content-Type: application/x-www-form-urlencoded\r\n"));
        client.fastrprint(F("Connection: close\r\n"));
        client.fastrprint(F("Content-Length: "));
        client.print(dataLength);
        client.fastrprint("\r\n");
        client.println();
        client.print(data0);
        client.print(data1);
        client.print(data2);
        client.print(data3);
        client.println();

        Serial.println(F("\nSent:"));
        Serial.println(data0);
        Serial.println(data1);
        Serial.println(data2);
        Serial.println(data3);

        Serial.println("\n----------");
        unsigned long lastRead = millis();
        while(client.connected() && (millis() - lastRead < 3000)) {
            while (client.available()) {
                char c = client.read();
                Serial.print(c);
                lastRead = millis();
            }
        }
        Serial.println("\n----------");
    } else {
        Serial.println(F("failed"));
        return;
    }

    delay(1000);
    client.close();
}

bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

// Minimalist time server query; adapted from Adafruit Gutenbird sketch,
// which in turn has roots in Arduino UdpNTPClient tutorial.
unsigned long getTime(void) {

  uint8_t       buf[48];
  unsigned long ip, startTime, t = 0L;

  Serial.print(F("Locating time server..."));

  // Hostname to IP lookup; use NTP pool (rotates through servers)
  if(cc3000.getHostByName("pool.ntp.org", &ip)) {
    static const char PROGMEM
      timeReqA[] = { 227,  0,  6, 236 },
      timeReqB[] = {  49, 78, 49,  52 };

    Serial.println(F("\r\nAttempting connection..."));
    startTime = millis();
    do {
      client = cc3000.connectUDP(ip, 123);
    } while((!client.connected()) &&
            ((millis() - startTime) < connectTimeout));

    if(client.connected()) {
      Serial.print(F("connected!\r\nIssuing request..."));

      // Assemble and issue request packet
      memset(buf, 0, sizeof(buf));
      memcpy_P( buf    , timeReqA, sizeof(timeReqA));
      memcpy_P(&buf[12], timeReqB, sizeof(timeReqB));
      client.write(buf, sizeof(buf));

      Serial.print(F("\r\nAwaiting response..."));
      memset(buf, 0, sizeof(buf));
      startTime = millis();
      while((!client.available()) &&
            ((millis() - startTime) < responseTimeout));
      if(client.available()) {
        client.read(buf, sizeof(buf));
        t = (((unsigned long)buf[40] << 24) |
             ((unsigned long)buf[41] << 16) |
             ((unsigned long)buf[42] <<  8) |
              (unsigned long)buf[43]) - 2208988800UL;
        Serial.print(F("OK\r\n"));
      }
      client.close();
    }
  }
  if(!t) Serial.println(F("error"));
  return t;
}

// Return time as a String in the format
// YYYY-MM-DDTHH:MM:SS
String getCurrentTime() {
    int temp;
    String t = String(year()) + "-";
    temp = month();
    if(temp <= 9)
        t += "0";
    t += String(temp) + "-";
    temp = day();
    if(temp <= 9)
        t += "0";
    t += String(temp) + "T";
    temp = hour();
    if(temp <= 9)
        t += "0";
    t += String(temp) + ":";
    temp = minute();
    if(temp <= 9)
        t += "0";
    t += String(temp) + ":";

    temp = second();
    if(temp <= 9)
        t += "0";
    t += String(temp);
    return t;
}