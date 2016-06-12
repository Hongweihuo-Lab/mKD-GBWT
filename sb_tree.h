/**
* @Author: kkzone
* @Date: 10/19/2015
* @Desc: The main function of `class sb_tree` is to create a string B-tree and put it into an index-file.
* 		 The main function of `class DiskSBT` is to read the index-file and to offer searching capacity.
**/

#ifndef SB_TREE_H
#define SB_TREE_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <queue>
#include <utility>
#include <math.h>
#include "critbit_tree.h"
#include "sbtmpfile.h"

using std::queue;
using std::pair;

/* 
	Disk Layout description of the string B-tree index file:
	0~4095				: [m_n][bits_per_suffix][bits_per_pos][m_b][m_B][m_height][empty space]
	4096~B+4096			: root disk page (B bytes)
	followed by 		: [SB-tree leaf nodes]
	followed by 		: [rest of the SB-tree internal pages]

	Therefore:	root pages always at file offset 4096.
*/
#define SBT_ROOT_OFFSET  4096

typedef struct{
	uint64_t data[0];
} sb_diskpage_t;


class sb_tree{
 public:
 	uint64_t		m_B;					/* page size */
 	uint64_t		m_b;					/* each node in the tree contains b suffixes at most */
 	uint64_t		m_n;					/* # of suffixes or size of input text */
 	uint8_t*		m_T;
 	uint64_t*		m_sa;
 	uint64_t		m_nsa;
 	uint64_t 		m_step;

 	uint64_t		m_height;				/* height of the SB-tree */
 	uint64_t		bits_per_suffix;		/* bits used per suffix = log2(n) */
 	uint64_t		bits_per_pos;			/* max lcp -> determines the max size of the pos array entries in the critbit-tree */
 	
 	FILE* 			m_fd;					/* open file descriptor of the index */
 	FILE* 			m_textfd;				/* open file descriptor to the text */
 	critbit_tree*	m_root;					/* root node stays in main memory */

 	char* 			m_text_file;
 	char*			m_index_file;				/* delete[] them */
 	char* 			m_lcp_file;
 	char*			m_sa_file;

 	uint64_t   		m_io;					/* 进行一次suffix range查询，所耗费的 I/O次数 */
 
 	uint64_t   		m_blocks;   			/* the number of all page blocks of string B-tree. */

public:
	// constructors
	sb_tree(const char* text_file, uint64_t B, uint64_t step);
	// deconstructor
	~sb_tree();
					

/*************Helper function*******************/
private:
	void build();
	void create_tree(sbtmpfile_t* suffixes, sbtmpfile_t* lcps, FILE* sbt_fd);
	void create_tree_helper(sbtmpfile* suffixes, 
						sbtmpfile* lcps,
						FILE* sbt_fd,
						uint64_t offset, 
						uint64_t level, 
						queue<pair<uint64_t, uint64_t> >* cur_off );

	void create_tree_root(sbtmpfile_t* suffixes, 
						sbtmpfile_t* lcps,
						FILE* sbt_fd,
						uint64_t offset,
						uint64_t level, 
						queue<pair<uint64_t, uint64_t> >* cur_off);

	/* calcualting the SB-Tree height */
	uint64_t calc_height(){
		return (uint64_t) (log(this->m_n) / log(this->m_b)) + 1;
	}

	uint64_t size_in_bytes(){
		return m_blocks * m_B + SBT_ROOT_OFFSET;
	}		

	/* file I/O */
	void write_header(FILE* out);
	void read_header(FILE* in);
	void add_padding(FILE* out, uint64_t bytes);   /*write bytes into (FILE* out)*/


private:
	/**
	* 1.) 通过SA的计算Rank数组，然后计算所有后缀的LCP的值。
	* 2.) 通过LCP数组，计算那些相邻的m_sa[i]%m_step==0的那些后缀之间的LCP的值.(直接取它们之间的最小值即可.)
	* 3.）相邻的满足 m_sa[i]%m_step==0，的i1 和 i2，可能相距很远，也有可能是直接相邻的(i1 + 1 == i2). 所以求取最小的LCP值时要小心。
	* 4.) 将采样过后的 LCP 数组，写入到文件中，在构造critbit-tree的时候使用。
	* 5.) 返回这些采样的LCP数组中的最大的lcp的值。用以计算一个磁盘块最多可以存储多少个后缀。
	* 6.) 求取LCP的信息非常重要，可以极大的加快构建一棵critbit_tree的过程，否则构建SBT的时间将变得不可忍受。
	**/
	uint64_t get_max_lcp();
	uint64_t get_max_lcp02();
	
	uint64_t calc_branch_factor();

	uint64_t calc_branch_factor_with_pointer();
	uint64_t calc_branch_factor_without_pointer();

	uint64_t calc_branch_factor_with_maxlcp();
	uint64_t calc_branch_factor_without_maxlcp();
};

#endif // SB_TREE_H