/**
 * WAS Part of PhiloFax https://github.com/ExtremeElectronics/PhiloFax
 *
 * sound.c
 *
 * Code to implement sound features - Copyright (c) 2024 Derek Woodroffe <tesla@extremeelectronics.co.uk>
 * on a Pi PicoW
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include "malloc.h"
#include "stdarg.h"
#include <stdio.h>
#include <stdlib.h>

//pico stuff
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

#include "g711/g711.h"

#include "sound.h"

#include "nokiaring.c"

#include "settings.h"


//sound buffers
uint8_t MikeBuffer[SOUNDBUFFMAX];
uint16_t MikeIn=0;
uint16_t MikeOut=0;
uint8_t RTPSndBuffer[SOUNDBUFFMAX];

volatile uint8_t SpkBuffer[SOUNDBUFFMAX];
volatile uint16_t SpkIn=0;
volatile uint16_t SpkOut=0;

int16_t spkrvolume=8; //loudness of uncoming speach 
uint16_t mikevolume=4; //loudness of uncoming speach 

//sound counter
int sc=0;

int lc=0;

extern volatile uint8_t SendSound;

extern int loopback_audio;
extern int loopback_video;
//extern const char *dest_addr[MAXDESTS]; // = DESTINATION_ADDR ;
//extern int dest_port[MAXDESTS]; // = DESTINATION_PORT ;
//extern int dest_channel[MAXDESTS];// =0;
extern uint8_t dest;
//extern volatile uint8_t receiving;

//must be pins on the same slice
#define soundIO1 8
#define soundIO2 9
#define PWMrate 90
#define Mike 26 //gpio pin
#define MikeADC 0 //adc channel

#define AUDIOFILTER 6 // delay between consecutive samples of noise max 15 smaller more noise



uint PWMslice;
struct repeating_timer stimer;

extern void send_rtp_udp_blocking(char* msg,int msglen);

//void AddToSpkBuffer(uint8_t v);

uint16_t fringpos=0;
uint16_t ftextpos=0;

#define FRINGMAX 15656

uint16_t fring(uint16_t s_in){
    // output 0-65536
    if(fringpos){
        if(fringpos==FRINGMAX){
            fringpos=0;  
        }else{
            return nokia6210ringring[fringpos++] << 8; //8 bits to 16 bits.
//            return 32768+MuLaw_Decode(nokia6210ringring[fringpos++]);
        }  
    }else{
        return s_in;
    }  
}

void SetPWM(void){
    gpio_init(soundIO1);
    gpio_set_dir(soundIO1,GPIO_OUT);
    gpio_set_function(soundIO1, GPIO_FUNC_PWM);

    gpio_init(soundIO2);
    gpio_set_dir(soundIO2,GPIO_OUT);
    gpio_set_function(soundIO2, GPIO_FUNC_PWM);

    //get slice from gpio pin
    PWMslice=pwm_gpio_to_slice_num (soundIO1);
    
    pwm_set_clkdiv(PWMslice,1);//125mhz
    pwm_set_both_levels(PWMslice,512,512);

    pwm_set_output_polarity(PWMslice,true,false);

    pwm_set_wrap (PWMslice, 1024); 
//    pwm_set_wrap (PWMslice, 256);
    pwm_set_enabled(PWMslice,true);

}

void SetA2D(void){
    adc_init();
    gpio_set_dir(Mike,GPIO_IN);
    adc_gpio_init(Mike);
    adc_select_input(MikeADC);
}

void ZeroMikeBuffer(void){
   MikeIn=0;
   MikeOut=0;
}

void ZeroSpkBuffer(void){
   SpkIn=0;
   SpkOut=0;
}

void AddToSpkBuffer(uint8_t v){
    SpkBuffer[SpkIn]=v;
    SpkIn++;
    if (SpkIn==SOUNDBUFFMAX){
        SpkIn=0;
    }
}

uint8_t GetFromSpkBuffer(void){
    uint8_t c=0;
    if(SpkIn!=SpkOut){
        c=SpkBuffer[SpkOut];
        SpkOut++;
        if (SpkOut==SOUNDBUFFMAX){
            SpkOut=0;
        }
    }
    return c;
}

void AddToMikeBuffer(uint8_t v){
    MikeBuffer[MikeIn]=v;
    MikeIn++;
    if (MikeIn==SOUNDBUFFMAX){
          MikeIn=0;
    }
}


//sample microphone and output buffer to pwm every 125uS use MuLaw encoding/decoding
//20ms 8kBYTES (64Kbits) per second
bool Sound_Timer_Callback(struct repeating_timer *t){
    //a-d and PWM both set to use 0-4096 values.
    uint16_t a;
    uint8_t s; //8 bit values to/from UDP transmission

    //todo, find hardware quiecent point and make mike in signed

    //get a2d into mike buffer
    a=adc_read() * mikevolume; //get a2d value make 16 bit unsigned
//    busy_wait_us(AUDIOFILTER);
//    a=a+adc_read();
//    busy_wait_us(AUDIOFILTER);
//    a=a+adc_read();
//    busy_wait_us(AUDIOFILTER);
//    a=a+adc_read();
 
      s=MuLaw_Encode(a);    //encode as a single byte

    if (loopback_audio==0){
        AddToMikeBuffer(s);        
        s=GetFromSpkBuffer();
    }
    
    a=32768+(MuLaw_Decode(s)*spkrvolume); //14 bits +/- mid point in 16 bit uint

    a=fring(a); //if ringing substitute ring sample full 16 bits (loud) 
    
    a=a >> 6 ; //only use top 10 bits 0-1024 for pwm
    pwm_set_both_levels(PWMslice,a,a);

//after SamplesPerPacket samples send a packet
    if (MikeIn==SamplesPerPacket){
        int x;
        //copy to RDP Buffer
        for(x=0;x<SamplesPerPacket;x++){
            RTPSndBuffer[x]=MikeBuffer[x];          
        }
        SendSound=1;
        MikeIn=0;
    }

    lc++;
    if (lc>8000){
      lc=0;
//      printf("^");
    }
    return 1; // make repeating
}

void init_sound(void){
    SetPWM();
    SetA2D();
    MikeIn=1;
    MikeBuffer[0]=255;
    printf("Sound INIT\n");
    
    //REMOVE!!!!!
//    loopback_audio=1;
}

void StartSoundTimer(void){
   add_repeating_timer_us(SampleTime, Sound_Timer_Callback, NULL, &stimer);
   printf("Start Sound timer \n");
}
