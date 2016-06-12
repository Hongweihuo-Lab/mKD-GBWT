#ifndef INCLUDED_TEST_DISKGBWT_H
#define INCLUDED_TEST_DISKGBWT_H

/**********
* 对DiskGBWT的测试: 从已经生成的磁盘文件中读取磁盘上的串B-树和磁盘上的KDTree.
* 并且进行模式匹配的测试。 
********/

class test_diskgbwt{

public:	
	
	static uint8_t *m_T;
	static uint64_t m_n;

	//终端读取数据，并且进行模式匹配
	static void test_01(const char* fileName, uint64_t B, uint64_t step);
	
	static void test_all(const char* fileName, uint64_t B, uint64_t step, const char* resultFile);

	static void test_final(const char* fileName, uint64_t B, uint64_t step, const char* resultFile);

	static void test_average(const char* fileName, uint64_t B, 
							uint64_t step, 
							const char* testTextFile,
							const char* resultFile);		
	
	static uint64_t** get_test_data_position(uint64_t* pCount);

	static uint64_t* read_position(const char* filename, uint64_t& size);

	static uint64_t get_random_position(uint64_t* pos, uint64_t size);

	static void read_pattern(uint64_t sufpos, uint8_t *P, uint64_t m);
};



#endif