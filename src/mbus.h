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

void mbus_short_frame(byte address, byte C_field) {
  byte data[6];

  data[0] = 0x10;
  data[1] = C_field;
  data[2] = address;
  data[3] = data[1]+data[2];
  data[4] = 0x16;
  data[5] = '\0';

  Serial1.write((char*)data);
}

void mbus_control_frame(byte address, byte C_field, byte CI_field)
{
  byte data[10];
  data[0] = MBUS_FRAME_LONG_START;
  data[1] = 0x03;
  data[2] = 0x03;
  data[3] = MBUS_FRAME_LONG_START;
  data[4] = C_field;
  data[5] = address;
  data[6] = CI_field;
  data[7] = data[4] + data[5] + data[6];
  data[8] = MBUS_FRAME_STOP;
  data[9] = '\0';

  Serial1.write((char*)data);
}

bool mbus_get_response(byte *pdata, unsigned char len_pdata) {
  byte bid = 0;             // current byte of response frame
  byte bid_end = 255;       // last byte of frame calculated from length byte sent
  byte bid_checksum = 255;  // checksum byte of frame (next to last)
  byte len = 0;
  byte checksum = 0;
  bool long_frame_found = false;
  bool complete_frame  = false;
  bool frame_error = false;
 
  unsigned long timer_start = millis();
  while (!frame_error && !complete_frame && (millis()-timer_start) < MBUS_TIMEOUT)
  {
    while (Serial1.available()) {
      byte received_byte = (byte) Serial1.read();

      // Try to skip noise
      if (bid == 0 && received_byte != 0xE5 && received_byte != 0x68) {
        if (DEBUG) Serial.print(F(">"));
        continue;
      }
      
      if (bid > len_pdata) {
        Serial.print("mbus: error: frame length exceeded variable size of ");
        Serial.println(len_pdata);
        return MBUS_BAD_FRAME;
      }
      pdata[bid] = received_byte;

      // Single Character (ACK)
      if (bid == 0 && received_byte == 0xE5) {
        if (DEBUG) Serial.println(F("mbus: single character (ack)"));
        return MBUS_GOOD_FRAME;
      }
      
      // Long frame start
      if (bid == 0 && received_byte == 0x68) {
        if (DEBUG) Serial.println(F("mbus: start long frame"));
        long_frame_found = true;
      }

      if (long_frame_found) {
        // 2nd byte is the frame length
        if (bid == 1) {
          len = received_byte;
          frameLength = len; // Copy the length WB 
          bid_end = len+4+2-1;
          bid_checksum = bid_end-1;
        }
            
        if (bid == 2 && received_byte != len) {                          // 3rd byte is also length, check that its the same as 2nd byte
          if (DEBUG) Serial.println(F("mbus: frame length byte mismatch"));
          frame_error = true;
        }
        if (bid == 3 && received_byte != 0x68) {        ;                // 4th byte is the start byte again
          if (DEBUG) Serial.println(F("mbus: missing second start byte in long frame"));
          frame_error = true;
        }
        if (bid > 3 && bid < bid_checksum) checksum += received_byte;    // Increment checksum during data portion of frame
        
        if (bid == bid_checksum && received_byte != checksum) {          // Validate checksum
          if (DEBUG) Serial.println(F("mbus: frame failed checksum"));
          frame_error = true;
        }
        if (bid == bid_end && received_byte == 0x16) {                   // Parse frame if still valid
          complete_frame  = true;
        }
      }
      bid++;
    }
  }

  if (complete_frame && !frame_error) {
    // good frame
    return MBUS_GOOD_FRAME;
  } else {
    // bad frame
    return MBUS_BAD_FRAME;
  }
}


// Prints a whole response as a string for debugging
void print_bytes(byte *bytes, unsigned char len_bytes) {
  for (int i = 0; i < len_bytes; i++) {
    Serial.print(String(bytes[i], HEX));
    Serial.print(F(" "));
  } 
  Serial.println();
}

// ---------------------------------------------------------------

void mbus_normalize(byte address) {
  mbus_short_frame(address,0x40);
}

void mbus_request_data(byte address) {
  mbus_short_frame(address,0x5b);
}

void mbus_application_reset(byte address) {
  mbus_control_frame(address,0x53,0x50);
}

void mbus_request(byte address,byte telegram) {
 
  byte data[15];
  byte i=0;
  data[i++] = MBUS_FRAME_LONG_START;
  data[i++] = 0x07;
  data[i++] = 0x07;
  data[i++] = MBUS_FRAME_LONG_START;
  
  data[i++] = 0x53;
  data[i++] = address;
  data[i++] = 0x51;

  data[i++] = 0x01;
  data[i++] = 0xFF;
  data[i++] = 0x08;
  data[i++] = telegram;

  unsigned char checksum = 0;
  for (byte c=4; c<i; c++) checksum += data[c];
  data[i++] = (byte) checksum;
  
  data[i++] = 0x16;
  data[i++] = '\0';
  
  Serial1.write((char*)data);
}

int mbus_scan() {
  unsigned long timer_start = 0;
  for (byte retry=1; retry<=3; retry++) {
    for (byte address = 1; address <= 254; address++) {
      Serial.print("Scanning address: ");
      Serial.println(address);
      mbus_normalize(address);
      timer_start = millis();
      while (millis()-timer_start<256) {
        if (Serial1.available()) {
          byte val = Serial1.read();
          Serial.print("received: ");
          Serial.println(val, HEX);
          if (val==0xE5) return address;
        }
      }
    }
  }
  return -1;
}

void read_mbus() {
    bool checkList[] = {false, false, false};

    const int arrLen = sizeof(mbusAccessNumbers) / sizeof(mbusAccessNumbers[0]);

    for (int count = 0; count < arrLen; count++) {
      if (mbusAccessNumbers[count] <= 0) continue;

      bool mbus_good_frame = false;
      byte mbus_data[MBUS_DATA_SIZE] = { 0 };

      if (DEBUG) Serial.println("--------------- ADR: " + String(mbusAccessNumbers[count]) + " ---------------");
      if (DEBUG) Serial.print(F("mbus: requesting data from address: "));
      if (DEBUG) Serial.println(mbusAccessNumbers[count]);

      mbus_request_data(mbusAccessNumbers[count]);
      mbus_good_frame = mbus_get_response(mbus_data, sizeof(mbus_data));

      if (mbus_good_frame) {
        if (DEBUG_DETAIL) Serial.println(F("mbus: good frame: "));
        int packet_size = mbus_data[1] + 6; 
        if (DEBUG_DETAIL) print_bytes(mbus_data, packet_size);
        if (DEBUG_DETAIL) Serial.println(F("Creating payload buffer..."));
        MBusinoLib payload(JSON_DATA_SIZE);
        if (DEBUG_DETAIL) Serial.print(F("Packet size: ")); Serial.println(packet_size);
        if (DEBUG_DETAIL) Serial.print(F("Start Address: ")); Serial.println(Startadd);
        if (DEBUG_DETAIL) Serial.println(F("Decoding..."));

        //DynamicJsonDocument jsonBuffer(2048);
        //JsonArray root = jsonBuffer.createNestedArray();  
        //uint8_t fields = payload.decode(&mbus_data[Startadd], packet_size - Startadd - 2, root); 

        JsonDocument jsonBuffer;
        JsonArray root = jsonBuffer.add<JsonArray>();  
        fields = payload.decode(&mbus_data[Startadd], packet_size - Startadd - 2, root); 
        
        serializeJson(root, jsonstring); // store the json in a global array

        // Output full json
        //serializeJsonPretty(root, Serial);

        if (DEBUG_DETAIL) Serial.print("Detected data fields: ");
        if (DEBUG_DETAIL) Serial.println(fields);
        
        if (payload.getError() == 0 && fields < 120) {
          checkList[count] = true;
          checkListCount[count] = checkListCount[count] + 1;

          serializeJson(root, mbusDeviceJSONArray[count]);

          bool voltageUpdated = false;
          bool curretnUpdated = false;

          for (uint8_t i=0; i<fields; i++) {
            double value = root[i]["value_scaled"].as<double>();
            uint8_t code = root[i]["code"].as<int>();
            uint8_t subUnit = root[i]["subUnit"].as<int>();
            uint8_t tariff = root[i]["tariff"].as<int>();
            const char* name = root[i]["name"];
            String valueString = root[i]["value_string"];

            // Totaal verbruik
            if (code == 1 && subUnit == NULL && tariff == NULL) {
              mbusDeviceData[count][0] = value;
            }

            // Momenteel verbruik
            if (code == 13 && subUnit == NULL) {
              mbusDeviceData[count][1] = value;
            }

            // Spanning
            if (code == 57 && value != 0) {
              mbusDeviceData[count][2] = value;
              voltageUpdated = true;
            }

            // Stroom
            if (code == 58 && value != 0) {
              mbusDeviceData[count][3] = value;
              curretnUpdated = true;
            }

            // Device
            if (code == 41) {
              mbusDeviceNames[count] = valueString;
            };
          }

          if (!voltageUpdated) {
            mbusDeviceData[count][2] = 0;
          }

          if (!curretnUpdated) {
            mbusDeviceData[count][3] = 0;
          }
        } else {
          if (DEBUG_DETAIL) Serial.print("Detected error: "); 
          if (DEBUG_DETAIL) Serial.println(payload.getError());
        }
        if (DEBUG_DETAIL) Serial.println();
      } else {
        if (DEBUG_DETAIL) Serial.print(F("mbus: bad frame: "));
        if (DEBUG_DETAIL) print_bytes(mbus_data, sizeof(mbus_data));
      }

      if (DEBUG_DETAIL) Serial.println("-----------------------------");
      if (DEBUG_DETAIL) Serial.println("Total usage: " + String(mbusDeviceData[count][0]));
      if (DEBUG_DETAIL) Serial.println("Current usage: " + String(mbusDeviceData[count][1]));
      if (DEBUG_DETAIL) Serial.println("Voltage: " + String(mbusDeviceData[count][2]));
      if (DEBUG_DETAIL) Serial.println("Current: " + String(mbusDeviceData[count][3]));
      if (DEBUG_DETAIL) Serial.println("Meter model: " + String(mbusDeviceNames[count]));
      if (DEBUG_DETAIL) Serial.println("-----------------------------");

      delay(1000);
    }

    totalRequestAttempts++;

    if (DEBUG_DETAIL) Serial.print("Meter 5: ");
    if (DEBUG_DETAIL) Serial.println(String(checkList[0]) + " (" + String(checkListCount[0]) + ")");

    if (DEBUG_DETAIL) Serial.print("Meter 10: ");
    if (DEBUG_DETAIL) Serial.println(String(checkList[1]) + " (" + String(checkListCount[1]) + ")");

    if (DEBUG_DETAIL) Serial.print("Meter 23: ");
    if (DEBUG_DETAIL) Serial.println(String(checkList[2]) + " (" + String(checkListCount[2]) + ")");

    if (DEBUG_DETAIL) Serial.print("Total attempts: ");
    if (DEBUG_DETAIL) Serial.println(String(totalRequestAttempts));
}