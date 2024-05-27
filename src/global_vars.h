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
#include <settings.h>
#include <Arduino.h>

#ifndef GLOBAL_VARS_H
#define GLOBAL_VARS_H

extern Preferences preferences;
extern int meterMode;
extern String meterModeString;
extern String meterModeText;
extern String masterRegisterIP;
extern String triggerIP;
extern String version;
extern bool isResetting;
extern String ssid;
extern String pass;

/* Pulse */
extern long pulseCount; 
//extern long pulseCountCache = 0;
extern float pulseConversionValue;
extern String pulseUnit;
//extern int pulseRegisterNumber = -1;
extern int sendDataMultiplier;
extern int pulseRegisterNumber;

extern String statusLog; // This string will be used to show the current status of what is happening. ex: reading meter, etc etc
extern String digitalMeterRawData;
extern int digitalMeterRegisters[25];

// * Set to store received telegram
extern char telegram[P1_MAXLINELENGTH];

// * Set to store the data values read
extern long CONSUMPTION_LOW_TARIF; // day
extern long CONSUMPTION_HIGH_TARIF; // night
extern long CONSUMPTION_TOTAL; // sum

extern long RETURNDELIVERY_LOW_TARIF; // day
extern long RETURNDELIVERY_HIGH_TARIF; // night
extern long RETURNDELIVERY_TOTAL; // sum

extern long ACTUAL_CONSUMPTION;
extern long ACTUAL_RETURNDELIVERY;
extern long GAS_METER_M3;
extern long WATER_METER_M3;

extern long L1_INSTANT_POWER_USAGE;
extern long L2_INSTANT_POWER_USAGE;
extern long L3_INSTANT_POWER_USAGE;
extern long L1_INSTANT_POWER_CURRENT;
extern long L2_INSTANT_POWER_CURRENT;
extern long L3_INSTANT_POWER_CURRENT;
extern long L1_VOLTAGE;
extern long L2_VOLTAGE;
extern long L3_VOLTAGE;

extern long CURRENT_AVG_DEMAND;
extern long MONTH_AVG_DEMAND;
extern long MONTH13_AVG_DEMAND;


// Set to store data counters read
extern long ACTUAL_TARIF;
// extern long SHORT_POWER_OUTAGES;
// extern long LONG_POWER_OUTAGES;
// extern long SHORT_POWER_DROPS;
// extern long SHORT_POWER_PEAKS;

// * Set during CRC checking
extern unsigned int currentCRC;

/* M-Bus */
extern int currentMbusPage;
extern int mbusBaudrate;
extern int baudrateArray[3];
extern String mbusDeviceNames[3];
extern String mbusDeviceJSONArray[3];
extern int mbusAccessNumbers[3];
extern float mbusDeviceData[3][5];
extern int mbusRegisters[3][5];

extern int totalRequestAttempts;
extern int checkListCount[];

extern int Startadd;
extern byte frameLength;
extern uint8_t fields;
extern char jsonstring[3074];

/* Trigger */
extern float triggerMap[1][4];
extern long triggerMapTimer[1][4];
extern int triggerRegister[1][4];
extern bool triggerMapActivated[1][4];

/* Consumption */
extern long dayConsumption[][8];

/* Web */

extern String authUsername;
extern String authPassword;
extern String authDefaultUsername;
extern String authDefaultPassword;

extern String authAdminUsername;
extern String authAdminPassword;
extern bool authAdminMode;
extern unsigned long authAdminTimer;

#endif