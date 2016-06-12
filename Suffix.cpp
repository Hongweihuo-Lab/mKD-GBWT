#include <stdlib.h>
#include "Suffix.h"

Suffix::Suffix(){
	firstChara = 0;
	exist =false;
	left = -1;
	right = -2;

	diskNum = new int32_t[256];

	for(int i = 0; i < 256; i++)
		diskNum[i] = -1;
}

Suffix::~Suffix(){

	delete []diskNum;
}