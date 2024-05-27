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

// **********************************
// *   Duotecno Energy Meter        *
// **********************************
#define DEBUG true
#define DEBUG_DETAIL true // More in depth debug.
#define DEBUG_WEB_LOGGING false // Send logging to webserver.

#define LED_RED 18
#define LED_GREEN 19

// **********************************
// *   P1 Settings                  *
// **********************************

// * Baud rate for both hardware and software 
#define P1_BAUD_RATE 115200
#define P1_RX_BUFFER_SIZE 1024
#define P1_RX_PIN 16
#define P1_TX_PIN -1
#define P1_VALUEMAXLENGTH 11 // If value greater than 11, ignore it.  
// * Max telegram length
#define P1_MAXLINELENGTH 1050

// **********************************
// *   Pulse Settings                  *
// **********************************

#define PULSE_PIN 25

// **********************************
// *   M-Bus Settings                  *
// **********************************
#define MBUS_RX_PIN 32
#define MBUS_TX_PIN 33
#define MBUS_ADDRESS_BROADCAST 0xFE // broadcoast 254
#define MBUS_BAUD_RATE 2400   // slave baudrate
#define MBUS_ADDRESS 10     // slave address 5 or 23, broadcast 254
#define MBUS_TIMEOUT 2000     // milliseconds
#define MBUS_DATA_SIZE 255
#define MBUS_GOOD_FRAME true
#define MBUS_BAD_FRAME false
#define JSON_DATA_SIZE 2000
#define MBUS_FRAME_SHORT_START          0x10
#define MBUS_FRAME_LONG_START           0x68
#define MBUS_FRAME_STOP                 0x16
#define MBUS_CONTROL_MASK_SND_NKE       0x40
#define MBUS_CONTROL_MASK_DIR_M2S       0x40
#define MBUS_ADDRESS_NETWORK_LAYER      0xFE
#define MBUS_ACK                        0xE5 (229)