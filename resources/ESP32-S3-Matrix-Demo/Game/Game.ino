#include "WS_QMI8658.h"
#include "WS_Matrix.h"

// English: Please note that the brightness of the lamp bead should not be too high, which can easily cause the temperature of the board to rise rapidly, thus damaging the board !!!
// Chinese: 请注意，灯珠亮度不要太高，容易导致板子温度急速上升，从而损坏板子!!! 
extern IMUdata Accel; 
IMUdata game;
void setup()
{
  QMI8658_Init();
  Matrix_Init();
}


uint8_t X_EN = 0, Y_EN = 0, Time_X_A = 0, Time_X_B = 0, Time_Y_A = 0, Time_Y_B = 0;
void loop()
{
  QMI8658_Loop();
  if(Accel.x > 0.15 || Accel.x < 0  || Accel.y > 0.15 || Accel.y < 0  || Accel.z > -0.9 || Accel.z < -1.1  ){
    if(Accel.x > 0.15){
      Time_X_A = Time_X_A + Accel.x * 10;
      Time_X_B = 0;
    }
    else if(Accel.x < 0){
      Time_X_B = Time_X_B + std::abs(Accel.x) * 10;
      Time_X_A = 0;
    }
    else{
      Time_X_A = 0;
      Time_X_B = 0;
    }
    if(Accel.y > 0.15){
      Time_Y_A = Time_Y_A + Accel.y * 10;
      Time_Y_B = 0;
    }
    else if(Accel.y < 0){
      Time_Y_B = Time_Y_B + std::abs(Accel.y) * 10;
      Time_Y_A = 0;
    }
    else{
      Time_Y_A = 0;
      Time_Y_B = 0;
    }
    if(Time_X_A >= 10){
      X_EN = 1;
      Time_X_A = 0;
      Time_X_B = 0;
    }
    if(Time_X_B >= 10){
      X_EN = 2;
      Time_X_A = 0;
      Time_X_B = 0;
    }
    if(Time_Y_A >= 10){
      Y_EN = 2;
      Time_Y_A = 0;
      Time_Y_B = 0;
    }
    if(Time_Y_B >= 10){
      Y_EN = 1;
      Time_Y_A = 0;
      Time_Y_B = 0;
    }
    Game(X_EN,Y_EN);
    X_EN = 0;
    Y_EN = 0;
  }
  delay(10);
}
