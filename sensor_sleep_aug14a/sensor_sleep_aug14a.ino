#include <WiFi.h>
#include "time.h"
#include "sntp.h"
#include <BLEDevice.h>

const char* ssid       = "test";
const char* password   = "testtest";

const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long  gmtOffset_sec = -18000;
const int   daylightOffset_sec = 3600;

int LED_PIN = 3;// declare and initialize a variable to held the LED pin 3 of the arduino
int trig = 12;// declare and initialize a variable to held the pin 12 of the arduino
int echo = 13;// declare and initialize a variable to held the pin 13 of the arduino
float duration, distance;// declare two variable of type float for the time and the the distance

unsigned long ledStartTime = 0; // set a variable that will hold the time the LED turns on
const unsigned long ledDuration = 30000; // set a variable that will hold the LED on duration (30 seconds)

#define msToSConverter 1000000  // Conversion factor for micro seconds to seconds
#define sleepDuration  1800        // Time ESP32 will go to sleep (in seconds)

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("No time available (yet)");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t)
{
  Serial.println("Got time adjustment from NTP!");
  printLocalTime();
}

void setup() {
  pinMode(trig, OUTPUT);// initialize trig as an output
  pinMode(echo, INPUT);// initialize echo as an input
  pinMode(LED_PIN, OUTPUT);// initialize LED as an input
  digitalWrite(LED_PIN, LOW); // initialize the LED light off
  Serial.begin(115200);// initialize serial communication at 115200 bits per seconds
  sntp_set_time_sync_notification_cb( timeavailable );
  sntp_servermode_dhcp(1);    // (optional)

  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.mode(WIFI_STA); // turn wifi to station mode
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" WIFI CONNECTED");

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  printLocalTime();

  //disconnect WiFi
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // disconnect Bluetooth
  BLEDevice::deinit();
}

void loop() {
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  
  int hour = timeinfo.tm_hour;
  int min = timeinfo.tm_min;
  int sec = timeinfo.tm_sec;

  if ((hour == 17 && min <= 0 && sec >= 0) || (hour == 17 && min >= 30 && sec >= 0)) {
    digitalWrite(trig, LOW);// set trig to LOW
    delayMicroseconds(2);// wait 2 microseconds
    digitalWrite(trig, HIGH);// set trig to HIGH
    delayMicroseconds(10);// wait 10 microseconds
    digitalWrite(trig, LOW);// set trig to LOW
    duration = pulseIn(echo, HIGH, 50000);// use the function pulsein to detect the time of the echo when it is in a high state
    distance = (duration * 0.0343 / 2) * 0.393701; // get the distance in inches

    if (distance > 0.0 && distance < 60.0) { // if object is within 5 feet
      digitalWrite(LED_PIN, HIGH);  // turn light on
      printLocalTime();
      ledStartTime = millis(); // set ledStartTime to current run time
      Serial.print("Distance: ");// print "Distance: "
      Serial.print(distance);// print the value of distance
      Serial.println(" in");// print " in"
    } else if (millis() - ledStartTime < ledDuration) { // if the total runtime - the led start time is shorter than the wanted duration...
      digitalWrite(LED_PIN, HIGH); // keep light on
    } else { // if both conditions above are false...
      digitalWrite(LED_PIN, LOW); // turn/keep light off
    }
  } else {
    digitalWrite(LED_PIN, LOW);
    esp_sleep_enable_timer_wakeup(sleepDuration * msToSConverter);
    Serial.println("Going to sleep now");
    Serial.flush(); 
    esp_deep_sleep_start();
  }

  delay(500);// wait half a second
}  
