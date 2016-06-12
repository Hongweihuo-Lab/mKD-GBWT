#include <cstdio>
#include <string>

#include "test_sbt.h"
#include "test_gbwt.h"
#include "test_kdtree.h"
#include "test_diskgbwt.h"

using namespace std;

int main(int argc, char* argv[]){

	if(argc != 6){
		fprintf(stderr, "Error!! argc = %d\n", argc);
		exit(EXIT_FAILURE);
	}

	uint64_t B, step;
	B = stoull(argv[2]);
	step = stoull(argv[3]);

	// test_diskgbwt::test_01(argv[1], B, step);
	test_diskgbwt::test_average(argv[1], B, step, argv[4], argv[5]);

}