#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#define BUZZER_PIN 9
#define BUTTON_PIN 8
#define RAIN_PIN A4
#define RAIN_THRESHOLD 500  

SoftwareSerial gpsSerial(10, 11);  
SoftwareSerial gsmSerial(3, 2);  

const int trigPin1 = A1;
const int echoPin1 = A0;
const int trigPin2 = A3;
const int echoPin2 = A2;

const char PHONE_NUMBER[] = "+9199756174xx";

float fallbackLatitude = 18.628608;
float fallbackLongitude = 73.8230272;

float currentLat = 18.628608;
float currentLon = 73.8230272;
String locationSource = "Default";

unsigned long buzzerTimer = 0;
unsigned long buzzerInterval = 0;
int buzzerState = LOW;
int buzzerBeepCount = 0;
int buzzerPattern = 0;

TinyGPSPlus gps;

void setup() {
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RAIN_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.begin(9600);
  gpsSerial.begin(9600);

  Serial.println("Initializing A7670C GSM Module...");

  gsmSerial.begin(115200);
  gsmSerial.listen();
  delay(1000);
  gsmSerial.println("AT+IPR=9600");
  delay(500);

  gsmSerial.begin(9600);
  gsmSerial.listen();
  delay(1000);

  Serial.println("Configuring SMS and Network settings...");
  sendATCommand("AT", 1000);
  sendATCommand("AT+CPIN?", 1000);           // Check SIM status
  sendATCommand("AT+CREG?", 1000);           // Check network registration
  sendATCommand("AT+CMGF=1", 1000);          // Set SMS Text Mode
  sendATCommand("AT+CNMI=2,2,0,0,0", 1000);  // Forward incoming SMS to serial
  sendATCommand("AT+CGATT=1", 2000);         // Attach to packet data network for cell tower LBS
  sendATCommand("AT+CNETSTART", 3000);       // Activate network bearer on A7670C for AT+CLBS

  Serial.println("System Ready");
}

void loop() {
  measureUltrasonicDistances();
  handleBuzzerPattern();
  checkRainSensorAndAlert();

  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Button pressed! Sending SOS SMS...");
      SendMessage("I Am In Problem Plz Help Me. ");
      delay(1000);
    }
  }

  readGsmForTrackCommand();
}

String sendATCommand(const char* cmd, unsigned long waitTime) {
  gsmSerial.listen();
  while (gsmSerial.available() > 0) {
    gsmSerial.read();
  }
  gsmSerial.println(cmd);
  String resp = "";
  unsigned long start = millis();
  while (millis() < start + waitTime) {
    while (gsmSerial.available() > 0) {
      char c = gsmSerial.read();
      Serial.write(c);
      resp += c;
    }
  }
  return resp;
}

void measureUltrasonicDistances() {
  digitalWrite(trigPin1, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin1, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin1, LOW);
  long duration1 = pulseIn(echoPin1, HIGH, 30000);
  if (duration1 == 0) duration1 = 30000;
  int distance1 = (duration1 / 2) / 29;

  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);
  long duration2 = pulseIn(echoPin2, HIGH, 30000);
  if (duration2 == 0) duration2 = 30000;
  int distance2 = (duration2 / 2) / 29;

  if (distance1 <= 15) {
    setBuzzerPattern(1);
  } else if (distance2 <= 15) {
    setBuzzerPattern(2);
  } else if (distance1 <= 20 || distance2 <= 20) {
    setBuzzerPattern(3);
  } else {
    setBuzzerPattern(0);
  }
}

void setBuzzerPattern(int pattern) {
  if (buzzerPattern != pattern) {
    buzzerPattern = pattern;
    buzzerBeepCount = 0;
    buzzerState = LOW;
    digitalWrite(BUZZER_PIN, buzzerState);
    buzzerTimer = millis();

    switch (pattern) {
      case 1: buzzerInterval = 200; break;
      case 2: buzzerInterval = 600; break;
      case 3: buzzerInterval = 1000; break;
      default: buzzerInterval = 0; break;
    }
  }
}

void handleBuzzerPattern() {
  if (buzzerPattern == 0) {
    digitalWrite(BUZZER_PIN, LOW);
    return;
  }

  unsigned long currentMillis = millis();

  if (currentMillis >= buzzerTimer + buzzerInterval) {
    buzzerTimer = currentMillis;
    buzzerState = !buzzerState;
    digitalWrite(BUZZER_PIN, buzzerState);

    if (buzzerState == LOW) {
      buzzerBeepCount++;
      if ((buzzerPattern == 1 && buzzerBeepCount >= 3) || (buzzerPattern == 2 && buzzerBeepCount >= 2) || (buzzerPattern == 3)) {
        buzzerPattern = 0;
        digitalWrite(BUZZER_PIN, LOW);
      }
    }
  }
}

void checkRainSensorAndAlert() {
  static unsigned long lastRainAlert = 0;
  int rainValue = analogRead(RAIN_PIN);
  unsigned long now = millis();

  if (rainValue < RAIN_THRESHOLD && now > lastRainAlert + 4000) {
    Serial.println("Rain Alert! Analog Value: " + String(rainValue));
    for (int i = 0; i < 2; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(300);
      digitalWrite(BUZZER_PIN, LOW);
      delay(300);
    }
    lastRainAlert = now;
  }
}

bool getGsmTowerLocation() {
  Serial.println("GPS unavailable. Requesting GSM Cell Tower Location (LBS)...");
  gsmSerial.listen();

  sendATCommand("AT+CNETSTART", 2000);

  String response = sendATCommand("AT+CLBS=1", 7000);

  int idx = response.indexOf("+CLBS:");
  if (idx >= 0) {
    int comma1 = response.indexOf(',', idx);
    int comma2 = response.indexOf(',', comma1 + 1);
    int comma3 = response.indexOf(',', comma2 + 1);

    if (comma1 > 0 && comma2 > 0) {
      int statusCode = response.substring(idx + 6, comma1).toInt();

      if (statusCode == 0) {
        String val1Str = response.substring(comma1 + 1, comma2);
        String val2Str = (comma3 > 0) ? response.substring(comma2 + 1, comma3) : response.substring(comma2 + 1);

        float val1 = val1Str.toFloat();
        float val2 = val2Str.toFloat();

        if (val1 != 0.0 && val2 != 0.0) {
          
          if (val1 > 60.0 && val2 < 40.0) {
            currentLat = val2;
            currentLon = val1;
          } else {
            currentLat = val1;
            currentLon = val2;
          }

          Serial.print("Tower LAT=");
          Serial.println(currentLat, 6);
          Serial.print("Tower LONG=");
          Serial.println(currentLon, 6);
          return true;
        }
      } else {
        Serial.print("CLBS failed with error code: ");
        Serial.println(statusCode);
      }
    }
  }
  return false;
}

void updateLocationCoordinates() {
  Serial.println("Step 1: Checking GPS module for satellite fix...");
  gpsSerial.listen();

  unsigned long start = millis();

  while (millis() < start + 5000) {
    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }
    if (gps.location.isUpdated() && gps.location.isValid()) {
      currentLat = gps.location.lat();
      currentLon = gps.location.lng();
      locationSource = "GPS";

      Serial.print("GPS Fix Found! LAT=");
      Serial.println(currentLat, 6);
      Serial.print("LONG=");
      Serial.println(currentLon, 6);

      gsmSerial.listen();
      return;
    }
  }

  if (getGsmTowerLocation()) {
    locationSource = "GSM Tower";
    gsmSerial.listen();
    return;
  }

  Serial.println("Both GPS and GSM LBS unavailable. Using fallback coordinates.");
  currentLat = fallbackLatitude;
  currentLon = fallbackLongitude;
  locationSource = "Last Known";
  gsmSerial.listen();
}

void readGsmForTrackCommand() {
  if (gsmSerial.available() > 0) {
    String incomingData = gsmSerial.readString();
    Serial.print("GSM Received: ");
    Serial.println(incomingData);

    if (incomingData.indexOf("Track") >= 0 || incomingData.indexOf("track") >= 0) {
      Serial.println("Track command detected! Fetching location...");
      SendMessage("Your Car Location ");
    }
  }
}

void SendMessage(const char* prefixText) {

  updateLocationCoordinates();

  gsmSerial.listen();
  while (gsmSerial.available() > 0) {
    gsmSerial.read();
  }

  sendATCommand("AT+CMGF=1", 1000);

  Serial.println("Sending AT+CMGS command...");
  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(PHONE_NUMBER);
  gsmSerial.println("\"");

  unsigned long waitPrompt = millis();
  bool promptReceived = false;
  while (millis() < waitPrompt + 3000) {
    if (gsmSerial.available() > 0) {
      char c = gsmSerial.read();
      Serial.write(c);
      if (c == '>') {
        promptReceived = true;
        break;
      }
    }
  }

  gsmSerial.print(prefixText);
  gsmSerial.print("(");
  gsmSerial.print(locationSource);
  gsmSerial.print("): https://www.google.com/maps?q=");
  gsmSerial.print(currentLat, 6);
  gsmSerial.print(",");
  gsmSerial.print(currentLon, 6);
  delay(200);

  gsmSerial.write(26);
  Serial.println("\nCtrl+Z sent, waiting for network confirmation (+CMGS)...");

  unsigned long start = millis();
  while (millis() < start + 7000) {
    while (gsmSerial.available() > 0) {
      Serial.write(gsmSerial.read());
    }
  }
  Serial.println();
}