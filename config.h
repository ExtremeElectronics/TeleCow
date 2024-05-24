

//UDP Settings
//#define LOCAL_PORT 50666 //my port
#define TRANSPORT_LOWER  "udp"
#define TRANSPORT_UPPER  "UDP"
#define SOCKET_RX_TIMEOUT_MSEC 200

//IO 
#define RING 10
#define DIAL 11
#define ANSWER 12
#define HANGUP 13

#define PARSE_MAX 99

//colours
#define TXTEXT printf("\e[1;31m"); //red
#define RXTEXT printf("\e[1;32m"); //green
#define STATETEXT printf("\e[1;34m"); //blue
#define CLTEXT printf("\e[m"); //clear

