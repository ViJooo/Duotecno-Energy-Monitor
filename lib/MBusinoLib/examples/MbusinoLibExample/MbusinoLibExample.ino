/*
# Example for MBusinoLib

https://github.com/Zeppelin500/MBusinoLib/

## Licence
****************************************************
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************
*/

#include <MBusinoLib.h>  // Library for decode M-Bus
#include <ArduinoJson.h> // is used to transfer the decoded recordings

#define START_ADDRESS 0x13  // Start address for decoding
unsigned long timerMbus = 0;

void setup() {
Serial.begin(9600);
}


void loop() {
 
  if(millis() - timerMbus > 5000){
    timerMbus = millis();

    // M-Bus Telegram of a Sensostar U
    byte mbus_data[] = {0x68,0xC1,0xC1,0x68,0x08,0x00,0x72,0x09,0x34,0x75,0x73,0xC5,0x14,0x00,0x0D,0x43,0x00,0x00,0x00,0x04,0x78,0x41,0x63,0x65,0x04,0x04,0x06,0xAA,0x29,0x00,0x00,0x04,0x13,0x40,0xA1,0x75,0x00,0x04,0x2B,0x00,0x00,0x00,0x00,0x14,0x2B,0x3C,0xF3,0x00,0x00,0x04,0x3B,0x48,0x06,0x00,0x00,0x14,0x3B,0x4E,0x0E,0x00,0x00,0x02,0x5B,0x19,0x00,0x02,0x5F,0x19,0x00,0x02,0x61,0xFA,0xFF,0x02,0x23,0xAC,0x08,0x04,0x6D,0x03,0x2A,0xF1,0x2A,0x44,0x06,0x92,0x0C,0x00,0x00,0x44,0x13,0x2D,0x9B,0x1C,0x00,0x42,0x6C,0xDF,0x2C,0x01,0xFD,0x17,0x00,0x03,0xFD,0x0C,0x05,0x00,0x00,0x84,0x10,0x06,0x1A,0x00,0x00,0x00,0xC4,0x10,0x06,0x05,0x00,0x00,0x00,0x84,0x20,0x06,0x00,0x00,0x00,0x00,0xC4,0x20,0x06,0x00,0x00,0x00,0x00,0x84,0x30,0x06,0x00,0x00,0x00,0x00,0xC4,0x30,0x06,0x00,0x00,0x00,0x00,0x84,0x40,0x13,0x00,0x00,0x00,0x00,0xC4,0x40,0x13,0x00,0x00,0x00,0x00,0x84,0x80,0x40,0x13,0x00,0x00,0x00,0x00,0xC4,0x80,0x40,0x13,0x00,0x00,0x00,0x00,0x84,0xC0,0x40,0x13,0x00,0x00,0x00,0x00,0xC4,0xC0,0x40,0x13,0x00,0x00,0x00,0x00,0x75,0x16};

    int packet_size = mbus_data[1] + 6; 
    MBusinoLib payload(254);  
    JsonDocument jsonBuffer;
    JsonArray root = jsonBuffer.add<JsonArray>();  
    uint8_t fields = payload.decode(&mbus_data[START_ADDRESS], packet_size - START_ADDRESS - 2, root); 
    char jsonstring[2048] = { 0 };
    uint8_t address = mbus_data[5]; 
    serializeJson(root, jsonstring);

    Serial.println("###################### new message ###################### "); 
    Serial.println(String("error = " + String(payload.getError())).c_str()); 
    Serial.println(String("jsonstring = " + String(jsonstring)).c_str());  
    Serial.println(String("SlaveAddress = " + String(address)).c_str());      
    
    for (uint8_t i=0; i<fields; i++) {
      uint8_t code = root[i]["code"].as<int>();
      const char* name = root[i]["name"];
      const char* units = root[i]["units"];           
      float value = root[i]["value_scaled"].as<float>(); 
      const char* valueString = root[i]["value_string"];            

      //values comes as number or as ASCII string or both
      if(valueString != NULL){  // send the value if a ascii value is aviable (variable length)
        Serial.println(String(String(i+1) + "_vs_" + String(name)+ " = " + String(valueString)).c_str()); 
      }

      Serial.println(String(String(i+1) + "_" + String(name) + " = " + String(value,3)).c_str()); //send the value if a real value is aviable (standard)
            
      if(units != NULL){ // send the unit if is aviable
        Serial.println(String(String(i+1) + "_" + String(name) +"_unit = " + String(units)).c_str());
      }
    }
    Serial.println("###################### end of message ###################### ");      
  }
}
