#include "stdbool.h"
#include "enums.h"

volatile enum SipState cl_state; // = SS_IDLE;
volatile enum PhoneState ph_state;

char c_server_ip[20]; // sip servers ip
char c_user[100]; // sip username
char c_pwd[100];  //sippassword
char c_my_ip[20]; // my ip address
char c_uri[100]; // "sip:" + server_ip
char c_to_uri[100];  //"sip:" + user + "@" + server_ip
char c_to_contact[100]; //
char c_to_tag[100];
char c_login[100];

uint32_t c_sip_sequence_number;
char c_call_id[100];
char c_call[100];


//digest auth stuff
char c_response[1024]; 
char c_realm[100];
char c_nonce[100];
char c_cnonce[100];
char c_nc[100];
char c_tag[100];

//branch and session
char c_branch[100];
uint32_t c_sdp_session_id;

//rtp
char rtp_ip[100];
char rtp_port[100];


void request_dial(const char* local_number, const char* caller_display);
void request_cancel();
void tx();
void rx();
void send_sip_register(int);
void send_sip_invite(int,int);
void send_sip_cancel();
void send_sip_bye();
void send_sip_bye_it();
void send_sip_ack();
void send_sip_options();
void sip_reply_header(char * code,char * buffer);
void sip_header(const char * command, const char * request_uri, char * from_uri, char * to_uri, char * from_tag, char * to_tag, char * buffer);
bool read_param(const char * line, const char * parac_name, char * output);
void compute_auth_response(const char * method, const char * uri);
void log_state_transition(enum SipState old_state, enum SipState new_state);
void send_sip_ok(int withsdp);
void to_hex(char * dest, const unsigned char* data, int len);
void printState(enum SipState state);
void send_sip_ringing();
void send_request_terminated();
void send_sip_busy();
void answer_ringing();
void populate_sdp(char * sdp_buffer);
 
extern enum Status p_status;
extern bool parse(); 
void sip_init();

void printPhoneState(enum PhoneState status);
void printMDState(enum Method state);
void parsed_headers(void);
 
  