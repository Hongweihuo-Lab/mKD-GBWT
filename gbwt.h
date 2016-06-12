#ifndef INCLUDED_GBWT_H
#define	INCLUDED_GBWT_H

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cstdint>

#include "sb_tree.h"
#include "KDTree.h"
#include "Suffix.h"
#include "point.h"

/******
* GBWT索引由两部分构成，第一部分是String B-tree,第二个部分是KDB-tree.
*******/



class GBWT {

public:
	char	*m_textFile;	//文本文件.
	char	*m_sbtFile;		//存储string B-tree的文件.
	char	*m_kdtFile;		//存储KD-tree的文件。

	char	m_sa_file[256];

	uint8_t		*m_T;			//文本.
	uint64_t 	 m_n;			//文本的长度。

	uint64_t	 m_g;			//采样后缀数组的长度。
	uint64_t	*m_sa;			//采样的后缀数组

	uint64_t	 m_step;		//采样步长。

	uint64_t	 m_B;			//disk page的大小。

	sb_tree		*m_sbt;			//string B-tree，将数据存储到磁盘上。
	// KDTree 		*m_kdt;			//KD-Tree 将数据存储到磁盘上
	
	Suffix 		m_suf[256];		//存储每个字符的信息。
	int32_t		m_nChar;		//文本中不相同的字符个数
	uint8_t		m_char[256];
	uint32_t	m_CharIdx[256];	

	DiskFile	*m_diskOut;		//向磁盘上写入数据。

//counting information
	char*		m_resFile;
	FILE 		*m_out;			//write result into result file.
	uint64_t 	m_points;		//the number of points of GBWT
	uint64_t	m_maxPoints;    // the max nubmer of points of each kd-tree.

public:
	

	GBWT(const char* textFile, uint64_t B, uint64_t step);
	~GBWT();


	uint64_t  sa_count(const char* sa_file);
	uint64_t* sa_create(const char* sa_file);

	// void create_point(std::vector<uint64_t>* coord_x, std::vector<uint64_t>* coord_y);
	void create_point(void* point);
	void save_point(std::vector<uint64_t>* coord_x, std::vector<uint64_t>* coord_y);

	void suf_create();
	void create_MultiKDT(std::vector<uint64_t>* coord_x, std::vector<uint64_t>* coord_y);
	void create_MultiKDT(void* p);

	/***
	* 将点集(vx, vy)中的点，按照首字母charx 和 chary进行划分
	* 划分之后的点存储在px和py中，此时已经对点进行了处理。
	***/
	void get_point(vector<uint64_t>* vx, vector<uint64_t>* vy,
				vector<uint64_t>* px[][256], vector<uint64_t>* py[][256]);
	void get_point(void* vp, void* mp[][256]);

	int32_t writeMultiKDTHeader();

	int64_t writePadding(uint64_t bytes, FILE* out);
};


//存储每个字符的信息：后缀范围 + 以这个字符为首字母的磁盘页



#endif
