#ifndef INCLUDED_DISKSBT_H
#define INCLUDED_DISKSBT_H


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <queue>
#include <utility>

#define SBT_ROOT_OFFSET 4096

#include "critbit_tree.h"
#include "sbtmpfile.h"

using std::pair;


class DiskSBT{

public:

	uint64_t		m_B; 	//每个磁盘页的大小
	uint64_t 		m_b;	//每个磁盘也存储的后缀数组的大小
	
	uint64_t		m_n;	//文本的长度
	
	uint64_t		m_step;	//采样的长度
	uint64_t 		m_sa;	//采样后缀的个数

	
	char*			m_textFile;	//文本文件。
	char*			m_sbtFile;	//存储串B-树的文件。

	FILE*			m_sbtFd;

	critbit_tree 	*m_root;	//从跟节点加载的critbit_tree


	uint64_t		m_io;		//进行suffix range查询的I/O次数。
	uint64_t		m_blocks;	// the number of all disk pages in string B-tree

	uint64_t 		bits_per_suffix;
	uint64_t 		bits_per_pos;
	uint64_t 		m_height;

public:

	/*	Disk Layout description of the index file.
		
		0~4065			: [B][b][n] [empty space]
		4096~B+4096		: root disk page (B bytes)
		followed by		: [rest of the String b-tree internal pages]
		followed by		: [suffix array pages]
		
		therefore:		root pages always at file offset 4096.
	*/
	
	//构造函数
	DiskSBT(const char *textFile, const char *sbtFile);
	
	//析构函数
	~DiskSBT();

	//读取sbtFile header
	void readHeader(FILE* in);
	
	//加载一块磁盘页
	critbit_tree*	loadDiskPage(uint64_t offset);
	
	//加载String B-tree的根节点
	critbit_tree*	loadRootNode();
	
	/*给定一个磁盘页号，得到对应的偏移。
	* 跟节点对应的磁盘页号为0.
	*TODO: 需要修改一下create_tree中的内容。
	****/	
	uint64_t getOffset(uint64_t diskNum);

	//给定一个磁盘页号，从磁盘中读出对应的磁盘页。
	uint8_t* getMemFromDisk(uint64_t diskNum);

	bool isLeaf(critbit_tree* cbt);
	
	uint64_t sizeInBytes();
	/****
	*TODO: Given a pattern, query the string B-tree, and get the suffix range of a pattern
	*	   And calculate the I/O times.
	****/
	
	int64_t left_suffix_range(uint8_t* P, uint64_t m);
	int64_t	right_suffix_range(uint8_t* P, uint64_t m);
	
	pair<int64_t, int64_t> suffix_range(uint8_t* P, uint64_t m);


	/*****
	*TODO: 对求取String B-tree的suffix range 进行一些改进。
	* 当在String B-tree的节点中求取Suffix range[left, right]的时候，如果求取Left 和 Right
	* 所经过的节点路径有重叠，那么可以减少I/O次数。
	* 极端情况下: 求取[left, right]所经过的节点完全一样，I/O次数最少。可以作为一个创新点。
	******/
	pair<int64_t, int64_t> suffix_range2(uint8_t* P, uint64_t m);

	int64_t suffix_range2_left_position(uint8_t* P, uint64_t m, uint64_t offset, bool perfix_of_P);
	int64_t suffix_range2_right_position(uint8_t* P, uint64_t m, uint64_t offset);

	//每次访问suffix_range() 之后，访问getIOCounts()得到求取后缀范围的查询I/O次数。
	uint64_t getIOCounts();

	void collectLocation(pair<int64_t, int64_t> sr, vector<uint64_t>* location = NULL);
};


#endif