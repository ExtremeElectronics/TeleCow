
//settings - from SD eventually
//do not change after read from SD


//defaults if there is no SD card.
#define SIP_USERNAME "NOTTHIS"
#define SIP_PASSWORD "NOTTHIS"
#define SIP_LOGIN "NOTTHIS"
#define SIP_NAME "NOTTHIS"
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "WIFIPASSWORD"
#define SERVER_ADDR "10.0.0.1"
#define SERVER_PORT 8888
#define LOCAL_SIP_PORT 9999
#define LOCAL_RTP_PORT 17080
#define SERVER_REALM "Asterisk"
#define DEBUG 1

//hostname
#define HOSTNAME "PiTeleCow"

//set max for SD card
#define MAXSERVERS 10
#define MAXWIFI 3

//Agent name
#define USER_AGENT "PicoSip/0.0.1"

//use FatFS
#include "ff.h"
#include "f_util.h"

//time till next reregister
#define SIP_REREGISTRATIONDELAY 300 //in tenths of a sec

int getlastsession();
int savelastsession(int x);

int get_settings(void);
int log_file_init(char * from);
int log_file_save(char * from,char * buffer);
int splash(void);



