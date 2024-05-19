#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "config.h"

#include "md5.h"
#include "sip_client.h"
#include "settings.h"

//from settings
extern const char *server_addr[]; // = DESTINATION_ADDR ;
extern int server_port[]; // = DESTINATION_PORT ;
extern const char *server_name[];
extern const char *sip_username[];
extern const char *sip_password[];
extern const char *sip_login[];

extern int sip_server_port;
extern int sip_local_port;
extern int rtp_local_port;

extern int server;

extern int registered;

extern int SendSound;
extern uint8_t enablertp;
// 
extern void send_sip_udp_blocking(char *,int,int);

extern char p_realm[];
extern char p_nonce[];
extern char p_media[];
extern char p_contact[];
extern char p_to_tag[];
extern char p_contant_type[];
extern char p_to[];
extern char p_from[];
extern char p_call_id[];
extern char p_cseq[];
extern char p_via[];
extern char p_qop[];
extern char p_opaque[];
extern char p_cip[];
extern char p_mport[];

extern enum Method p_method;
extern enum ContentType p_content_type;


extern int rdp_og_port;

int nc =0;

int strpos(const char *haystack, const char *needle, int offset){
   const char *p = strstr(haystack+offset, needle);
   if (p)
      return p - haystack+offset;
   return -1;
}

void substr(char *source, int a, int b, char *result) {
        strncpy(result, source+a, b);
}

void printMDState(enum Method state){
    STATETEXT
    printf("\nMethod:");
    switch (state) {
    case MD_NOTIFY:
        printf("MD_NOTIFY");
        break;
    case MD_BYE:
        printf("MD_BYE");
        break;
    case MD_INFO:
        printf("MD_INFO");
        break;
    case MD_INVITE:
        printf("MD_INVITE");
        break;
    case MD_UNKNOWN:
        printf("MD_UNKNOWN");
        break;
    case MD_CANCEL:
        printf("MD_CANCEL");
        break;
    case MD_OPTIONS:
        printf("MD_OPTIONS");
        break;    
    default:
        printf("METHOD NOT DEFINED"); 
    }    
    CLTEXT
    printf("\n");
}
        
void printState(enum SipState state){
    STATETEXT
    printf("\nSipState:");
    switch (state) {
      case SS_IDLE:
//        printf("IDLE");
        break;
      case SS_REGISTER_UNAUTH:
        printf("SS_REGISTER_UNAUTH");
        break;
      case SS_REGISTER_AUTH:
        printf("SS_REGISTER_AUTH");
        break;
      case SS_REGISTERED:
        printf("SS_REGISTERED");
        break;
      case SS_INVITE_UNAUTH:
        printf("SS_INVITE_UNAUTH");
        break;
      case SS_INVITE_UNAUTH_SENT:
        printf("SS_INVITE_UNAUTH_SENT");
        break;
      case SS_INVITE_AUTH:
        printf("SS_INVITE_AUTH");
        break;
      case SS_RINGING:
        printf("SS_RINGING");
        break;
      case SS_RINGTONE:
        printf("SS_RINGTONE");
        break;
      case SS_CALL_START:
        printf("SS_CALL_START");
        break;
      case SS_CALL_IN_PROGRESS_IH: //inititated here
        printf("SS_CALL_IN_PROGRESS_IH ");
        break;
      case SS_CALL_IN_PROGRESS_IT: //initiated there
        printf("SS_CALL_IN_PROGRESS_IT");
        break;
        
      case SS_CANCELLED:
        printf("SS_CANCELLED");
        break;
      case SS_ERROR:
        printf("SS_ERROR");
        break;
      default:
        printf("SS NOT DEFINED");
    } 
    CLTEXT
    printf("\n");
}

void printRStatus(enum Status status){
    STATETEXT
    printf("\nStatus:");
    switch (status) {

      case ST_TRYING_100:
        printf("ST_TRYING_100");
        break;
      case ST_RINGING_180:
        printf("ST_RINGING_180");
        break;
      case ST_SESSION_PROGRESS_183:
        printf("ST_SESSION_PROGRESS_183");
        break;
      case ST_OK_200:
        printf("ST_OK_200");
        break;
      case ST_UNAUTHORIZED_401:
        printf("ST_UNAUTHORIZED_401");
        break;
      case ST_PROXY_AUTH_REQ_407:
        printf("ST_PROXY_AUTH_REQ_407");
        break;
      case ST_BUSY_HERE_486:
        printf("ST_BUSY_HERE_486");
        break;
      case ST_REQUEST_CANCELLED_487:
        printf("ST_REQUEST_CANCELLED_487");
        break;
      case ST_SERVER_ERROR_500:
        printf("ST_SERVER_ERROR_500");
        break;
      case ST_DECLINE_603:
        printf("ST_DECLINE_603");
        break;
      case ST_CANCEL_127:
        printf("ST_CANCEL_127");
        break;    
      case ST_UNKNOWN:
        printf("ST_UNKNOWN");
        break;
      default:
        printf("ST NOT DEFINED");    
    }
    CLTEXT
    printf("\n");

}

void printPhoneState(enum PhoneState status){
   STATETEXT
    printf("\nPhoneState:");
    switch (status) {
      case PS_IDLE:
        printf("PS_IDLE");
        break;
//      case PS_REGISTERED:
//        printf("PS_REGISTERED");
//        break;
      case PS_RINGING:
        printf("PS_RINGING");
        break;
      case PS_DIALLING:
        printf("PS_DIALLING");
        break;
      case PS_ANSWER:
        printf("PS_ANSWER");
        break;
      case PS_HANGUP:
        printf("PS_HANGUP");
        break;
      case PS_DECLINED:
        printf("PS_DECLINED");
        break;
      case PS_ESTABLISHED:
        printf("PS_ESTABLISHED");
        break;
      default:
        printf("NOT DEFINED");

    }
    CLTEXT
    printf("\n");

}

void sip_init(){
        printf("Sip Init\n");

        sprintf(c_server_ip,"%s",server_addr[server] );
        sprintf(c_uri, "sip:%s", c_server_ip);
        sprintf(c_login, "%s", sip_login[server]);
        sprintf(c_user, "%s", sip_username[server]);
        sprintf(c_pwd,"%s",sip_password[server]);

        sprintf(c_to_uri, "%s@%s",c_user,c_server_ip);
        sprintf(c_my_ip,"%s",ip4addr_ntoa(netif_ip4_addr(netif_list)) );

        sprintf(c_call_id,"%i",rand() % 2147483647);
        sprintf(c_tag,"%i",rand() % 2147483647);
        sprintf(c_cnonce,"%i",rand() % 2147483647);

        nc++;

        sprintf(c_nc,"%08i",nc);        

        char buffer[500];        

        sprintf(buffer,"\nSIP INIT\n");
        sprintf(buffer,"%sServer:IP %s\n",buffer,c_server_ip);
        sprintf(buffer,"%sLogin:%s\n",buffer,c_login);
        sprintf(buffer,"%sUser:%s\n",buffer,c_user);
        sprintf(buffer,"%sRealm:%s\n",buffer,c_realm);

        sprintf(buffer,"%s\n\n",buffer);
        printf("%s",buffer);

        sprintf(buffer,"%sPass:%s\n",buffer,c_pwd);
        log_file_save("## Sip Setings:\n",buffer);                
}


void request_ring(const char* call_no, const char* caller_display){
    if (registered){         
        printf("Request to call %s...\n", call_no);
//        c_call_id = rand() % 2147483647;
        sprintf(c_call_id,"%i",rand() % 2147483647);
        sprintf(c_call,"%s",call_no);          
        ph_state=PS_DIALLING;
        cl_state = SS_INVITE_UNAUTH;
        tx();
             
        }else{
             printf("Not registered\n");
        }  
    }

    //request cancel from THIS phone.
    void request_cancel()
    {
        if ((cl_state == SS_RINGING)){
            printf( "Request to BUSY when RINGING call\n");
            send_sip_busy();
        }        
        if ((cl_state == SS_CALL_IN_PROGRESS_IH)) {
            printf( "Request to CANCEL ESTABLISHED call IH\n");
//            c_sip_sequence_number=atoi(p_cseq);
            send_sip_bye();
            
        }
        if ((cl_state == SS_CALL_IN_PROGRESS_IT)) {
            printf( "Request to CANCEL ESTABLISHED call IT\n");
//            c_sip_sequence_number=atoi(p_cseq);
            send_sip_bye_it();
            
        }
        
        ph_state = PS_IDLE;
        cl_state = SS_IDLE;
        enablertp=0;
        tx();
    }



    //send responce on cl_state
    void tx(){
        TXTEXT
        char temp[50];
        int ssrc;
        switch (cl_state) {
        case SS_IDLE:
            //fall-through
            break;
        case SS_REGISTER_UNAUTH:
            //sending REGISTER without auth
            printf("TX SS_REGISTER_UNAUTH\n");
            printf(c_tag,"%i",rand() % 2147483647);
            c_branch = rand() % 2147483647;
            send_sip_register();
            sprintf(c_tag,"%i",rand() % 2147483647);
            c_branch = rand() % 2147483647;
            break;
        case SS_REGISTER_AUTH:
            printf("TX SS_REGISTER_AUTH\n");
            //sending REGISTER with auth
            //must be the same as uri in Response
            sprintf(temp,"sip:%s:%i", c_my_ip,sip_local_port);
            compute_auth_response("REGISTER", temp);
            send_sip_register();
            break;
        case SS_REGISTERED:
            printf("TX SS_REGISTERED\n");
//            ph_state=PS_REGISTERED;
             registered=1;
            //options
//            if(p_method==MD_OPTIONS){
//                send_sip_options();// should be 200
//            }
            cl_state = SS_IDLE;
            break;
        case SS_INVITE_UNAUTH:
            printf("TX SS_INVITE_UNAUTH\n");
            //sending INVITE without auth
            //c_tag = rand() % 2147483647;
            sprintf(c_nonce,"");
            sprintf(c_realm,"");
            sprintf(c_response,"");

            c_response[0]=0; // ensure no pre vious responce is used
            sprintf(c_response,"");
            c_sdp_session_id = rand();
            send_sip_invite();
            break;
        case SS_INVITE_UNAUTH_SENT:
            printf("TX SS_INVITE_UNAUTH_SENT\n");
            break;
        case SS_INVITE_AUTH:
            printf("TX SS_INVITE_AUTH\n");
            //sending INVITE with auth
            c_branch = rand() % 2147483647;
            char uri[100];
            sprintf(uri,"sip:%s@%s",c_call,c_server_ip); 
            compute_auth_response("INVITE", uri);
            send_sip_invite();
            break;
        case SS_RINGING:
            printf("TX SS_RINGING\n");
            // RTP ssrc generation
            ssrc = rand() % 2147483647;
            if (p_method==MD_CANCEL){
                send_sip_ok(0);
//                sleep_ms(100);
                send_request_terminated();
//                ph_state=PS_REGISTERED;
                cl_state=SS_IDLE;              
            }
            if (ph_state==PS_DECLINED){
                printf("Sending cancel from DECLINE request\n");
                send_sip_cancel();
                ph_state=PS_IDLE;
                cl_state=SS_IDLE;
                enablertp=0;
            }
            break;
        case SS_CALL_START:
            printf("TX CALL_START\n");
            send_sip_ack();
            break;
        case SS_CALL_IN_PROGRESS_IT:
            //falle through    
        case SS_CALL_IN_PROGRESS_IH:
            printf("TX SS_CALL_IN_PROGRESS IT/IH\n");
//            if (ph_state==PS_ESTABLISHED){
//                printf( "Sending bye from ESTABLISHED request");
//                send_sip_bye();
//                ph_state=PS_IDLE;
//                cl_state=SS_IDLE;
//            }
            break;
        case SS_CANCELLED:
            printf("TX SS_CANCELED\n");
            send_sip_ack();
            printf(c_tag,"%i",rand() % 2147483647);
            c_branch = rand() % 2147483647;
            break;
        case SS_ERROR:
            printf("TX SS_ERROR %i\n",cl_state);
            break;
        }
        CLTEXT
    }

    void rx()
    {
        RXTEXT
    
        printf ("\n\nRX ");
        printState(cl_state);
        printf(" ");
        printRStatus(p_status);

        if (cl_state == SS_REGISTERED) {
           printf("RX REGISTERED\n");
   //             cl_state = SS_INVITE_UNAUTH;
        } else if (cl_state == SS_ERROR) {
            printf("RX SS_ERROR\n");
//            sleep_ms(100);
            c_sip_sequence_number++;
            cl_state = SS_IDLE;
            return;
        } else if (cl_state == SS_INVITE_UNAUTH) {
            printf("RX INVITE_UNAUTH\n");
            cl_state = SS_INVITE_UNAUTH_SENT;
        } else if (cl_state == SS_CALL_START) {
            printf("RX CALL_START\n");
//            cl_state = SS_CALL_IN_PROGRESS;
            return;
        } 
        
        // Parce received packet
        if (!parse()) {
            printf("!!!!!!!!!!!!!!!!! Parsing the packet failed !!!!!!!!!!!!!!!!!!!!!!!!!!!");
            return;
        }
        
        printf("After Parse\n");        
        printRStatus(p_status);
        printMDState(p_method);
        printPhoneState(ph_state);
        
        
        enum Status reply = p_status;   //replay= parced status
        printf( "Parsing the packet ok, reply code=%d\n", p_status); 

        if (reply == ST_SERVER_ERROR_500) {
            printf("RX SERVER_ERROR_500\n");
            //log_state_transition(cl_state, SS_ERROR);
            cl_state = SS_ERROR;
            return;
        } else if ((reply == ST_UNAUTHORIZED_401) || (reply == ST_PROXY_AUTH_REQ_407)) {
            printf("RX UNAUTHORIZED_401 PROXY_AUTH_REQ_407\n");
            sprintf(c_realm,"%s",p_realm );
            strncat(c_nonce,p_nonce,PARSE_MAX);
        } else if ((reply == ST_RINGING_180)){
            cl_state=SS_RINGTONE;
            printf("RX RINGTONE\n");    
        } else if ((reply == ST_UNKNOWN) && ((p_method == MD_NOTIFY) || (p_method == MD_BYE) || (p_method == MD_INFO) || (p_method == MD_INVITE) || (p_method == MD_CANCEL) || (p_method == MD_OPTIONS) ) ) {
            printf("RX MD_Various :)\n");
             //ok to immediatly answer, or ringing...    
//            send_sip_ok();
            
            if ((p_method == MD_INVITE)) {
                printf("RX MD_INVITE :)\n");
                printf("p_media: %s\n",p_media);
                int m1=strpos(p_media," ",0); 
            
                int m2=strpos(p_media," ",m1+1);
                printf("%i %i\n",m1,m2);
                substr(p_media,m1 + 1, m2 - m1 - 1,rtp_port);                
                printf("%s",rtp_port);

                sprintf(rtp_ip,"%s",p_cip);                    
                //RTP TO DO
                
                printf("RTP IP %s\n",p_cip);
                sprintf(c_call_id,"%s",p_call_id);
                //get port from invite
                rdp_og_port=atoi(p_mport);
                
                send_sip_ringing();
                cl_state=SS_RINGING;
            }
            
            if ((p_method == MD_BYE)) {
                printf("RX BYE\n");
                send_sip_ok(0);
                cl_state=SS_IDLE;
                ph_state=PS_IDLE;
                enablertp=0;
            
            }
            
             if ((p_method == MD_CANCEL)) {
                printf("RX CANCEL\n");
                 send_sip_ok(0);
                cl_state=SS_IDLE;
                ph_state=PS_IDLE;
                enablertp=0;
             }
             
             if ((p_method == MD_OPTIONS)) {
                printf("RX OPTIONS\n");
                 send_sip_ok(0);
//                cl_state=SS_IDLE;
//                ph_state=PS_IDLE;
                enablertp=0;
             }
   
            
        }
/*
        if(reply == ST_OK_200 && ph_state == PS_DIALLING){
           cl_state = SS_INVITE_AUTH;
           tx();
        }
*/
       
        

        if (p_contact[0]) {
            sprintf(c_to_contact,"%",p_contact);
        }

        if (p_to_tag[0]) {
            sprintf(c_to_tag,"%s",p_to_tag);
        }

        enum SipState old_state = cl_state;
//        printf ("Start of RX switch\n");
        printState(cl_state);        
        switch (cl_state) {
        case SS_IDLE:
            break;
        case SS_REGISTER_UNAUTH:
            printf("RX SS_REGISTER_UNAUTH \n");
            cl_state = SS_REGISTER_AUTH;
            c_sip_sequence_number++;
            break;
        case SS_REGISTER_AUTH:
            printf("RX SS_REGISTER_AUTH \n");
            if (reply == ST_OK_200) {
                printf("RX ST_OK_200 )\n");
                c_sip_sequence_number++;
                sprintf(c_nonce,"");
                sprintf(c_realm,"");
                sprintf(c_response,"");
                printf( "REGISTER - OK :)");
                
                cl_state = SS_REGISTERED;
//                ph_state = PS_REGISTERED;
                registered=1;
            } else {
                cl_state = SS_ERROR;
                printf("RX SS_ERROR \n");
            }
            break;
        case SS_REGISTERED:
            printf("RX SS_REGISTERED \n");
//            if (p_method == MD_INVITE  ) {            
            //answer incoming call here ?
//              if(ph_state==PS_ANSWER){             
                //received an invite, answered it already with ok, so new call is established, because someone called us
//                cl_state = SS_CALL_START;
//                printf ("State changed to CALL_START\n");
//                printf("Send OK to answer \n");
//                send_sip_ok();
//                ph_state==PS_ESTABLISHED;
//              }  
//              if(ph_state==PS_DIALLING){
//                  
//              }
//            }
            break;
        case SS_INVITE_UNAUTH_SENT:
            printf("RX SS_INVITE_UNAUTH SENT\n");
//            c_sip_sequence_number++;
            send_sip_ack();
        case SS_INVITE_UNAUTH:
            printf("RX SS_INVITE_UNAUTH - reply %i\n",reply);
            if ((reply == ST_UNAUTHORIZED_401) || (reply == ST_PROXY_AUTH_REQ_407)) {
                printf("RX ST_UNAUTHORIZED_401 / ST_PROXY_AUTH_REQ_407 \n");
                cl_state = SS_INVITE_AUTH;
                c_sip_sequence_number++;
            } else if ((reply == ST_OK_200) || (reply == ST_SESSION_PROGRESS_183)) {
                printf("ST_OK_200 / ST_SESSION_PROGRESS_183\n");
                cl_state = SS_RINGING;
                sprintf(c_nonce,"");
                sprintf(c_realm,"");
                sprintf(c_response,"");
                printf( "Start RINGing...");
            } else if (reply != ST_TRYING_100) {
                cl_state = SS_ERROR;
            }
            break;
        case SS_INVITE_AUTH:
            printf("RX SS_INVITE_AUTH - reply %i\n",reply);
            if ((reply == ST_UNAUTHORIZED_401) || (reply == ST_PROXY_AUTH_REQ_407)) {
                printf("ST_UNAUTHORIZED_401 / ST_PROXY_AUTH_REQ_407\n");
                cl_state = SS_ERROR;
            } else if ((reply == ST_OK_200) || (reply == ST_SESSION_PROGRESS_183) || (reply == ST_TRYING_100)) {
                printf("ST_OK_200 / ST_SESSION_PROGRESS_183 / reply == ST_TRYING_100\n");
                //trying is not yet ringing, but change state to not send invite again
                cl_state = SS_RINGING;
                sprintf(c_nonce,"");
                sprintf(c_realm,"");
                sprintf(c_response,"");
                printf( "Start RINGing...");
            } else {
                cl_state = SS_ERROR;
            }
            break;
        case SS_RINGTONE:
            if (reply == ST_OK_200) {
                printf("RINGTONE ST_OK_200\n");
                //other side picked up, send an ack
                send_sip_ack();
                cl_state = SS_CALL_IN_PROGRESS_IH;
                ph_state = PS_ESTABLISHED;
                
                printf("###RDP 1 ### set RDP %s:%s\n",p_cip,p_mport);
                rdp_og_port=atoi(p_mport);
                enablertp=1;
                
            }   
            break; 
        case SS_RINGING:
            printf("SS_RINGING\n");
            if (reply == ST_SESSION_PROGRESS_183) {
                printf("ST_SESSION_PROGRESS_183\n");
                //TODO parse session progress reply and send appropriate answer
                //cl_state = SS_ERROR;
            } else if (reply == ST_OK_200) {
                printf("ST_OK_200\n");
                
                //I think this is wrong, you answer, you dont receive a 200 !
                
                 printf("###RDP 2 ### set RDP %s:%s\n",p_cip,p_mport);                 
                 enablertp=1;    
                 
                 rdp_og_port=atoi(p_mport);
                 
                              
                cl_state = SS_CALL_START;
                ph_state=PS_ESTABLISHED;
            } else if (reply == ST_REQUEST_CANCELLED_487) {
                cl_state = SS_CANCELLED;
                 printf("SS_CANCELLED\n");
                 cl_state = SS_IDLE;
            } else if (reply == ST_PROXY_AUTH_REQ_407) {
                printf("ST_PROXY_AUTH_REQ_407\n");
                send_sip_ack();
                c_sip_sequence_number++;
                cl_state = SS_INVITE_AUTH;
                printf( "Go back to send invite with auth...");
            } else if ((reply == ST_DECLINE_603) || (reply == ST_BUSY_HERE_486)) {
                printf("ST_DECLINE_603 ST_BUSY_HERE_486\n");
                send_sip_ack();
                c_sip_sequence_number++;
                c_branch = rand() % 2147483647;
                cl_state = SS_IDLE;
                enum CancelReason cancel_reason = CR_CALL_DECLINED;
                if (reply == ST_BUSY_HERE_486) {
                    cancel_reason = CR_TARGET_BUSY;
                }
                // might need to send an actual cancel here
                cl_state = SS_IDLE;
            }
            break;
        case SS_CALL_START:
            //should not reach this point
            printf("Call START-ERROR\n");
            break;
        case SS_CALL_IN_PROGRESS_IT:
           //fallthrough    
        case SS_CALL_IN_PROGRESS_IH:
            printf("SS_CALL_IN_PROGRESS IT/IH\n");
            if (p_method == MD_BYE) {
                c_sip_sequence_number++;
                send_sip_ack();
                cl_state = SS_IDLE;
                ph_state = PS_IDLE;
                enablertp=0;
//                ss_state = 
            } else if ((p_method == MD_INFO) && (p_content_type == CT_APPLICATION_DTMF_RELAY)) {
                printf("MD_INFO / CT_APPLICATION_DTMF_RELAY\n");
            }
            break;
        case SS_CANCELLED:
            printf("SS_CANCELLED\n");
            if (reply == ST_OK_200) {
                c_sip_sequence_number++;
                send_sip_ack();
                cl_state = SS_IDLE;
            }
            break;
        case SS_ERROR:
            printf("SS_ERROR\n");
            c_sip_sequence_number++;
            cl_state = SS_IDLE;
            break;
        }

        if (old_state != cl_state) {
            //log_state_transition(old_state, cl_state);
        }
        CLTEXT
    }

    void  answer_ringing(){
       printf("Answer Ringing\n");
       
       send_sip_ok(1);
       enablertp=1;
       cl_state=SS_CALL_IN_PROGRESS_IT;
       ph_state=PS_ESTABLISHED;
    }

    void send_sip_register()
    {
        printf("Send SIP Register\n");
        char buffer[2048];
        
        char request_uri[100];
        sprintf(request_uri,"sip:%s:%i", c_my_ip,sip_local_port);
        
        char to_uri[100];
        sprintf(to_uri,"%s@%s",c_login,c_my_ip);
        
        char from_uri[100];
        sprintf(from_uri,"%s@%s",c_login,c_my_ip);              

        sip_header("REGISTER", request_uri, from_uri, to_uri, c_tag, c_to_tag, buffer);
        sprintf(buffer,"%sContact: <sip:%s@%s:%i;%s>\r\n",buffer,c_login,c_my_ip,sip_local_port,TRANSPORT_LOWER  );

        if (c_response[0]) {
            sprintf(buffer,"%sAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", algorithm=MD5, response=\"%s\", qop=%s, opaque=\"%s\", cnonce=\"%s\", nc=%s\r\n",
               buffer, c_user ,c_realm ,c_nonce, request_uri ,c_response,p_qop,p_opaque,c_cnonce,c_nc );            
        }
        sprintf(buffer,"%sAllow: INVITE, ACK, CANCEL, BYE, REFER, NOTIFY, MESSAGE, SUBSCRIBE, INFO, REGISTER\r\n",buffer);
        sprintf(buffer,"%sExpires: 600\r\n",buffer);
        sprintf(buffer,"%sContent-Length: 0\r\n",buffer);
        sprintf(buffer,"%s\r\n",buffer);

        //printf("\n\nSIP Register - Send UDP \n%s\nLEN %i\n",buffer,strlen(buffer));
        printf("SIP Register \n");
        send_sip_udp_blocking(buffer,strlen(buffer)-1,0);
    }
    
    void send_sip_busy(){
        printf("Send SIP Busy\n");
        char buffer[2048];
        sprintf(buffer,"");
        sip_reply_header("486 Busy Here"  ,  buffer);
        sprintf(buffer,"%sAllow: INVITE, OPTIONS, INFO, BYE, CANCEL, ACK, UPDATE, REFER, SUBSCRIBE, NOTIFY\n\r",buffer);
        sprintf(buffer,"%sContent-Length: 0\r\n",buffer);
        sprintf(buffer,"%s\r\n",buffer);
        send_sip_udp_blocking(buffer,strlen(buffer)-1,0);
    }
    
    void send_sip_options(){
        printf("Send SIP Options\n");
        char buffer[2048];
        sprintf(buffer,"");
        sip_reply_header("200 OK"  ,  buffer);
        sprintf(buffer,"%sAllow: INVITE, OPTIONS, INFO, BYE, CANCEL, ACK, UPDATE, REFER, SUBSCRIBE, NOTIFY\n\r",buffer);
        sprintf(buffer,"%sContent-Length: 0\r\n",buffer);
        sprintf(buffer,"%s\r\n",buffer);

        //printf("%s\n",buffer);
        printf("SIP Options \n");
        send_sip_udp_blocking(buffer,strlen(buffer)-1,0);
    }

    void populate_sdp(char * sdp_buffer){    
        sprintf(sdp_buffer,"v=0\r\no=- 0 4 IN IP4 %s\r\n",c_my_ip);
        sprintf(sdp_buffer,"%ss=SDP data\r\nt=0 0\r\n",sdp_buffer);
//        sprintf(sdp_buffer,"%sm=audio %i RTP/AVP 0 8 101\r\n",sdp_buffer,rtp_local_port);
        sprintf(sdp_buffer,"%sm=audio %i RTP/AVP 0 101\r\n",sdp_buffer,rtp_local_port);
        sprintf(sdp_buffer,"%sc=IN IP4 %s\r\n",sdp_buffer,c_my_ip);        
        sprintf(sdp_buffer,"%sa=rtcp:%i IN IP4 %s\r\n",sdp_buffer,rtp_local_port+1,c_my_ip);
        sprintf(sdp_buffer,"%sa=sendrecv\r\n",sdp_buffer);
        sprintf(sdp_buffer,"%sa=rtpmap:0 PCMU/8000\r\n",sdp_buffer);
//        sprintf(sdp_buffer,"%sa=rtpmap:8 PCMA/8000\r\n",sdp_buffer);
        sprintf(sdp_buffer,"%sa=rtpmap:101 telephone-event/8000\r\n",sdp_buffer);
        sprintf(sdp_buffer,"%sa=ptime:20\r\n",sdp_buffer);
        sprintf(sdp_buffer,"%sa=fmtp:101 0-15\r\n",sdp_buffer);
    }

    void send_sip_invite()
    {
        printf("Send SIP Invite\n");
        char buffer[2048];
        char sdp_buffer[1024];
        char from_uri[100];
        char to_uri[100];

        sprintf(to_uri,"%s@%s",c_call,c_server_ip);         
        sprintf(from_uri,"%s@%s",c_login,c_server_ip); 
        
        char request_uri[100];
        sprintf(request_uri,"sip:%s",to_uri);
        
        sip_header("INVITE", request_uri, from_uri ,to_uri , c_tag, "", buffer);
        sprintf(buffer,"%sContact: \"%s\" <sip:%s@%s:%i>\r\n",buffer,c_user,c_login,c_my_ip,sip_local_port );
        
        if (c_response[0]) {
              sprintf(buffer,"%sAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"sip:%s\", algorithm=MD5, response=\"%s\", qop=%s, opaque=\"%s\", cnonce=\"%s\", nc=%s\r\n"
                  ,buffer, c_user ,c_realm ,c_nonce,to_uri ,c_response,p_qop,p_opaque,c_cnonce,c_nc );
        }
        sprintf(buffer,"%sContent-Type: application/sdp\r\n",buffer);
        sprintf(buffer,"%sAllow: INVITE, ACK, CANCEL, BYE, REFER, NOTIFY, MESSAGE, SUBSCRIBE, INFO, REGISTER\r\n",buffer);
        
        populate_sdp(sdp_buffer);

        sprintf(buffer,"%sContent-Length: %i\r\n",buffer,strlen(sdp_buffer));
        sprintf(buffer,"%s\r\n%s",buffer,sdp_buffer);
        printf("SIP Invite\n");
        
        send_sip_udp_blocking(buffer,strlen(buffer)-1,0);
    }
    
    

    /**
     * CANCEL a pending INVITE
     *
     * To match the INVITE, the following parameter must not be changed:
     * * CSeq
     * * From tag value
     */
    void send_sip_cancel()
    {
        printf("Send Sip Cancel\n");
        char buffer[2048];
        char from_uri[100];
        sprintf(from_uri,"%s@%s",c_login,c_my_ip);
        
        sip_header("CANCEL", c_uri, from_uri, c_to_uri, c_tag, c_to_tag, buffer);
        if (c_response[0]) {
            sprintf(buffer,"%sContact: \"%s\" <sip:%s@%s:%i;transport=%s>\r\n",buffer,c_user,c_user ,c_my_ip,sip_local_port,TRANSPORT_LOWER );
            sprintf(buffer,"%sContent-Type: application/sdp\r\n",buffer);
            sprintf(buffer,"%sAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s,\"\r\n",buffer,c_user,c_realm ,c_nonce,c_uri,c_response );
        }
        sprintf(buffer,"%sContent-Length: 0\r\n",buffer);
        sprintf(buffer,"%s\r\n",buffer);

        printf("%s\n",buffer);
        printf("SIP Cancel\n");
        send_sip_udp_blocking(buffer,strlen(buffer)-1,0);
    }


    //BYE for calls initiated remotely
    void send_sip_bye_it(){
        printf("BYE INITIATED THERE");
        char buffer[2048];
        
        c_sip_sequence_number++;

        char request_uri[100];
        sprintf(request_uri,"sip:%s@%s:%i",c_login,c_server_ip,sip_local_port);

        sprintf(buffer,"BYE %s SIP/2.0\r\n",request_uri);

        sprintf(buffer,"%sCSeq: %i BYE\r\n",buffer,c_sip_sequence_number );
        sprintf(buffer,"%sCall-ID: %s\r\n",buffer,c_call_id);
        sprintf(buffer,"%sMax-Forwards: 70\r\n",buffer);
        sprintf(buffer,"%sUser-Agent: %s\r\n",buffer,USER_AGENT);

        sprintf(buffer,"%sTo: %s\r\n",buffer,p_from);
        sprintf(buffer,"%sFrom: %s\r\n",buffer,p_to);

        sprintf(buffer,"%sVia: SIP/2.0/%s %s:%i;branch=z9hG4bK-%i\r\n",buffer,TRANSPORT_UPPER,c_my_ip ,sip_local_port,c_branch);

        sprintf(buffer,"%sContent-Length: 0\r\n",buffer);
        sprintf(buffer,"%s\r\n",buffer);
        printf("%s\n",buffer);

        send_sip_udp_blocking(buffer,strlen(buffer)-1,0);
    }


    //BYE for calls initiated HERE
    void send_sip_bye()
    {
        char buffer[2048];
        
        c_sip_sequence_number++;
        char to_uri[100];
        char from_uri[100];
        sprintf(to_uri,"%s@%s",c_login,c_server_ip);
        sprintf(from_uri,"%s@%s",c_login,c_server_ip);

        char request_uri[100];
        sprintf(request_uri,"sip:%s",to_uri);

        sip_header("BYE", request_uri, from_uri, to_uri, c_tag, c_to_tag, buffer);

        if (c_response[0]) {
            sprintf(buffer,"%sContact: \"%s\" <sip:%s@%s:%i;transport=%s>\r\n",buffer,c_user,c_user ,c_my_ip,sip_local_port,TRANSPORT_LOWER );
            sprintf(buffer,"%sContent-Type: application/sdp\r\n",buffer);
            sprintf(buffer,"%sMax-Forwards: 70\r\n",buffer);
            sprintf(buffer,"%sAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s,\"\r\n",buffer,c_user,c_realm ,c_nonce,c_uri,c_response );
        }
        sprintf(buffer,"%sContent-Length: 0\r\n",buffer);
        sprintf(buffer,"%s\r\n",buffer);
        printf("%s\n",buffer);
        printf("SIP Bye \n");
        send_sip_udp_blocking(buffer,strlen(buffer)-1,0);
    }

    void send_sip_ack()
    {
        char buffer[2048];      
        char sdp_buffer[1024];
        
        char to_uri[100];
        char from_uri[100];
        char request[100];
                
        if (cl_state == SS_CALL_START) {
            printf("ACK Call Start\n");
            sprintf(to_uri,"sip:%s@%s",c_user,c_server_ip);
            sprintf(from_uri,"sip:%s@%s",c_login,c_my_ip);

            sip_header("ACK", c_to_contact, from_uri, c_to_uri,c_tag, c_to_tag, buffer);          
            sprintf(sdp_buffer,"v=0\r\n");
            sprintf(sdp_buffer,"t=0 0\r\n");
            sprintf(buffer,"%sContent-Type: application/sdp\r\n",buffer);
//            sprintf(buffer,"%sAllow-Events: telephone-event\r\n",buffer);
            sprintf(buffer,"%sContent-Length: %s\r\n",buffer,strlen(sdp_buffer));
            sprintf(buffer,"%s\r\n%s",buffer,sdp_buffer);
        } else {
            printf("ACK NOT Call Start\n");
            sprintf(to_uri,"%s@%s",c_call,c_server_ip);
            sprintf(from_uri,"%s@%s",c_login,c_server_ip);
            sprintf(request,"sip:%s@%s:%i",c_call,c_server_ip,sip_local_port);
            sip_header("ACK", request, from_uri,to_uri,c_tag, c_to_tag, buffer);
            sprintf(buffer,"%sContent-Length: 0\r\n",buffer);
            sprintf(buffer,"%s\r\n",buffer);
        }
        //printf("%s\n",buffer);
        printf("SIP ACK\n");
        send_sip_udp_blocking(buffer,strlen(buffer)-1,0);
    }

    void send_sip_ok(int withsdp){
        printf("Send Sip OK\n");
        char buffer[2048];
        char sdp_buffer[1024];
        sprintf(buffer,"");
        sip_reply_header("200 OK"  ,  buffer);
//        sprintf(buffer,"%sUser-Agent: %s\r\n",buffer,USER_AGENT);
        sprintf(buffer,"%sContact: <sip:%s@%s:%i>\r\n",buffer,c_login,c_my_ip,sip_local_port);
        sprintf(buffer,"%sAllow: INVITE, OPTIONS, INFO, BYE, CANCEL, ACK, UPDATE, REGISTER, SUBSCRIBE, NOTIFY\r\n",buffer);
        if(withsdp){
            printf("  With SDP\n");
            populate_sdp(sdp_buffer);
            sprintf(buffer,"%sContent-Type: application/sdp\r\n",buffer);
            sprintf(buffer,"%sContent-Length: %i\r\n",buffer,strlen(sdp_buffer));           
            sprintf(buffer,"%s\r\n%s",buffer,sdp_buffer);
        }else{
            sprintf(buffer,"%sContent-Length: 0\r\n\r\n",buffer);
        }
        //sprintf(buffer,"%s",buffer);
        //printf("%s\n",buffer);
//        printf("SIP OK\n");
//        log_file_save("## SIP OK:\n",buffer);
        send_sip_udp_blocking(buffer,strlen(buffer)-1,0);
        
    }

    void send_request_terminated(){
        printf("Send SipRequest Terminatedu\n");
        char buffer[2048];
        sprintf(buffer,"");
        sip_reply_header("487 Request Terminated"  ,  buffer);
        sprintf(buffer,"%sUser-Agent: %s\r\n",buffer,USER_AGENT);
        sprintf(buffer,"%sContent-Length: 0\r\n",buffer);
        sprintf(buffer,"%s\r\n",buffer);
        
        //printf("%s\n",buffer);
        printf("SIP Request Terminated\n");
        send_sip_udp_blocking(buffer,strlen(buffer)-1,0);
        
    }

    void send_sip_ringing()
    {
        printf("Send Sip Ringing\n");
        //add tag to TO
        int tag=rand() % 2147483647;
        sprintf(p_to,"%s;tag=%i",p_to,tag);

        char buffer[2048];
        sip_reply_header("180 Ringing"  ,  buffer);
        sprintf(buffer,"%sContact: \"%s\" <sip:%s@%s:%i>\r\n",buffer,c_user,c_login ,c_my_ip,sip_local_port);
        sprintf(buffer,"%sAllow: INVITE, ACK, BYE, OPTIONS, NOTIFY, REGISTER, CANCEL\r\n",buffer);
        sprintf(buffer,"%sContent-Length: 0\r\n",buffer);
        sprintf(buffer,"%s\r\n",buffer);
                
//        printf("SIP Ringing\n");
        send_sip_udp_blocking(buffer,strlen(buffer)-1,0);
        
    }

    void sip_header(const char * command, const char * request_uri, char * from_uri, char * to_uri, char * from_tag, char * to_tag, char * buffer)
    {
        printf("sip_header CMD:%s\n",command);
        sprintf(buffer,"%s %s SIP/2.0\r\n",command,request_uri);  

        sprintf(buffer,"%sCSeq: %i %s \r\n",buffer,c_sip_sequence_number,command );
        sprintf(buffer,"%sCall-ID: %s\r\n",buffer,c_call_id);
        sprintf(buffer,"%sMax-Forwards: 70\r\n",buffer);
        sprintf(buffer,"%sUser-Agent: %s\r\n",buffer,USER_AGENT);
        
        
        if (strcmp(command,"INVITE")==0) {
            printf("Header INVITE");
             if(from_tag[0]){
                sprintf(buffer,"%sFrom: \"%s\" <sip:%s>;tag=%s\r\n",buffer,c_user,from_uri,from_tag);
             }else{
                sprintf(buffer,"%sFrom: \"%s\" <sip:%s>\r\n",buffer,c_user,from_uri);
             }   
        } else if (strcmp(command,"REGISTER")==0) {
            printf("Header REGISTER\n");
              if(from_tag[0]){
                  sprintf(buffer,"%sFrom: <sip:%s>;tag=%s\r\n",buffer,from_uri,from_tag);
              }else{
                  sprintf(buffer,"%sFrom: <sip:%s>\r\n",buffer,from_uri);
              }    
        } else {
            printf("Header not REGISTER or INVITE\n");
            if(from_tag){
                sprintf(buffer,"%sFrom: <sip:%s>;tag=%s\r\n",buffer,from_uri,from_tag);
            }else{
                sprintf(buffer,"%sFrom: <sip:%s>\r\n",buffer,from_uri);
            }
            
        }
        sprintf(buffer,"%sVia: SIP/2.0/%s %s:%i;branch=z9hG4bK-%i\r\n",buffer,TRANSPORT_UPPER,c_my_ip ,sip_local_port,c_branch);

        if ((strcmp(command,"ACK")!=0) && (c_to_tag[0])) {
            printf("notACK\n");
            if(to_tag[0]>0){ 
                sprintf(buffer,"%sTo: <sip:%s>;tag=%s\r\n",buffer,to_uri,to_tag);   
            }else{
                sprintf(buffer,"%sTo: <sip:%s>\r\n",buffer,to_uri);
            }
        } else {
            printf("ACK\n");
            if(to_tag>0){
                sprintf(buffer,"%sTo: <sip:%s>;tag=%s\r\n",buffer,to_uri,to_tag);
            }else{
                sprintf(buffer,"%sTo: <sip:%s>\r\n",buffer,to_uri);
            }
        }
    }


    void sip_reply_header(char * code,char * buffer)
    {
        printf("Sip Reply header %s\r\n",code);
        sprintf(buffer,"SIP/2.0 %s\r\n",code);
        sprintf(buffer,"%sTo: %s\r\n",buffer,p_to);
        sprintf(buffer,"%sFrom: %s\r\n",buffer,p_from);
        sprintf(buffer,"%sVia: %s\r\n",buffer,p_via);
        sprintf(buffer,"%sCSeq: %s\r\n",buffer,p_cseq);
        sprintf(buffer,"%sCall-ID: %s\r\n",buffer,p_call_id);
        sprintf(buffer,"%sMax-Forwards: 70\r\n",buffer);      
        sprintf(buffer,"%sUser-Agent: %s\r\n",buffer,USER_AGENT);        
        printf("Sip Reply Header End\n");
        
    }
    

    bool read_param_client(const char * line, const char * parac_name, char * output)
    {
        char param[100];
        sprintf(param,"%s=\"",parac_name);
        int pos = strpos(line,param,0);
        if (pos == -1) {
            return false;
        }
        pos += strlen(param);
        int pos_end = strpos(line,param,pos);
        if (pos_end == -1) {
            return false;
        }
        substr(line,pos,pos_end,output);
        return true;
    }

    void compute_auth_response(const char * method, const char * request_uri)
    {
        char ha1_text[50]; //32char long hex of 16 chars
        char ha2_text[50];
        char ccnonce[100];
        sprintf(c_response,"");
        char data[1024];
        MD5_CTX mdContext;
        
        //HA1 from user realm password
        sprintf(data,"%s:%s:%s",c_user,c_realm,c_pwd);
        
        MD5Init (&mdContext);
        MD5Update (&mdContext,data,strlen(data));
        MD5Final (&mdContext);

        to_hex(ha1_text, mdContext.digest, 16);
        
#ifdef MDDEBUG        
        printf("\n                      user:realm:pwd\n");
        printf("Calculating md5 for : %s\n", data);
        printf("Hex ha1 is %s\n", ha1_text);
#endif
        //HA2 Method request_uri
        //data = method + ":" + request_uri;
        sprintf(data,"%s:%s",method,request_uri);
        
        MD5Init (&mdContext);
        MD5Update (&mdContext,data,strlen(data));
        MD5Final (&mdContext);

        to_hex(ha2_text, mdContext.digest, 16);
        
#ifdef MDDEBUG
        printf( "\n                    method:request_uri\n");
        printf( "Calculating md5 for : %s\n", data);
        printf( "Hex ha2 is %s\n", ha2_text);
#endif

        if(p_qop[0]==0){
           //without QOP
           //responce HA1 nonce HA2         
#ifdef MDDEBUG           
           printf("\n                ha1:nonce:ha2\n");
#endif
           sprintf(data,"%s:%s:%s",ha1_text,c_nonce, ha2_text);
        }else{
           //with QOP
           //responce HA1 nonce cnonce qop HA2

#ifdef MDDEBUG
           printf("\nQOP AUTH\n");
           printf("\n                ha1:nonce:nc,cnonce:qop:ha2\n");
#endif

           sprintf(data,"%s:%s:%s:%s:%s:%s",ha1_text,c_nonce,c_nc,c_cnonce,p_qop,ha2_text);

        } 

        MD5Init (&mdContext);
        MD5Update (&mdContext,data,strlen(data));
        MD5Final (&mdContext);

        to_hex(c_response,  mdContext.digest, 16);
        
#ifdef MDDEBUG
        printf("\nCalculating md5 for : %s\n", data);
        printf("Hex response is %s\n", c_response);
#endif

    }

    void to_hex(char * dest, const unsigned char* data, int len)
    {
        static const char hexits[17] = "0123456789abcdef";
        int c=0;
        for (int i = 0; i < len; i++) {
            dest[c]=hexits[data[i] >> 4];
            c++;
            dest[c]=hexits[data[i] & 0x0F];
            c++;
        }
        dest[c]=0;
    }
