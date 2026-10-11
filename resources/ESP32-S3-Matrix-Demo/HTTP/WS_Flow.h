#ifndef _WS_Flow_H_
#define _WS_Flow_H_

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#define RGB_Control_PIN   14    

extern Adafruit_NeoMatrix Matrix;

void colorWipe(uint32_t c, uint8_t wait);
int getCharWidth(char c);
int getStringWidth(const char* str);
void Text_Flow(char* Text);
void Matrix_Init();       
#endif
