#include <string.h>
#include "malloc.h"
#include <stdio.h>
#include <stdlib.h>

//pico stuff
#include "pico/multicore.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "settings.h"

#define KEYXMAX 3

#define KEYYMAX 5

//gpio pins for keyboard x & y
uint8_t kbdpinsX[3]={14,15,16};
uint8_t kbdpinsY[5]={17,18,19,20,21};

//convert scan codes to ascii
uint8_t sc2ascii[16]={0,'1','2','3','4','5','6','7','8','9','*','0','#','A','B','C'};

void kbd_pin_init(uint8_t pin,uint8_t direction){
  gpio_init(pin);
  if(direction==GPIO_IN)gpio_pull_down(pin);
  gpio_set_dir(pin,direction);
}


void init_keyboard(){
    int x,y;
    for(x=0;x<KEYXMAX;x++)kbd_pin_init(kbdpinsX[x],GPIO_OUT);
    for(y=0;y<KEYYMAX;y++)kbd_pin_init(kbdpinsY[y],GPIO_IN);
}

void setXpin(uint8_t xp){
    int x;
    for(x=0;x<KEYXMAX;x++){
        if(x==xp){
            gpio_put(kbdpinsX[x],1);
        }else{
            gpio_put(kbdpinsX[x],0);
        }
    }
}

uint8_t scan_keys(){
    int x,y,sc=0,r=0;
    for(x=0;x<KEYXMAX;x++){
        setXpin(x);
        for(y=0;y<KEYYMAX;y++){
           sc=x+1+y*3;
           sleep_ms(1);
           if(gpio_get(kbdpinsY[y])==1){
               r=sc;
//               printf("%i,%i,%i\n",sc,x,y);
           }
        }
    } 
    return sc2ascii[r];
}
