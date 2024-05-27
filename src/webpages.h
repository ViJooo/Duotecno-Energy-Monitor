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
#include <global_vars.h>
#include <Arduino.h>
#include <settings.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <AsyncElegantOTA.h>
#include <Preferences.h>
#include <HTTPClient.h>

//Preferences preferences;

AsyncWebServer server(80);

bool isInArray(int num, int numbers[]) {
  int arrayLength = sizeof(numbers) / sizeof(numbers[0]);

  for (int i = 0; i < arrayLength; i++) {
    if (numbers[i] == num) {
      return true;
    }
  }
  return false;
}

// Send data to registermap using http request.
void sendToRegisterMap(String registerIP, int registerNumber, String registerValue) {
  // Check register numbers and values.
  if (registerNumber == -1 || registerNumber > 1023 || registerNumber < 0 || registerValue == "-1" || registerIP == "0.0.0.0" || registerIP == "0.0.0.1") {
    return;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    // Send http request.
    HTTPClient http;
  
    String serverPath = "http://" + registerIP + ":8080/registermap/" + registerNumber + "/" + registerValue;

    if (DEBUG) Serial.println("[sendToRegisterMap] Builded URL: " + serverPath);

    http.begin(serverPath.c_str());
    
    // Node-RED/server authentication
    // http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      //Serial.print("[sendToRegisterMap] HTTP Response code: ");
      //Serial.println(httpResponseCode);
      //String payload = http.getString();
      //Serial.println(payload);
    } else {
      if (DEBUG) Serial.print("[sendToRegisterMap] Error code: ");
      if (DEBUG) Serial.println(httpResponseCode);
    }
  
    http.end();
  } else {
    if (DEBUG) Serial.println("[sendToRegisterMap] WiFi is Disconnected");
  }
}

// Send data to trigger registermap using http request.
void sendToTriggerMap(String registerIP, int registerNumber) {
  if (registerNumber == -1 || registerNumber > 255 || registerNumber < 0 || registerIP == "0.0.0.0" || registerIP == "0.0.0.1") {
    return;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    // Send http request.
    HTTPClient http;
  
    String serverPath = "http://" + registerIP + ":8080/trigger/" + registerNumber;

    if (DEBUG) Serial.println("[sendToTriggerMap] Builded URL: " + serverPath);

    http.begin(serverPath.c_str());

    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      if (DEBUG_DETAIL) Serial.print("[sendToTriggerMap] HTTP Response code: ");
      if (DEBUG_DETAIL) Serial.println(httpResponseCode);
      String payload = http.getString();
      if (DEBUG_DETAIL) Serial.println(payload);
    } else {
      if (DEBUG) Serial.print("[sendToTriggerMap] Error code: ");
      if (DEBUG) Serial.println(httpResponseCode);
    }
  
    http.end();
  } else {
    if (DEBUG) Serial.println("[sendToTriggerMap] WiFi is Disconnected");
  }
}

// When a webpage is loaded, it will replace all text like %METERMODE% to the corresponding value. So we can display different data from the ESP to the webpage.
String processor(const String& var) {
  // Global values first.
  if (var == "METERMODE") {
    return String(meterMode);
  } else if (var == "METERMODETEXT") {
    return meterModeText;
  } else if (var == "MASTERREGISTERIP") {
    return masterRegisterIP;
  } else if (var == "TRIGGERIP") {
    return triggerIP;
  } else if (var == "STATUSLOG") {
    return statusLog;
  } else if (var == "VERSION") {
    return version;
  // Meter specific values.
  // Pulse
  } else if (meterMode == 1) {
    if (var == "PULSECOUNT") {
      return String(pulseCount);
    } else if (var == "CALCULATEDPULSE") {
      return String(pulseCount * pulseConversionValue, 7);
    } else if (var == "PULSEUNIT") {
      return String(pulseUnit);
    } else if (var == "CURRENTCONVERSION") {
      return String(pulseConversionValue, 7);
      //return dtostr()
    } else if (var == "PULSEREGISTERNUMBER") {
        return String(pulseRegisterNumber);
    }

  // M-Bus
  } else if (meterMode == 2) {
    int mbusIndex = currentMbusPage - 1;
    
    if (var == "MBUSBAUDRATE") {
      return String(mbusBaudrate);
    } else if (var == "MBUSCURRDEVICE") {
      return String(mbusIndex);
    } else if (var == "MBUSCURRPAGE") {
      return String(currentMbusPage);
    } else if (var == "MBUSACCESSNUMBER") {
      return String(mbusAccessNumbers[mbusIndex]);
    } else if (var == "MBUS_AN_1") {
      return String(mbusAccessNumbers[0]);
    } else if (var == "MBUS_AN_2") {
      return String(mbusAccessNumbers[1]);
    } else if (var == "MBUS_AN_3") {
      return String(mbusAccessNumbers[2]);
    } else if (var == "MBUS_DATA_1") {
      return String(mbusRegisters[mbusIndex][0]);
    } else if (var == "MBUS_DATA_2") {
      return String(mbusRegisters[mbusIndex][1]);
    } else if (var == "MBUS_DATA_3") {
      return String(mbusRegisters[mbusIndex][2]);
    } else if (var == "MBUS_DATA_4") {
      return String(mbusRegisters[mbusIndex][3]);
    } else if (var == "MBUS_DATA_5") {
      return String(mbusRegisters[mbusIndex][4]);
    } else if (var == "MBUS_TOTAL_USAGE") {
      return String(mbusDeviceData[mbusIndex][0] / 1000);
    } else if (var == "MBUS_CURR_USAGE") {
      return String(mbusDeviceData[mbusIndex][1]);
    } else if (var == "MBUS_VOLTAGE") {
      return String(mbusDeviceData[mbusIndex][2]);
    } else if (var == "MBUS_CURRENT") {
      return String(mbusDeviceData[mbusIndex][3]);
    } else if (var == "MBUS_MODELNAME") {
      return String(mbusDeviceNames[mbusIndex]);
    } else if (var == "MBUS_JSON") {
      return mbusDeviceJSONArray[mbusIndex];
    }

  // Digital Meter
  } else if (meterMode == 3) {
    if (var == "P1RAWDATA") {
        return digitalMeterRawData;

    } else if (var == "P1_DATA_1") {
      return String(digitalMeterRegisters[0]);
    } else if (var == "P1_DATA_2") {
      return String(digitalMeterRegisters[1]);
    } else if (var == "P1_DATA_3") {
      return String(digitalMeterRegisters[2]);
    } else if (var == "P1_DATA_4") {
      return String(digitalMeterRegisters[3]);
    } else if (var == "P1_DATA_5") {
      return String(digitalMeterRegisters[4]);
    } else if (var == "P1_DATA_6") {
      return String(digitalMeterRegisters[5]);
    } else if (var == "P1_DATA_7") {
      return String(digitalMeterRegisters[6]);
    } else if (var == "P1_DATA_8") {
      return String(digitalMeterRegisters[7]);
    } else if (var == "P1_DATA_9") {
      return String(digitalMeterRegisters[8]);
    } else if (var == "P1_DATA_10") {
      return String(digitalMeterRegisters[9]);
    } else if (var == "P1_DATA_11") {
      return String(digitalMeterRegisters[10]);
    } else if (var == "P1_DATA_12") {
      return String(digitalMeterRegisters[11]);
    } else if (var == "P1_DATA_13") {
      return String(digitalMeterRegisters[12]);
    } else if (var == "P1_DATA_14") {
      return String(digitalMeterRegisters[13]);
    } else if (var == "P1_DATA_15") {
      return String(digitalMeterRegisters[14]);
    } else if (var == "P1_DATA_16") {
      return String(digitalMeterRegisters[15]);
    } else if (var == "P1_DATA_17") {
      return String(digitalMeterRegisters[16]); 
    } else if (var == "P1_DATA_18") {
      return String(digitalMeterRegisters[17]); 
    } else if (var == "P1_DATA_19") {
      return String(digitalMeterRegisters[18]); 
    } else if (var == "P1_DATA_20") {
      return String(digitalMeterRegisters[19]);
    } else if (var == "P1_DATA_21") {
      return String(digitalMeterRegisters[20]); 
    } else if (var == "P1_DATA_22") {
      return String(digitalMeterRegisters[21]); 
    } else if (var == "P1_DATA_23") {
      return String(digitalMeterRegisters[22]); 
    } else if (var == "P1_DATA_24") {
      return String(digitalMeterRegisters[23]); 
    } else if (var == "P1_DATA_25") {
      return String(digitalMeterRegisters[24]); 
    } else if (var == "P1_CONSUMPTION") {
      return String((float) ACTUAL_CONSUMPTION / 1000, 3);
    } else if (var == "P1_R_CONSUMPTION") {
      return String((float) ACTUAL_RETURNDELIVERY / 1000, 3);
    } else if (var == "P1_CONSUMPTION_T1") {
      return String((float) CONSUMPTION_LOW_TARIF / 1000, 3);
    } else if (var == "P1_CONSUMPTION_T2") {
      return String((float) CONSUMPTION_HIGH_TARIF / 1000, 3);
    } else if (var == "P1_CONSUMPTION_TOTAL") {
      return String((float) CONSUMPTION_TOTAL / 1000, 3);
    } else if (var == "P1_INJECTION_T1") {
      return String((float) RETURNDELIVERY_LOW_TARIF / 1000, 3);
    } else if (var == "P1_INJECTION_T2") {
      return String((float) RETURNDELIVERY_HIGH_TARIF / 1000, 3);
    } else if (var == "P1_INJECTION_TOTAL") {
      return String((float) RETURNDELIVERY_TOTAL / 1000, 3);
    } else if (var == "P1_AVG_DEMAND") {
      return String((float) CURRENT_AVG_DEMAND / 1000, 3);
    } else if (var == "P1_MONTH_AVG_DEMAND") {
      return String((float) MONTH_AVG_DEMAND / 1000, 3);
    } else if (var == "P1_GAS") {
      return String((float) GAS_METER_M3 / 1000, 3);
    } else if (var == "P1_WATER") {
      return String((float) WATER_METER_M3 / 1000, 3);
    } else if (var == "P1_L1CONSUMPTION") {
      return String((float) L1_INSTANT_POWER_USAGE / 1000, 3);
    } else if (var == "P1_L1VOLTAGE") {
      return String((float) L1_VOLTAGE / 1000, 1);
    } else if (var == "P1_L1CURRENT") {
      return String((float) L1_INSTANT_POWER_CURRENT / 1000, 3);
    } else if (var == "P1_L2CONSUMPTION") {
      return String((float) L2_INSTANT_POWER_USAGE / 1000, 3);
    } else if (var == "P1_L2VOLTAGE") {
      return String((float) L2_VOLTAGE / 1000, 1);
    } else if (var == "P1_L2CURRENT") {
      return String((float) L2_INSTANT_POWER_CURRENT / 1000, 3);
    } else if (var == "P1_L3CONSUMPTION") {
      return String((float) L3_INSTANT_POWER_USAGE / 1000, 3);
    } else if (var == "P1_L3VOLTAGE") {
      return String((float) L3_VOLTAGE / 1000, 1);
    } else if (var == "P1_L3CURRENT") {
      return String((float) L3_INSTANT_POWER_CURRENT / 1000, 3);
    } else if (var == "P1_TRIGGER_1") {
      return String(triggerMap[0][0], 0);
    } else if (var == "P1_TRIGGER_2") {
      return String(triggerMap[0][1], 0);
    } else if (var == "P1_TRIGGER_3") {
      return String(triggerMap[0][2], 0);
    } else if (var == "P1_TRIGGER_4") {
      return String(triggerMap[0][3], 0);
    } else if (var == "P1_TRIGGER_T_1") {
      return String(triggerMapTimer[0][0]);
    } else if (var == "P1_TRIGGER_T_2") {
      return String(triggerMapTimer[0][1]);
    } else if (var == "P1_TRIGGER_T_3") {
      return String(triggerMapTimer[0][2]);
    } else if (var == "P1_TRIGGER_T_4") {
      return String(triggerMapTimer[0][3]);
    } else if (var == "P1_TRIGGER_REG_1") {
      return String(triggerRegister[0][0]);
    } else if (var == "P1_TRIGGER_REG_2") {
      return String(triggerRegister[0][1]);
    } else if (var == "P1_TRIGGER_REG_3") {
      return String(triggerRegister[0][2]);
    } else if (var == "P1_TRIGGER_REG_4") {
      return String(triggerRegister[0][3]);
    } else if (var == "P1_TRIGGER_STATUS_1") {
      return String(triggerMapActivated[0][0]);
    } else if (var == "P1_TRIGGER_STATUS_2") {
      return String(triggerMapActivated[0][1]);
    } else if (var == "P1_TRIGGER_STATUS_3") {
      return String(triggerMapActivated[0][2]);
    } else if (var == "P1_TRIGGER_STATUS_4") {
      return String(triggerMapActivated[0][3]);
    } else if (var == "P1_CONSUMPTION_TODAY" ) {
      return String((float) dayConsumption[0][1] / 1000, 3);
    } else if (var == "P1_INJECTION_TODAY" ) {
      return String((float) dayConsumption[0][3] / 1000, 3);
    } else if (var == "P1_CONSUMPTION_YDAY" ) {
      return String((float) dayConsumption[0][5] / 1000, 3);
    } else if (var == "P1_INJECTION_YDAY" ) {
      return String((float) dayConsumption[0][7] / 1000, 3);
    }
  }
  //return String();
  return "NULL";
}

void setupWebPages() {
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
      // Authenticate, prevent unwanted access.
      if(!request->authenticate(authUsername.c_str(), authPassword.c_str()) && !authAdminMode) {
        return request->requestAuthentication();
      }
      
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });
    server.serveStatic("/", SPIFFS, "/");

    // Route for test page
    /*
    server.on("/test", HTTP_GET, [](AsyncWebServerRequest * request) {
      if(!request->authenticate(authUsername.c_str(), authPassword.c_str()) && !authAdminMode) {
        return request->requestAuthentication();
      }
      
      request->send(SPIFFS, "/test.html", "text/html", false, processor);
    });
    */

    server.on("/admin", HTTP_GET, [](AsyncWebServerRequest * request) {
      if(!request->authenticate(authAdminUsername.c_str(), authAdminPassword.c_str())) {
        return request->requestAuthentication();
      }

      if (authAdminMode) {
        request->send(200, "text/txt", "Admin mode is already on.");
        return; 
      }

      authAdminTimer = millis();
      authAdminMode = true;
      
      request->send(200, "text/html", "Device is now in admin mode, auth is disabled.");
    });

    // Logout re-auth required again.
    server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request) {
      authAdminMode = false;
      request->send(401);
    });

    server.on("/updatelogin", HTTP_POST, [](AsyncWebServerRequest *request){
      //nothing and dont remove it
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if(!request->authenticate(authUsername.c_str(), authPassword.c_str()) && !authAdminMode) {
        return request->requestAuthentication();
      }

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, (const char*)data);

      if (error) {
        return;
      }
      
      String password = doc["password"];
      String passwordConfirm = doc["passwordconfirm"];

      if (password == "" || password.indexOf(" ") > -1 || password.length() > 30 || password.length() < 8) {
        request->send(400, "text/plain", "");
        return;
      }

      if (password != passwordConfirm) {
        request->send(400, "text/plain", "");
        return;
      }

      authPassword = password;

      Serial.println(password);
      Serial.println(passwordConfirm);

      preferences.begin("saved-data", false);
      preferences.putString("auth_pass", password);
      preferences.end();

      request->send(200, "text/plain", "");
    });

    // Route for test page send
    /*
    server.on("/testsendhttp", HTTP_POST, [](AsyncWebServerRequest * request) {
      if(!request->authenticate(authUsername.c_str(), authPassword.c_str()) && !authAdminMode) {
        return request->requestAuthentication();
      }
      
      // A POST request is submitted, check what parameters are given and do something with them.
      String registerIP = "0.0.0.0";
      int registerNumber = -1;
      int registerValue = -1;
     
      int params = request->params();
      for (int i = 0; i < params; i++) {
        AsyncWebParameter* p = request->getParam(i);
        if (p->isPost()) {
          if (p->name() == "registerip") {
            registerIP = p->value().c_str();
            
            if (DEBUG) Serial.print("registerip set to: ");
            if (DEBUG) Serial.println(registerIP);
          }
          
          if (p->name() == "registernumber") {
            registerNumber = p->value().toInt();
            
            if (DEBUG) Serial.print("registerNumber set to: ");
            if (DEBUG) Serial.println(registerNumber);
          }

          if (p->name() == "registervalue") {
            registerValue = p->value().toInt();
            
            if (DEBUG) Serial.print("registerValue set to: ");
            if (DEBUG) Serial.println(registerValue);
          }
        }
      }

      // Check registernubers etc.
      if (registerNumber > 1023 || registerNumber < -1 || registerValue < 0 || registerIP == "0.0.0.0") {
        request->send(200, "text/txt", "Invalid value(s).");
        return;
      }

      sendToRegisterMap(registerIP, registerNumber, String(registerValue,0));
 
      request->send(200, "text/html", "Send.<meta http-equiv='refresh' content='1;url=/test' />");
    });
    */

     if (meterMode == 1) {
      /* 
       *
       *  Pulse Mode
       * 
      */

      server.on("/pulseconfig", HTTP_GET, [](AsyncWebServerRequest * request) {
        if(!request->authenticate(authUsername.c_str(), authPassword.c_str()) && !authAdminMode) {
          return request->requestAuthentication();
        }
        
        request->send(SPIFFS, "/pulseconfig.html", "text/html", false, processor);
      });

      // registerpulse
      server.on("/registerpulse", HTTP_POST, [](AsyncWebServerRequest *request){
        //nothing and dont remove it
      }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if(!request->authenticate(authUsername.c_str(), authPassword.c_str()) && !authAdminMode) {
          return request->requestAuthentication();
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, (const char*)data);

        if (error) {
          //Serial.print(F("deserializeJson() failed: "));
          //Serial.println(error.f_str());
          return;
        }

        if (doc["id"] == 0) {
            long value1 = doc["pulsecount"];
            float value2 = doc["pulseconversion"];
            String value3 = doc["pulseunit"];

            pulseCount = value1;
            pulseConversionValue = value2;

            if (value3 != "") {
              pulseUnit = value3;
            }

            preferences.begin("saved-data", false);
            preferences.putLong("pulsecount", pulseCount);
            preferences.putFloat("pulseconval", pulseConversionValue);
            preferences.putString("pulseunit", pulseUnit);
            preferences.end();

        } else if (doc["id"] == 1) {
            String value1 = doc["masterip"];
            int value2 = doc["registernumber"];

            masterRegisterIP = value1;
            pulseRegisterNumber = value2;

            preferences.begin("saved-data", false);
            preferences.putString("masterregip", masterRegisterIP);
            preferences.putInt("pulseregnum", pulseRegisterNumber);
            preferences.end();
        }

        request->send(200, "text/plain", "");
      });

    } else if (meterMode == 2) {
      /* 
       *
       *  M-Bus Mode
       *
      */

      server.on("/mbusconfig", HTTP_GET, [](AsyncWebServerRequest * request) {
        if(!request->authenticate(authUsername.c_str(), authPassword.c_str()) && !authAdminMode) {
          return request->requestAuthentication();
        }
        
        request->send(SPIFFS, "/mbusconfig.html", "text/html", false, processor);
      });

      server.on("/mbusdeviceconfig", HTTP_GET, [](AsyncWebServerRequest * request) {
        if(!request->authenticate(authUsername.c_str(), authPassword.c_str()) && !authAdminMode) {
          return request->requestAuthentication();
        }

        // Check if the page got the id parameter which is needed for what mbus device we need to edit.
        if (request->hasParam("id"))
        {
            String param = request->getParam("id")->value();

            if (param == "") return;

            int id = param.toInt();
            const int maxMbusDevices = sizeof(mbusAccessNumbers) / sizeof(mbusAccessNumbers[0]);

            if (id > maxMbusDevices) return;
            
            currentMbusPage = id;
        } else {
          request->send(200, "text/html", "Invalid request.");
          return;
        }
        
        request->send(SPIFFS, "/mbusdeviceconfig.html", "text/html", false, processor);
      });

      server.on("/registermbus", HTTP_POST, [](AsyncWebServerRequest *request){
        //nothing and dont remove it
      }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if(!request->authenticate(authUsername.c_str(), authPassword.c_str()) && !authAdminMode) {
          return request->requestAuthentication();
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, (const char*)data);

        if (error) {
          return;
        }
        
        const int arrLen = sizeof(mbusAccessNumbers) / sizeof(mbusAccessNumbers[0]);

        for (int i = 0; i < arrLen; i++) {
          int value = doc["values"][i];

          if (value > 63 || value < 0) {
            value = -1;
          }

          mbusAccessNumbers[i] = value;

          if (DEBUG) {
            Serial.print("[/registermbus] ");
            Serial.print(i);
            Serial.print(" - Set val: ");
            Serial.println(mbusAccessNumbers[i]);
          }
        }

        preferences.begin("saved-data", false);
        preferences.putBytes("mbusaccessn", (byte*)(&mbusAccessNumbers), sizeof(mbusAccessNumbers));
        preferences.end();

        request->send(200, "text/plain", "");
      });

      server.on("/registermbusdevice", HTTP_POST, [](AsyncWebServerRequest *request){
        //nothing and dont remove it
      }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if(!request->authenticate(authUsername.c_str(), authPassword.c_str()) && !authAdminMode) {
          return request->requestAuthentication();
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, (const char*)data);

        if (error) {
          //Serial.print(F("deserializeJson() failed: "));
          //Serial.println(error.f_str());
          return;
        }

        int currentDevice = doc["id"];
        int mbusIndex = currentMbusPage - 1;

        if (currentDevice != mbusIndex) {
          request->send(400, "text/plain", "");
          return;
        }

        const int arrLen = sizeof(mbusRegisters[0]) / sizeof(mbusRegisters[0][0]);

        for (int i = 0; i < arrLen; i++) {
          int value = doc["values"][i];

          if (value > 63 || value < 0) {
            value = -1;
          }

          mbusRegisters[mbusIndex][i] = value;
          
          /*
          Serial.print("Int: ");
          Serial.print(i);
          Serial.print(" - Set val: ");
          Serial.println(mbusRegisters[mbusIndex][i]);
          */
        }
        
        preferences.begin("saved-data", false);
        preferences.putBytes("mbusreg", (byte*)(&mbusRegisters), sizeof(mbusRegisters));
        preferences.end();
      
        request->send(200, "text/plain", "");
      });

      server.on("/handlembus", HTTP_POST, [](AsyncWebServerRequest * request) {
        
        if(!request->authenticate(authUsername.c_str(), authPassword.c_str()) && !authAdminMode) {
          return request->requestAuthentication();
        }

        int params = request->params();

        preferences.begin("saved-data", false);
         
        for (int i = 0; i < params; i++) {
          AsyncWebParameter* p = request->getParam(i);

          if (p->isPost()) {
            if (DEBUG) Serial.println(p->name());

            if (p->name() == "baudrate") {
              mbusBaudrate = p->value().toInt();

              if (!isInArray(mbusBaudrate, baudrateArray)) {
                request->send(200, "text/html", "Invalid baudrate.");
                return;
              }

              preferences.putInt("mbusbaud", mbusBaudrate);

              if (DEBUG) Serial.print("mbusBaudrate set to: ");
              if (DEBUG) Serial.println(mbusBaudrate);
            }

            if (p->name() == "registerip") {
              masterRegisterIP = p->value();

              preferences.putString("masterregip", masterRegisterIP);
                                      
              if (DEBUG) Serial.print("masterRegisterIP set to: ");
              if (DEBUG) Serial.println(masterRegisterIP);
            }
          }
        }

        preferences.end();

        request->send(200, "text/html", "Done.<meta http-equiv='refresh' content='1;url=/mbusconfig' />");
        
    });
    
    } else if (meterMode == 3) {
      /* 
       *
       *  Digital Meter Mode
       *
      */
      // digitalmeterconfig
      server.on("/digitalmeterconfig", HTTP_GET, [](AsyncWebServerRequest * request) {
        if(!request->authenticate(authUsername.c_str(), authPassword.c_str()) && !authAdminMode) {
          return request->requestAuthentication();
        }
        
        request->send(SPIFFS, "/digitalmeterconfig.html", "text/html", false, processor);
      });

      // registerdigitalmeter
      server.on("/registerdigitalmeter", HTTP_POST, [](AsyncWebServerRequest *request){
        //nothing and dont remove it
      }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if(!request->authenticate(authUsername.c_str(), authPassword.c_str()) && !authAdminMode) {
          return request->requestAuthentication();
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, (const char*)data);

        if (error) {
          //Serial.print(F("deserializeJson() failed: "));
          //Serial.println(error.f_str());
          return;
        }

        if (doc["id"] == 0) {
          const int arrLen = sizeof(digitalMeterRegisters) / sizeof(digitalMeterRegisters[0]);

          for (int i = 0; i < arrLen; i++) {
            int value = doc["values"][i];

            if (value > 1023 || value < 0) {
              value = -1;
            }

            digitalMeterRegisters[i] = value;

            /*
            Serial.print("Int: ");
            Serial.print(i);
            Serial.print(" - Set val: ");
            Serial.println(digitalMeterRegisters[i]);
            */
          }

          preferences.begin("saved-data", false);
          preferences.putBytes("digitalmreg", (byte*)(&digitalMeterRegisters), sizeof(digitalMeterRegisters));
          preferences.end();
        } else if (doc["id"] == 1) {
            String value = doc["value"];
            masterRegisterIP = value;

            preferences.begin("saved-data", false);
            preferences.putString("masterregip", masterRegisterIP);
            preferences.end();
        }

        request->send(200, "text/plain", "");
      });

      server.on("/triggers", HTTP_GET, [](AsyncWebServerRequest * request) {
        if(!request->authenticate(authUsername.c_str(), authPassword.c_str()) && !authAdminMode) {
          return request->requestAuthentication();
        }
        
        request->send(SPIFFS, "/triggers.html", "text/html", false, processor);
      });

      server.on("/registertrigger", HTTP_POST, [](AsyncWebServerRequest *request){
        //nothing and dont remove it
      }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if(!request->authenticate(authUsername.c_str(), authPassword.c_str()) && !authAdminMode) {
          return request->requestAuthentication();
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, (const char*)data);

        if (error) {
          //Serial.print(F("deserializeJson() failed: "));
          //Serial.println(error.f_str());
          return;
        }

        if (doc["id"] == 0) {
          const int arrLen = sizeof(triggerMap[0]) / sizeof(triggerMap[0][0]);

          for (int i = 0; i < arrLen; i++) {
            float value = doc["values"][i];

            if (value > 1000000 || value < 0) {
              value = -1;
            }

            triggerMap[0][i] = value;

            /*
            Serial.print("Int: ");
            Serial.print(i);
            Serial.print(" - Set val: ");
            Serial.println(triggerMap[0][i]);
            */
          }

          const int arrLen2 = sizeof(triggerMapTimer[0]) / sizeof(triggerMapTimer[0][0]);

          for (int i = 0; i < arrLen2; i++) {
            int value = doc["timers"][i];

            if (value > 300000 || value < 1000) {
              value = -1;
            }

            triggerMapTimer[0][i] = value;

            /*
            Serial.print("Int: ");
            Serial.print(i);
            Serial.print(" - Set val: ");
            Serial.println(triggerMapTimer[0][i]);
            */
          }

          const int arrLen3 = sizeof(triggerRegister[0]) / sizeof(triggerRegister[0][0]);

          for (int i = 0; i < arrLen3; i++) {
            int value = doc["registers"][i];

            if (value > 63 || value < 0) {
              value = -1;
            }

            triggerRegister[0][i] = value;

            /* 
            Serial.print("Int: ");
            Serial.print(i);
            Serial.print(" - Set val: ");
            Serial.println(triggerRegister[0][i]);
            */
          }

          preferences.begin("saved-data", false);
          preferences.putBytes("triggermap", (byte*)(&triggerMap), sizeof(triggerMap));
          preferences.putBytes("triggertimer", (byte*)(&triggerMapTimer), sizeof(triggerMapTimer));
          preferences.putBytes("triggerreg", (byte*)(&triggerRegister), sizeof(triggerRegister));
          preferences.end();
        } else if (doc["id"] == 1) {
            String value = doc["value"];
            
            //Serial.print("> > > Value: ");
            //Serial.println(value);
            
            triggerIP = value;

            preferences.begin("saved-data", false);
            preferences.putString("triggerip", triggerIP);
            preferences.end();
        } else {
          return;
        }

        request->send(200, "text/plain", "");
      });
    }

    server.on("/setmode", HTTP_GET, [](AsyncWebServerRequest * request) {
      if(!request->authenticate(authUsername.c_str(), authPassword.c_str()) && !authAdminMode) {
        return request->requestAuthentication();
      }
        
      if (request->hasParam("mode"))
      {
          String param = request->getParam("mode")->value();

          if (param == "") return;

          int mode = param.toInt();

          if (mode == 1 || mode == 2 || mode == 3) {
            preferences.begin("saved-data", false);
            preferences.putInt("metermode", mode);
            preferences.end();
          } else {
            request->send(200, "text/html", "Invalid request.");
            return;
          }

          request->send(200, "text/html", "Meter mode changed, rebooting.<meta http-equiv='refresh' content='5;url=/' />");
          delay(3500);
          ESP.restart();
      }
    });

    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest * request) {
      if(!request->authenticate(authUsername.c_str(), authPassword.c_str()) && !authAdminMode) {
        return request->requestAuthentication();
      }

      isResetting = true;

      // Clear settings
      preferences.begin("saved-data", false);
      preferences.clear();
      preferences.end();
      
      request->send(200, "text/html", "Resetted WiFi and switching back to Access Point: <a href=\"http://192.168.4.1\">http://192.168.4.1</a>");
      delay(5000);
      ESP.restart();
    });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest * request) {
      if(!request->authenticate(authUsername.c_str(), authPassword.c_str()) && !authAdminMode) {
        return request->requestAuthentication();
      }
      
      request->send(200, "text/html", "Rebooting device.<meta http-equiv='refresh' content='5;url=/' />");
      delay(3500);
      ESP.restart();
    });

     // Show device information.
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest * request) {    // /list files in spiffs on webpage
      if(!request->authenticate(authUsername.c_str(), authPassword.c_str()) && !authAdminMode) {
        return request->requestAuthentication();
      }
      
      if (!SPIFFS.begin(true)) {
        if (DEBUG) Serial.println("An Error has occurred while mounting SPIFFS");
        return;
      }
      String str = "";
      
      str += "Uptime in hour(s): ";
      str += esp_timer_get_time() / 3600000000;
      str += " hours";
      
      str += "\r\n";
      
      str += "Uptime in minute(s): ";
      str += esp_timer_get_time() / 60000000;
      str += " minutes";
      
      str += "\r\n";
      
      str += "Uptime in seconds: ";
      str += esp_timer_get_time() / 1000000;
      str += " seconds";
      str += "\r\n";
      str += "\r\n";

      File root = SPIFFS.open("/");
      File file = root.openNextFile();
      
      str += "Files on device: ";
      str += "\r\n";
      str += "\r\n";
      
      while (file) {
        str += " / ";
        str += file.name();
        str += "\r\n";
        file = root.openNextFile();
      }
      str += "\r\n";
      str += "\r\n";
      str += "totalBytes   ";
      str += SPIFFS.totalBytes();
      str += "\r\n";
      str += "usedBytes    ";
      str += SPIFFS.usedBytes();
      str += "\r\n";
      str += "freeBytes    ";
      str += SPIFFS.totalBytes()-SPIFFS.usedBytes();
      str += "\r\n";
      str += "\r\n";
      
      request->send(200, "text/txt", str);
    });

    AsyncElegantOTA.begin(&server);    // Start ElegantOTA
    server.begin();
}

void setupWebPagesAP() {
 // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/setup.html", "text/html");
    });

    server.serveStatic("/", SPIFFS, "/");

    server.on("/", HTTP_POST, [](AsyncWebServerRequest * request) {
      int params = request->params();
      for (int i = 0; i < params; i++) {
        AsyncWebParameter* p = request->getParam(i);
        if (p->isPost()) {
          // HTTP POST ssid value

          preferences.begin("saved-data", false);
          
          const char* PARAM_INPUT_1 = "ssid";
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            if (DEBUG) Serial.print("SSID set to: ");
            if (DEBUG) Serial.println(ssid);
            // Write file to save value
            //writeFile(SPIFFS, ssidPath, ssid.c_str());
            preferences.putString("ssid", ssid);
          }
          // HTTP POST pass value
          const char* PARAM_INPUT_2 = "pass";
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            if (DEBUG) Serial.print("Password set to: ");
            if (DEBUG) Serial.println(pass);
            // Write file to save value
            //writeFile(SPIFFS, passPath, pass.c_str());
            preferences.putString("ssid-pass", pass);
          }

          preferences.end();
          
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }

      request->send(200, "text/html", "Applied settings, please change your connection. Rebooting.");
      
      delay(3500);
      
      ESP.restart();
    });
    server.begin();
}