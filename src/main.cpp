#pragma clang diagnostic ignored "-Wdeprecated-volatile"

#include "WanglingEngine.h"

int main(){
	WanglingEngine wanglingengine;

	do{
		wanglingengine.draw();
	}while(!wanglingengine.shouldClose());

	return 0;
}
