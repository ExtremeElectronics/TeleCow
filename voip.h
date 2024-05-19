
//#define PARSE_MAX 99
void send_sip_udp(char* msg,int msglen);
void send_rdp_udp(char* msg,int msglen);
//void printPhoneState(ph_state);
void send_sip_udp_blocking(char* msg,int msglen,int debug);
void send_rdp_udp_blocking(char* msg,int msglen,int debug);
void process_RTP_in(char *,uint16_t );
