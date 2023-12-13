/**
 * @file christmasLightController.ino
 * @brief Christmas Lights Controller using ESP8266 Thing Dev board.
 *        This program turns the lights on at ON_TIME_HOUR and off at OFF_TIME_HOUR.
 */

#include <ArduinoWiFiServer.h>
#include <BearSSLHelpers.h>
#include <CertStoreBearSSL.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiGratuitous.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiServer.h>
#include <WiFiServerSecure.h>
#include <WiFiServerSecureBearSSL.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

#define ON_TIME_HOUR 16                             ///< Time when the lights turn on (24 hours)
#define OFF_TIME_HOUR 1                             ///< Time when the lights turn off (24 hours), needs to be after midnight
#define TIME_ZONE -5                                ///< Time zone (Eastern Standard Time)
#define RELAY_PIN 14                                ///< Relay control pin
#define LOCAL_PORT  8888                            ///< Local port for UDP packets
#define NTP_PACKET_SIZE  48                         ///< NTP packet size
#define NTP_SERVER_NAME "pool.ntp.org"              ///< NTP Server

#define SSID "YOUR SSID HERE"                       ///< WiFi SSID
#define PASSWORD "YOUR PASSWORD HERE"               ///< WiFi PASSWORD


WiFiUDP Udp;                                        ///< UDP instance for NTP
byte packetBuffer[NTP_PACKET_SIZE];                 ///< Buffer for NTP packets

time_t getNtpTime();
void sendNTPpacket(IPAddress &address);

ESP8266WebServer server(80);                        ///< Web server instance
bool lightState = false;                            ///< Current state of light


/**
 * @brief Setup function to initialize the controller
 */
void setup() {
    Serial.begin(115200);                           // Initialize serial communication
    pinMode(RELAY_PIN, OUTPUT);                      // Set relay pin as output
    digitalWrite(RELAY_PIN, LOW);                    // Initialize relay to off

    WiFi.begin(SSID, PASSWORD);                     // Connect to Wi-Fi network
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);                                // Wait for connection
        Serial.println("Connecting to WiFi...");    // Log connection status
    }
    Serial.println("Connected to WiFi");            // Log successful connection
    Serial.print("IP Address: ");                   // Log IP address
    Serial.println(WiFi.localIP());

    server.on("/", handleRoot);                     // Route for root path
    server.on("/on", handleLightsOn);               // Route to turn lights on
    server.on("/off", handleLightsOff);             // Route to turn lights off
    server.on("/checktime", handleCheckTime);       // Route to check time

    Udp.begin(LOCAL_PORT);                           // Start UDP client for NTP
    setSyncProvider(getNtpTime);                    // Set NTP time sync provider
    setSyncInterval(43200);                         // Sync time every 12 hours

    server.begin();                                 // Start the web server
}

/**
 * @brief Main loop function
 */
void loop() {
    server.handleClient();                          // Handle client requests
    int currentHour = hour();                       // Get current hour
    if (currentHour == ON_TIME_HOUR){
        if (!lightState){
            handleLightsOn();                       // Turn lights on at specified hour
        }
    } else if (currentHour == OFF_TIME_HOUR) {
        if (lightState){
            handleLightsOff();                      // Turn lights off at specified hour
        }
    }
}

/**
 * @brief Handles the root URL ("/")
 */
void handleRoot() {
    // HTML content for the web page
    String html = "<html><head><title>Christmas Lights Controller</title><style>"
                  "body { font-family: Arial, sans-serif; background-color: #7DBD7D; color: #FFF; text-align: center; }"
                  "h1 { color: #FF6F61; }"
                  "button { background-color: #FFD700; color: #FFF; border: none; padding: 15px 32px; text-align: center;"
                  "text-decoration: none; display: inline-block; font-size: 16px; margin: 4px 2px; cursor: pointer; }"
                  "#status, #timeStatus { color: #FFF; margin-top: 20px; font-size: 20px; }"
                  "</style><script>"
                  "function toggleLight(state) {"
                  "var xhttp = new XMLHttpRequest();"
                  "xhttp.onreadystatechange = function() {"
                  "if (this.readyState == 4 && this.status == 200) {"
                  "document.getElementById('status').innerHTML = this.responseText;"
                  "}};"
                  "xhttp.open('GET', state, true);"
                  "xhttp.send();"
                  "}"
                  "function checkTime() {"
                  "var xhttp = new XMLHttpRequest();"
                  "xhttp.onreadystatechange = function() {"
                  "if (this.readyState == 4 && this.status == 200) {"
                  "document.getElementById('timeStatus').innerHTML = this.responseText;"
                  "}};"
                  "xhttp.open('GET', 'checktime', true);"
                  "xhttp.send();"
                  "}"
                  "</script></head><body>"
                  "<h1>Christmas Lights Controller</h1>"
                  "<button onclick=\"toggleLight('on')\">Turn On</button>"
                  "<button onclick=\"toggleLight('off')\">Turn Off</button>"
                  "<p id='status'></p>"
                  "<button onclick=\"checkTime()\">Check Current Time</button>"
                  "<p id='timeStatus'></p>"
                  "</body></html>";

    server.send(200, "text/html", html);            // Send the web page
}

/**
 * @brief Handles the "/checktime" URL for NTP time check
 */
void handleCheckTime() {
    // Format and send current time
    String currentTime = String(hour()) + ":" + String(minute()) + ":" + String(second());
    server.send(200, "text/plain", "Current NTP Time: " + currentTime);
}

/**
 * @brief Turns the lights on
 */
void handleLightsOn() {
    digitalWrite(RELAY_PIN, HIGH);                   // Turn on the relay
    lightState = true;                              // Update light state
    Serial.println("Lights turned ON");             // Log to serial
    server.send(200, "text/plain", "Lights turned ON");
}

/**
 * @brief Turns the lights off
 */
void handleLightsOff() {
    digitalWrite(RELAY_PIN, LOW);                    // Turn off the relay
    lightState = false;                             // Update light state
    Serial.println("Lights turned OFF");            // Log to serial
    server.send(200, "text/plain", "Lights turned OFF");
}

/**
 * @brief Retrieves the time from an NTP server
 * @return The current time
 */
time_t getNtpTime() {
    IPAddress ntpServerIP;                          // NTP server's IP address

    while (Udp.parsePacket() > 0) ;                 // Discard any previously received packets
    Serial.println("Transmit NTP Request");         // Log NTP request transmission
    WiFi.hostByName(NTP_SERVER_NAME, ntpServerIP);    // Resolve NTP server IP
    sendNTPpacket(ntpServerIP);                     // Send NTP packet

    uint32_t beginWait = millis();                  // Start waiting for response
    while (millis() - beginWait < 1500) {
        int size = Udp.parsePacket();
        if (size >= NTP_PACKET_SIZE) {
            Serial.println("Receive NTP Response"); // Log NTP response receipt
            Udp.read(packetBuffer, NTP_PACKET_SIZE);// Read packet into the buffer
            unsigned long secsSince1900 = (unsigned long)packetBuffer[40] << 24;
            secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
            secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
            secsSince1900 |= (unsigned long)packetBuffer[43];
            return secsSince1900 - 2208988800UL + SECS_PER_HOUR * TIME_ZONE; // Calculate and return time
        }
    }
    Serial.println("No NTP Response :-(");         // Log failure to receive NTP response
    return 0;                                      // Return 0 if unable to get the time
}

/**
 * @brief Sends an NTP request packet to a server
 * @param address The IP address of the NTP server
 */
void sendNTPpacket(IPAddress &address) {
    memset(packetBuffer, 0, NTP_PACKET_SIZE);      // Clear the buffer
    // Initialize values for NTP request
    packetBuffer[0] = 0b11100011;                  // LI, Version, Mode
    packetBuffer[1] = 0;                           // Stratum, or type of clock
    packetBuffer[2] = 6;                           // Polling Interval
    packetBuffer[3] = 0xEC;                        // Peer Clock Precision
    // Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;

    Udp.beginPacket(address, 123);                 // NTP requests are to port 123
    Udp.write(packetBuffer, NTP_PACKET_SIZE);      // Write packet data
    Udp.endPacket();                               // Send the packet
}
