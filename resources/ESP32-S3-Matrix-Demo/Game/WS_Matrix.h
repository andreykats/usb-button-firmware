#ifndef _WS_Matrix_H_
#define _WS_Matrix_H_
#include <Adafruit_NeoPixel.h>

#define RGB_Control_PIN   14       
#define Matrix_Row        8     
#define Matrix_Col        8       
#define RGB_COUNT         64     

void RGB_Matrix();                         

void Game(uint8_t X_EN,uint8_t Y_EN);
void Matrix_Init();                                     
#endif
