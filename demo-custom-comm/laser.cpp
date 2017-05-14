#include <iostream>
#include <thread>
#include <unistd.h>

double power;

double currTemp;

//calculates currTemp using derivative definition
double calcCurrTemp(void);

int main(int argx, char**argv)
{

	std::thread calcThread(calcCurrTemp);

	double pow;

	while(true){//sets power whenever entered

		std::cin >> pow;//read in power, if -1 then is read command , if -2 then is kill command

		if(pow >= 0){
			power = pow;
		}else if(pow == -1){//read currTemp
			std::cout << currTemp << std::endl;
		}else{//end processes
			break;
		}
	}
	
	//terminates thread
	calcThread.~thread();

	return 0;		

}

double calcCurrTemp(void){
	
	while(true){

		double k = 0.0005;
		double Tenv = 22.3 + power*10.4;

		currTemp += -k*(currTemp - Tenv);//add random noise to simulate reality?

		usleep(1);
	}
}
