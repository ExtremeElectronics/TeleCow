#include <stdbool.h>
#include "enums.h"

    bool parse();
    bool parse_header();
    bool parse_body();
    bool read_param_parse(const char* line, const char* param_name, char* output);
    enum Status convert_status(uint32_t code);
    enum Method convert_method(const char* input);
    enum ContentType convert_content_type(const char* input);

// variables passed from parse.
    uint8_t s_buffer[2048]; 

    enum Status p_status;
    enum Method p_method;
    enum ContentType p_content_type;
    uint32_t p_content_length;

    char p_media[100]; //m= string
    char p_cip[100];  //RDP IP
    char p_mport[100]; //RDP port from m= string

    char p_realm[100];
    char p_nonce[100];
    char p_contact[100];
    char p_to_tag[100];
    char p_cseq[100];
    char p_call_id[100];
    char p_to[100];
    char p_from[100];
    char p_via[100];
    char p_dtmf_signal;
    char p_qop[100];
    char p_opaque[100];

    uint16_t p_dtmf_duration;
    const char* p_body;

