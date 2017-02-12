#include <iostream>
#include <chrono>
#include <cmath>

#define BROADCAST_TIMOUT_MILLIS 500
#define CALC_TIMOUT_MILLIS 10

//celsius
#define ENV_TEMP_MAX 100
#define ENV_TEMP_MIN 20


#define K_CONSTANT log(80)/5.0

double temp = 20;

using namespace std;

double getLaserTemp(void);

unsigned long startTime;

unsigned long prevCalcTime;
unsigned long prevBroadcastTime;


unsigned long getTimeMillis(void);

int main(int argc, char** argv){
  
  startTime = getTimeMillis();
  
  prevCalcTime = startTime;
  prevBroadcastTime = startTime;
  
  while(true){
    
    if(getTimeMillis() - prevCalcTime > CALC_TIMOUT_MILLIS){
      temp = getLaserTemp();
      prevCalcTime = getTimeMillis();
    }
    
    if(getTimeMillis() - prevBroadcastTime > BROADCAST_TIMOUT_MILLIS){
      cout << temp << endl;
      prevBroadcastTime = getTimeMillis();
    }
  }
  
  return 0;
}

double getLaserTemp(void){
  
  double dt = (double)(getTimeMillis() - prevCalcTime)/1000.0;
  
  double envTemp;
  double dT;
  
  if(((getTimeMillis() - startTime) % 10000) < 5000){// simulate cooling
    envTemp = ENV_TEMP_MAX;
  }else{// simulate heating
    envTemp = ENV_TEMP_MIN;
  }
  
  dT = -K_CONSTANT*(temp - envTemp)*(double)dt;
  
  
  temp += dT;
  
  return temp;
}

unsigned long getTimeMillis(void){
  return std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
}