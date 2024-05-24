/**
 * TELECOW 
 *
 * https://github.com/ExtremeElectronics/TeleCow
 *
 * voip.c
 *
 * Copyright (c) 2024 Derek Woodroffe <tesla@extremeelectronics.co.uk>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h" 
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"


#include "lwip/pbuf.h"
#include "lwip/udp.h"

#include "ff.h"
#include "f_util.h"

#include "config.h"
#include "settings.h"
#include "sip_client.h"

#include "sound/sound.h"

#include "display/display.h"
#include "keyboard/keyboard.h"

#include "RTP/RTPPacket.h"

#include "voip.h"

//time messages are on display 
#define DISPTIME 10

int ReRegistrationTimer=0;
int SipTimerCounter=0;
int SipTimerCounter10=0;
struct repeating_timer siptimer;

int registered=0; //non zero when registered with server

uint8_t usedwifi=0;

uint8_t ringcnt=0;

//from settings.c
extern const char *wifi_ssid[];
extern const char *wifi_pass[];

extern int maxservers;
extern int debug;

extern int SDStatus; // 0 no sd, 1 sd detected, 2 read ini
extern int sip_local_port; //my sip port
extern int rtp_local_port; //my rtp port
extern int maxwifi; // max number of wifi details in SD card
extern const char *server_name[]; // = DESTINATION_name ;
extern const char *server_addr[]; // = DESTINATION_ADDR ;
extern const char *autodial[]; // = AUTO Dial number ;
extern const int server_port[]; // = DESTINATION_PORT ;
extern const char *server_realm[]; //realm = DNS name of server mostlikly
extern const char *sip_login[]; //realm = DNS name of server mostlikly

//from sip_parse
extern char p_cip[]; //client RDP ip ???? 
extern uint16_t p_mport; //client RDP port
extern char p_from[]; //from from parse
extern enum Method p_method;

//display backgrounds
extern char telecow1306[];
extern char telecow1306a[];
//keep track of what we have displayed.
enum PhoneState LastPhState;

extern uint8_t RTP_buff[]; //secondary outgoing RTP buffer

int server=0; // server settings selected.

int sip_server_port; //port for sip server
int rdp_og_port;

//keyboard vars & digit store
char lastkey;
char dialdigits[40];
uint8_t ddcnt=0; 

//sound
//extern uint8_t MikeBuffer[];
//extern uint16_t MikeIn;
extern uint8_t RTPSndBuffer[];

//udp lwip settings
struct udp_pcb* sip_pcb ;
struct udp_pcb* rtp_pcb ;
ip_addr_t sip_target_addr;
ip_addr_t rdp_target_addr;
ip_addr_t my_addr;

int rdp_target_port=5062; //backstop - should be set from parse.

//int GotSipData=0;
int GotRtpData=0;

//status flags
volatile uint8_t online=0;
volatile uint8_t SendSound=0;
volatile uint8_t sendingudp=0;

uint8_t enablertp=0;

uint8_t s_buffer[9][2048]; //sip buffer - 10 packets 
uint16_t b_in=0;  //buffer pointers
uint16_t b_out=0; //buffer pointers
uint8_t r_buffer[1024]; //rtp UDP buffer
extern uint16_t fringpos; //ringer pointer

//display vars
enum PhoneState disp_state;
//volatile uint8_t Core1Display=0; //when ready hand over display to core1

void halt(void){
    //dfa ...
    printf("\n\n####### HALTED #########");
    while(1){
        sleep_ms(100);
    }
}

/* ##################################### UDP Functions ################################# */

static void udpReceiveSipCallback(void *_arg, struct udp_pcb *_sip_pcb,struct pbuf *_p, const ip_addr_t *_addr,uint16_t _port) {
    char *_pData = (char *)_p->payload;
    uint16_t x; 
    for(x=0;x<_p->len;x++){
       s_buffer[b_in][x]=_pData[x];
    }
    s_buffer[b_in][x]=0; //zero terminate, just in case :) 
    b_in++;
    if (b_in>9){b_in=0;}
    if(b_in==b_out){
        printf("\n\n!!!!!!! Overrun on SIP RX !!!!!!! \n");
    }     
     
     
    pbuf_free(_p); // don't forget to release the buffer!!!!
//    log_file_save("## SIP Receive:\n",s_buffer);
}

static void udpReceiveRtpCallback(void *_arg, struct udp_pcb *_rtp_pcb,struct pbuf *_p, const ip_addr_t *_addr,uint16_t _port) {
    char *_pData = (char *)_p->payload;

    if (enablertp){
        RTPPacket_deserialize(_pData, _p->len);
        for(uint16_t x=0;x<SamplesPerPacket;x++){
            AddToSpkBuffer(_pData[x+12]);
        }
    }
    
    pbuf_free(_p); // don't forget to release the buffer!!!!
//    log_file_save("## RTP Receive:\n",s_buffer);
}

void init_udp(){
    err_t er;
    ipaddr_aton(server_addr[server], &sip_target_addr);

    //sip 
    sip_pcb = udp_new();
    er=udp_bind(sip_pcb, IP_ADDR_ANY, sip_local_port);
    if (er != ERR_OK) {
            printf("Failed to bind RX SIP UDP! error=%d", er);
            halt();
    }
    udp_recv(sip_pcb, udpReceiveSipCallback, NULL);    
    sip_server_port=server_port[server]; //set server port
    
    //rtp
    rtp_pcb = udp_new();
    er=udp_bind(rtp_pcb, IP_ADDR_ANY, rtp_local_port);
    if (er != ERR_OK) {
            printf("Failed to bind RX RTP UDP! error=%d", er);
            halt();
    }
    udp_recv(rtp_pcb, udpReceiveRtpCallback, NULL);

}

void send_udp_sip(char* msg,int msglen){
    //length WITHOUT trailing 0
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, msglen, PBUF_RAM);
    char *req = (char *)p->payload;
    memset(req, 0, msglen+1);
    int a;
    for(a=0;a<msglen;a++) req[a]=msg[a];
    req[a]=0;
    err_t er = udp_sendto(sip_pcb, p, &sip_target_addr, sip_server_port);
    pbuf_free(p);
    if (er != ERR_OK) {
        printf("Failed to send UDP packet! error=%d", er);
    } else {
//        printf("Sent packet \n\n");
    }
//    log_file_save("## Send:\n",msg);
}

void send_sip_udp_blocking(char* msg,int msglen,int debug){
  //length WITHOUT trailing 0
   if (online){
       while(sendingudp==1) sleep_ms(1);
       sendingudp=1;
       send_udp_sip( msg,msglen);
       sendingudp=0;
       if(debug){
          TXTEXT
          printf("\nSIP TX:\n%s\n",msg);
          CLTEXT
       }   
   }else{
       printf("Not online\n");
   }    
}

void send_udp_rdp(char* msg,int msglen){
    //length WITHOUT trailing 0
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, msglen+1, PBUF_RAM);
    char *req = (char *)p->payload;
    memset(req, 0, msglen+1);
    int a;
    for(a=0;a<msglen;a++) req[a]=msg[a];
    req[a]=0;
    //use p_cip and p_media as RDP ports.
    ipaddr_aton(p_cip, &rdp_target_addr);
    if(rdp_og_port<99){
      printf("RDP PORT INVALID %i \n",rdp_og_port);
    }
    err_t er = udp_sendto(rtp_pcb, p, &rdp_target_addr, rdp_og_port);
    pbuf_free(p);
    if (er != ERR_OK) {
        printf("Failed to send UDP packet! error=%d", er);
    } else {
//        printf("Sent packet \n\n");
    }
//    log_file_save("## Send:\n",msg);
}

void send_rdp_udp_blocking(char* msg,int msglen,int debug){
  //length WITHOUT trailing 0
    if (online){
        while(sendingudp==1) sleep_ms(1);
        sendingudp=1;
        send_udp_rdp( msg,msglen);
        sendingudp=0;
        if(debug) printf("RDP_UDP_SEND\n%s\n",msg);
    }else{
        printf("Not online\n");
    }    
}

//setup gpio port for control
void gpio_conf(void){
    gpio_set_dir(RING,GPIO_IN);
    gpio_pull_down(RING);
    gpio_set_dir(DIAL,GPIO_IN);
    gpio_pull_down(DIAL);
    gpio_set_dir(ANSWER,GPIO_IN);
    gpio_pull_down(ANSWER);
    gpio_set_dir(HANGUP,GPIO_IN);
    gpio_pull_down(HANGUP);
    
}

void set_host_name(const char*hostname)
{
    cyw43_arch_lwip_begin();
    struct netif *n = &cyw43_state.netif[CYW43_ITF_STA];
    netif_set_hostname(n, hostname);
    netif_set_up(n);
    cyw43_arch_lwip_end();
}


void init_network(){

    if (cyw43_arch_init()){
        printf("cyw43 failed to initialise\n");
        halt();
    }
    cyw43_arch_enable_sta_mode();

    char hostname[20];
    sprintf(hostname,"%s-%s",HOSTNAME,sip_login[server]);
    printf("Set hostname to %s\n",hostname);
    
    set_host_name(hostname);
    //set_host_name("PiTeleCow");

    if(maxwifi==0){
        printf("No wifi details maxwifi=0\n");
        halt();
    }

    printf("Connecting to Wi-Fi %s ...\n",wifi_ssid[usedwifi]);
      
    int x=1;
    int t=10;
    
    char statline[30];
    while(x){
        x=cyw43_arch_wifi_connect_timeout_ms(wifi_ssid[usedwifi], wifi_pass[usedwifi], CYW43_AUTH_WPA2_AES_PSK, 5000);
        if (x){
            printf("trying. SSID[%i] %s \n",usedwifi,wifi_ssid[usedwifi]);
            snprintf(statline,13,"W:%s",wifi_ssid[usedwifi]);
            drawStatus(statline,STATUSLINEP);

            sleep_ms(100);
            t=t+1;
            if (t>1){
                 usedwifi++;
                 t=0;
                 if ((usedwifi>maxwifi) || (wifi_ssid[usedwifi][0]==0) ){
                      printf("\n\nSorry, given up with the WIFI here\n");
                      drawStatusCentered("No Wifi Conn",STATUSLINEP,10);
                      sleep_ms(1000);
                      halt();
                 }
            }
        } else {
            printf("\n\n************ Connected to %s **************\n\n",wifi_ssid[usedwifi]);
            drawStatusCentered("WIFI Conn",STATUSLINEP,10);
            printf("MyIP %s - Ports SIP:%i RTP:%i\n",ip4addr_ntoa(netif_ip4_addr(netif_list)),sip_local_port,rtp_local_port);
            printf("SIP Server Ip %s:%i Realm %s\n\n",server_addr[server],server_port[server],server_realm[server] );
            printf("*************************************\n\n");
            char buffer[100];
            sprintf(buffer,"MyIP %s:%i:%i -  Dest Ip %s:%i Realm:%s \n\n",ip4addr_ntoa(netif_ip4_addr(netif_list)),sip_local_port,rtp_local_port,server_addr[server],server_port[server],server_realm[server] );
            log_file_save("## WiFi :/n",buffer);
            online=1;
        }
    }

    printf("\nWiFi Ready...\n");
    init_udp();
}

void contactfromfrom(char * from, char *r){
    //find "'s 
    uint8_t ic1=0x0ff,ic2=0x0ff, nc1=0x0ff, nc2=0x0ff;
    int x=0,c=0;
    r[0]=0;
    while(from[x]){
      if(from[x]=='\"'){
          if(ic1==0xff){
            ic1=x;
          }else{
            ic2=x;
          }
      }
      if(from[x]==':')nc1=x;
      if(from[x]=='>')nc2=x;    
      x++;
    }
    if((ic1!=0xff) && (ic2!=0xff)){
       for(x=ic1+1;x<ic2;x++){
         r[c]=from[x];
         c++;
       }
       r[c]=0;    
    }else{
      if((nc1!=0xff) && (nc2|=0xff)){
         for(x=nc1+1;x<nc2;x++){
           r[c]=from[x];
           c++;
         }
         r[c]=0;
      }
    }

    printf("disp %s \n",r);

}

void StatusClear(void){
    drawStatus("               ",STATUSLINE1);   //fill rectangle ????????
    drawStatus("               ",STATUSLINE2);
    drawStatus("               ",STATUSLINE3);
}


//update display from ph_status changes.
void display_ph_status(){
    if(ph_state!=LastPhState){
        LastPhState=ph_state;
        printf("Disp State change \n");
        printPhoneState(LastPhState);
        
        if(LastPhState==PS_IDLE){
            drawStatus("  ",STATUSLINE1);
            DispConn(" ");
        }
        if(LastPhState==PS_REGISTERED){
            drawStatusCentered("Registered",STATUSLINE1,4);
            drawStatusCentered(sip_login[server],STATUSLINE2,4);
            sleep_ms(DISPTIME);
        }                   
        if(LastPhState==PS_NOT_REGISTERED){
            drawStatusCentered("NOT Registered",STATUSLINE1,4);
            sleep_ms(DISPTIME);
        }                   
        if(LastPhState==PS_RINGING){
            if (fringpos==0)fringpos=1;
            printf("################################################ STATED RINGER\n");
            drawStatusCentered("Ring Ring",STATUSLINE1,4);

            char r[99];
            contactfromfrom(p_from,r);
            DispConn(r);
            sleep_ms(1500);             
        }
        if(LastPhState==PS_DIALLING){
            printf("################################################ DIALING\n");
            drawStatusCentered(" Dialing ",STATUSLINE1,4);

            char r[99];
            sprintf(r,"%s",c_call);
            DispConn(r);
            sleep_ms(DISPTIME);             
        }
        if(LastPhState==PS_RINGTONE){
            printf("################################################ GOT RINGTONE\n");
            drawStatusCentered("  Calling ",STATUSLINE1,4);

            char r[99];
            sprintf(r,"%s",c_call);
            DispConn(r);
            sleep_ms(DISPTIME);
        }
        if(LastPhState==PS_OG_INCALL){ //initiated here
            printf("################################################ CONNECTED IH\n");
            drawStatusCentered(" Connected",STATUSLINE1,4);             
             
            char r[99];
            sprintf(r,"%s",c_call);
            DispConn(r);
            sleep_ms(DISPTIME);
        }
        if(LastPhState==PS_IC_INCALL){ //initiated there
            printf("################################################ CONNECTED IT\n");
            drawStatusCentered(" Connected",STATUSLINE1,4);

            char r[99];
            contactfromfrom(p_from,r);
            DispConn(r);
            sleep_ms(DISPTIME);
        }
        if(cl_state==SS_ERROR){ //initiated there
            printf("################################################ ERROR\n");
            drawStatusCentered("ERROR",STATUSLINE1,4);
        }
        

    }//if
}

// ******************************* CORE 1 *************************************

void Core1Main(void){
     printf("Core 1 -\n");
     StartSoundTimer();

     int dc=0;

     sleep_ms(1000);
     while(1){
         tight_loop_contents(); //twiddle your peripherials. 
     }//while
}


// ******************************** SIP Timer ************************************
//timers 0=stopped 1=needs action >1 timing

void tenthsec(){
    //10 times a sec
}

void doring(){
    if(LastPhState==PS_RINGING){
        if (fringpos==0)fringpos=1;
    }
}

void sec1(){
       // every sec
        if (ReRegistrationTimer>1){
            ReRegistrationTimer--;
        }
        ringcnt++;
        if(ringcnt>2){
          ringcnt=0;
          doring();
        }
//        printf("SIPTIMER:%i\n",ReRegistrationTimer);
}

void sec10(){
    //every 10 seconds
}


bool SIP_Timer_Callback(struct repeating_timer *t){
    //10 times a sec
    tenthsec();

    SipTimerCounter++;
    if(SipTimerCounter>10){
        SipTimerCounter=0;
        sec1();        
                
        SipTimerCounter10++;
        if(SipTimerCounter10>10){
            SipTimerCounter10=0;
            sec10();
        }    

    }    
    return 1; // make repeating
    
}

void init_timer(){
    add_repeating_timer_ms(100, SIP_Timer_Callback, NULL, &siptimer);
    printf("Start SIP timer \n");
}


// ******************************** Main ************************************
int main() {
    char k;
    stdio_init_all();
    gpio_conf();
    init_sound();    
    sleep_ms(2000); //wait for serial usb
    char temp[100];
    
    CLTEXT //force white text
    splash();    
    
    printf("**************** STARTING **************** \n\n");
    
    //display Init
    printf("Display Init\n");
    SSD1306_init(0x3C,SSD1306_W128xH64);

    //Tele Cow
    SSD1306_background_image(telecow1306);
    SSD1306_sendBuffer();

    //SD card and settings
    printf("Getting settings\n");
    int sd_state;
    if (sd_state=get_settings()>0){
        if(sd_state==1){
            printf("No SD Card\n");
            drawStatus(" No SD Card",STATUSLINEP);
        }
        if(sd_state==2){
            printf("SD Failed to Parse\n");
            drawStatus("Parse Failed",STATUSLINEP);
        }
        halt();
    }
    log_file_init("### STARTLOG\n");    
    
    if(debug){
       sleep_ms(200);
       printf("**************** DEBUG %i **************** \n\n",debug);
    }
        
    //keyboard init
    init_keyboard();
    
    //RTP init
    RTPPacket_init();
        
    //Start Core 1
//    multicore_reset_core1();
    multicore_launch_core1(Core1Main);
    printf("\n################### Core 1 Started #################################\n\n");
    log_file_save("Core 1 started ","\n");
    sleep_ms(100);

    //pick which server conf to use
    server=getlastsession();    //get last server conf

    k=scan_keys();  //are keys pressed if so use them
    if(k>0 ){
       server=k-'0';
       if(server<0)server=0;
    }
    
    if (server>maxservers){ //sanity chack
        printf("Server %i over max servers %i, defaulting to 0\n\r",server,maxservers);
        server=0;
    }
    
    savelastsession(server);    
    
    sprintf(temp,"  Server %i ",server);
    drawStatus(temp,STATUSLINEP);
    sprintf(temp," %s ",server_name[server]);
    drawStatusCentered(temp,STATUSLINEP+10,8);

    sleep_ms(600);
    
    printf("\nContacting Server:%i - %s...\n",server,server_name[server]);

    drawStatus("  Conn WiFi",STATUSLINEP);
    init_network();
    
    printf("After Init Network\n");
    sleep_ms(100);

    //setrandom
    srandfrommike();
    
    sip_init();
//    printf("After Init Sip\n");
    sleep_ms(100);

    //start reregistration timer
    init_timer();

    printState(cl_state);

    //set up display
    SSD1306_background_image(telecow1306a);
    SSD1306_sendBuffer();


    //force inital register
    cl_state=SS_REGISTER_UNAUTH;
    ReRegistrationTimer=SIP_REREGISTRATIONDELAY;
    tx();

    while(1){

        //timers
        if (ReRegistrationTimer==1){
            printf("Reregister\n");
            cl_state=SS_REGISTER_UNAUTH;
            tx();
            //restart timer
            ReRegistrationTimer=SIP_REREGISTRATIONDELAY;
        }
               
        k=scan_keys();
        if(k>0 && lastkey==0){
            printf("KEY:%c\n",k);
            lastkey=k;
               
            //HANGUP   
            if (k=='C'){
                printf("\n**************************** Hangup ********************************\n");
                request_cancel();
                printf("\nTx\n");
                printState(cl_state);
                tx();
                printState(cl_state);
                printf("\n\nReady...\n");
                log_file_save("## BUTTON "," HANGUP\n");
            }      
            
            //CALL 
            if (k=='B'){
                printf("\n\n\n\n**************************** Make CALL To %s ************************ \n",autodial[server]);            
                log_file_save("## BUTTON "," Make Call To autodial number\n");
                request_dial(autodial[server], "Me");
            }   

            //ANSWER
             if (k=='A'){
                if(ddcnt>0){
                    dialdigits[ddcnt]=0;
                    request_dial(dialdigits,"What???");
                    DispConn(dialdigits);
                    drawStatus("Dialing",STATUSLINE1);
                    ddcnt=0;
                }else{
                    printf("\n\n\n\n**************************** ANSWER ************************ \n");
                    log_file_save("## BUTTON "," Answer Call \n");
                    if(cl_state == SS_RINGING){
                        answer_ringing();
                    }else{
                        printf("Not Ringing ??? \n");
                    }    
                }
            }
            
            if ( ((k>='0') && (k<='9')) || k=='#' || k=='*' ){
               dialdigits[ddcnt]=k;
               ddcnt++;
               dialdigits[ddcnt]=0;
               DispConn(dialdigits);
            }

        }
        if(k==0)lastkey=0;
      
       //SIP RX
        if(b_in!=b_out){
            if(debug){
                RXTEXT
                printf("SIP RX:\n%s\n",s_buffer[b_out]);
                CLTEXT    
            }
            rx(b_out);
            b_out++;
            if(b_out>9){b_out=0;}
            tx();
            printState(cl_state);
            printPhoneState(ph_state);
        }

        //RTP TX
        if ((SendSound==1) && (enablertp==1)){
           int plen=0;
           plen=RTPPacket_RTPPacket(RTPSndBuffer,SamplesPerPacket);

           send_rdp_udp_blocking(RTP_buff,plen,0);
           SendSound=0;
        }        
        
        display_ph_status();
        
        sleep_ms(10);
        //tight_loop_contents();
    }
        
    cyw43_arch_deinit();
    printf("Deinit\n");
    halt();
}


   