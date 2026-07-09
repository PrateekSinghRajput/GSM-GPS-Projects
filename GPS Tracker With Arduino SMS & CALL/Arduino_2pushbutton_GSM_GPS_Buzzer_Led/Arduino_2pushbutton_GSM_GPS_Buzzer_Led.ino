#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#define GSM_RX 2
#define GSM_TX 3
#define GPS_RX 11
#define GPS_TX 10

#define BUTTON_SMS 7
#define BUTTON_CALL 8 

#define LED_POWER 4  
#define LED_GSM 5     
#define LED_GPS 6     
#define BUZZER 12    

#define DEMO_LAT "18.6279087"
#define DEMO_LON "73.8219257"
#define PHONE_NUMBER "+918830584864"

SoftwareSerial GSM(GSM_RX, GSM_TX);
SoftwareSerial neo(GPS_RX, GPS_TX);
TinyGPSPlus gps;

String textMessage = "";
bool lastButtonSMSState = HIGH;
bool lastButtonCallState = HIGH;

unsigned long lastGSMCheck = 0; 
bool inCall = false;            
bool gsmConfigured = false; 

void smartDelay(unsigned long ms, String &lati, String &longi);
void sendSMS(SoftwareSerial &GSM, const String &message);
void startCall(SoftwareSerial &GSM);
void beepBuzzer(int delayMs);
void beepThreeTimes();


void setup() {
  Serial.begin(9600);
  
  unsigned long waitStart = millis();
  while (!Serial && (millis() - waitStart < 3000)) {}
  delay(1000); 
  
  textMessage.reserve(150);

  Serial.println(F("\n\n========================================"));
  Serial.println(F("    SMART TRACKER & CALLER SYSTEM       "));
  Serial.println(F("========================================"));
  
  pinMode(BUTTON_SMS, INPUT_PULLUP);
  pinMode(BUTTON_CALL, INPUT_PULLUP);
  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_GSM, OUTPUT);
  pinMode(LED_GPS, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(LED_POWER, HIGH);

  beepBuzzer(300);
  delay(100);
  beepBuzzer(300);

  GSM.begin(9600);
  neo.begin(9600);
  
  GSM.setTimeout(500);
  GSM.listen();
  
  Serial.println(F("Waiting 10 seconds for GSM Module & SIM to boot..."));
  delay(10000);

  Serial.println(F("Applying Initial GSM Settings..."));
  GSM.println(F("AT+CMGF=1")); 
  delay(1000);
  GSM.println(F("AT+CNMI=2,2,0,0,0")); 
  delay(1000);
  GSM.println(F("AT+CMGD=1,4")); 
  delay(1000);
  
  Serial.println(F("SYSTEM READY & RUNNING"));
  GSM.println(F("AT+CSQ")); 
}


void loop() {
  GSM.listen();
  textMessage = "";
  
  if (GSM.available() > 0) {
    textMessage = GSM.readString(); 
  }

  // --- PROCESS INCOMING DATA ---
  if (textMessage.length() > 0) {
    Serial.println(F("\n[GSM EVENT] Data received:"));
    Serial.print(textMessage);

    // Convert the incoming message to UPPERCASE to fix smartphone autocorrect issues
    String upperMsg = textMessage;
    upperMsg.toUpperCase();

    // 1. Incoming SMS Action (Now Case-Insensitive)
    if (upperMsg.indexOf(F("GETLOC")) >= 0) {
      Serial.println(F("[ACTION] 'GETLOC' Command received!"));
      beepBuzzer(200); 
      
      String lati = "";
      String longi = "";
      smartDelay(1500, lati, longi); // Fetch GPS
      
      String pesan = F("GPS Location http://maps.google.com/maps?q=");
      pesan += lati;
      pesan += F(",");
      pesan += longi;
      
      sendSMS(GSM, pesan);
      
      // Delete the SMS immediately so memory doesn't fill up
      GSM.println(F("AT+CMGD=1,4")); 
    }
    
    // Safety Net: If the SIM card stored the message instead of printing it live
    else if (upperMsg.indexOf(F("+CMTI:")) >= 0) {
      Serial.println(F("[WARNING] SMS was saved to memory. Reading it now..."));
      GSM.println(F("AT+CMGR=1")); // Read message at index 1
      delay(500);
      GSM.println(F("AT+CMGD=1,4")); // Delete it after reading
    }

    // 2. Call Disconnect Detection
    if (upperMsg.indexOf(F("NO CARRIER")) >= 0 || upperMsg.indexOf(F("BUSY")) >= 0) {
      if (inCall) {
        Serial.println(F("[CALL EVENT] Call ended or rejected!"));
        beepThreeTimes(); 
        inCall = false;   
      }
    }

    // 3. Update GSM LED
    int csqIndex = upperMsg.indexOf(F("+CSQ: "));
    if (csqIndex != -1) {
      int commaIndex = upperMsg.indexOf(F(","), csqIndex);
      if (commaIndex != -1) {
        String rssiStr = upperMsg.substring(csqIndex + 6, commaIndex);
        int rssi = rssiStr.toInt();
        if (rssi > 0 && rssi <= 31) {
          digitalWrite(LED_GSM, HIGH); 
        } else {
          digitalWrite(LED_GSM, LOW);  
        }
      }
    }
  }

  // --- HEARTBEAT: Maintain Connection & Settings (Every 10 seconds) ---
  if (millis() - lastGSMCheck > 10000 && !inCall) {
    GSM.println(F("AT+CSQ")); 
    
    // Periodically enforce SMS settings just in case the module dropped them
    if (!gsmConfigured) {
      GSM.println(F("AT+CMGF=1"));
      delay(100);
      GSM.println(F("AT+CNMI=2,2,0,0,0"));
      gsmConfigured = true; // Don't spam it every 10 seconds, just once later on
    }
    
    lastGSMCheck = millis();
  }

  // --- BUTTON 1: SMS ONLY ---
  bool buttonSMSState = digitalRead(BUTTON_SMS);
  if (lastButtonSMSState == HIGH && buttonSMSState == LOW) {  
    Serial.println(F("\n[BUTTON 1] SMS Button Pressed!"));
    beepBuzzer(500); 
    
    String lati = "";
    String longi = "";
    smartDelay(1000, lati, longi);
    
    String pesan = F("Alert! GPS Location: http://maps.google.com/maps?q=");
    pesan += lati;
    pesan += F(",");
    pesan += longi;
    
    sendSMS(GSM, pesan);
  }
  lastButtonSMSState = buttonSMSState;

  // --- BUTTON 2: CALL ONLY ---
  bool buttonCallState = digitalRead(BUTTON_CALL);
  if (lastButtonCallState == HIGH && buttonCallState == LOW) {  
    Serial.println(F("\n[BUTTON 2] Call Button Pressed!"));
    beepBuzzer(500); 
    startCall(GSM);
  }
  lastButtonCallState = buttonCallState;
}


void smartDelay(unsigned long ms, String &lati, String &longi) {
  Serial.println(F("[GPS] Fetching Coordinates..."));
  neo.listen(); 
  
  unsigned long start = millis();
  bool gotLocation = false;
  
  do {
    while (neo.available()) {
      gps.encode(neo.read());
    }
    
    if (gps.location.isValid() && gps.location.age() < 2000) {
      lati = String(gps.location.lat(), 8);
      longi = String(gps.location.lng(), 6);
      digitalWrite(LED_GPS, HIGH); 
      gotLocation = true;
    } else {
      digitalWrite(LED_GPS, LOW);
    }
  } while (millis() - start < ms && !gotLocation);

  if (!gotLocation) {
    lati = DEMO_LAT;
    longi = DEMO_LON;
    Serial.println(F("[GPS] Live GPS not found! Using Demo."));
  } else {
    Serial.println(F("[GPS] SUCCESS: Live GPS signal locked."));
  }
  
  GSM.listen(); 
}

void sendSMS(SoftwareSerial &GSM, const String &message) {
  Serial.println(F("[SMS] Preparing to send..."));
  
  GSM.print(F("AT+CMGS=\""));
  GSM.print(F(PHONE_NUMBER));
  GSM.println(F("\""));
  delay(500);
  
  GSM.print(message);
  GSM.write(0x1a); 
  delay(2000);     
  Serial.println(F("[SMS] Sent."));
}

void startCall(SoftwareSerial &GSM) {
  Serial.println(F("[CALL] Dialing..."));
  GSM.print(F("ATD"));
  GSM.print(F(PHONE_NUMBER));
  GSM.println(F(";")); 
  inCall = true;    
}

void beepBuzzer(int delayMs) {
  digitalWrite(BUZZER, HIGH);
  delay(delayMs);
  digitalWrite(BUZZER, LOW);
}

void beepThreeTimes() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(200);
    digitalWrite(BUZZER, LOW);
    delay(200);
  }
}