#include <string.h>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "sbt_util.h"
#include "DiskSBT.h"
#include "test_sbt.h"

using namespace std;

void test_sbt::test_01(){
	const char* text_file = "data/bible";
	// const char* text_file = "sources.100MB";

	uint64_t B = 4*1024;
	uint64_t step = 4;

	sb_tree sbt(text_file, B, step);	
}


void test_sbt::test_02() {

	const char* text_file = "data/bible";
	// const char* text_file = "data/sources.100MB";

	uint64_t B = 4*1024;
	uint64_t step = 1;

	sb_tree sbt(text_file, B, step);

	uint8_t P[1000];
	char* str;
	uint64_t m;
	int64_t left ;
	int64_t right;
	int64_t times;
	
	while(true){
		fprintf(stdout, ">");
		str = gets(reinterpret_cast<char*>(P));  /* gets() doesn't include the "\n", and abandon it. */
		m = strlen(reinterpret_cast<char*>(P));
		if(m == 0)
			break;
		pair<int64_t, int64_t> sr = sbt.suffix_range2(P, m);
		left = sr.first;
		right = sr.second;
		times = right - left + 1;
		fprintf(stdout, "'%s' (of length %lu) suffix range is [%ld, %ld].\n", P, m, left, right);
		fprintf(stdout, "'%s' occurs %ld times.\n", P, times);		
	}
}


void test_sbt::test_03(){
	const char* text_file = "kernel";
	const char* out_file ="./raw_data/kernel.sbt";

	uint64_t B = 32*1024;
	uint64_t step = 8;
	sb_tree sbt(text_file, B, step);
}

void test_sbt::test_04(){

	const char* text_file = "data/bible";
	// const char* text_file = "sources.100MB";

	uint64_t step = 1;
	sb_tree sbt(text_file, step);   /*load sbt from disk*/

	uint8_t P[1000];
	char* str;
	uint64_t m;
	int64_t left;
	int64_t right;
	int64_t times;

	while(true){
		fprintf(stdout, ">");
		str = gets(reinterpret_cast<char*>(P));  /* gets() doesn't include the "\n", and abandon it. */
		m = strlen((char*)P);
		if(m == 0)
			break;
		pair<int64_t, int64_t> sr = sbt.suffix_range(P, m);
		left = sr.first;
		right = sr.second;
		times = right - left + 1;
		fprintf(stdout, "'%s' (of length %lu) suffix range is [%ld, %ld].\n", P, m, left, right);
		fprintf(stdout, "'%s' occurs %ld times.\n", P, times);
		fprintf(stdout, "IO-times = %lu.\n", sbt.get_io_counts());			
	}


}// end of test_04();


void test_sbt::test_05(){
	// const char* text_file = "bible";
	const char* text_file = "sources.100MB";

	uint64_t step = 4;
	sb_tree sbt(text_file, step); 

	uint8_t P[1000];
	char* str;
	uint64_t m;
	int64_t left;
	int64_t right;
	int64_t times;

	while(true){
		fprintf(stdout, ">");
		str = gets((char*)P);  /* gets() doesn't include the "\n", and abandon it. */
		m = strlen((char*)P);
		if(m == 0)
			break;
		pair<int64_t, int64_t> sr = sbt.suffix_range(P, m);
		left = sr.first;
		right = sr.second;
		times = right - left + 1;
		fprintf(stdout, "'%s' (of length %lu) suffix range is [%ld, %ld].\n", P, m, left, right);
		fprintf(stdout, "'%s' occurs %ld times.\n", P, times);
		fprintf(stdout, "IO-times = %lu.\n", sbt.get_io_counts());		
	}

}


void test_sbt::test_sbt_size(char* out_file){
	FILE* out = fopen(out_file, "a");
	if(!out){
		fprintf(stderr, "file '%s' can't open.\n", out_file);
		exit(EXIT_FAILURE);
	}

	const char* file1 = "./data/bible";
	const char* file2 = "./data/sources.100MB";
	const char* file3 = "./data/xml.100MB";

	char data_file[3][256];
	strcpy(data_file[0], file1);
	strcpy(data_file[1], file2);
	strcpy(data_file[2], file3);

	uint64_t B = 4*1024;  /* the size of disk page */
	uint64_t step ;		   /* block factor */
	int i ;
	uint64_t sb_size;

	for(i = 0; i < 3; i++){
		for(step = 1; step <= 8; step += 2){

			sb_tree* sbt = new sb_tree(data_file[i], B, step);
			sb_size = sbt->size_in_bytes();
			float size = (sb_size) / (1024.0 * 1024.0);
			fprintf(out, data_file[i]);
			fprintf(out, "	step = %lu ", step);
			fprintf(out, "	B = %lu KB ", B/1024);
			fprintf(out, "	size = %f MB.\n\n", size);
			delete sbt;
			sbt = NULL;
			fflush(out);
			if(step == 1)
				step = 0;		
		}
		
	}

	fclose(out);
}

void test_sbt::test_06(){

	// const char* textFile = "data/bible";
	const char* textFile = "data/sources.100MB";

	uint64_t step = 1;
	const char* sbtFile = get_outfile_name(textFile, step);
	DiskSBT sbt(textFile, sbtFile);   /*load sbt from disk*/

	uint8_t P[1000];
	char* str;
	uint64_t m;
	int64_t left;
	int64_t right;
	int64_t times;

	while(true){
		fprintf(stdout, ">");
		str = gets((char*)P);  /* gets() doesn't include the "\n", and abandon it. */
		m = strlen((char*)P);
		if(m == 0)
			break;
		pair<int64_t, int64_t> sr = sbt.suffix_range2(P, m);
		left = sr.first;
		right = sr.second;
		times = right - left + 1;
		fprintf(stdout, "'%s' (of length %lu) suffix range is [%ld, %ld].\n", P, m, left, right);
		fprintf(stdout, "'%s' occurs %ld times.\n", P, times);
		fprintf(stdout, "IO-times = %lu.\n", sbt.getIOCounts());			
	}

}// end of test_04();