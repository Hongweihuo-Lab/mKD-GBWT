#include <cstdio>
#include <string>

#include "test_sbt.h"
#include "test_gbwt.h"
#include "test_kdtree.h"
#include "test_diskgbwt.h"
#include "sbt_util.h"

using namespace std;

int main(int argc, char* argv[]){

	//argc == 4 || 5
	if(argc != 4){
		fprintf(stderr, "Error!! argc = %d\n", argc);
		exit(EXIT_FAILURE);
	}

	uint64_t B, step;
	B = stoull(argv[2]);
	step = stoull(argv[3]);
	char* resFile = getTestFile(argv[1]);
	 test_diskgbwt::test_01(argv[1], B, step);


	// test_diskgbwt::test_all(argv[1], B, step, resFile);
	// test_diskgbwt::test_final(argv[1], B, step, resFile);

	delete[] resFile;
}