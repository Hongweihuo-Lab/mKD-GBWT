#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "test_gbwt.h"
#include "gbwt.h"

#define FILE_NUM 4

void test_gbwt::test_01(const char* text_file, uint64_t B, uint64_t step){

	// const char* text_file = "./data/bible";
	// const char* text_file = "./data/sources.100MB";
	// const char* text_file = "./data/world_leaders";

	// uint64_t B = 4*1024;
	// uint64_t step = 4;

	GBWT *gbwt = new GBWT(text_file, B, step);
	delete gbwt;
}
