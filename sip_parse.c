#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "config.h"

#include "sip_parse.h"

#define LINE_ENDING "\r\n"
static const size_t LINE_ENDING_LEN = 2;

#define SIP_2_0_SPACE "SIP/2.0 "
#define WWW_AUTHENTICATE "WWW-Authenticate"
#define PROXY_AUTHENTICATE "Proxy-Authenticate"
#define CONTACT "Contact: <"
#define TO "To: "
#define FROM "From: "
#define VIA "Via: "
#define RPORT "rport;"
#define C_SEQ "CSeq: "
#define CALL_ID "Call-ID: "
#define CONTENT_TYPE "Content-Type: "
#define CONTENT_LENGTH "Content-Length: "
#define REALM "realm"
#define NONCE "nonce"
#define NOTIFY "NOTIFY "
#define BYE "BYE "
#define INFO "INFO "
#define INVITE "INVITE "
#define APPLICATION_DTMF_RELAY "application/dtmf-relay"
#define SIGNAL "Signal="
#define DURATION "Duration="
#define MEDIA "m="
#define CPORT "m=audio "
#define CIP "c=IN IP4 "
#define QOP "qop"
#define OPAQUE "opaque"
#define CANCEL "CANCEL "
#define OPTIONS "OPTIONS" 

#define nullptr ((void*)0)

// set debug level    
#define PARSEDEBUG 0

char* replaceWord(const char* s, const char* oldW,const char* newW){ 
    char* result; 
    int i, cnt = 0; 
    int newWlen = strlen(newW); 
    int oldWlen = strlen(oldW); 
 
    // Counting the number of times old word 
    // occur in the string 
    for (i = 0; s[i] != '\0'; i++) { 
        if (strstr(&s[i], oldW) == &s[i]) { 
            cnt++; 
            // Jumping to index after the old word. 
            i += oldWlen - 1; 
        } 
    } 
 
    // Making new string of enough length 
    result = (char*)malloc(i + cnt * (newWlen - oldWlen) + 1); 
 
    i = 0; 
    while (*s) { 
        // compare the substring with the result 
        if (strstr(s, oldW) == s) { 
            strcpy(&result[i], newW); 
            i += newWlen; 
            s += oldWlen; 
        } 
        else
            result[i++] = *s++; 
    } 
    result[i] = '\0'; 
    return result; 
} 


void string(char * out, char * in,int max){
   int c=0;
   while((in[c]!=0) && c<max){
     out[c]=in[c];
     c++;
   }  
   out[c]=0;  
}

void stringptr(char * out, char * in,char * end){
   int c=0;
   while((in[c]!=0) && c+in<end){
     out[c]=in[c];
     c++;
   }  
   out[c]=0;  
}

bool parse(){
    printf("Parse\n");
    bool result = parse_header();
    if (!result)
    {
        return false;
    }
    parse_body();
    return true;
}

bool parse_header(){
    if (PARSEDEBUG>0) printf("Parse Header \n");
    const char* start_position = s_buffer;
    const char* buff_beginning = s_buffer;
    char* end_position = strstr(start_position,LINE_ENDING);

    // parse init
    p_method = MD_UNKNOWN;
    p_status = ST_UNKNOWN;
    p_content_type = CT_UNKNOWN;
    p_content_length = 0;
    p_cseq[0]=0;
    p_call_id[0]=0;
    p_to[0]=0;
    p_from[0]=0;
    p_via[0]=0;
    p_dtmf_signal = ' ';
    p_dtmf_duration = 0;
    p_body = nullptr;
    p_qop[0]=0;
    p_opaque[0]=0;
    p_mport[0]=0;
    

    if (end_position == nullptr){
        if (PARSEDEBUG>1) printf( "No line ending found in %s\n", s_buffer);
        return false;
    }

    uint32_t line_number = 0;
    do{
        size_t length = end_position - start_position;
        if (length == 0) //line only contains the line ending
        {
            if (PARSEDEBUG>1)printf( "Valid end of header detected\n");
            if (p_content_length==0)
            {
                // no remaining data in buffer, so no body
                if (PARSEDEBUG>2)printf("No Body to Process\n");
                p_body = nullptr;
            }
            else
            {
                p_body = end_position + LINE_ENDING_LEN;
                if (PARSEDEBUG>2)printf("P_body %s\n",p_body);
            }
            return true;
        }
        const char* next_start_position = end_position + LINE_ENDING_LEN;
        line_number++;

        //create a proper null terminated c string from here on string functions may be used!
        memset(end_position, 0, LINE_ENDING_LEN);
        if (PARSEDEBUG>3)printf( "Parsing line: %s\n", start_position);

        if (strstr(start_position, SIP_2_0_SPACE) == start_position){
            long code = strtol(start_position + strlen(SIP_2_0_SPACE), nullptr, 10);
            if (PARSEDEBUG>1)printf( "Detect status %l\n", code);
            p_status = convert_status(code);
        }
        else if ((strncmp(WWW_AUTHENTICATE, start_position, strlen(WWW_AUTHENTICATE)) == 0)
                || (strncmp(PROXY_AUTHENTICATE, start_position, strlen(PROXY_AUTHENTICATE)) == 0)){
            if (PARSEDEBUG>2)printf( "Detect authenticate line\n");
            //read realm and nonce from authentication line
            if (!read_param_parse(start_position, REALM, p_realm)){
                if (PARSEDEBUG>1)printf( "Failed to read realm in authenticate line\n");
            }
            if (!read_param_parse(start_position, NONCE, p_nonce)){
                if (PARSEDEBUG>1)printf( "Failed to read nonce in authenticate line\n");
            }
            if (PARSEDEBUG>2)printf( "Realm is %s and nonce is %s\n", p_realm, p_nonce);
            
            if (!read_param_parse(start_position, QOP, p_qop)){
                if (PARSEDEBUG>1)printf( "Failed to read qop in authenticate line\n");
            }
            if (!read_param_parse(start_position, OPAQUE, p_opaque)){
                if (PARSEDEBUG>1)printf( "Failed to read nonce in authenticate line\n");
            }
            if (PARSEDEBUG>2)printf( "Opaque is %s and Qop is %s \n",p_opaque,p_qop);
        }
        else if (strncmp(CONTACT, start_position, strlen(CONTACT)) == 0){
            if (PARSEDEBUG>2)printf( "Detect contact line\n");
            const char* last_pos = strstr(start_position, ">");
            if (last_pos == nullptr)
            {
                if (PARSEDEBUG>1)printf( "Failed to read content of contact line\n");
            }
            else
            {
                stringptr(p_contact,start_position + strlen(CONTACT), last_pos);
            }
        }
        else if (strncmp(TO, start_position, strlen(TO)) == 0){
            if (PARSEDEBUG>2)printf( "Detect to line\n");
            const char* tag_pos = strstr(start_position, ">;tag=");
            if (tag_pos != nullptr){
                string(p_to_tag,tag_pos + strlen(">;tag="),PARSE_MAX);
            }
            string(p_to,start_position + strlen(TO),PARSE_MAX);
        }
        else if (strstr(start_position, FROM) == start_position){
            string(p_from,start_position + strlen(FROM),PARSE_MAX);
        }
        else if (strstr(start_position, VIA) == start_position){
            string(p_via,start_position + strlen(VIA),PARSE_MAX);
            if (strstr(p_via, RPORT)!= NULL){
               char* result = NULL;
               result = replaceWord(p_via, RPORT, "rport=5060;");           // BODGE should use local sip port if not 5060
//               sprintf(p_via,"%s",result);
               string(p_via,result,PARSE_MAX);
               free(result);
            }
        }
        else if (strstr(start_position, C_SEQ) == start_position){
            string(p_cseq,start_position + strlen(C_SEQ),PARSE_MAX);
        }
        else if (strstr(start_position, CALL_ID) == start_position){
            string(p_call_id ,start_position + strlen(CALL_ID),PARSE_MAX);
        }
        else if (strstr(start_position, CONTENT_TYPE) == start_position){
            p_content_type = convert_content_type(start_position + strlen(CONTENT_TYPE));
        }
        else if (strstr(start_position, CONTENT_LENGTH) == start_position){
            long length = strtol(start_position + strlen(CONTENT_LENGTH), nullptr, 10);
            if (length < 0){
                if (PARSEDEBUG>1)printf( "Invalid content length %ld\n", length);
            }
            else{
                p_content_length = length;
            }
        }

        else if (line_number == 1) {
            //first line, but no response
            p_method = convert_method(start_position);
        }
        //go to next line
        start_position = next_start_position;
        end_position = strstr(start_position, LINE_ENDING);
    } while(end_position);

    //no line only containing the line ending found :(
    return false;
}

bool parse_body(){
    if (p_body == nullptr){
        if (PARSEDEBUG>2)printf("NO BODY HERE???\n");
        return true;
    }

    const char* start_position = p_body;
    char* end_position = strstr(start_position, LINE_ENDING);

    if (end_position == nullptr){
        if (PARSEDEBUG>1)printf( "No line ending found in %s\n", p_body);
        return false;
    }

    do{
        size_t length = end_position - start_position;
        if (length == 0){ //line only contains the line ending
            if (PARSEDEBUG>1)printf("WE GOT NOBODY(zero length)???\n");
            return true;
        }

        const char* next_start_position = end_position + LINE_ENDING_LEN;

        //create a proper null terminated c string
        //from here on string functions may be used!
        memset(end_position, 0, LINE_ENDING_LEN);
        if (PARSEDEBUG>2)printf( "Parsing line: %s\n", start_position);

        if (strstr(start_position, SIGNAL) == start_position){
            p_dtmf_signal = *(start_position + strlen(SIGNAL));
        }
        else if (strstr(start_position, DURATION) == start_position){
            long duration = strtol(start_position + strlen(DURATION), nullptr, 10);
            if (duration < 0){
                if (PARSEDEBUG>1)printf( "Invalid duration %ld\n", duration);
            }
            else{
                p_dtmf_duration = duration;
                if (PARSEDEBUG>2)printf("DTMF_DURATION %i\n",duration);
            }
        }
        else if (strstr(start_position, MEDIA) == start_position){
            string(p_media,start_position + strlen(MEDIA),PARSE_MAX);
            //extract port from media
            if (strstr(start_position, CPORT) == start_position){
               string(p_mport,start_position + strlen(CPORT),PARSE_MAX);
               int a;
               for(a=0;a++;a<10){
                  if(p_mport[a]==' ')p_mport[a]=0;
               }
            }
            if (PARSEDEBUG>2)printf("MEDIA %s PORT %s\n",p_media,p_mport);
//            printf("MEDIA %s\n",p_media);
        }
        else if (strstr(start_position, CIP) == start_position){
            //extract IP from cline
            string(p_cip ,start_position + strlen(CIP),PARSE_MAX);
            if (PARSEDEBUG>2)printf("CIP %s\n",p_cip);
//            printf("CIP %s\n",p_cip);
        }

        //go to next line
        start_position = next_start_position;
        end_position = strstr(start_position, LINE_ENDING);
    } while(end_position);

    return true;
}

bool read_param_parse(const char* line, const char* param_name, char* output){
    const char* pos = strstr(line, param_name);
    if (pos == nullptr){
        return false;
    }
    if (*(pos + strlen(param_name)) != '='){
        return false;
    }

    if (*(pos + strlen(param_name) + 1) != '"'){
        return false;
    }

    pos += strlen(param_name) + 2;
    const char* pos_end = strchr(pos, '"');
    if (pos_end == nullptr){
        return false;
    }
    string(output,pos, pos_end-pos);
    if (PARSEDEBUG>2)printf("Parse %s OUTPUT !%s!\n",param_name,output); 
    return true;
}

enum Status convert_status(uint32_t code){
    switch (code){
        case 200: return ST_OK_200;
        case 401: return ST_UNAUTHORIZED_401;
        case 100: return ST_TRYING_100;
        case 180: return ST_RINGING_180;
        case 127: return ST_CANCEL_127;
        case 183: return ST_SESSION_PROGRESS_183;
        case 500: return ST_SERVER_ERROR_500;
        case 486: return ST_BUSY_HERE_486;
        case 487: return ST_REQUEST_CANCELLED_487;
        case 407: return ST_PROXY_AUTH_REQ_407;
        case 603: return ST_DECLINE_603;
    }
    printf("Can't convert Status %i\n",code);
    return ST_UNKNOWN;
}

enum Method convert_method(const char* input){
    printf("Convert Method !%s!",input);
    if (strstr(input, NOTIFY)==input){
        return MD_NOTIFY;
    }
    if (strstr(input, BYE)==input){
        return MD_BYE;
    }
    if (strstr(input, INFO)==input){
        return MD_INFO;
    }
    if (strstr(input, INVITE)==input){
        return MD_INVITE;
    }
    if (strstr(input, CANCEL)==input){
        return MD_CANCEL;
    }
    if (strstr(input, OPTIONS)==input){
        return MD_OPTIONS;
    }
    
    return MD_UNKNOWN;
}

enum ContentType convert_content_type(const char* input)
{
    if (strstr(input, APPLICATION_DTMF_RELAY)){
        return CT_APPLICATION_DTMF_RELAY;
    }
    return CT_UNKNOWN;
}
