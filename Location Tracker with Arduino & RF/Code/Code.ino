#include "config.h"

// Object Definitions
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);
SoftwareSerial gsmSerial(GSM_RX_PIN, GSM_TX_PIN);
RCSwitch rfReceiver = RCSwitch();
TinyGPSPlus gps;

// ==========================================
// USER SETTINGS
// ==========================================
String phoneNumber = "+918857944864";  // Change to your number 

// Replace these with your actual RF codes from the Serial Monitor
const long RF_CODE_SMS      = 7079028;  // Button 1
const long RF_CODE_CALL     = 7079032;  // Button 2
const long RF_CODE_SMS_CALL = 7079026;  // Button 3
const long RF_CODE_HELLO    = 7079025;  // Button 4

// Demo coordinates for when actual GPS signal is lost
const float DEMO_LAT = 18.6276424;  //8.6276424,73.821878
const float DEMO_LON = 73.821878; 
// ==========================================

void setup() {
  // Initialize serial communications
  Serial.begin(9600);
  gpsSerial.begin(9600);
  gsmSerial.begin(9600);

  // Initialize RF Receiver (Interrupt 0 = Pin D2)
  rfReceiver.enableReceive(0);  

  // Setup New Hardware Pins
  pinMode(PUSH_BUTTON_PIN, INPUT_PULLUP); // INPUT_PULLUP means button goes to GND
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Turn off buzzer initially
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("======================================");
  Serial.println("System Ready!");
  Serial.println("Waiting for RF remote or Physical Button press...");
  Serial.println("======================================");
  
  // Beep twice to indicate system is online and ready
  beepBuzzer(2, 200); 
  delay(3000);
}

void loop() {
  
  // ----------------------------------------------------
  // 1. CHECK PHYSICAL PUSH BUTTON
  // ----------------------------------------------------
  if (digitalRead(PUSH_BUTTON_PIN) == LOW) {
    Serial.println("\n--- Physical SOS Button Pressed! ---");
    beepBuzzer(1, 500); 
    
    float latitude, longitude;
    bool gpsAvailable = getGPSData(latitude, longitude);
    sendSMS(gpsAvailable, latitude, longitude);
    
    // Wait slightly to ensure network is ready before any next action
    delay(2000); 
  }

  // ----------------------------------------------------
  // 2. CHECK RF REMOTE RECEIVER
  // ----------------------------------------------------
  if (rfReceiver.available()) {
    long receivedData = rfReceiver.getReceivedValue();
    
    Serial.println("\n--- New Signal Detected ---");
    Serial.print("RF Code Received: ");
    Serial.println(receivedData);

    if (receivedData == 0) {
      Serial.println("Status: Unknown encoding or weak signal.");
      beepBuzzer(3, 100); 
    } 
    else if (receivedData == RF_CODE_SMS) {
      Serial.println("Action: Button 1 (Send SMS + Location)");
      beepBuzzer(1, 300); 
      float latitude, longitude;
      bool gpsAvailable = getGPSData(latitude, longitude);
      sendSMS(gpsAvailable, latitude, longitude);
    } 
    else if (receivedData == RF_CODE_CALL) {
      Serial.println("Action: Button 2 (Call Only)");
      beepBuzzer(1, 300); 
      makeCall();
    } 
    else if (receivedData == RF_CODE_SMS_CALL) {
      Serial.println("Action: Button 3 (SMS + Call)");
      beepBuzzer(1, 300); 
      float latitude, longitude;
      bool gpsAvailable = getGPSData(latitude, longitude);
      sendSMS(gpsAvailable, latitude, longitude);
      
      // Crucial Delay: Give GSM module time to recover from sending SMS before calling
      Serial.println("Preparing to dial network...");
      delay(3000); 
      makeCall();
    } 
    else if (receivedData == RF_CODE_HELLO) {
      Serial.println("Action: Button 4 (Hello SMS)");
      beepBuzzer(1, 300);
      sendHelloSMS();
    } 
    else {
      Serial.println("Status: WARNING - This code does not match any saved buttons!");
      beepBuzzer(3, 100); 
    }

    // Reset the receiver for the next button press
    rfReceiver.resetAvailable();
  }
}

// ==========================================
// HELPER FUNCTIONS
// ==========================================

void beepBuzzer(int times, int duration) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(duration);
    digitalWrite(BUZZER_PIN, LOW);
    delay(duration);
  }
}

bool getGPSData(float &lat, float &lon) {
  Serial.println("Searching for GPS... (Waiting 5 seconds)");
  unsigned long start = millis();
  while (millis() - start < 5000) {  
    while (gpsSerial.available()) {
      gps.encode(gpsSerial.read());
    }
    if (gps.location.isUpdated()) {
      lat = gps.location.lat();
      lon = gps.location.lng();
      Serial.println("GPS Fix Acquired!");
      return true;
    }
  }
  Serial.println("GPS Fix Failed. Using Demo Location.");
  return false;
}

void sendSMS(bool gpsAvailable, float lat, float lon) {
  // Clear any junk data from the GSM Serial buffer before sending
  while(gsmSerial.available()) gsmSerial.read();

  String message = "Alert! SOS Button Pressed!\n";
  if (gpsAvailable) {
    message += "Location: http://maps.google.com/maps?q=" + String(lat, 6) + "," + String(lon, 6);
  } else {
    message += " Location: http://maps.google.com/maps?q=" + String(DEMO_LAT, 6) + "," + String(DEMO_LON, 6);
  }

  Serial.println("Sending SMS...");
  gsmSerial.println("AT+CMGF=1");
  delay(1000);
  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(phoneNumber);
  gsmSerial.println("\"");
  delay(1000);
  gsmSerial.println(message);
  gsmSerial.write(26);  // End SMS character (Ctrl+Z)
  delay(5000);
  Serial.println("SMS Sent!");
  beepBuzzer(2, 500); 
}

void sendHelloSMS() {
  while(gsmSerial.available()) gsmSerial.read(); // Clear buffer
  
  String message = "Hello! This is a test message from your Arduino system.";
  Serial.println("Sending Hello SMS...");
  gsmSerial.println("AT+CMGF=1");
  delay(1000);
  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(phoneNumber);
  gsmSerial.println("\"");
  delay(1000);
  gsmSerial.println(message);
  gsmSerial.write(26);  
  delay(5000);
  Serial.println("Hello SMS Sent!");
  beepBuzzer(2, 500);
}

void makeCall() {
  // 1. Clear out the serial buffer so no leftover SMS data interferes
  while(gsmSerial.available()) gsmSerial.read();

  Serial.println("Dialing...");
  
  // 2. Send the AT Dial command
  gsmSerial.print("ATD");
  gsmSerial.print(phoneNumber);
  gsmSerial.println(";");

  // 3. Wait 30 seconds, AND print the GSM module's responses to the monitor
  unsigned long callStartTime = millis();
  while (millis() - callStartTime < 30000) {
    while (gsmSerial.available()) {
      // This will print "RING", "NO CARRIER", or "BUSY" so you can see what is failing
      Serial.write(gsmSerial.read()); 
    }
  }

  // 4. Hang up
  gsmSerial.println("ATH");  
  Serial.println("\nCall Ended.");
  beepBuzzer(1, 1000); 
}
