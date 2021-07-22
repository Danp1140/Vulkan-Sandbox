#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-volatile"

#include "GraphicsHandler.h"

int main(){
	GraphicsHandler primaryviewport=GraphicsHandler();

	do{
		primaryviewport.draw();
	}while(!primaryviewport.shouldClose());

	return 0;
}
