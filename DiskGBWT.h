#ifndef INCLUDED_DISKGBWT_H
#define INCLUDED_DISKGBWT_H

#include <cstdint>
#include <cstdlib>

#include "DiskKDTree.h"
#include "DiskSBT.h"
#include "sbt_util.h"
#include "rect.h"
#include "Suffix.h"


class DiskGBWT{

public:

	char*	m_textFile;	/*存储文本的文件*/
	char*	m_sbtFile;	/*存储String B-tree的文件*/
	char*	m_kdtFile;	/*存储KD-Tree的文件*/

	uint64_t 	m_B;
	uint64_t 	m_step;	/*采样步长*/

	DiskSBT*	m_sbt;	/*从磁盘中读取串B-树*/
	DiskKDTree*	m_kdt;	/*从磁盘中读取KDTree*/

	uint64_t  	m_sbt_io;	/*进行一次模式匹配过程中，访问串B-树的I/O次数*/
	uint64_t  	m_kdt_io;	/*进行一次模式匹配过程中，访问KD-树的I/O次数*/

	struct Suffix**		m_suf;
	FILE*		m_in;		/*read multi kdtree file*/

public:	
	
	/**** 构造函数
	*@param textFile: 原始文本文件。
	*@param B		: 磁盘页大小
	*@param step	: 采样步长
	****/
	DiskGBWT(const char* textFile, uint64_t B, uint64_t step);
	
	~DiskGBWT();

	/****
	*
	****/
	uint64_t Locate(uint8_t* P, uint64_t m, vector<uint64_t>* location = NULL);
	void locate_helper(uint64_t* result, 
					uint8_t* P, 
					uint64_t m,
					uint64_t Pmin, 
					uint64_t Pmax, 
					uint64_t k,
					vector<uint64_t>* location = NULL);

	void readMultiKDTHeader();
	void initSuf();


	//串B-树和KD-树占用的空间,也就是GBWT占用的空间
	uint64_t all_size_in_bytes();
	//串B树占用的空间
	uint64_t sbt_size_in_bytes();
	//KD-树占用的空间
	uint64_t kdt_size_in_bytes();

	//进行一次模式匹配GBWT所需要的I/O次数。
	uint64_t get_all_io_counts();
	//进行一次模式匹配Strint B-tree所需要的I/O次数
	uint64_t get_sbt_io_counts();
	//进行一次模式匹配KD-tree所需要的I/O次数。
	uint64_t get_kdt_io_counts();

	uint64_t get_sbt_leaf_node_io_counts(Rect r);
		
	void setPosition(int32_t diskNum);

	static int64_t GetPosition(int32_t diskNum, int32_t B);

};


#endif
