#include <cstdio>
#include <string>

#include "test_sbt.h"
#include "test_gbwt.h"
#include "test_kdtree.h"


using namespace std;

int main(int argc, char* argv[]){

	// test_sbt::test_06();
	// test_sbt::test_02();

/***********************************************************/	
	if(argc != 4){
		fprintf(stderr, "Error!! argc = %d\n", argc);
		exit(EXIT_FAILURE);
	}

	uint64_t B, step;
	B = stoull(argv[2]);
	step = stoull(argv[3]);
	test_gbwt::test_01(argv[1], B, step);

/****************test for KDTree********************************/
	// test_kdtree::test_SaveToDisk();
	// test_kdtree::test_ReadFromDisk();

}