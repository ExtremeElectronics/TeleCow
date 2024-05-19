/* inspired by https://github.com/Harbys/pico-ssd1306 */

#include <string.h>
#include "malloc.h"
#include <stdio.h>
#include <stdlib.h>

//pico stuff
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "hardware/i2c.h"


#include "settings.h"
#include "display.h"
#include "font8x8.h"
#include "telecow.h"

#define FRAMEBUFFER_SIZE 1024

//I2C
#define I2CInst i2c1
#define I2C_SDA_PIN 6
#define I2C_SCL_PIN 7
//#define I2C_BAUDRATE 400000 //400Khz
#define I2C_BAUDRATE 100000 //100Khz

uint16_t SSD1306_Address;
uint8_t SSD1306_height;
uint8_t SSD1306_width;
uint8_t SSD1306_inverted;
enum SSD1306_Size display_Size; 

char SSD1306_framebuffer[FRAMEBUFFER_SIZE];


void FrameBuffer_byteOVERWRITE(int n, unsigned char byte) {
    // return if index outside 0 - buffer length - 1
    if (n > (FRAMEBUFFER_SIZE-1)) return;
    SSD1306_framebuffer[n] = byte;
}

void FrameBuffer_byteOR(int n, unsigned char byte) {
    // return if index outside 0 - buffer length - 1
    if (n > (FRAMEBUFFER_SIZE-1)) return;
    SSD1306_framebuffer[n] |= byte;
}

void FrameBuffer_byteAND(int n, unsigned char byte) {
    // return if index outside 0 - buffer length - 1
    if (n > (FRAMEBUFFER_SIZE-1)) return;
    SSD1306_framebuffer[n] &= byte;
}

void FrameBuffer_byteXOR(int n, unsigned char byte) {
    // return if index outside 0 - buffer length - 1
    if (n > (FRAMEBUFFER_SIZE-1)) return;
    SSD1306_framebuffer[n] ^= byte;
}


void FrameBuffer_clear() {
    //zeroes out the buffer via memset function from string library
    memset(SSD1306_framebuffer, 0, FRAMEBUFFER_SIZE);
}

void SSD1306_I2C_init(){
 //SDA
    gpio_init(I2C_SDA_PIN);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    //SCL
    gpio_init(I2C_SCL_PIN);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SCL_PIN);

    i2c_init(I2CInst, I2C_BAUDRATE);
    sleep_ms(250);
}


void SSD1306_init( uint16_t Address, enum SSD1306_Size size) {
    // Set class instanced variables
    SSD1306_Address=Address;
    SSD1306_I2C_init();    

    printf("I2C Init %X\n",Address);

    SSD1306_width = 128;
    display_Size=size;

    if (size == SSD1306_W128xH64) {
        SSD1306_height = 64;
    } else {
        SSD1306_height = 32;
    }

    // display is not inverted by default
    SSD1306_inverted = false;

    // this is a list of setup commands for the display
    uint8_t setup[] = {
        SSD1306_DISPLAY_OFF,
        SSD1306_LOWCOLUMN,
        SSD1306_HIGHCOLUMN,
        SSD1306_STARTLINE,

        SSD1306_MEMORYMODE,
        SSD1306_MEMORYMODE_HORZONTAL,

        SSD1306_CONTRAST,0xFF,

        SSD1306_INVERTED_OFF,

        SSD1306_MULTIPLEX,63,

        SSD1306_DISPLAYOFFSET,0x00,

        SSD1306_DISPLAYCLOCKDIV,0x80,

        SSD1306_PRECHARGE,0x22,

        SSD1306_COMPINS,0x12,

        SSD1306_VCOMDETECT,0x40,

        SSD1306_CHARGEPUMP,0x14,

        SSD1306_DISPLAYALL_ON_RESUME,
        SSD1306_DISPLAY_ON
    };

    // send each one of the setup commands
    uint8_t x=0;
    for (x=0;x<sizeof(setup);x++) {
        SSD1306_cmd(setup[x]);
    }
    
    printf("I2C, sent %i setup\n",x);

    // clear the buffer and send it to the display
    // if not done display shows garbage data
    SSD1306_clear();
    SSD1306_sendBuffer();

}

void SSD1306_setPixel(int16_t x, int16_t y, enum SSD1306_WriteMode mode) {
    // return if position out of bounds
    if ((x < 0) || (x >= SSD1306_width) || (y < 0) || (y >= SSD1306_height)) return;

    // byte to be used for buffer operation
    uint8_t byte;

    // display with 32 px height requires doubling of set bits, reason to this is explained in readme
    // this shifts 1 to byte based on y coordinate
    // remember that buffer is a one dimension array, so we have to calculate offset from coordinates
    if (display_Size == SSD1306_W128xH32) {
        y = (y << 1) + 1;
        byte = 1 << (y & 7);
        char byte_offset = byte >> 1;
        byte = byte | byte_offset;
    } else {
        byte = 1 << (y & 7);
    }

    // check the write mode and manipulate the frame buffer
    if (mode == SSD1306_ADD) {
        FrameBuffer_byteOR(x + (y / 8) * SSD1306_width, byte);
    } else if (mode == SSD1306_SUBTRACT) {
        FrameBuffer_byteAND(x + (y / 8) * SSD1306_width, ~byte);
    } else if (mode == SSD1306_INVERT) {
        FrameBuffer_byteXOR(x + (y / 8) * SSD1306_width, byte);
    } else if (mode == SSD1306_OVERWRITE) {
        FrameBuffer_byteOVERWRITE(x + (y / 8) * SSD1306_width, byte);
    }
}

void SSD1306_sendBuffer() {
    SSD1306_cmd(SSD1306_PAGEADDR); //Set page address from min to max
    SSD1306_cmd(0x00);
    SSD1306_cmd(0x07);
    SSD1306_cmd(SSD1306_COLUMNADDR); //Set column address from min to max
    SSD1306_cmd(0x00);
    SSD1306_cmd(127);

    // create a temporary buffer of size of buffer plus 1 byte for startline command aka 0x40
    unsigned char data[FRAMEBUFFER_SIZE + 1];

    data[0] = SSD1306_STARTLINE;

    // copy framebuffer to temporary buffer
    memcpy(data + 1, SSD1306_framebuffer, FRAMEBUFFER_SIZE);

    // send data to device
    i2c_write_blocking(I2CInst, SSD1306_Address, data, FRAMEBUFFER_SIZE + 1, false);
}

void SSD1306_clear() {
    FrameBuffer_clear();
}

void SSD1306_setOrientation(bool orientation) {
    // remap columns and rows scan direction, effectively flipping the image on display
    if (orientation) {
        SSD1306_cmd(SSD1306_CLUMN_REMAP_OFF);
        SSD1306_cmd(SSD1306_COM_REMAP_OFF);
    } else {
        SSD1306_cmd(SSD1306_CLUMN_REMAP_ON);
        SSD1306_cmd(SSD1306_COM_REMAP_ON);
    }
}

void SSD1306_addBitmapImage(int16_t anchorX, int16_t anchorY, uint8_t image_width, uint8_t image_height,
                            uint8_t *image,enum SSD1306_WriteMode mode) {
    uint8_t byte;
    // goes over every single bit in image and sets pixel data on its coordinates
    for (uint8_t y = 0; y < image_height; y++) {
        for (uint8_t x = 0; x < image_width / 8; x++) {
            byte = image[y * (image_width / 8) + x];
            for (uint8_t z = 0; z < 8; z++) {
                if ((byte >> (7 - z)) & 1) {
                    SSD1306_setPixel(x * 8 + z + anchorX, y + anchorY, mode);
                }
            }
        }
    }

}

void SSD1306_invertDisplay() {
    SSD1306_cmd(SSD1306_INVERTED_OFF | !SSD1306_inverted);
    SSD1306_inverted = !SSD1306_inverted;
}

void SSD1306_cmd(unsigned char command) {
    // 0x00 is a byte indicating to ssd1306 that a command is being sent
    uint8_t data[2] = {0x00, command};
    i2c_write_blocking(I2CInst,SSD1306_Address, data, 2, false);
}

void SSD1306_setContrast(unsigned char contrast) {
    SSD1306_cmd(SSD1306_CONTRAST);
    SSD1306_cmd(contrast);
}

//void SSD1306_setBuffer(unsigned char * buffer) {
//    SSD1306_frameBuffer.setBuffer(buffer);
//}

void SSD1306_turnOff() {
    SSD1306_cmd(SSD1306_DISPLAY_OFF);
}

void SSD1306_turnOn() {
    SSD1306_cmd(SSD1306_DISPLAY_ON);
}

void drawStatus( const char *text,uint8_t anchor_y){
    SSD1306_fillRect( 10, anchor_y, SSD1306_width-10, anchor_y+8,SSD1306_SUBTRACT);
    SSD1306_drawText(font_8x8,text, 10,anchor_y,SSD1306_ADD,SSD1306_DEG0);
    SSD1306_sendBuffer();
}

void drawText( const char *text, uint8_t anchor_x,uint8_t anchor_y){
    SSD1306_drawText(font_8x8,text, anchor_x,anchor_y,SSD1306_ADD,SSD1306_DEG0);             
    SSD1306_sendBuffer();
}

void SSD1306_fillRect( uint8_t x_start, uint8_t y_start, uint8_t x_end, uint8_t y_end,enum SSD1306_WriteMode mode) {
    for (uint8_t x = x_start; x <= x_end; x++) {
        for (uint8_t y = y_start; y <= y_end; y++) {
            SSD1306_setPixel(x, y, mode);
        }
    }
}

void SSD1306_drawText(const unsigned char *font, const char *text, uint8_t anchor_x,
             uint8_t anchor_y, enum SSD1306_WriteMode mode, enum SSD1306_Rotation rotation) {
    if( !font || !text) return;

    uint8_t font_width = font[0];

    uint16_t n = 0;
    while (text[n] != '\0') {
        switch (rotation) {
            case SSD1306_DEG0:
                SSD1306_drawChar(font, text[n], anchor_x + (n * font_width), anchor_y, mode, rotation);
                break;
            case SSD1306_DEG90:
                SSD1306_drawChar(font, text[n], anchor_x, anchor_y + (n * font_width), mode, rotation);
                break;
        }
        n++;
    }
}

void SSD1306_drawChar(const unsigned char *font, char c, uint8_t anchor_x, uint8_t anchor_y,
                  enum SSD1306_WriteMode mode, enum SSD1306_Rotation rotation) {
        if( !font || c < 32) return;

        uint8_t font_width = font[0];
        uint8_t font_height = font[1];

        uint16_t seek = (c - 32) * (font_width * font_height) / 8 + 2;

        uint8_t b_seek = 0;

        for (uint8_t x = 0; x < font_width; x++) {
            for (uint8_t y = 0; y < font_height; y++) {
                if (font[seek] >> b_seek & 0b00000001) {
                    switch (rotation) {
                        case SSD1306_DEG0:
                            SSD1306_setPixel(x + anchor_x, y + anchor_y, mode);
                            break;
                        case SSD1306_DEG90:
                            SSD1306_setPixel(-y + anchor_x + font_height, x + anchor_y, mode);
                            break;
                    }
                }
                b_seek++;
                if (b_seek == 8) {
                    b_seek = 0;
                    seek++;
                }
            }
        }
}

void SSD1306_background_image(unsigned char *bitmap){
    int x;
    for(x=0;x<FRAMEBUFFER_SIZE;x++){
        SSD1306_framebuffer[x]=bitmap[x];
    }
}

/*
void SSD1306_background_image(unsigned char *bitmap){ 
//this is all sorts of wrong because I reused code from anothe project
//and its all arse about face...
   uint8_t x;
   uint8_t y;
   uint8_t d;
   uint8_t v;
   uint8_t b;
   printf("Width:%i Height:%i\n",SSD1306_width,SSD1306_height);
   for(y=0;y<SSD1306_width;y++){
     x=0;
     for(d=0;d<SSD1306_height/8;d++){
        v=bitmap[d+y*SSD1306_height/8];
        for(b=0;b<8;b++){
            if(!(v & 1<<b)){
              //buffer[x]=fg;
              SSD1306_setPixel(y, SSD1306_height-x, SSD1306_ADD);
            }else{
//              buffer[x]=bg;
            }
            x++;
        }
     }
  }
}
*/

