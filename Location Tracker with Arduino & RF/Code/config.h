#ifndef CONFIG_H
#define CONFIG_H

#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <RCSwitch.h>

// ==========================================
// PIN DEFINITIONS
// ==========================================
#define GPS_RX_PIN 11
#define GPS_TX_PIN 10
#define GSM_RX_PIN 4
#define GSM_TX_PIN 3

// --- NEW HARDWARE PINS ---
#define PUSH_BUTTON_PIN 5  // Connect to Button. Other button leg goes to GND.
#define BUZZER_PIN 12      // Connect to Buzzer (+). Other leg goes to GND.

// ==========================================
// OBJECT DECLARATIONS
// ==========================================
extern SoftwareSerial gpsSerial;
extern SoftwareSerial gsmSerial;
extern RCSwitch rfReceiver;
extern TinyGPSPlus gps;

// ==========================================
// USER SETTINGS (Variables defined in main file)
// ==========================================
extern String phoneNumber;
extern const long RF_CODE_SMS;
extern const long RF_CODE_CALL;
extern const long RF_CODE_SMS_CALL;
extern const long RF_CODE_HELLO;

// ==========================================
// FUNCTION DECLARATIONS
// ==========================================
bool getGPSData(float &lat, float &lon);
void sendSMS(bool gpsAvailable, float lat, float lon);
void sendHelloSMS();
void makeCall();
void beepBuzzer(int times, int duration);  // New function for buzzer

#endif
