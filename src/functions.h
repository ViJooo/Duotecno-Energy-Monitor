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

/* p1.h */
bool read_p1_hardwareserial();
long getValue(char *buffer, int maxlen, char startchar, char endchar); // temp, not needed to declare for all files.


/* mbus.h */
void mbus_short_frame(byte address, byte C_field);
void mbus_control_frame(byte address, byte C_field, byte CI_field);
bool mbus_get_response(byte *pdata, unsigned char len_pdata);
void print_bytes(byte *bytes, unsigned char len_bytes);
void mbus_normalize(byte address);
void mbus_request_data(byte address);
void mbus_application_reset(byte address);
void mbus_request(byte address,byte telegram);
int mbus_scan();
void read_mbus();

/* webpages.h */
void sendToRegisterMap(String registerIP, int registerNumber, String registerValue);
void sendToTriggerMap(String registerIP, int registerNumber);
void setupWebPages();
void setupWebPagesAP();