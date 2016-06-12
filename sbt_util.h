#ifndef SBT_UTIL_H
#define SBT_UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <algorithm>
#include <assert.h>
#include <cstring>

#define GBWT_INDEX_POSITION "/media/软件/GBWT-Index-Position/"
//#define GBWT_INDEX_POSITION "./raw_data/"

static inline void * sb_malloc(size_t bytes) {

	void * buf = (void*)malloc(bytes);
	if(buf == NULL) {
		fprintf(stderr, "error allocating '%lu' bytes.\n", bytes);
		exit(EXIT_FAILURE);
	}

	return buf;
}

static inline uint64_t sb_file_size(const uint8_t* filename){

	/* return the size of the file  in bytes */
	FILE* in = fopen((char*)filename, "r");
	if(!in) {
		fprintf(stderr, "error sb_file_size: can not open file '%s'.\n",filename);
		assert(false);
		exit(EXIT_FAILURE);
	}
	fseek(in, 0, SEEK_END);
	uint64_t size = ftell(in);
	fclose(in);
	return size;
}


static inline uint64_t sb_read_file(const uint8_t* filename, uint8_t** T) {

	uint64_t bytes = sb_file_size(filename); /* the size of the file */
	(*T) = (uint8_t*) malloc(bytes);
	FILE* in = fopen((char*)filename, "r");
	if(!in) {
		fprintf(stderr, "error sb_read_file: can not open file.\n");
		exit(EXIT_FAILURE);
	}

	uint64_t nsize = fread(*T, 1, bytes,  in);
	if(nsize != bytes) {
		fprintf(stderr, "error sb_read_file: read file error.\n");
		exit(EXIT_FAILURE);
	}

	fclose(in);
	return bytes;
}


static inline uint64_t sb_read_subfile(const uint8_t* filename ,
									uint8_t** T, 
									uint64_t begin, 
									uint64_t length,
									uint64_t B,
									uint64_t* m_io){

	uint64_t bytes = sb_file_size(filename);  /* the size of the file */
	assert(bytes > begin);
	
	uint64_t num = std::min(bytes - begin, length);
	assert(bytes > (begin + num -1));
	
	(*T) = (uint8_t*)malloc(num);
	if((*T) == NULL){
		fprintf(stderr, "error!! sbt_util::sb_read_subfile : allocate memory error!\n");
		exit(EXIT_FAILURE);
	}
	FILE* in = fopen((char*)filename, "r");
	if(!in){
		fprintf(stderr, "error sb_read_subfile: can not open file.\n");
		exit(EXIT_FAILURE);
	}
	fseek(in, begin, SEEK_SET);
	uint64_t nsize = fread(*T, 1, num, in);
	if(nsize != num) {
		fprintf(stderr, "error sb_read_subfile: read file error.\n");
		exit(EXIT_FAILURE);
	}
	fclose(in);

/* count the number of I/O */	
	uint64_t begin_idx = begin / B;
	uint64_t end_idx = (begin + num - 1) / B;
	uint64_t IOs = end_idx - begin_idx + 1;
	*m_io += IOs;

	return num;
}

/*********
* 根据文本文件fileName，得到存储串B-树的文件。
* 1. 串B-树的文件存储在 文件夹 /raw_data/中，所以在当前文件夹下面一定要有 raw_data。
* 2. 文件后缀为_<setp>.sbt
* 3. 当文件名不用的时候，要记得释放返回的文件名: delete xxx。
**********/
static  char* get_outfile_name(const char* filename, uint64_t step){

	char file1[256];
	char* outfile = new char[256];	//申请空间。

	strcpy(file1, filename);
	int i , j;
	int len = strlen(filename);
	for(i = len; i >= 0; i--){
		if(file1[i] == '/')
			break;
	}
	if(i != -1){
		for(j = 0, i++; i < len; i++, j++)
			file1[j] = file1[i];
		file1[j] = '\0';
	}
	/* now, the file1 saves the filename */
	strcpy(outfile, GBWT_INDEX_POSITION);
	strcat(outfile, file1);
	strcat(outfile, "._");
	file1[0] = (step + '0');
	file1[1] = '\0';
	strcat(outfile, file1);
	strcat(outfile, ".sbt");	//文件名后缀。

	return outfile;
}

/*********
* 根据文本文件fileName，得到存储串B-树的文件。
* 1. 串B-树的文件存储在 文件夹 /raw_data/中，所以在当前文件夹下面一定要有 raw_data。
* 2. 文件后缀为_<setp>.sbt
* 3. 当文件名不用的时候，要记得释放返回的文件名。
**********/
static  char* getSBTFile(const char* filename, uint64_t step){
	char file1[256];
	char* outfile = new char[256];

	strcpy(file1, filename);
	int i , j;
	int len = strlen(filename);
	for(i = len; i >= 0; i--){
		if(file1[i] == '/')
			break;
	}
	if(i != -1){
		for(j = 0, i++; i < len; i++, j++)
			file1[j] = file1[i];
		file1[j] = '\0';
	}
	/* now, the file1 saves the filename */
	strcpy(outfile, GBWT_INDEX_POSITION);
	strcat(outfile, file1);
	strcat(outfile, "._");
	file1[0] = (step + '0');
	file1[1] = '\0';
	strcat(outfile, file1);
	strcat(outfile, ".sbt");

	return outfile;	
}


/*******
* 根据文件名 fileName和step(采样步长)得到存储磁盘上KD-Tree的文件名。
* 1. 磁盘上的KD-tree是存储在./raw_data/文件中的，所以工作目录下面一定要有 raw_data 文件加。
* 2. 文件名后缀为: _<step>.kdt
* 3. 当返回的文件名不用的时候，要记得释放内存。
********/
static char* getKDTFile(const char* filename, uint64_t step){
	char file1[256];
	char* outfile = new char[256];	//申请内存，作为结果返回。

	strcpy(file1, filename);
	int i , j;
	int len = strlen(filename);
	for(i = len; i >= 0; i--){
		if(file1[i] == '/')
			break;
	}
	if(i != -1){
		for(j = 0, i++; i < len; i++, j++)
			file1[j] = file1[i];
		file1[j] = '\0';
	}
	/* now, the file1 saves the filename */
	strcpy(outfile, GBWT_INDEX_POSITION);
	strcat(outfile, file1);
	strcat(outfile, "._");
	file1[0] = (step + '0');
	file1[1] = '\0';
	strcat(outfile, file1);
	strcat(outfile, ".kdt");	//文件名的后缀。

	return outfile;
}

static char* getSAFile(const char* filename, uint64_t step){
	char* sbtFile = getSBTFile(filename, step);
	char* outFile = new char[1024];
	strcpy(outFile, sbtFile);
	strcat(outFile, ".saraw");

	delete []sbtFile;
	return outFile;
}

static char* getLCPFile(const char* filename, uint64_t step){
	char* sbtFile = getSBTFile(filename, step);
	char* outFile = new char[1024];
	strcpy(outFile, sbtFile);
	strcat(outFile, ".lcp");

	delete []sbtFile;
	return outFile;
}

static char* getTextFile(const char* filename){
	uint64_t n = strlen(filename);
	char* outFile = new char[n+1];
	strcpy(outFile, filename);
	return outFile;
}

static char* getXPointFile(const char* filename, uint64_t step){
	char* kdtFile = getKDTFile(filename, step);;
	char* outFile = new char[1024];
	strcpy(outFile, kdtFile);
	strcat(outFile, ".xpoint");

	delete []kdtFile;
	return outFile;
}

static char* getYPointFile(const char* filename, uint64_t step){
	char* kdtFile = getKDTFile(filename, step);;
	char* outFile = new char[1024];
	strcpy(outFile, kdtFile);
	strcat(outFile, ".ypoint");

	delete []kdtFile;
	return outFile;	
}

static char* getTestFile(const char* fileName){
	char file1[256];
	char* outFile = new char[1024];	//申请内存，作为结果返回。

	strcpy(file1, fileName);
	int i , j;
	int len = strlen(fileName);
	for(i = len; i >= 0; i--){
		if(file1[i] == '/')
			break;
	}
	if(i != -1){
		for(j = 0, i++; i < len; i++, j++)
			file1[j] = file1[i];
		file1[j] = '\0';
	}

	strcat(outFile, "./test/");
	strcat(outFile, file1);
	strcat(outFile, ".test_result");

	return outFile;
}


#endif  // SBT_UTIL_H
