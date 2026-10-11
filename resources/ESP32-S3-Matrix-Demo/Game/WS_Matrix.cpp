#include "WS_Matrix.h"
// English: Please note that the brightness of the lamp bead should not be too high, which can easily cause the temperature of the board to rise rapidly, thus damaging the board !!!
// Chinese: 请注意，灯珠亮度不要太高，容易导致板子温度急速上升，从而损坏板子!!! 
uint8_t RGB_Data[3] = {30,30,30}; 
uint8_t Matrix_Data[8][8];  
Adafruit_NeoPixel pixels(RGB_COUNT, RGB_Control_PIN, NEO_RGB + NEO_KHZ800); 

void RGB_Matrix() {
  for (int row = 0; row < Matrix_Row; row++) {
    for (int col = 0; col < Matrix_Col; col++) {
    // int hue = ((i * 256 / RGB_COUNT) % 256)*2;
    // pixels.setPixelColor(i, pixels.ColorHSV(hue, 255, 10)); 
      if(Matrix_Data[row][col] == 1)      
      {
        pixels.setPixelColor(row*8+col, pixels.Color(RGB_Data[0], RGB_Data[1], RGB_Data[2]));   
      }
      else
      {
        pixels.setPixelColor(row*8+col, pixels.Color(0, 0, 0)); 
      }
    }
  }
  pixels.show();
}


uint8_t x=4,y=4;
void Game(uint8_t X_EN,uint8_t Y_EN) 
{
  Matrix_Data[x][y] = 0;
  if(X_EN && Y_EN){
    if(X_EN == 1)
      x=x+1;
    else
      x=x-1;
    if(Y_EN == 1)
      y=y+1;
    else
      y=y-1;
  }
  else if(X_EN){
    if(X_EN == 1)
      x=x+1;
    else
      x=x-1;
  }
  else if(Y_EN){
    if(Y_EN == 1)
      y=y+1;
    else
      y=y-1;
  }
  if(x < 0) x = 0;
  if(x == 8) x = 7;
  if(x > 8) x = 0;
  if(y < 0) y = 0;
  if(y == 8) y = 7;
  if(y > 8) y = 0;
  printf("%d\r\n",y);
  Matrix_Data[x][y]=1;
  RGB_Matrix();
}
void Matrix_Init() {
  pixels.begin();
  // English: Please note that the brightness of the lamp bead should not be too high, which can easily cause the temperature of the board to rise rapidly, thus damaging the board !!!
  // Chinese: 请注意，灯珠亮度不要太高，容易导致板子温度急速上升，从而损坏板子!!! 
  pixels.setBrightness(60);                       // set brightness  
  memset(Matrix_Data, 0, sizeof(Matrix_Data)); 
}