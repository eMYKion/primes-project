#include <iostream>
#include <fstream>
#include <cmath>

#define POINTS 3
#define SMALL 5000.0
#define LOG_FILE "calibration.log"

static double powerList[POINTS];
static double tempList[POINTS];	
std::ofstream outfile;

double findTemp(double power);

void logRegression(void);

int main(int argc, char **argv){

	//uses 5 points for regression
	for( int i = 0; i < POINTS; i++){

		powerList[i] = 10 * i;
		tempList[i] = findTemp(powerList[i]);//takes some time
	}

	outfile.open(LOG_FILE);
	outfile << "finished finding temperatures!" << std::endl;
	outfile << "power , temperature" << std::endl;

	for( int i = 0; i < POINTS; i++){

		outfile << powerList[i] << " , " << tempList[i] << std::endl;

	}
	outfile.close();
	std::cout << -2 << std::endl;

	//regression math to a file
	logRegression();

	
	return 0;
}

double findTemp(double power){
	//sets power
	std::cout << power << std::endl;

	std::cout << -1 << std::endl;
	double currTemp;
	std::cin >> currTemp;

	std::cout << -1 << std::endl;
	double prevTemp;
	std::cin >> prevTemp;

	double difference = currTemp - prevTemp;
	outfile.open(LOG_FILE, std::ios_base::app);
	while(false){//while difference is still too large
		
		std::cout << -1 << std::endl;//read
		
		std::cin >> currTemp;
		

		outfile << "current temperature " << currTemp << std::endl;

		//exponential filter necessary?

		difference = currTemp - prevTemp;

		prevTemp = currTemp; 
	}
	outfile.close();
	return currTemp;
}

void logRegression(void){

	//means
	double meanTemp;
	double meanPower;

	for( int i = 0; i < POINTS; i++){
		meanTemp += tempList[i];
		meanPower += powerList[i];
	}

	meanTemp /= POINTS;
	meanPower /= POINTS;

	
	//standard deviations	
	double stdDevTemp;
	double stdDevPower;
	for( int i = 0; i < POINTS; i++){
		stdDevTemp += pow(tempList[i] - meanTemp, 2);
		stdDevPower += pow(powerList[i] - meanPower, 2);
	}

	stdDevTemp /= POINTS - 1;
	stdDevPower /= POINTS - 1;

	stdDevTemp = pow(stdDevTemp, 0.5);
	stdDevPower = pow(stdDevPower, 0.5);

	//correlation
	double correlation;
	for( int i = 0; i < POINTS; i++){
		correlation += (tempList[i] - meanTemp)/stdDevTemp * (powerList[i] - meanPower)/stdDevPower;
	}		
	correlation /= POINTS - 1;

	//temp = a+b(power)
	
	double b = correlation*stdDevTemp/stdDevPower;
	double a = meanTemp - meanPower*b;
	outfile.open(LOG_FILE, std::ios_base::app);
	outfile << "calibration: t = a + bp\nestimated temperature = " << a << " + " << b << " * power" << std::endl;
	outfile.close();	
}
