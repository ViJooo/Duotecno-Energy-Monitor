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

unsigned int CRC16(unsigned int crc, unsigned char *buf, int len) {
	for (int pos = 0; pos < len; pos++) {
		crc ^= (unsigned int)buf[pos]; // * XOR byte into least sig. byte of crc
    // * Loop over each bit
    for (int i = 8; i != 0; i--) {
      // * If the LSB is set
      if ((crc & 0x0001) != 0) {
          // * Shift right and XOR 0xA001
          crc >>= 1;
          crc ^= 0xA001;
      } else {
      // * Else LSB is not set
      // * Just shift right
        crc >>= 1;
      }
    }
	}

	return crc;
}

bool isNumber(char *res, int len) {
    for (int i = 0; i < len; i++) {
        if (((res[i] < '0') || (res[i] > '9')) && (res[i] != '.' && res[i] != 0)) {
          return false;
        }
    }

    return true;
}

int FindCharInArrayRev(char array[], char c, int len) {
    for (int i = len - 1; i >= 0; i--) {
        if (array[i] == c)
            return i;

    }
    return -1;
}

bool isValidOBIS (char array[], char startchar, char endchar, int len) {
  int totalStartChar = 0;
  int totalEndChar = 0;

  for (int i = len - 1; i >= 0; i--) {
      if (array[i] == startchar) {
        totalStartChar++;
        continue;
      }

      if (array[i] == endchar) {
        totalEndChar++;
        continue;
      }
  }

  Serial.println("");

  if (totalStartChar > 1 || totalEndChar > 1) return false;

  return true;
}

long getValue(char *buffer, int maxlen, char startchar, char endchar) {
    // for (int i = 0; i < (maxlen - 2); i++) {
    //   Serial.print(String(buffer[i]));
    // }

    // Serial.println("");


    int s = FindCharInArrayRev(buffer, startchar, maxlen - 2);
    int l = FindCharInArrayRev(buffer, endchar, maxlen - 2) - s - 1;

    // for (int i = s; i < l; i++) {
    //   Serial.print(String(buffer[i]));
    // }

    // Serial.println("");


    if (s < 0 || l < 0) return 0; // Fix for wrong OBIS codes.
    
    /*
    if (!isValidOBIS(buffer, startchar, endchar, maxlen - 2)) {
       if (DEBUG_DETAIL) Serial.println("[isValidOBIS] is not valid.");
      //return 0;
    }
    */

    if (((l + s + 1) - s) > P1_VALUEMAXLENGTH) { // Prevent buffer overflow (Stack smashing protect failure!)
      if (DEBUG_DETAIL) Serial.println("[getValue] Fail: Value is greather than 12. (" + String(l - s) + ")");
      return 0;
    }

    char res[16];
    memset(res, 0, sizeof(res));

    //Serial.println("[BUFF] " + String((l + s + 1) - s));
    
    if (strncpy(res, buffer + s + 1, l)) {
      if (endchar == '*') {
          if (isNumber(res, l)) {
              // * Lazy convert float to long
              return (1000 * atof(res));
          }
      } else if (endchar == ')') {
          if (isNumber(res, l)) {
            return atof(res);
          }
      }
    }

    return 0;
}

bool decode_telegram(int len)
{
    int startChar = FindCharInArrayRev(telegram, '/', len);
    int endChar = FindCharInArrayRev(telegram, '!', len);
    bool validCRCFound = false;

    //temp
    // if (endChar >= 0) {
    //   validCRCFound = true;
    // }

    //String captureString = "";

    String currentLine = "";
    int openingBrackets = 0;
    int closingBrackets = 0;
    bool hasLetter = false;
    bool hasStar = false;
    bool hasExclamation = false;
    bool hasOtherIssue = false;

    // Log OBIS
    for (int cnt = 0; cnt < len; cnt++) {
      if (DEBUG_DETAIL) Serial.print(telegram[cnt]);

      digitalMeterRawData = digitalMeterRawData + telegram[cnt];
      currentLine = currentLine + telegram[cnt];

      // Check for invalid OBIS codes. They can cause crashes. If invalid, ignore them.
      if (DEBUG_DETAIL) {
        if (String(telegram[cnt]) == "(")
          openingBrackets++;
        if (String(telegram[cnt]) == ")")
          closingBrackets++;
        if (isAlpha(telegram[cnt]))
          hasLetter = true;
        if (String(telegram[cnt]) == "*")
          hasStar = true;
        if (String (telegram[cnt]) == "!")
          hasExclamation = true;
        if (cnt == 0 && String(telegram[cnt]) == ":") 
          hasOtherIssue = true;
      }
    }

    if (DEBUG_DETAIL) {
      if(currentLine.indexOf("(00)") > 0) {
        Serial.println("! [IGNORE] has (00)");
        //return false;
      }

      if(!hasExclamation && currentLine.indexOf("-") == -1) {
        Serial.println("! [IGNORE] no -");
        //return false;
      }

      if (openingBrackets != closingBrackets) {
       Serial.println("! [IGNORE] Opening: " + String(openingBrackets) + " Closing: " + String(closingBrackets));
        //return false;
      }

      if (!hasExclamation && hasLetter != hasStar) {
        Serial.println("! [IGNORE] hasLetter: " + String(hasLetter) + " hasStar: " + String(hasStar));
        //return false;
      }

      if (hasOtherIssue) {
        Serial.println("! [IGNORE] has other issue.");
        //return false;
      }
    }

    // CRC
    if (startChar >= 0) {
        if (DEBUG_DETAIL) Serial.println("[CRC] 1");
        // * Start found. Reset CRC calculation
        currentCRC = CRC16(0x0000,(unsigned char *) telegram+startChar, len-startChar);
    } else if (endChar >= 0) {
        if (DEBUG_DETAIL) Serial.println("[CRC] 2");
        // * Add to crc calc
        currentCRC = CRC16(currentCRC,(unsigned char*)telegram+endChar, 1);

        char messageCRC[5];
        strncpy(messageCRC, telegram + endChar + 1, 4);

        messageCRC[4] = 0;   // * Thanks to HarmOtten (issue 5)
        validCRCFound = (strtol(messageCRC, NULL, 16) == currentCRC);

  
        if (validCRCFound) 
          if (DEBUG_DETAIL) Serial.println(F("[P1] CRC Valid!"));
        else
          if (DEBUG_DETAIL) Serial.println(F("[P1] CRC Invalid!"));
          
        currentCRC = 0;
    } else {
      if (DEBUG_DETAIL) Serial.println("[CRC] 3");
      currentCRC = CRC16(currentCRC, (unsigned char*) telegram, len);

      // for (int cnt = 0; cnt < len; cnt++) {
      //   digitalMeterRawData = digitalMeterRawData + telegram[cnt];
      // }
    }
    
    if (DEBUG_DETAIL) Serial.println("[DATA] 1");
    
    // 1-0:1.8.1(000992.992*kWh)
    // 1-0:1.8.1 = Elektra verbruik laag tarief (DSMR v4.0)
    if (strncmp(telegram, "1-0:1.8.1", strlen("1-0:1.8.1")) == 0)
    {
        CONSUMPTION_LOW_TARIF = getValue(telegram, len, '(', '*');
    }

    // 1-0:1.8.2(000560.157*kWh)
    // 1-0:1.8.2 = Elektra verbruik hoog tarief (DSMR v4.0)
    if (strncmp(telegram, "1-0:1.8.2", strlen("1-0:1.8.2")) == 0)
    {
        CONSUMPTION_HIGH_TARIF = getValue(telegram, len, '(', '*');
    }

    CONSUMPTION_TOTAL = CONSUMPTION_LOW_TARIF + CONSUMPTION_HIGH_TARIF;
	
    // 1-0:2.8.1(000560.157*kWh)
    // 1-0:2.8.1 = Elektra teruglevering laag tarief (DSMR v4.0)
    if (strncmp(telegram, "1-0:2.8.1", strlen("1-0:2.8.1")) == 0)
    {
        RETURNDELIVERY_LOW_TARIF = getValue(telegram, len, '(', '*');
    }

    // 1-0:2.8.2(000560.157*kWh)
    // 1-0:2.8.2 = Elektra teruglevering hoog tarief (DSMR v4.0)
    if (strncmp(telegram, "1-0:2.8.2", strlen("1-0:2.8.2")) == 0)
    {
        RETURNDELIVERY_HIGH_TARIF = getValue(telegram, len, '(', '*');
    }

    RETURNDELIVERY_TOTAL = RETURNDELIVERY_LOW_TARIF + RETURNDELIVERY_HIGH_TARIF;

    // 1-0:1.7.0(00.424*kW) Actueel verbruik
    // 1-0:1.7.x = Electricity consumption actual usage (DSMR v4.0)
    if (strncmp(telegram, "1-0:1.7.0", strlen("1-0:1.7.0")) == 0)
    {
        ACTUAL_CONSUMPTION = getValue(telegram, len, '(', '*');
    }

    // 1-0:2.7.0(00.000*kW) Actuele teruglevering (-P) in 1 Watt resolution
    if (strncmp(telegram, "1-0:2.7.0", strlen("1-0:2.7.0")) == 0)
    {
        ACTUAL_RETURNDELIVERY = getValue(telegram, len, '(', '*');
    }

    // 1-0:21.7.0(00.378*kW)
    // 1-0:21.7.0 = Instantaan vermogen Elektriciteit levering L1
    if (strncmp(telegram, "1-0:21.7.0", strlen("1-0:21.7.0")) == 0)
    {
        L1_INSTANT_POWER_USAGE = getValue(telegram, len, '(', '*');
    }

    // 1-0:41.7.0(00.378*kW)
    // 1-0:41.7.0 = Instantaan vermogen Elektriciteit levering L2
    if (strncmp(telegram, "1-0:41.7.0", strlen("1-0:41.7.0")) == 0)
    {
        L2_INSTANT_POWER_USAGE = getValue(telegram, len, '(', '*');
    }

    // 1-0:61.7.0(00.378*kW)
    // 1-0:61.7.0 = Instantaan vermogen Elektriciteit levering L3
    if (strncmp(telegram, "1-0:61.7.0", strlen("1-0:61.7.0")) == 0)
    {
        L3_INSTANT_POWER_USAGE = getValue(telegram, len, '(', '*');
    }

    // 1-0:31.7.0(002*A)
    // 1-0:31.7.0 = Instantane stroom Elektriciteit L1
    if (strncmp(telegram, "1-0:31.7.0", strlen("1-0:31.7.0")) == 0)
    {
        L1_INSTANT_POWER_CURRENT = getValue(telegram, len, '(', '*');
    }
    // 1-0:51.7.0(002*A)
    // 1-0:51.7.0 = Instantane stroom Elektriciteit L2
    if (strncmp(telegram, "1-0:51.7.0", strlen("1-0:51.7.0")) == 0)
    {
        L2_INSTANT_POWER_CURRENT = getValue(telegram, len, '(', '*');
    }
    // 1-0:71.7.0(002*A)
    // 1-0:71.7.0 = Instantane stroom Elektriciteit L3
    if (strncmp(telegram, "1-0:71.7.0", strlen("1-0:71.7.0")) == 0)
    {
        L3_INSTANT_POWER_CURRENT = getValue(telegram, len, '(', '*');
    }

    // 1-0:32.7.0(232.0*V)
    // 1-0:32.7.0 = Voltage L1
    if (strncmp(telegram, "1-0:32.7.0", strlen("1-0:32.7.0")) == 0)
    {
        L1_VOLTAGE = getValue(telegram, len, '(', '*');
    }
    // 1-0:52.7.0(232.0*V)
    // 1-0:52.7.0 = Voltage L2
    if (strncmp(telegram, "1-0:52.7.0", strlen("1-0:52.7.0")) == 0)
    {
        L2_VOLTAGE = getValue(telegram, len, '(', '*');
    }   
    // 1-0:72.7.0(232.0*V)
    // 1-0:72.7.0 = Voltage L3
    if (strncmp(telegram, "1-0:72.7.0", strlen("1-0:72.7.0")) == 0)
    {
        L3_VOLTAGE = getValue(telegram, len, '(', '*');
    }

    // 0-1:24.2.1(150531200000S)(00811.923*m3)
    // 0-1:24.2.1 = Gas (DSMR v4.0) on Kaifa MA105 meter
    // Jelle
    // 0-1:24.2.3(240423121002S)(02616.214*m3)
    // if (strncmp(telegram, "0-1:24.2.1", strlen("0-1:24.2.1")) == 0)
    // {
    //     GAS_METER_M3 = getValue(telegram, len, '(', '*');
    // }
    if (strncmp(telegram, "0-1:24.2.3", strlen("0-1:24.2.3")) == 0)
    {
        GAS_METER_M3 = getValue(telegram, len, '(', '*');
    }

    // Possibly 0-2:24.2.1
    // https://github.com/keukenrol/P1-ESP-BE
    // Jelle
    // 0-2:24.2.1(240423121135S)(00020.393*m3)

    if (strncmp(telegram, "0-2:24.2.1", strlen("0-2:24.2.1")) == 0)
    {
        WATER_METER_M3 = getValue(telegram, len, '(', '*');
    }

    // 0-0:96.14.0(0001)
    // 0-0:96.14.0 = Actual Tarif
    // if (strncmp(telegram, "0-0:96.14.0", strlen("0-0:96.14.0")) == 0)
    // {
    //     ACTUAL_TARIF = getValue(telegram, len, '(', ')');
    // }

    // Peak power consumption now
    // 1-0:1.4.0
    // 1-0:1.4.0(01.263*kW)
    if (strncmp(telegram, "1-0:1.4.0", strlen("1-0:1.4.0")) == 0)
    {
      CURRENT_AVG_DEMAND = getValue(telegram, len, '(', '*');
    }

    // Peak power consumption this month
    // 1-0:1.6.0
    // 1-0:1.6.0(240404111500S)(04.404*kW)
    if (strncmp(telegram, "1-0:1.6.0", strlen("1-0:1.6.0")) == 0)
    {
      MONTH_AVG_DEMAND = getValue(telegram, len, '(', '*');
    }

    // Peak power consumption last 13 months
    // 0-0:98.1.0(13)(1-0:1.6.0)(1-0:1.6.0)(230401000000S)(230305131500W)(06.259*kW)(230501000000S)(230424204500S)(04.604*kW)(230601000000S)(230509130000S)(05.471*kW)(230701000000S)(230602114500S)(03.835*kW)(230801000000S)(230730094500S)(04.659*kW)(230901000000S)(230807174500S)(03.764*kW)(231001000000S)(230924143000S)(04.177*kW)(231101000000W)(231021113000S)(04.568*kW)(231201000000W)(231113074500W)(03.709*kW)(240101000000W)(231218124500W)(04.425*kW)(240201000000W)(240128180000W)(04.768*kW)(240301000000W)(240226153000W)(04.305*kW)(240401000000S)(240311191500W)(04.597*kW)
    // if (strncmp(telegram, "0-0:98.1.0", strlen("0-0:98.1.0 ")) == 0)
    // {
    //     MONTH13_AVG_DEMAND = getValue(telegram, len, '(', ')');
    // }

    if (DEBUG_DETAIL) Serial.println("[END] 1");

    //if (DEBUG_DETAIL) Serial.println("[DEBUG] decodeTelegram 6");

    return validCRCFound;
}

/*
void processLine(int len) {
    telegram[len] = '\n';
    telegram[len + 1] = 0;
    yield();

    if (DEBUG_DETAIL) Serial.println("[DEBUG] processLine 1");
    bool result = decode_telegram(len + 1);

    if (result) {
        //send_data_to_broker();
        //LAST_UPDATE_SENT = millis();
    }
}
*/

bool read_p1_hardwareserial()
{
    if (Serial2.available())
    {
      statusLog = "Reading P1...";

      if (DEBUG) Serial.println("[P1] Reading data...");

      memset(telegram, 0, sizeof(telegram));
      
      while (Serial2.available()) {
          int len = Serial2.readBytesUntil('\n', telegram, P1_MAXLINELENGTH);

          telegram[len] = '\n';
          telegram[len + 1] = 0;

          yield();

          bool result = decode_telegram(len + 1);

          // When the CRC is check which is also the end of the telegram
          // if valid decode return true
          if (result) return true;
      }

      if (DEBUG) Serial.println("[P1] Finished reading...");
    } else {
      if (DEBUG_DETAIL) Serial.println("[P1] Serial2 not available.");
    }

    return false;
}