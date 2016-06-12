#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_critbit_tree.h"

void test_critbit_tree::test_01(){

	char str[] = "mississipi";
	uint64_t sa[] = {0, 1, 2, 3};
	critbit_tree cbt;
	cbt.set_text((uint8_t*)str, 10);
	for(int i = 0; i < 4; i++)
		cbt.insert_suffix(sa[i]);  /* insert a suffix in critbit tree */

	fprintf(stdout, "g = %lu\n", cbt.m_g);
	fprintf(stdout, "%lu.\n", cbt.contains((uint8_t*)str, 10));
	fprintf(stdout, "%lu.\n", cbt.contains((uint8_t*)&(str[1]),9));
	fprintf(stdout, "%lu.\n", cbt.contains((uint8_t*)"ssis", 4));
	fprintf(stdout, "%lu.\n", cbt.contains((uint8_t*)"sissipi", 7));
	fprintf(stdout, "%lu.\n", cbt.contains((uint8_t*)"issipi", 6));

	uint8_t sym1 = 'a'; /* 97  : 01100001 */
    uint8_t sym2 = 't'; /* 116 : 01110100 */
    uint8_t sym3 = 'z'; /* 122 : 01111010 */
    uint8_t sym4 = ','; /* 44  : 00101100 */
    uint8_t sym5 = 212; /* 212 : 11010100 */
    uint8_t sym6 = 'u'; /* 117 : 01110101 */


    uint64_t critbit_bit_pos12 = 3;		/*从左向右数(begin with 0)，找到首个不同的地方 */
    uint64_t critbit_bit_pos13 = 3;
    uint64_t critbit_bit_pos14 = 1;
    uint64_t critbit_bit_pos23 = 4;
    uint64_t critbit_bit_pos24 = 1;
    uint64_t critbit_bit_pos34 = 1;

    uint64_t critbit_bit_pos45 = 0;
    uint64_t critbit_bit_pos26 = 7;

/* 因为critbit_tree是string-B-tree的关键性组成部分，所以需要保证critbit_tree的正确性 */
 fprintf(stdout, "---------------------------------------------------------\n");
 	fprintf(stdout, "%d.\n", cbt.get_critbit_pos(sym1, sym2) == critbit_bit_pos12);
 	fprintf(stdout, "%d.\n", cbt.get_critbit_pos(sym1, sym3) == critbit_bit_pos13);
 	fprintf(stdout, "%d.\n", cbt.get_critbit_pos(sym1, sym4) == critbit_bit_pos14);
 	fprintf(stdout, "%d.\n", cbt.get_critbit_pos(sym2, sym3) == critbit_bit_pos23);
 	fprintf(stdout, "%d.\n", cbt.get_critbit_pos(sym2, sym4) == critbit_bit_pos24);
 fprintf(stdout, "---------------------------------------------------------\n");

}


void test_critbit_tree::test_02(){
	char str[] = "mississipi"; // length = 10
	uint64_t sa[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	critbit_tree cbt;

	cbt.set_text((uint8_t*)str, 10);
	for(int i = 0; i < 10; i++)
		cbt.insert_suffix(sa[i]);

/* test critbit_tree's insert and delete operation */

/* ------test delete_suffix------ */	
	fprintf(stdout, "g = %lu\n", cbt.m_g);
	fprintf(stdout, "%lu\n", cbt.contains((uint8_t*)"issis", 5));
	fprintf(stdout, "%lu\n", cbt.contains((uint8_t*)"issi", 4));
	cbt.delete_suffix(1);  // delete suffix T[1..n]
	fprintf(stdout, "%lu\n", cbt.contains((uint8_t*)"issis", 5)); /* doesn't exist*/
	fprintf(stdout, "%lu\n", cbt.contains((uint8_t*)"issi", 4));  /* still exist */

	cbt.insert_suffix(1);
	fprintf(stdout, "%lu\n", cbt.contains((uint8_t*)"issis", 5)); /* exist! */
}



void test_critbit_tree::test_03(){

	char str[] = "mississipi";
	uint64_t sa[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	critbit_tree cbt;
	cbt.set_text((uint8_t*)str, 10);
	// cbt.create_from_suffixes(sa, 4);
	for(int i = 0; i < 10; i++)
		cbt.insert_suffix(sa[i]);
/*--------------- test collecting all the suffix ------------------------*/
	fprintf(stdout, "g = %lu\n", cbt.m_g);
	fprintf(stdout, "%lu\n", cbt.contains((uint8_t*)"issi", 4));

	uint64_t* results;
	uint64_t count;
	uint8_t* P = (uint8_t*)"issi";
	uint64_t m = 4;
	count = cbt.suffixes(P, m, &results);

	fprintf(stdout, "'%s' has %lu suffixes.\n", P, count);
	for(uint64_t i = 0; i < count; i++)
		fprintf(stdout, "%lu  ", results[i]);
	fprintf(stdout, "\n");

/*---------------------------------------------------------------------------*/
	P = (uint8_t*)"s";
	m = 1;
	fprintf(stdout, "-----------------------------------------------\n");
	count = cbt.suffixes(P, m, &results);
	fprintf(stdout, "'%s' has %lu suffixes.\n", P, count);
	for(uint64_t i = 0; i < count; i++)
		fprintf(stdout, "%lu  ", results[i]);
	fprintf(stdout, "\n");

/*-----------------------------------------------------------------*/	

	P = (uint8_t*)"i";
	m = 1;

	fprintf(stdout,"---------------------------------------------\n");
	count = cbt.suffixes(P, m, &results);

	fprintf(stdout, "'%s' has %lu suffixes.\n", P, count);

	for(uint64_t i = 0; i < count; i++)
		fprintf(stdout, "%lu  ", results[i]);

	fprintf(stdout, "\n");
/*****************************************************************/
	P = (uint8_t*)"$";
	m = 1;

	fprintf(stdout,"---------------------------------------------\n");
	count = cbt.suffixes(P, m, &results);

	fprintf(stdout, "'%s' has %lu suffixes.\n", P, count);

	for(uint64_t i = 0; i < count; i++)
		fprintf(stdout, "%lu  ", results[i]);

	fprintf(stdout, "\n");	

/********************************************************************/
	P = (uint8_t*)"mississipi$";
	m = 11;

	fprintf(stdout,"---------------------------------------------\n");
	count = cbt.suffixes(P, m, &results);

	fprintf(stdout, "'%s' has %lu suffixes.\n", P, count);

	for(uint64_t i = 0; i < count; i++)
		fprintf(stdout, "%lu  ", results[i]);

	fprintf(stdout, "\n");	
/********************************************************************/
	P = (uint8_t*)"";
	m = 0;

	fprintf(stdout,"---------------------------------------------\n");
	count = cbt.suffixes(P, m, &results);

	fprintf(stdout, "'%s' has %lu suffixes.\n", P, count);

	for(uint64_t i = 0; i < count; i++)
		fprintf(stdout, "%lu  ", results[i]);

	fprintf(stdout, "\n");	


/****** It looks right!  Maybe need more test! ******************/

}


void test_critbit_tree::test_help_contains(critbit_tree& cbt, uint8_t* P, uint64_t m){

	fprintf(stdout,"\n----------------contains-----------------------------\n");
	if(cbt.contains(P, m))
		fprintf(stdout, "'%s' : yes.\n", P);
	else 
		fprintf(stdout, "'%s' : no.\n", P);
}

void test_critbit_tree::test_help_suffixes(critbit_tree& cbt, uint8_t* P, uint64_t m, uint64_t** results){
	fprintf(stdout, "\n--------------suffixes-------------------------------\n");
	uint64_t count = cbt.suffixes(P, m, results);
	fprintf(stdout, " '%s' occurs %lu times.\n", P, count);
	free(*results);	
}

void test_critbit_tree::test_bible(){
	uint8_t* filename = (uint8_t*)"./bible";
	uint8_t* T;
	fprintf(stdout, "\n------ begin reading file '%s' ------\n", filename);
	uint64_t bytes =  sb_read_file(filename, &T);
	fprintf(stdout, "'\n------file size is %lu bytees.-------------\n", bytes);
	
	critbit_tree cbt;
	cbt.set_text(T, bytes);

	fprintf(stdout, "\n--------------start inserting suffixes --------------\n");
	for(uint64_t i = 0; i < bytes; i++)
		cbt.insert_suffix(i);
	fprintf(stdout, "\n--------------end inserting %lu suffixes-------------\n", bytes);

	uint8_t* P = (uint8_t*)"god";  /*should be 347 times*/
	uint64_t m = 3;
	uint64_t* results = NULL;
	test_help_contains(cbt, P, m);
	test_help_suffixes(cbt, P, m, &results);

	P = (uint8_t*)"and";  /* should be 43878 times */
	m = 3;
	results = NULL;
	test_help_suffixes(cbt, P, m, &results);

	P = (uint8_t*)"Mehujael begat Methusael:"; /*should be 1 times */
	m = strlen((char*)P);
	results = NULL;
	test_help_suffixes(cbt, P, m, &results);

	P = (uint8_t*)"Mehujael begat Methusael:a"; /*should be 0 times */
	m = strlen((char*)P);
	results = NULL;
	test_help_suffixes(cbt, P, m, &results);
}

void test_critbit_tree::test_kernel() {

	uint8_t* filename = (uint8_t*)"./sources.100MB";
	uint8_t* T;
	fprintf(stdout, "\n------ begin reading file '%s' ------\n", filename);
	uint64_t bytes = sb_read_file(filename, &T);

	fprintf(stdout, "\n------ file size is %lu bytes--------.\n", bytes);

	critbit_tree cbt;
	cbt.set_text(T, bytes);

	fprintf(stdout, "\n------ start inserting suffixes.-----\n");
	for(uint64_t i = 0; i < bytes; i++)
		cbt.insert_suffix(i);

	fprintf(stdout, "\n------ end inserting suffixes. ------\n");

	uint8_t* P = (uint8_t*)"int";  
	uint64_t m = 3;
	uint64_t* results = NULL;
	test_help_suffixes(cbt, P, m, &results);

	P = (uint8_t*) "set";
	m = 3;
	results = NULL;
	test_help_suffixes(cbt, P, m, &results);
}


void test_critbit_tree::test_04(){
	/* test file I/O, write critbit_tree into file*/
	uint8_t* T = (uint8_t*)"mississipi";
	uint64_t n = 10;

	critbit_tree cbt;
	cbt.set_text(T, n);

	uint64_t SA[] = {0, 1, 3, 4, 2, 5, 9, 6, 8, 7};
	uint64_t nsa = 10;
	cbt.create_from_suffixes(SA, nsa);
	FILE* out = fopen("./raw_data/missi.cbt", "w");
	if(out == NULL){
		fprintf(stderr, "write critbit_tree error.\n");
		exit(EXIT_FAILURE);
	}

	uint64_t size = cbt.write(out);
	fprintf(stdout, "-----------write %ld bytes  datas-------------\n", size);
	fclose(out);
}


void test_critbit_tree::test_05(){
	/* test file I/O: read critbit tree from file .cbt */
	critbit_tree cbt;
	uint8_t* T = (uint8_t*)"mississipi";
	uint64_t n = 10;
	cbt.set_text(T, n);

	uint8_t* filename = (uint8_t*)"./raw_data/missi.cbt";
	uint64_t fsize = sb_file_size(filename); /*in bytes*/
	fsize = fsize >> 3;  /* in 64 bits */

	FILE* in = fopen((char*)filename, "r");
	if(!in){
		fprintf(stderr, "read critbit_tree error: open file.\n");
		exit(EXIT_FAILURE);
	}

	uint64_t* mem = (uint64_t*)malloc(sizeof(uint64_t)*fsize);
	uint64_t x = fread(mem, sizeof(uint64_t), fsize, in);
	if( x!= fsize){
		fprintf(stderr, "read critbit_tree error: read men.\n");
		exit(EXIT_FAILURE);
	}
fprintf(stdout, "%ld elements read from '%s'\n ", x, filename);
	
	cbt.load_from_mem(mem, fsize);
/* test critbit tree */

	/* ------------------ test contains and delete ---------------------------------------- */
	fprintf(stdout, "g = %lu\n", cbt.m_g);
	fprintf(stdout, "%lu\n", cbt.contains((uint8_t*)"issis", 5));
	fprintf(stdout, "%lu\n", cbt.contains((uint8_t*)"issi", 4));
	cbt.delete_suffix(1);
	fprintf(stdout, "%lu\n", cbt.contains((uint8_t*)"issis", 5));
	fprintf(stdout, "%lu\n", cbt.contains((uint8_t*)"issi", 4));

	cbt.insert_suffix(1);
	fprintf(stdout, "%lu\n", cbt.contains((uint8_t*)"issis", 5));
	fprintf(stdout, "%lu\n", cbt.contains((uint8_t*)"i", 1));

/*-------- It looks correct  ----------*/

	uint64_t* results;
	uint64_t sufcount = cbt.suffixes((uint8_t*)"i", 1, &results); 
	for(uint64_t i = 0; i < sufcount; i++)
		fprintf(stdout, "%lu ", results[i]);
	fprintf(stdout, "\n");
	free(results);		
}


void test_critbit_tree::test_cbt_write(){
	uint8_t* filename = (uint8_t*)"sources.100MB";
	uint8_t* outfilename = (uint8_t*)"./raw_data/sources.100MB.cbt";
	uint8_t* T;
	fprintf(stdout, "\n==============begin reading file %s=================\n", filename);
	uint64_t bytes = sb_read_file(filename, &T);

	fprintf(stdout, "\n--------------file size is %lu bytes-----------------.\n", bytes);

	critbit_tree cbt;
	cbt.set_text(T, bytes);

	fprintf(stdout, "\n------ start inserting suffixes.-----\n");
	for(uint64_t i = 0; i < bytes; i++)
		cbt.insert_suffix(i);

	fprintf(stdout, "\n------ end inserting suffixes. ------\n");

	fprintf(stdout, "\n------ begin to write ---------\n");
	FILE* out = fopen((char*)outfilename, "w");
	if(!out){
		fprintf(stdout, "error test_kernel_write: open file error.\n");
		exit(EXIT_FAILURE);
	}

	uint64_t written = cbt.write(out);
	fprintf(stdout, "\n------- end written %lu bytes ------\n",written);
	fclose(out);
}


void test_critbit_tree::test_cbt_read() {

	uint8_t* filename = (uint8_t*)"sources.100MB";
	uint8_t* infilename = (uint8_t*)"./raw_data/sources.100MB.cbt";

	uint8_t* T;
	fprintf(stdout, "\n------ begin reading file '%s' ------\n", filename);
	uint64_t bytes = sb_read_file(filename, &T);

	fprintf(stdout, "\n------ file size is %lu bytes--------.\n", bytes);

	critbit_tree cbt;
	cbt.set_text(T, bytes);

	fprintf(stdout, "\n------ start reading raw_data.-----\n");

	uint64_t fsize = sb_file_size(infilename);
	fsize = fsize >> 3;
	uint64_t* mem = (uint64_t*)malloc(fsize * 8);

	FILE* in = fopen((char*)infilename, "r");
	if(!in){
		fprintf(stderr, "%s\n", "error open file error");
		exit(EXIT_FAILURE);
	}

	uint64_t readcount  = fread(mem, sizeof(uint64_t), fsize, in);
	if(readcount != fsize) {
		fprintf(stdout, " reading file error.(%lu, %lu)\n", readcount, fsize);
		exit(EXIT_FAILURE);
	}

	fclose(in);

	cbt.load_from_mem(mem, fsize);

	fprintf(stdout, "\n------ end loading raw_data. ------\n");

	uint8_t* P = (uint8_t*)"int";     /*258350 times*/
	// uint8_t* P = (uint8_t*)"god";  /*347 times*/
	uint64_t m = 3;
	uint64_t* results = NULL;
	test_help_suffixes(cbt, P, m, &results);

	P = (uint8_t*) "set"; m = 3;    /**87438 times, right/
	// P = (uint8_t*)"help"; m = 4; /*181 times*/
	results = NULL;
	test_help_suffixes(cbt, P, m, &results);
}



































