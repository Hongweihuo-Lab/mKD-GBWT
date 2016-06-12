#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>

#include "sbt_util.h"
#include "DiskGBWT.h"
#include "test_diskgbwt.h"

#define FILE_NUM 4

using namespace std;

uint8_t* test_diskgbwt::m_T = NULL;
uint64_t test_diskgbwt::m_n = 0;

//测试数据
void test_diskgbwt::test_01(const char* fileName, uint64_t B, uint64_t step){
	
	DiskGBWT *dgbwt = new DiskGBWT(fileName, B, step);
	uint64_t sbtSize = dgbwt->sbt_size_in_bytes();
	uint64_t kdtSize = dgbwt->kdt_size_in_bytes();
	
	fprintf(stdout, "fileName is '%s'.\n", fileName);
	fprintf(stdout, "step = %lu, B = %lu\n", step, B);
	fprintf(stdout, "sb_tree size is %luB = %fMB.\n", sbtSize, (sbtSize/(1024.0*1024.0)));
	fprintf(stdout, "kd_tree size is %luB = %fMB.\n", kdtSize, (kdtSize/(1024.0*1024.0)));

	uint8_t P[1000];
	char* str;
	uint64_t m;
	int64_t times;

	while(true){
		fprintf(stdout, "(input pattern)>");
		str = gets((char*)P);
		m =strlen((char*)P);
		if(m == 0)
			break;

		times = dgbwt->Locate(P, m);
		uint64_t sbtIO = dgbwt->get_sbt_io_counts();
		uint64_t kdtIO = dgbwt->get_kdt_io_counts();
		fprintf(stdout, "\t\toccurs %lu times.\n", times);
		fprintf(stdout, "\t\tsbt IO Counts = %lu.\n", sbtIO);
		fprintf(stdout, "\t\tkdt IO Counts = %lu.\n", kdtIO);
	}
}

void test_diskgbwt::test_final(const char* fileName, uint64_t B, uint64_t step, const char* resultFile){
	//open textFile and read text
	m_n = sb_read_file((const uint8_t*)fileName, &m_T);

	uint64_t size[3];		//GBWT, SBT, KDT size(in bytes)
	uint64_t IOs[3];        //the I/Os of GBWT, SBT, and KDT
	uint64_t maxIOs[3];		//the max I/Os of GBWT, SBT and KDT
	uint64_t ntest;			//进行随机测试的次数，每次随机选取模式
	uint64_t plen;			//随机选取的模式的长度

	//对上述变量进行初始化操作
	for(int i = 0; i < 3; i++){
		IOs[i] = 0;
		maxIOs[i] = 0;
	}

	ntest = 10000;		/*随机进行一万次测试*/
	plen = 10;			/*随机选取的模式长度*/

	uint8_t* P = new uint8_t[plen+1];

	FILE* out = fopen(resultFile, "a");	/*append the file*/
	if(!out){
		fprintf(stderr, "open file '%s' error!\n", resultFile);
		exit(EXIT_FAILURE);
	}	

	/********
	* 要保证(sbtFile 和 kdFile)已经在磁盘上创建好
	********/
	DiskGBWT* gbwt = new DiskGBWT(fileName, B, step);
	
	size[0] = gbwt->all_size_in_bytes();
	size[1] = gbwt->sbt_size_in_bytes();
	size[2] = gbwt->kdt_size_in_bytes();

	srand(time(0));

	fprintf(stderr, "\n--------------begin test----------------\n");
	fprintf(stderr, "Source-File = '%s', B = %lu, step = %lu.\n", fileName, B, step);

	for(int64_t k = 0; k < ntest; k++){
		/*不同的i会选取不同的模式类型: 5000, 20000, 20000以上*/
		uint64_t sufpos = rand()%(m_n - plen);
		assert(sufpos < m_n - plen);
		/*从文本中，读取这个模式*/
		read_pattern(sufpos, P, plen);
		/*对此模式进行模式匹配*/
		uint64_t count = gbwt->Locate(P, plen);
		// fprintf(stderr, "%s %lu\n", P, count);	/*进行日志记录*/
		
		/*记录相应的I/O次数*/
		IOs[0] += gbwt->get_all_io_counts();
		IOs[1] += gbwt->get_sbt_io_counts();
		IOs[2] += gbwt->get_kdt_io_counts();

		maxIOs[0] = std::max(maxIOs[0], gbwt->get_all_io_counts());
		maxIOs[1] = std::max(maxIOs[1], gbwt->get_sbt_io_counts());
		maxIOs[2] = std::max(maxIOs[2], gbwt->get_kdt_io_counts());
	}
	
	fprintf(stderr, "\n----------------end test---------------\n");

	fprintf(out, "\n\nmKD-GBWT(pattern-matching): \n");
	fprintf(out, "test-data: %s\n", fileName);
	fprintf(out, "test-data-size: %d bytes = %.2f MB\n", m_n, (m_n / (1024.0*1024.0)));
	fprintf(out, "B(disk-page-size)= %lu bytes\n", B);
	fprintf(out, "step(blocking-factor) = %lu\n", step);
	fprintf(out, "test pattern len = %lu\n", plen);
	fprintf(out, "the number of random test = %lu\n", ntest);

	fprintf(out, "------------test string B-tree, kd_tree and GBWT-------.\n\n");
	fprintf(out, "\t\tmkd-gbwt_size_in_bytes: %lu bytes = %.2f MB\n", size[0], (size[0] / (1024.0*1024.0)));
	fprintf(out, "\t\tsbt_size_in_bytes: %lu bytes = %.2f MB\n", size[1], (size[1] / (1024.0*1024.0)));
	fprintf(out, "\t\tkdb_size_in_bytes: %lu bytes = %.2f MB\n", size[2], (size[2] / (1024.0*1024.0)));

	fprintf(out, "------------test I/O counts-----.\n");
	fprintf(out, "\t\t(all, sbt, kdt) average IOs per pattern-matching = %.2f %.2f %.2f\n",
			((double)(IOs[0])/ntest), ((double)(IOs[1])/ntest),
			((double)(IOs[2])/ntest));
	fprintf(out, "\t\t(all, sbt, kdt) max-I/Os per pattern-matching = %lu %lu %lu\n",
			maxIOs[0], maxIOs[1], maxIOs[2]);	
	// fprintf(out, "----------------------------------------------\n\n");

	fclose(out);
	
	delete []P;
	delete gbwt;

	free(m_T);
	m_T = NULL;
}

void test_diskgbwt::test_all(const char* fileName, uint64_t B, uint64_t step, const char* resultFile){
	
	/*open textFile and read text*/
	m_n = sb_read_file((const uint8_t*)fileName, &m_T);

	uint64_t** pos; 			/*存储模式的起始位置*/
	uint64_t pCount[FILE_NUM];	/*表示不同类型的模式的个数*/

	uint64_t size[3];			/*GBWT SBT KDT的大小(size in bytes)*/
	uint64_t IOs[3][FILE_NUM];	/*收集模式匹配的I/O次数.(GBWT SBT KDT)x(不同的模式)*/
	
	uint64_t ntest;				/*进行随机测试的次数，每次随机选取模式*/
	uint64_t plen;				/*随机选取的模式的长度*/
	
	/*对上述变量进行初始化操作*/
	for(int i = 0; i < 3; i++){
		for(int j = 0; j < FILE_NUM; j++){
			IOs[i][j] = 0;
		}
	}
	
	ntest = 10000;		/*随机进行一万次测试*/
	plen = 10;			/*随机选取的模式长度*/

	uint8_t* P = new uint8_t[plen+1];
	
	/*首先得到所有模式的起始位置*/
	pos = get_test_data_position(pCount);
	

	FILE* out = fopen(resultFile, "a");	/*append the file*/
	if(!out){
		fprintf(stderr, "open file '%s' error!\n", resultFile);
		exit(EXIT_FAILURE);
	}
	
	/********
	* 要保证(sbtFile 和 kdFile)已经在磁盘上创建好
	********/
	DiskGBWT* gbwt = new DiskGBWT(fileName, B, step);
	
	size[0] = gbwt->all_size_in_bytes();
	size[1] = gbwt->sbt_size_in_bytes();
	size[2] = gbwt->kdt_size_in_bytes();

	srand(time(0));

	fprintf(stderr, "\n--------------begin test----------------\n");
	fprintf(stderr, "Source-File = '%s', B = %lu, step = %lu.\n", fileName, B, step);
	for(int64_t i = 0; i < FILE_NUM; i++){
		fprintf(stderr, "FILE_NUM: %ld\n",i);
		for(int64_t k = 0; k < ntest; k++){
			/*不同的i会选取不同的模式类型: 5000, 20000, 20000以上*/
			uint64_t sufpos = get_random_position(pos[i], pCount[i]);
			/*从文本中，读取这个模式*/
			read_pattern(sufpos, P, plen);
			/*对此模式进行模式匹配*/
			uint64_t count = gbwt->Locate(P, plen);
			// fprintf(stderr, "%s %lu\n", P, count);	/*进行日志记录*/
			
			/*记录相应的I/O次数*/
			IOs[0][i] += gbwt->get_all_io_counts();
			IOs[1][i] += gbwt->get_sbt_io_counts();
			IOs[2][i] += gbwt->get_kdt_io_counts();
		}
	}
	fprintf(stderr, "\n----------------end test---------------\n");
	
	fprintf(out, "gbwt_05: Multi-kdt\n");
	fprintf(out, "test-data: %s\n", fileName);
	fprintf(out, "B(disk-page-size)= %lu\n", B);
	fprintf(out, "step(blocking-factor) = %lu\n", step);
	fprintf(out, "test pattern len = %lu\n", plen);
	fprintf(out, "the number of random test = %lu\n", ntest);

	fprintf(out, "\n\n------------test string B-tree, kd_tree and GBWT.\n\n");
	fprintf(out, "\t\tmkd-gbwt_size_in_bytes: %lu\n", size[0]);
	fprintf(out, "\t\tsbt_size_in_bytes: %lu\n", size[1]);
	fprintf(out, "\t\tkdb_size_in_bytes: %lu\n\n", size[2]);

	fprintf(out, "\n\n------------test I/O counts.\n");
	const char* str[FILE_NUM] = {"1000:\n", "5000:\n", "10000:\n", "20000:\n"};
	
	for(int i = 0; i < FILE_NUM; i++){
		fprintf(out, "%s", str[i]);
		fprintf(out, "\t\t(all, sbt, kdt) total I/Os = %lu %lu %lu\n",
				IOs[0][i], IOs[1][i], IOs[2][i]);

		fprintf(out, "\t\t(all, sbt, kdt) total I/Os = %f %f %f\n",
				((double)(IOs[0][i])/ntest), ((double)(IOs[1][i])/ntest),
				((double)(IOs[2][i])/ntest));
	}
	
	fprintf(out, "\n----------------------------------------------\n");

	fclose(out);
	
	delete []P;
	delete gbwt;

	for(int i = 0; i < FILE_NUM; i++)
		free(pos[i]);
	free(pos);

	free(m_T);
	m_T = NULL;
}



void test_diskgbwt::test_average(const char* fileName, 
								uint64_t B, 
								uint64_t step, 
								const char* testTextFile, 
								const char* resultFile){

	m_n = sb_read_file((const uint8_t*)testTextFile, &m_T);
	uint64_t* pos;		/**/
	uint64_t pCount;	/**/
	
	uint64_t size[3];  	/*GBWT, SBT, KDT的大小(size in bytes)*/
	uint64_t IOs[3];	/*收集模式匹配过程中的I/O次数.(GBWt, SBT, KDT)*/
	uint64_t maxIOs[3]; /*collect the max IOs during pattern matching*/
	uint64_t IO1, IO2, IO3;

	uint64_t ntest;		/*进行随机测试的次数，每次随机选取模式*/
	uint64_t plen;      /*随机选取模式的长度*/

	for(int i = 0; i< 3; i++){
		IOs[i]=0;
		maxIOs[i] = 0;
	}

	ntest = 10000;		/*随机进行一万次测试*/
	plen = 8;			/*随机选取的模式长度*/

	uint8_t* P = new uint8_t[plen + 1];

	/*首先得到所有模式的起始位置*/
	char* testSufFile = new char[1000];
	strcat(testSufFile, testTextFile);
	strcat(testSufFile, "_1500.suf");

	pos = read_position(testSufFile, pCount);

	// pCount = 100*1024*1024 - plen;
	// pos = new uint64_t[pCount];
	// for(int32_t i = 0; i < pCount; i++)
	// 	pos[i] = i;


	FILE* out = fopen(resultFile, "a");  /*追加在文件尾部*/
	if(!out){
		fprintf(stderr, "test_diskgbwt::test_average() -- open file '%s' error!\n", resultFile);
		exit(EXIT_FAILURE);
	}

	/********
	* 需要保证(SBTfile 和 kdtFile) 已经在磁盘上创建好。
	*********/
	DiskGBWT* gbwt = new DiskGBWT(fileName, B ,step);
	size[0] = gbwt->all_size_in_bytes();
	size[1] = gbwt->sbt_size_in_bytes();
	size[2] = gbwt->kdt_size_in_bytes();

	srand(time(0));

	fprintf(stderr, "\n------------------begin test-------------------\n");
	fprintf(stderr, "File = '%s', B = %lu, step = %lu.\n", fileName, B, step);
	for(int64_t k = 0; k < ntest; k++){
		
		uint64_t sufpos = get_random_position(pos, pCount);  /*读取模式的后缀位置*/
		read_pattern(sufpos, P, plen);  /*在文本m_T中读取模式*/
		uint64_t count = gbwt->Locate(P, plen);  /*在disk_gbwt中进行模式匹配*/

		/*每一次模式匹配记录相应的I/o次数*/
		IO1 = gbwt->get_all_io_counts();
		IO2 = gbwt->get_sbt_io_counts();
		IO3 = gbwt->get_kdt_io_counts();
		IOs[0] += IO1;
		IOs[1] += IO2;
		IOs[2] += IO3;

		if(maxIOs[0] < IO1)
			maxIOs[0] = IO1;
		if(maxIOs[1] < IO2)
			maxIOs[1] = IO2;
		if(maxIOs[2] < IO3)
			maxIOs[2] = IO3;

	}

	fprintf(stderr, "\n------------------end test------------\n");
	

	fprintf(out, "gbwt_05: Multi-kdt\n");
	fprintf(out, "test data: %s\n", fileName);
	fprintf(out, "B = %lu, step = %lu\n", B, step);
	fprintf(out, "pattern len = %lu\n", plen);
	fprintf(out, "pattern occurs in '%s'\n", testSufFile);
	fprintf(out, "the number of random test = %lu\n", ntest);

	fprintf(out, "\n\n---------test string B-tree, kd_tree and GBWT.\n\n");
	fprintf(out, "\t\tall_size_in_bytes: %lu\n", size[0]);
	fprintf(out, "\t\tsbt_size_in_bytes: %lu\n", size[1]);
	fprintf(out, "\t\tkdt_size_in_bytes: %lu\n", size[2]);

	fprintf(out, "\n\n-----------test I/O counts.\n");
	fprintf(out, "随机选取模式位置:");
	fprintf(out, "\t\t总次数(all, sbt, kdt) total I/Os = %lu, %lu, %lu\n",
			IOs[0], IOs[1], IOs[2]);;
	
	fprintf(out, "\t\t平均次数(all, sbt, kdt) total I/Os = %f, %f, %f.\n",
			((double)(IOs[0])/ntest), ((double)(IOs[1])/ntest), ((double)(IOs[2])/ntest));

	fprintf(out, "\t\t Max Munimum IOs(all, sbt, kdt): = %lu, %lu, %lu\n",
			maxIOs[0], maxIOs[1], maxIOs[2]);

	fprintf(out, "\n----------------------------------------------\n");
	fclose(out);

	delete []P;
	delete gbwt;
	free(pos);

	free(m_T);
	m_T = NULL;

}



uint64_t** test_diskgbwt::get_test_data_position(uint64_t* pCount){


	const char* file1 = "../data/1000.suf";
	const char* file2 = "../data/5000.suf";
	const char* file3 = "../data/10000.suf";
	const char* file4 = "../data/20000.suf";

	
	uint64_t size1,size2, size3, size4;
	uint64_t file_num = 4;

	uint64_t** result = (uint64_t**)malloc(sizeof(uint64_t*)*FILE_NUM);
	result[0] = read_position(file1, size1);
	result[1] = read_position(file2, size2);
	result[2] = read_position(file3, size3);
	result[3] = read_position(file4, size4);

	pCount[0] = size1; pCount[1] = size2; pCount[2] = size3; pCount[3] = size4;
	return result;	

}

uint64_t* test_diskgbwt::read_position(const char* filename, uint64_t& size){
	
	FILE* fp = fopen(filename, "r");
	if(!fp){
		fprintf(stderr, " test_diskgbwt::read_position(): open file '%s' error!.\n", filename);
		exit(EXIT_FAILURE);
	}
	/* get file length: in bytes */
	fseek(fp, 0,SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);   /*because we read file next so, put the file pointer in the begin*/

	assert(size % 8 == 0);  /* in 64bits*/
	size = size / 8;
	/* malloc memory */
	uint64_t* pos = (uint64_t*)malloc(sizeof(uint64_t)*size);
	if(!pos){
		fprintf(stderr, "test_diskgbwt::read_position(): alloc memory error!.\n");
		exit(EXIT_FAILURE);
	}
	uint64_t n1;
	if((n1=fread(pos, sizeof(uint64_t), size, fp)) != size){
		fprintf(stderr, "test_diskgbwt::read_position(): read file '%s' error!.\n", filename);
		exit(EXIT_FAILURE);
	}
	fclose(fp);
	return pos;

}




inline uint64_t test_diskgbwt::get_random_position(uint64_t* pos, uint64_t size){

	/* get a random number from [0, size-1]*/
	uint64_t idx = (rand()%(size));
	assert(idx < size);
	return pos[idx];
}



inline void test_diskgbwt::read_pattern(uint64_t sufpos, uint8_t*P, uint64_t m){
	assert(P!=NULL);
	
	uint8_t* T = m_T; /*gbwt中的文本*/
	uint64_t n = m_n; /*文本的大小*/
	
	assert(sufpos+m <= n);
	/*从文本中选择模式的长度*/
	for(int i = 0; i < m; i++)
		P[i] = T[sufpos+i];
	
	P[m] = '\0';
} 
