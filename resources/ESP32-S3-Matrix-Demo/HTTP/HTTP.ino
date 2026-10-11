#include <Adafruit_NeoPixel.h>
#include "WS_Flow.h"
#include "WS_WIFI.h"

// English: Please note that the brightness of the lamp bead should not be too high, which can easily cause the temperature of the board to rise rapidly, thus damaging the board !!!
// Chinese: 请注意，灯珠亮度不要太高，容易导致板子温度急速上升，从而损坏板子!!! 

char Text[100] ="Waveshare ESP32-S3-Matrix Text Testing!";
uint8_t Flow_Flag = 0;

void setup()
{
// WIFI
  WIFI_Init();

// RGB
  Matrix_Init();
}

uint32_t Flag =0;
void loop(){
  WIFI_Loop();
  if(Flow_Flag == 1)
    Flag++;
  if(Flag == 100)
  {
    Text_Flow(Text);
    Flag = 0;
  }
}
