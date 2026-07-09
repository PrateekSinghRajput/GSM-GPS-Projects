# Arduino GPS Tracker with GSM & RF Alerts 🌍🛰️

A versatile, Arduino-based GPS tracking system that provides real-time location data via SMS. This project integrates GPS tracking, GSM cellular communication, wireless RF remote control, and physical auditory alerts into a single robust security device. 

<img width="1376" height="768" alt="11" src="https://github.com/user-attachments/assets/9726b991-c2c0-496b-a132-2821b3d40c00" />


## 🚀 Features
* **Real-Time Location Tracking:** Uses the Neo-6M GPS module to fetch precise coordinates.
* **SMS Communication:** Sends location data (Google Maps links) directly to your phone via the SIM800L GSM module.
* **Wireless Remote Control:** Trigger tracking or alerts remotely using a 433MHz RF transmitter/receiver.
* **Manual Panic/SOS Button:** A physical push button to instantly trigger an SMS alert and activate the buzzer.
* **Auditory Alerts:** Integrated buzzer for local alarms or status indicators.
* **Stable Power Architecture:** Uses an isolated 3.7V Lithium-ion battery for the GSM module to prevent brownouts during high-current network transmissions.

## 🛠️ Hardware Components
* **Microcontroller:** Arduino (Uno / Nano / Mega)
* **Cellular Module:** SIM800L GSM Module
* **GPS Module:** Neo-6M GPS Module (with antenna)
* **RF Modules:** 433MHz RF Transmitter (Remote) and Receiver
* **Alerts & Triggers:** Standard Push Button & 5V Active Buzzer
* **Power Supply:** * 3.7V Lithium-ion Battery (Specifically for the SIM800L)
    * Power source for the Arduino (5V power bank)

## ⚡ Important Power Supply Note
The SIM800L module requires a voltage between **3.4V and 4.4V** and can draw peak currents of up to **2A** during signal transmission. **Do not power the SIM800L from the Arduino's 5V or 3.3V pins**, as this will cause the module to shut down or damage the Arduino. 
* Connect the **3.7V Li-ion battery** directly to the `VCC` and `GND` of the SIM800L.
* **Crucial:** Ensure the ground (`GND`) of the 3.7V battery is connected to the ground (`GND`) of the Arduino to establish a common ground for serial communication.

## 🔌 Circuit / Wiring Guide

*(Note to Self: Replace the pins below with the actual pins used in your code)*

| Component | Arduino Pin | External Connection |
| :--- | :--- | :--- |
| **SIM800L TX** | Pin 3 (SoftwareSerial RX) | - |
| **SIM800L RX** | Pin 4 (SoftwareSerial TX) | - |
| **Neo-6M TX** | Pin 10 (SoftwareSerial RX) | - |
| **Neo-6M RX** | Pin 11 (SoftwareSerial TX) | - |
| **433MHz Data**| Pin 2 (Interrupt Pin) | - |
| **Push Button**| Pin 5 | GND (Use internal pull-up) |
| **Buzzer** | Pin 12 | GND |

## 💻 Software & Libraries Required
Before uploading the code, ensure you have the following libraries installed in your Arduino IDE:
* `SoftwareSerial.h` (Built-in)
* `TinyGPS++.h` (For parsing Neo-6M NMEA data)
* `RCSwitch.h` (For the 433MHz RF modules)

## ⚙️ Setup & Installation
1.  **SIM Card Prep:** Insert an active, 2G-compatible Micro SIM card into the SIM800L. Ensure SIM PIN lock is disabled.
2.  **Wiring:** Connect all components according to the wiring guide above. Double-check your common ground.
3.  **Code Configuration:** Open the `.ino` file and update the `targetPhoneNumber` variable with your actual mobile number.
    ```cpp
    String targetPhoneNumber = "+1234567890"; // Include your country code
    ```
4.  **Upload:** Connect the Arduino to your PC and upload the code.
5.  **Testing:** Go outside to get a clear line of sight to the sky (the Neo-6M needs this for an initial satellite lock, which can take a few minutes). 

## 📖 Usage
* **Press the Push Button:** Triggers an immediate "SOS" SMS with your current Google Maps location and sounds the buzzer.
* **Press the RF Remote:** Remotely requests the current location or arms/disarms the buzzer alarm system.
* *(Add any other specific ways your code behaves here)*

## 🤝 Contributing
Feel free to fork this repository, submit pull requests, or open issues if you find bugs or have suggestions for improvements!

## 📄 License
This project is open-source and available under the [MIT License](LICENSE).
