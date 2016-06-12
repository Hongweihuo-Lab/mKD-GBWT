#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "uint40.h"
#include "utils.h"

#define SCANBUFSIZE 100000

using namespace std;

int main(int argc, char* argv[]){

	if(argc != 2){
		fprintf(stderr, "usage: gen_sa_lcp <textFile>\n");
		exit(EXIT_FAILURE);
	}

	//file of text
	string textFile = string(argv[1]);
	string saFile = textFile + ".sa5";
	string lcpFile = textFile + ".lcp5";
	string sa64File = textFile + ".sa64";
	string lcp64File = textFile + ".lcp64";

	/*read sa data in uint40 format*/
	uint40* sa;
	long nsa;
	utils::read_objects_from_file(sa, nsa,saFile);

	/*open sa64 file, and write data */
	FILE* fsa64 = fopen(sa64File.c_str(), "w");
	if(!fsa64){
		fprintf(stderr, "gen_sa_lcp::Open file %s error!\n", sa64File.c_str());
		exit(EXIT_FAILURE);
	}
	uint64_t* buffer = new uint64_t[SCANBUFSIZE];
	uint64_t i, j, k;
	
	for(i = 0, j = 0; i < nsa; i++){
		buffer[j++] = sa[i].ull();
		if(j == SCANBUFSIZE){
			if(fwrite(buffer, sizeof(uint64_t), j, fsa64) != j){
				fprintf(stderr, "gen_sa_lcp::Write file %s error!\n", sa64File.c_str());
				exit(EXIT_FAILURE);
			}
			j = 0;
		}
	}
	if(j != 0){
		if(fwrite(buffer, sizeof(uint64_t), j, fsa64) != j){
			fprintf(stderr, "gen_sa_lcp::Write file %s error!\n", sa64File.c_str());
			exit(EXIT_FAILURE);
		}
	}
	fclose(fsa64);
	delete []sa;

	/*read lcp data in uint40 format*/
	uint40* lcp;
	long nlcp;
	utils::read_objects_from_file(lcp, nlcp, lcpFile);

	/*open lcp64 file, and write data*/
	FILE* flcp64 = fopen(lcp64File.c_str(), "w");
	if(!fsa64){
		fprintf(stderr, "gen_sa_lcp::open file %s error!.\n", sa64File.c_str());
		exit(EXIT_FAILURE);
	}

	for(i = 0, j = 0; i < nlcp; i++){
		if(i != 0)
			buffer[j++] = lcp[i].ull();
		else buffer[j++] = 0;

		if(j == SCANBUFSIZE){
			if(fwrite(buffer, sizeof(uint64_t), j, flcp64) != j){
				fprintf(stderr, "gen_sa_lcp::Write file %s error!\n", lcp64File.c_str());
				exit(EXIT_FAILURE);
			}
			j = 0;
		}
	}
	if(j != 0){
		if(fwrite(buffer, sizeof(uint64_t), j, flcp64) != j){
			fprintf(stderr, "gen_sa_lcp::Write file %s error!\n", lcp64File.c_str());
			exit(EXIT_FAILURE);
		}
	}
	fclose(flcp64);
	delete []lcp;


	if(unlink(saFile.c_str()) != 0){
		fprintf(stderr, "gen_sa_lcp::Unlink file %s erro!\n", saFile.c_str());
	}

	if(unlink(lcpFile.c_str()) != 0){
		fprintf(stderr, "gen_sa_lcp::Unlink file %s error!\n", lcpFile.c_str());
	}

	delete []buffer;
	return 0;
}