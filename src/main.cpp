/*
*
*       Duotecno Energy Monitor
*       duotecno.be
*
*       Author: VJ
*
*       2024
*
*/

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <AsyncElegantOTA.h>
//#include <ElegantOTA.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <settings.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "MBusinoLib.h"
#include <functions.h>

// Create an instance of the library preferences, this is used to save data on the flash.
Preferences preferences;

String statusLog = "Setup"; // This string will be used to show the current status of what is happening. ex: reading meter, etc etc
String digitalMeterRawData = "No data yet.";
const long triggerInterval = 3000; // Check all triggers every x ms
const long authAdminInterval = 600000; // Turn off admin mode (maintenace) after x ms
const long wifiInterval = 15000; // Check WiFi connection.

// Reboot interval for acces point mode - Restart the device every x ms
long rebootIntervalAP = 300000;

// * Set to store received telegram
char telegram[P1_MAXLINELENGTH];

// * Set to store the data values read
long CONSUMPTION_LOW_TARIF; // day
long CONSUMPTION_HIGH_TARIF; // night
long CONSUMPTION_TOTAL; // sum
long RETURNDELIVERY_LOW_TARIF; // day
long RETURNDELIVERY_HIGH_TARIF; // night
long RETURNDELIVERY_TOTAL; // sum
long ACTUAL_CONSUMPTION;
long ACTUAL_RETURNDELIVERY;
long GAS_METER_M3;
long WATER_METER_M3;
long L1_INSTANT_POWER_USAGE;
long L2_INSTANT_POWER_USAGE;
long L3_INSTANT_POWER_USAGE;
long L1_INSTANT_POWER_CURRENT;
long L2_INSTANT_POWER_CURRENT;
long L3_INSTANT_POWER_CURRENT;
long L1_VOLTAGE;
long L2_VOLTAGE;
long L3_VOLTAGE;
long CURRENT_AVG_DEMAND;
long MONTH_AVG_DEMAND;
long MONTH13_AVG_DEMAND;
long ACTUAL_TARIF;

// * Set during CRC checking
unsigned int currentCRC = 0;

String version = "2.0.0";
bool sendedFullTelegram = false;

unsigned long wifiCheckLast = 0;

// trigger
float triggerMap[1][4] = {{-1, -1, -1, -1}}; // Values that needs to be reached to trigger.
long triggerMapTimer[1][4] = {{5000, 5000, 5000, 5000}}; // If trigger is active for x seconds, send it.
unsigned long triggerMapTimerLast[][4] = {{0, 0, 0, 0}};
bool triggerMapActivated[1][4] = {{false, false, false, false}}; // Trigger activated or not, prevent spam.
int triggerRegister[1][4] = {{-1, -1, -1, -1}}; // Trigger register duotecno.
unsigned long triggerLast = 0;
float triggerValues[1][5] = {{0, 0, 0, 0, 0}};

// Day consumption.

int consumptionSavedDay = 0;

/*
  IDX 0: saved consumption start of day
  IDX 1: differance saved/now
  IDX 2: saved injection start of day
  IDX 3: differance saved/now
  ...
  same but for yesterday.
*/
long dayConsumption[][8] = {{0, 0, 0, 0, 0, 0, 0, 0}};
const long consumptionInterval = 15000;
unsigned long consumptionLast = 0;
bool consumptionStartup = true;

// General vars
bool isResetting = false;
String ssid;
String pass;

// Define NTP Client to get time
WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP);
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7200, 60000);
// Variables to save date and time
String formattedDate;
time_t epochTime;
String timeStamp;

int meterMode = 0; // 0 - No meter selected yet; 1 - Pulse; 2 - M-Bus; 3 - Digital Meter.
String meterModeString;
String meterModeText = "Not set";
String masterRegisterIP;
String triggerIP;

// Pulse vars
long pulseCount; 
long pulseCountCache = 0;
float pulseConversionValue;
String pulseUnit;
int pulseRegisterNumber = -1;

unsigned long savePulseLast = 0;
const long savePulseData = 30000;
unsigned long sendPulseLast = 0;
const long sendPulseData = 15000;


// M-Bus vars
int currentMbusPage = 0;
int mbusBaudrate = 2400;
String mbusDeviceNames[3] = {"", "", ""};
String mbusDeviceJSONArray[3] = {"", "", ""};
int mbusAccessNumbers[3] = {0, 0, 0}; // 1 per device
float mbusDeviceData[3][5] = {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}};
int mbusRegisters[3][5] = {{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1}}; // 1 Array per device.
int baudrateArray[3] = {2400, 4800, 9600}; // Array of valid baudrates to prevent unwanted inputs.
const long mbusReadInterval = 5000;

//float mbusValues[][5] = {{0, 0, 0, 0, 0}};


// Digital Meter vars
bool startupSendData = true; // Force send data on startup.
int digitalMeterRegisters[25] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}; // Assign a duotecno register number to each value from the meter. 
unsigned long digitalMeterIntervals[25] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
String digitalMeterLog;

int sendDataMultiplier;

unsigned long sendDataLast = 0;
const long sendDataInterval = 2000; // Check to send data every x ms

//unsigned long getDataLast = 0;
//const long getDataInterval = 5000; // ms
unsigned long digitalMeterReadLast = 0;
unsigned long mbusReadLast = 0;

const long digitalMeterReadInterval = 5000;

// Send each value after x amount ms.
// digitalMeterRegisters & digitalMeterIntervals needs updating too.
long digitalMeterSendInterval[] = {
    5000,  // ACTUAL_CONSUMPTION
    5000,  // ACTUAL_RETURNDELIVERY
    5000,  // Day Consumption
    5000,  // Day Injection
    5000,  // Yesterday Consumption
    5000,  // Yesterday injection
    30000, // CONSUMPTION_LOW_TARIF
    30000, // CONSUMPTION_HIGH_TARIF
    30000, // CONSUMPTION_TOTAL
    30000, // RETURNDELIVERY_LOW_TARIF
    30000, // RETURNDELIVERY_HIGH_TARIF
    30000, // RETURNDELIVERY_TOTAL
    60000, // CURRENT_AVG_DEMAND
    60000, // MONTH_AVG_DEMAND
    60000, // GAS_METER_M3
    60000, // WATER_METER_M3
    10000, // L1_INSTANT_POWER_USAGE
    10000, // L1_VOLTAGE
    10000, // L1_INSTANT_POWER_CURRENT
    10000, // L2_INSTANT_POWER_USAGE
    10000, // L2_VOLTAGE
    10000, // L2_INSTANT_POWER_CURRENT
    10000, // L3_INSTANT_POWER_USAGE
    10000, // L3_VOLTAGE
    10000, // L3_INSTANT_POWER_CURRENT
    10000  // Unused
};

String authUsername;
String authPassword;
String authDefaultUsername = "hidden";
String authDefaultPassword = "hidden";

String authAdminUsername = "hidden";
String authAdminPassword = "hidden";
bool authAdminMode = true; // TEMP ON

// Disable admin mode after x time.
unsigned long authAdminTimer = 0;

// Timer variables
unsigned long previousMillis = 0;
const long interval = 10000;  // interval to wait for Wi-Fi connection on startup (milliseconds)

// Initialize SPIFFS (filesystem)
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    //Serial.println("An error has occurred while mounting SPIFFS");
  }
  //Serial.println("SPIFFS mounted successfully");
}

void Wifi_connected(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (DEBUG) Serial.println("[WIFI] Successfully connected to the WIFI!");
}

void Get_IPAddress(WiFiEvent_t event, WiFiEventInfo_t info) {
  // Output wifi ip when logging into the router is not possible.
  Serial.print("[WIFI] IP: ");
  Serial.println(WiFi.localIP());
}

void Wifi_disconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (DEBUG) Serial.println("[WIFI] Disconnected from the WIFI!");
}

// Initialize WiFi
bool initWiFi() {
  if (ssid == "") {
    Serial.println("[WIFI] Undefined SSID wrong wifiroutername or wifirouterpassword");
    return false;
  }

  //WiFi.setHostname("DT-ESP-" + WiFi.macAddress().tostring());

  WiFi.mode(WIFI_STA);

  WiFi.setHostname("DuotecnoEnergy");

  if (DEBUG) Serial.println("[WIFI] Connecting to WiFi...");

  // Connect to wifi using saved ssid and pass.
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  // Keep trying every x seconds
  while (WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("[WIFI] Failed to connect.");
      return false;
    }
  }
  
  delay(500);
  
  // Serial.println("");
  // Serial.println(WiFi.localIP());
  
  return true;
}

void debugLog (String log) {
  if(meterMode != 3) {
    Serial.println(log);
  }
}

// Load saved data from flash using preferences library.
void loadSavedData()
{
  preferences.begin("saved-data", false);

/*
  preferences.putString("ssid", "");
  preferences.putString("ssid-pass", "");
  preferences.putLong("pulsecount", 0);
  preferences.getInt("metermode", 0);
  preferences.putString("ssid", "");
  preferences.putString("ssid-pass", "");
  preferences.putString("auth_user", authDefaultUsername);
  preferences.putString("auth_pass", authDefaultPassword);
  preferences.putLong("pulsecount", 0); 
  preferences.putFloat("pulseconval", 0.0000001);
  preferences.putString("pulseunit", "kWh");
  preferences.putString("pulseregisterip", "0.0.0.1");
  preferences.putInt("pulseregnum", -1);
  preferences.putInt("datamultiplier", 1000);
*/

  ssid = preferences.getString("ssid", "");
  pass = preferences.getString("ssid-pass", "");

  if (DEBUG) Serial.println("WiFi Info | SSID: " + ssid + " | Pass: " + pass);

  consumptionSavedDay = preferences.getInt("savedday", 0);


  // Metermode
  meterMode = preferences.getInt("metermode", 0); 

  if (meterMode == 1) {
    meterModeText = "Pulse";

    pulseCount = preferences.getLong("pulsecount", 0); 
    pulseConversionValue = preferences.getFloat("pulseconval", 0.0000001);
    pulseUnit = preferences.getString("pulseunit", "kWh");
    pulseRegisterNumber = preferences.getInt("pulseregnum", -1);
  } else if (meterMode == 2) {
    meterModeText = "M-Bus";

    mbusBaudrate = preferences.getInt("mbusbaud", 2400);
    preferences.getBytes("mbusaccessn", &mbusAccessNumbers, sizeof (mbusAccessNumbers));
    preferences.getBytes("mbusreg", &mbusRegisters, sizeof (mbusRegisters));
  } else if (meterMode == 3) {
    meterModeText = "Digital Meter";
  }
  
  //Serial.print("Meter mode: ");
  //Serial.println(meterModeText);

  // AUTH
  authUsername = preferences.getString("auth_user", authDefaultUsername);
  authPassword = preferences.getString("auth_pass", authDefaultPassword);

  masterRegisterIP = preferences.getString("masterregip", "0.0.0.0");
  triggerIP = preferences.getString("triggerip", "0.0.0.0");

  // Data
  //sendDataMultiplier = preferences.getInt("datamultiplier", 1000);
  
  preferences.getBytes("digitalmreg", &digitalMeterRegisters, sizeof (digitalMeterRegisters));

  //Serial.print("Array save test: ");
  //Serial.println(digitalMeterRegisters[0]);

  //prefs.putBytes("SavedIntegers", (byte*)(&dataStore), sizeof(dataStore));
  //prefs.getBytes("SavedIntegers", &dataRetrieve, sizeof(dataRetrieve));

  preferences.getBytes("dayconsumption", &dayConsumption, sizeof (dayConsumption));

  preferences.getBytes("triggermap", &triggerMap, sizeof (triggerMap));
  preferences.getBytes("triggertimer", &triggerMapTimer, sizeof (triggerMapTimer));
  preferences.getBytes("triggerreg", &triggerRegister, sizeof (triggerRegister));

  preferences.end();
}

/*

    sendToRegisterMap

*/



// Logging to webserver to find errors on remote.
void sendLogToServer(String log) {
  if (!DEBUG_WEB_LOGGING) return;

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;   
     
    http.begin("link");  
    http.addHeader("Content-Type", "application/json");         
     
    JsonDocument doc;

    doc["token"] = "token";
    doc["mac"] = WiFi.macAddress();
    doc["devicename"] = "DT ENERGY"; 
    doc["version"] = version; 
    doc["log"] = log;
     
    String requestBody;
    serializeJson(doc, requestBody);
     
    int httpResponseCode = http.POST(requestBody);
 
    if(httpResponseCode>0){
       
      String response = http.getString();                       
       
      //Serial.println(httpResponseCode);   
      //Serial.println(response);
     
    } else {
      if (DEBUG) Serial.print("[WEB] Error code: ");
      if (DEBUG) Serial.println(httpResponseCode);
    }
  } else {
    if (DEBUG) Serial.println("[WEB] WiFi is Disconnected");
  }
}

void startTrigger (int i, int j) {
    //if (DEBUG) Serial.println("[TRIGGERS] Trigger " + String(i) + "-" + String(j));

    // Check if the trigger was already activated, prevent spamming.
    if (!triggerMapActivated[i][j]) {
      // Start the timer
      if (triggerMapTimerLast[i][j] == 0) {
        triggerMapTimerLast[i][j] = millis();
      }

      // Do something with this trigger
      if (millis() - triggerMapTimerLast[i][j] > triggerMapTimer[i][j]) {
        triggerMapActivated[i][j] = true;
        sendToTriggerMap(triggerIP, triggerRegister[i][j]);
        //if (DEBUG) Serial.println("[TRIGGERS] Trigger " + String(i) + " - " + String(j) + " activated!");
      }
    }
};

void checkTrigger(int i, int j) {
  float triggerMapValue = triggerMap[i][j];

  if (triggerMap[i][j] != -1 && triggerValues[i][j] >= 0) {
    if ((j % 2) == 0 ) {
        if (triggerValues[i][j] >= triggerMapValue) {
          Serial.println("[TRIGGERS] START Trigger " + String(i) + "-" + String(j) + ": " + String(triggerValues[i][j]) + " > " + String(triggerMap[i][j]));
          startTrigger(i, j);
        } else {
          // Send to the trigger register.
          if (triggerMapActivated[i][j]) {
            Serial.println("[TRIGGERS] STOP Trigger " + String(i) + "-" + String(j) + ": " + String(triggerValues[i][j]) + " > " + String(triggerMap[i][j]));

            triggerMapActivated[i][j] = false;
            //sendToTriggerMap(triggerIP, triggerRegister[i][j]);
          }

          triggerMapTimerLast[i][j] = 0;
        }
    } else {
        if (triggerValues[i][j] < triggerMapValue) {
          Serial.println("[TRIGGERS] START Trigger " + String(i) + "-" + String(j) + ": " + String(triggerValues[i][j]) + " < " + String(triggerMap[i][j]));
          startTrigger(i, j);
        } else {
          if (triggerMapActivated[i][j]) {
            triggerMapActivated[i][j] = false;
            //sendToTriggerMap(triggerIP, triggerRegister[i][j]);
          }

          triggerMapTimerLast[i][j] = 0;
        }
    }
  } else {
    if (triggerMap[i][j] == -1) {
      triggerMapActivated[i][j] = false;
    }
  }
}

void buildTrigger() {
  // Build trigger array depending on metermode and user input.
  if (meterMode == 2) {

  }

  if (meterMode == 3) {
    triggerValues[0][0] = ACTUAL_CONSUMPTION;
    triggerValues[0][1] = triggerValues[0][0];
    triggerValues[0][2] = ACTUAL_RETURNDELIVERY;
    triggerValues[0][3] = triggerValues[0][2];
  }
}

// Increase pulse count function (attachInterrupt) This function should be as fast as possible.
void pulseCounter() {
  pulseCount++;
  if (DEBUG) Serial.print("[pulseCounter] Detected Pulse: ");
  if (DEBUG) Serial.println(String(pulseCount));
}

/*
*
*
*    P1 - Digital Meter
*
*
*/
#pragma region p1meter

void sendDigitalMeterData() {
  //if (DEBUG) Serial.println("---sendDigitalMeterData---");
  // Create data array to process all values.
  // Has to be in the right order
  long dataArray[] = {
    ACTUAL_CONSUMPTION, ACTUAL_RETURNDELIVERY, dayConsumption[0][1], dayConsumption[0][3], dayConsumption[0][5], dayConsumption[0][7], 
    CONSUMPTION_LOW_TARIF, CONSUMPTION_HIGH_TARIF, CONSUMPTION_TOTAL, RETURNDELIVERY_LOW_TARIF, RETURNDELIVERY_HIGH_TARIF, RETURNDELIVERY_TOTAL,
    CURRENT_AVG_DEMAND, MONTH_AVG_DEMAND, GAS_METER_M3, WATER_METER_M3, L1_INSTANT_POWER_USAGE, L1_VOLTAGE, L1_INSTANT_POWER_CURRENT, L2_INSTANT_POWER_USAGE, L2_VOLTAGE, L2_INSTANT_POWER_CURRENT, 
    L3_INSTANT_POWER_USAGE, L3_VOLTAGE, L3_INSTANT_POWER_CURRENT
  };

  // If true, send data to register even if its 0.
  bool dataUpdateZero[] = {
    true, true, true, true, true, true, 
    false, false, false, false, false, false,
    false, false, false, false, true, true, true, true, true, true, 
    true, true, true
  };

  const int arrLen = sizeof(dataArray) / sizeof(dataArray[0]);

  for (int i = 0; i < arrLen; i++) {
    if (millis() - digitalMeterIntervals[i] > digitalMeterSendInterval[i] || startupSendData) {
      digitalMeterIntervals[i] = millis();

      // Incoming values are multiplied by 1000 already, so divide by 10. In Duotecno software the value will be multiplied by 100 again.
      long value = dataArray[i] / 10;

      if (digitalMeterRegisters[i] != -1 ) {
        if (dataUpdateZero[i]) {
          //Serial.println ("IDX: " + String(i) + " - " +String(dataArray[i]));
          sendToRegisterMap(masterRegisterIP, digitalMeterRegisters[i], String(value));
        } else {
          if (value != 0) {
            sendToRegisterMap(masterRegisterIP, digitalMeterRegisters[i], String(value));
          }
        } 
      }

      //if (DEBUG) Serial.print("Updating: ");(
      //if (DEBUG) Serial.print(String(i));)
      //if (DEBUG) Serial.println(", interval = " + String(digitalMeterSendInterval[i]));

      delay (100);
    }
  }

  if (startupSendData) startupSendData = false;
}

#pragma endregion p1meter

/*
*
*
*   M-Bus
*
*
*/
#pragma region mbus

int totalRequestAttempts = 0;
int checkListCount[] = {0, 0, 0};

unsigned long timerMbus = 0;
unsigned long last_loop = 0;

bool firstrun = true;
int Startadd = 0x13;      // Start address for decoding
byte frameLength = 0;
uint8_t fields = 0;
char jsonstring[3074] = { 0 };

#pragma endregion mbus


/*
*
*
*    Setup
*
*
*/

// Setup, runs once on start.
void setup() {
  Serial.begin(115200);
  if (DEBUG) Serial.println("Setup...");

  esp_reset_reason();

  statusLog = "Running Setup...";

  // Setup pin 0 and 1 as input
  //pinMode(0, INPUT); // Reset
  //pinMode(1, INPUT); // Pulse count

  statusLog = "Initializing SPIFFS...";
  initSPIFFS();
  statusLog = "Loading saved data...";
  loadSavedData();

  delay(1000);

  if (meterMode == 1) {
    // Pulse

    if (DEBUG) Serial.println("[MODE] --> Pulse");

    pinMode(PULSE_PIN, INPUT_PULLUP);
    //Serial.println("read: " + String(digitalRead(PULSE_PIN)));
    attachInterrupt(digitalPinToInterrupt(PULSE_PIN), pulseCounter, RISING); // attachInterrupt will listen to pin x and will execute "pulseCounter" function to increase the pulse count.
  } else if (meterMode == 2) {
    // M-Bus

    if (DEBUG) Serial.println("[MODE] --> M-Bus");
    Serial1.begin(MBUS_BAUD_RATE, SERIAL_8E1, MBUS_RX_PIN, MBUS_TX_PIN); // mbus uses 8E1 encoding
    delay(1000); // let the serial initialize, or we get a bad first frame

    mbus_normalize(0xFE);
    delay(3000);
  } else if (meterMode == 3) {
    // Digital Meter
    if (DEBUG) Serial.println("[MODE] --> Digital Meter");

    Serial2.setRxBufferSize(P1_RX_BUFFER_SIZE); // Important for belgian meters.
    Serial2.begin(P1_BAUD_RATE, SERIAL_8N1, P1_RX_PIN, P1_TX_PIN, true); // Setup the serial on RX pin, with inversion ON.
  }

  //temp
  //ssid = "Test Koffer Duotecno";
  //pass = "duotecno";

  // LED setup, and turn it off.
  pinMode(LED_RED, OUTPUT); // RED
  pinMode(LED_GREEN, OUTPUT); // GREEN

  digitalWrite(LED_RED, HIGH);
  //digitalWrite(19, HIGH); 

  WiFi.onEvent(Wifi_connected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(Get_IPAddress, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(Wifi_disconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  
  // If the wifi connection is succesfull, setup all needed webpages.
  if (initWiFi()) {
    statusLog = "Initializing webserver...";

    //sendLogToServer("Device booted. Connected to WiFi: " + ssid + " (" + pass + ")");

    // Setup web pages.
    setupWebPages();

    statusLog = "Syncing time.";
    
    // Initialize a NTPClient to get time
    timeClient.begin();
    // Set offset time in seconds to adjust for your timezone, for example:
    // GMT +2 = 7200
    //timeClient.setTimeOffset(7200);
    timeClient.forceUpdate();

  } else {
    statusLog = "Initializing Access Point";
    // The wifi connection was not succesfull, create access point to setup the device.
    // Create AP (Access Point)
    if (DEBUG) Serial.println("Setting AP (Access Point)");

    meterMode = 0;

    // WiFi SSID
    String broadcastintheair = String("DT Energy (") + WiFi.macAddress().c_str() + ")";
    WiFi.softAP(broadcastintheair.c_str(), NULL);

    IPAddress IP = WiFi.softAPIP();
    if (DEBUG) Serial.print("AP IP address: ");
    if (DEBUG) Serial.println(IP);
    
    // Setup web pages access point
    setupWebPagesAP();

    statusLog = "Waiting for network credentials.";
  }

  if (DEBUG) Serial.println("Finished Setup.");

  statusLog = "Running.";

  /*
  // OBIS crash test
  if (DEBUG) Serial.println("--- ! ---");

  String obisString = "0-0:96.14.0(0002)455430303030323135333639)";
  //String obisString = "1-0:2.7.0(00.000*kW)0021.440*m3)";
  //String obisString = "1-0:1.8.1(000000.078*kWh)";
  char obisTelegram[P1_MAXLINELENGTH];
  obisString.toCharArray(obisTelegram, obisString.length());
  Serial.println(String (obisTelegram));
  delay(1000);

  if (strncmp(obisTelegram, "0-0:96.14.0", strlen("0-0:96.14.0")) == 0)
  {
    RETURNDELIVERY_LOW_TARIF = getValue(obisTelegram, obisString.length(), '(', '*');
    Serial.println("1-2");
  }

  Serial.println("1-1");
  if (strncmp(obisTelegram, "1-0:2.7.0", strlen("1-0:2.7.0")) == 0)
  {
    RETURNDELIVERY_LOW_TARIF = getValue(obisTelegram, obisString.length(), '(', '*');
    Serial.println("1-2");
  }

  Serial.println("2-1");
  if (strncmp(obisTelegram, "1-0:1.8.1", strlen("1-0:1.8.1")) == 0)
  {
    Serial.println("2-2");
    RETURNDELIVERY_LOW_TARIF = getValue(obisTelegram, obisString.length(), '(', '*');
  }
  */
 
}

void loop() {
    if (millis() > 10000 && millis() < 60000) {
      // Turn on LED for 10 seconds on startup.
      digitalWrite(LED_RED, LOW);
    }

    // Check WiFi and reconnect if disconnected.
    if (meterMode != 0 && (WiFi.status() != WL_CONNECTED) && (millis() - wifiCheckLast > wifiInterval)) {
      if (DEBUG) Serial.println("Reconnecting to WiFi network.");
      WiFi.disconnect();
      WiFi.reconnect();
      wifiCheckLast = millis();
    }

    // Reboot every x ms. For Acces Point mode every 5 minutes.
    if (meterMode == 0 && millis() > rebootIntervalAP) {
      if (DEBUG) Serial.println("[REBOOT] Daily reboot of the device in 5 seconds.");
      delay(5000);
      ESP.restart();
    }

    /*
     * 
     * 
     *  Pulse
     * 
     * 
    */
    if (meterMode == 1 && !isResetting) {
      if (pulseCount != pulseCountCache && millis() - savePulseLast > savePulseData) {
        savePulseLast = millis();
        pulseCountCache = pulseCount;

        // Save pulsecount on flash
        preferences.begin("saved-data", false);
        preferences.putLong("pulsecount", pulseCount);
        preferences.end();
        
        if (DEBUG) Serial.print("Saving pulse data: ");
        if (DEBUG) Serial.println(String(pulseCount));
      }

      if (millis() - sendPulseLast > sendPulseData) {
        sendPulseLast = millis();

        float pulseValue = (float) pulseCount * pulseConversionValue * 100;
        String pulseValueString = String(pulseValue, 0);

        if (DEBUG) Serial.print("sending pulse data: ");
        if (DEBUG) Serial.println(pulseValueString);

        // Send to registermap function
        sendToRegisterMap(masterRegisterIP, pulseRegisterNumber, pulseValueString);
      }
    }

    /*
     * 
     * 
     *  M-Bus
     * 
     * 
    */

    if (meterMode == 2 && !isResetting) {
      if (millis() - timerMbus > 30000) { // Request M-Bus Records
          if (DEBUG) Serial.println("normalizing...");
          timerMbus = millis();
          mbus_normalize(0xFE);
          delay(1000);
        }

        if (millis() - last_loop >= 15000 || firstrun) {
          if (DEBUG) Serial.println("Requesting MBUS data...");
          last_loop = millis(); 
          firstrun = false;
          
          read_mbus();
        }

        delay (1000);
    }

    // Digital Meter
    if (meterMode == 3 && !isResetting) {
        // Get data from the meter.
        if (millis() - digitalMeterReadLast > digitalMeterReadInterval) {
          if (DEBUG_DETAIL) Serial.println("Trying to read digital meter...");

          statusLog = "Trying to read digital meter...";

          digitalMeterRawData = "";

          digitalWrite(LED_GREEN, HIGH);

          if (read_p1_hardwareserial()) {
            if (DEBUG_DETAIL) Serial.println("Succesfull read.");
          } else {
            if (DEBUG_DETAIL) Serial.println("Failed read.");
          }

          digitalWrite(LED_GREEN, LOW);

          if (!sendedFullTelegram) {
              if (CONSUMPTION_LOW_TARIF > 0) {
                sendedFullTelegram = true;
                //sendLogToServer("reading P1.");
              }
          }

          digitalMeterReadLast = millis(); // reset the timer
        };

        // Check time and save daily consumption.
        if (millis() - consumptionLast > consumptionInterval) {
         if (DEBUG) Serial.println("[TIME] Running.");

          // if (timeClient.update()) {
          //   if (DEBUG) Serial.println("[TIME] Updating time.");
          //   timeClient.forceUpdate();
          // }

          epochTime = timeClient.getEpochTime();
          struct tm *ptm = gmtime ((time_t *)&epochTime);
          int dayNumber = ptm->tm_mday;

          // Check if saved day is different and update it. Also move the current data to yesterday and start a new day.
          if (consumptionSavedDay != dayNumber) {
            if (DEBUG) Serial.println("[TIME] Day has changed. New day: " + String(dayNumber) + " Old day " + String(consumptionSavedDay));

            consumptionSavedDay = dayNumber;

            // Consumption
            // Copy to the array for yesterday
            dayConsumption[0][4] = dayConsumption[0][0];
            dayConsumption[0][5] = dayConsumption[0][1];
            dayConsumption[0][6] = dayConsumption[0][2];
            dayConsumption[0][7] = dayConsumption[0][3];

            // Reset current day.
            dayConsumption[0][0] = 0;
            dayConsumption[0][1] = 0;
            dayConsumption[0][2] = 0;
            dayConsumption[0][3] = 0;

            preferences.begin("saved-data", false);
            preferences.putInt("savedday", consumptionSavedDay);
            preferences.putBytes("dayconsumption", (byte*)(&dayConsumption), sizeof(dayConsumption));
            preferences.end();

            consumptionStartup = true;

            // If the saved consumption on the beginning of the day is 0, assume it hasnt been set yet.
            long totalConsumption = (CONSUMPTION_LOW_TARIF + CONSUMPTION_HIGH_TARIF);
            long totalInjection = (RETURNDELIVERY_LOW_TARIF + RETURNDELIVERY_HIGH_TARIF);

            //if (DEBUG) Serial.println("[totalConsumption] " + String(totalConsumption, 3));
            //totalConsumption = totalConsumption / 1000;
            //totalInjection = totalInjection / 1000;
            //if (DEBUG) Serial.println("[totalConsumption] " + String(totalConsumption, 3));

            // Update the start value of the day consumption if its still 0
            if (consumptionStartup) {
              if (dayConsumption[0][0] == 0) {
                if (DEBUG) Serial.println("[TIME] Consumption is 0 --> updating...");

                dayConsumption[0][0] = totalConsumption;
              }

              if (dayConsumption[0][2] == 0) {
                if (DEBUG) Serial.println("[TIME] Injection is 0 --> updating...");

                dayConsumption[0][2] = totalInjection;
              }

              consumptionStartup = false;
            }

            dayConsumption[0][1] = totalConsumption -  dayConsumption[0][0];
            dayConsumption[0][3] = totalInjection - dayConsumption[0][2];

            preferences.begin("saved-data", false);
            preferences.putBytes("dayconsumption", (byte*)(&dayConsumption), sizeof(dayConsumption));
            preferences.end();

            if (DEBUG) Serial.println("[totalConsumption] " + String(dayConsumption[0][1], 3));
            if (DEBUG) Serial.println("[totalInjection] " + String(dayConsumption[0][3], 3));      
        }

        // Send data
        if (millis() - sendDataLast > sendDataInterval) {
          sendDataLast = millis();

          statusLog = "Sending digital meter data...";

          sendDigitalMeterData();
        };
    }

    // Triggers - Build values for the triggers and check if a trigger needs to be activated.
    if (meterMode != 0 && millis() - triggerLast > triggerInterval) { 
      //if (DEBUG) Serial.println("[TRIGGERS] Checking...");

      const int triggerArrayLength = sizeof(triggerMap) / sizeof(triggerMap[0]);

      // Gather the values of the meter.
      buildTrigger();

      for (int i = 0; i < triggerArrayLength; i++) {
        // Check if we have to trigger
        const int triggerArrayValueLength = sizeof(triggerMap[i]) / sizeof(triggerMap[i][0]);

        for (int j = 0; j < triggerArrayValueLength; j++) {
          checkTrigger(i, j);
          delay(150);
        }
      }

      triggerLast = millis();
    }

    // Check if admin mode is on and turn it off after x amount of time in case someone forgets to turn it off.
    if (authAdminMode && (millis() - authAdminTimer > authAdminInterval)) {  
      if (DEBUG) Serial.println("[AUTH] Turned off admin mode.");
      authAdminMode = false;
    }

    delay(1000);
}

#include <p1.h>
#include <mbus.h>
#include <webpages.h>