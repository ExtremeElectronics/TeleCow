/**
 * Part of PhiloFax https://github.com/ExtremeElectronics/PhiloFax
 *
 * settings.c
 *
 * Code to import settings -   on a Pi PicoW
 * Copyright (c) 2024 Derek Woodroffe <tesla@extremeelectronics.co.uk>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

//defaults from config.h
#include "settings.h"

//iniparcer
#include "dictionary.h"
#include "iniparser.h"


//sd card reader
#include "f_util.h"
#include "ff.h"

//spi config for FFat
#include "hw_config.h"

FRESULT fr;

int loopback_audio=0;
int loopback_video=0;

int SDStatus=0; // 0 no sd, 1 sd detected, 2 read ini
int sip_local_port = LOCAL_SIP_PORT ;
int rtp_local_port = LOCAL_RTP_PORT ;
int maxservers =0;
int maxwifi =0;

const char * wifi_ssid[MAXWIFI];
const char * wifi_pass[MAXWIFI];
const char * device_name;

const char *server_addr[MAXSERVERS]; // = DESTINATION_ADDR ;
int server_port[MAXSERVERS]; // = DESTINATION_PORT ; 
const char *server_name[MAXSERVERS];
const char *sip_username[MAXSERVERS];
const char *sip_password[MAXSERVERS];
const char *sip_login[MAXSERVERS];
const char *autodial[MAXSERVERS];
//const char *sip_realm[MAXSERVERS];

int SDFileExists(char * filename){
    FRESULT fr;
    FILINFO fno;
    fr = f_stat(filename, &fno);
    return fr==FR_OK;
}


int getlastsession(){
   if (SDFileExists("last.session")){
       FIL fix;
       FRESULT fr1;
       fr1 = f_open(&fix, "last.session",FA_READ);
       char lasts[100];
       f_gets(lasts, sizeof lasts, &fix);
       printf("last.session %s\n",lasts);
       int x=atoi(lasts);
       f_close(&fix);
       return x;
   }else{
       return 0;
   }

}

int savelastsession(int x){
    FIL fix;
    FRESULT fr1;
    fr1 = f_open(&fix, "last.session", FA_WRITE |  FA_CREATE_ALWAYS );
    uint written;
    char lasts[100];
    sprintf(lasts,"%i\n",x);
    f_write(&fix,lasts,strlen(lasts),&written);
    f_close(&fix);
}

int log_file_init(char * from){
     FIL fix;
     FRESULT fr1;
     fr1 = f_open(&fix, "sip.log", FA_WRITE |  FA_CREATE_ALWAYS );
     uint written;
     f_write(&fix,from,strlen(from),&written);
     f_close(&fix);
     return written;
}

int log_file_save(char * from,char * buffer){
    FIL fix; 
    FRESULT fr1;
    fr1 = f_open(&fix, "sip.log", FA_WRITE | FA_OPEN_APPEND );
    uint written;
    f_write(&fix,from,strlen(from),&written);
    f_write(&fix,buffer,strlen(buffer),&written);
    f_close(&fix);
    return written;
}

int SD_CardMount(FRESULT fr){
   // mount SD Card
        sd_card_t *pSD = sd_get_by_num(0);
        fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
        if (FR_OK != fr){
            // panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
            printf("SD INIT FAIL  \n\r");
            SDStatus=1;
            return 0;
        }

        printf("SD INIT OK \n\r");
        SDStatus=0;
        return 1;
}


void ini_file_parse(FRESULT fr){
        dictionary * ini ;
        char       * ini_name ;
        const char  *   s ;
        const char * inidesc;

        int overclock;
        int jpc;
        int iscf=0;
        int d;

        ini_name = "TeleCow.ini";
        char dname[100]; 

        if (SDFileExists(ini_name)){
          printf("Ini file %s Exists Loading ... \n\r",ini_name);

//########################################### INI Parser ######################################
          ini = iniparser_load(fr, ini_name);
          //iniparser_dump(ini, stdout);

//wifi Defs
          maxwifi = iniparser_getint(ini, "SELECTION:maxwifi",0 );

          if (maxwifi>MAXWIFI){maxwifi=MAXWIFI;}
          printf("Max WiFi %i \n",maxwifi);

          for (d=0;d<maxwifi;d++){
            
              sprintf(dname,"WIFI%i:pass",d);
              wifi_pass[d] = iniparser_getstring(ini, dname,WIFI_SSID);

              sprintf(dname,"WIFI%i:ssid",d);
              wifi_ssid[d] = iniparser_getstring(ini, dname,WIFI_PASSWORD);
 
          }
            
          sip_local_port = iniparser_getint(ini, "LOCAL:sip_port",LOCAL_SIP_PORT );
          rtp_local_port = iniparser_getint(ini, "LOCAL:rtp_port",LOCAL_RTP_PORT );

          device_name =  iniparser_getstring(ini, "LOCAL:device_name","PiTeleCow");

// SERVER Defs
          maxservers = iniparser_getint(ini, "SELECTION:maxservers",0 );
          if (maxservers>MAXSERVERS-1){maxservers=MAXSERVERS-1;}
          printf("Max Servers %i \n",maxservers);

          for(d=0;d<maxservers;d++){

              sprintf(dname,"SERVER%i:name",d);
              server_name[d] = iniparser_getstring(ini, dname,"");

              sprintf(dname,"SERVER%i:address",d);
              server_addr[d] = iniparser_getstring(ini, dname,SERVER_ADDR);
             
              sprintf(dname,"SERVER%i:port",d);
              server_port[d] = iniparser_getint(ini, dname,SERVER_PORT );

              sprintf(dname,"SERVER%i:sip_username",d);
              sip_username[d] = iniparser_getstring(ini, dname,SIP_USERNAME );

              sprintf(dname,"SERVER%i:sip_password",d);
              sip_password[d] = iniparser_getstring(ini, dname,SIP_PASSWORD );

              sprintf(dname,"SERVER%i:sip_login",d);
              sip_login[d] = iniparser_getstring(ini, dname,SIP_LOGIN );

              sprintf(dname,"SERVER%i:autodial",d);
              autodial[d] = iniparser_getstring(ini, dname,"0000" );

          }
          
   
         printf("Loaded INI\n\r");
         SDStatus=0;
    }else{
      printf("inifile does not exist, using compiled defaults");
      SDStatus=2;
    }  
}


int get_settings(void){
    if (SD_CardMount(fr)){
       ini_file_parse(fr);
    }
    return SDStatus;

}

int splash(void){
     for(uint8_t a=0;a<20;a++)printf("\n");

     printf("       Pi Tele Cow    \n");
     printf("       ___________    \n");
     printf("      | _________ |   \n");
     printf("      ||         ||   \n");
     printf("      ||         ||   \n");
     printf("      ||         ||   \n");
     printf("      ||_________||   \n");
     printf("      |           |   \n");
     printf("      |  1  2  3  |   \n");
     printf("      |  4  5  6  |   \n");
     printf("      |  7  8  9  |   \n"); 
     printf("      |  *  0  #  |   \n");     
     printf("      |           |   \n");     
     printf("      |  TELECOW  |   \n");          
     printf("      |___________|   \n");
     printf("                      \n");
     printf("      Extreme  Kits   \n");
     printf("         	           \n");
     printf("                      \n");

}
