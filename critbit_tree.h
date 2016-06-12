/**
* Author: 		kkzone
* Date:   		2015/10/18
* Function: 	从后缀构造crtibit_tree，并且支持critbit_tree的序列化和反序列化。
**/
#ifndef CRITBIT_TREE
#define CRITBIT_TREE

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "critbit_node.h"
#include "sdsl/int_vector.hpp"

using std::pair;
using sdsl::bit_vector;
using sdsl::int_vector;

/**
* critbit_tree: every node of a string B-tree is a critbit_tree
*
* Description: 
*	1) the suffixes to insert could be the sample suffix. so the m_g != n
*	2) m_level: 表示此critbit_tree所表示的串B-tree的节点在串B-tree中的第几层。串B-tree的最底层表示第0层。
*	3) m_child_off:	如果这棵critbit-tree表示串B-tree的内部节点，m_child_off表示这个串B-tree的内部节点的首个孩子所在的磁盘页号。
*					如果这棵critbit-tree标识串B-tree的叶子节点，m_child_off表示这个串B-tree的的叶子节点的SA的值所在的Index，用于计算 后缀范围。
*	4) m_child_num:	如果这棵critbit-tree表示串B-tree的内部节点，m_child_num表示串B-tree的一个节点的孩子节点的个数。
*					如果这棵critbit-tree表示串B-tree的叶子节点，m_child_num 等于0。
**/

class critbit_tree {
public:
	critbit_node_t* m_root;			
	uint64_t 		m_g;					/* the number of suffixes */
	uint8_t*		T;						/* the text of length n */
	uint64_t		n;
	uint64_t 		m_step;					/* 在序列化的时候，每个suffix/m_step, 在反序列化的时候每个suffix*m_step */

	uint32_t		m_child_off; 		
	uint32_t		m_child_num; 			/* the number of child blocks */
	uint32_t 		m_flag; 				/* some flags */
	uint32_t 		m_level; 				/* level in the String B-tree */
	/* 
	*  表示String B-tree中的每个节点中的critbit-tree内部节点的比特lcp的最大值。（用多少个bit可以表示这个最大值.）
	*  这个值其实是在串B-tree中求取得：文本T中任意两个后缀的LCP的最大值，然后求取bit表示，再加加上3bit。
	*  具体过程不用critbit-tree操心。它只是简单的接受 串B-tree给他的具体数目的后缀，然后构造一个critbit-tree，然后写入到一个磁盘页中。
	*  并且保证不会超过一个磁盘快大小。（由String b-tree 来控制，后缀的个数）
	*/
	uint64_t 		m_max_posarray_width;   

public:
	/* create a NULL critbit tree */
	critbit_tree(uint64_t step = 1);
	/* create a critbit tree from text T and suffix array SA */
	critbit_tree(uint8_t* T, uint64_t n, uint64_t* SA, uint64_t nsuf, uint64_t step = 1);
	/* clear all the resources */
	~critbit_tree();
	
	/* insert a suffix  T[sufpos ... n-1] into the critbit_tree*/
	void insert_suffix(uint64_t sufpos);
	/* if the critbit tree has the suffix T[sufpos, ... n-1], delete it. */
	void delete_suffix(uint64_t sufpos);

	/* the size of the critbit_tree (not the size  to be written to the disk.)*/
	uint64_t size_in_bytes();
	/* if the critbit tree contains a suffix that has a prefix of P, return 1,  otherwise return 0 */
	bool contains(const uint8_t* P, uint64_t m);
	/* set the text */
	void set_text(uint8_t* T, uint64_t n);
	/* set the text to be NULL 
	  The critbit_node doesn't has the text. It just use it.*/
	void delete_text();
	void set_step(uint64_t step) {this->m_step = step;}

	/** 
	* get all the suffixes that has a prefix of patten P.
	* (*results) contains all the suffix position, and the return value is the number of the results. 
	**/
	uint64_t suffixes(const uint8_t* P, uint64_t m, uint64_t** result);

	void set_flag(uint32_t flag);
	void set_level(uint32_t level);
	void set_child_offset(uint32_t offset);
	void set_child_num(uint32_t num);

	/**
	* @param P: pattern of length m
	* @param T_filename: read the text from file T_filename (open file, read B bytes data, close file)，字符文本文件存储在磁盘上
	* @param B: the size of disk page (in bytes).
	* @param m_io: 统计得到 left_position 中的 I/O次数。
	* @param LCP： 记录在一颗critbit-tree中进行比较时候的 与模式P 的最大的LCP。在String B-tree中搜索的时候有用。
	* @return:  P在critbit_tree中的位置，用于指导String B-tree 应搜索的孩子结点，或者，当critbit-tree表示String B-tree的根节点时候返回正确的 Suffix-Range的值。
	* 
	* find the position i of P in these critbit_tree, such that T[SA[i]..n] < P <= T[SA[i+1]..n]
	* if every suffix is less than P, return -1
	* if every suffix is larger than P, return -2.
	* because the suffix is ordered, so if the first suffix is equal to P, return -3.
	* 返回-3这种情况只可能出现在根节点，如果不是出现在根节点，在跟节点会返回T[SA[i]..n] < P的 i.
	**/
	int64_t get_left_position(uint8_t* P, uint64_t m, 
							const uint8_t* T_filename, 
							uint64_t B, 
							uint64_t* LCP,
							uint64_t* m_io);

	/**
	* @param P: pattern of length m.
	* @param T_filename: read the text from file T_filename (open file, read B bytes data, close file)
	* @param B: the size of disk page (in bytes).
	* @param m_io: 统计得到 left_position 中的 I/O次数。
	* @param LCP： 记录在一颗critbit-tree中进行比较时候的 与模式P 的最大的LCP。在String B-tree中搜索的时候有用。
	*@return:  P在critbit_tree中的位置，用于指导String B-tree 应搜索的孩子结点，或者，当critbit-tree表示String B-tree的根节点时候返回正确的 Suffix-Range的值。
	*
	* find the position i of P in these critbit_tree, such that T[SA[i]..n] <= P < T[SA[i+1]..n]
	* if every suffix is less than P, return "the last suffix" (because it's children may have suffixes that is larger than P). 
	* if every suffix is larger than P, return the -2.
	* if the last suffix is smaller than or equal than P, return "the last suffix".
	***/
	int64_t get_right_position(uint8_t* P, 
								uint64_t m,
								const uint8_t* T_filename, 
								uint64_t B, 
								uint64_t* LCP,
								uint64_t* m_io);

	/**
	* 同时在一棵critbit-tree上求取 P的 left position 和 right position.
	**/
	pair<int64_t, int64_t> get_left_and_right_position(uint8_t* P, 
													uint64_t m, 
													const uint8_t* T_filename, 
													uint64_t B, 
													uint64_t* m_LCP,
													uint64_t* m_io);	

	/* the crtibit_tree already has T and n, and need to be created some suffixes SA[0..nsuf-1] */
	void create_from_suffixes(uint64_t* SA, uint64_t nsuf);
	/**
	* version 2： 利用LCP信息，加快critbit_tree的创建速度。
	*			  时间复杂度从O(k*n^2)提升到O(k*n)，其中k表示一棵critbit_tree的树高, k=O(B)。
	**/
	void create_from_suffixes(uint64_t* SA, uint64_t* LCP, uint64_t nsuf); 
	void create_from_suffixes2(uint64_t* SA, uint64_t* LCP, uint64_t nsuf);/*the pratical interface that used in String B-tree*/

	/**
	* File I/O.
	* 将critbit_tree序列化到磁盘上(out指向的位置)，返回写入的字节个数
	**/
	uint64_t write(FILE* out);

	/**	
	* 从一棵空的critbit_tree，通过反序列化，得到一棵构造好的critbit_tree. 
	* mem: 	指向内存中的buffer，是从磁盘读入的序列化的信息。 
	* size:	buffer的大小，单位是64bit。
	**/
	void load_from_mem(uint64_t* mem, uint64_t size); 

	void getAllSuffixes(vector<uint64_t>& result);
	void getAllSuffixes_helper(vector<uint64_t>& result, critbit_node_t* locus);

private:
	void test_T();
	
	/* helper function */
	void collect_suffixes(critbit_node_t* locus, uint64_t** result, uint64_t* rsize, uint64_t* count);
	void clear();
	void delete_nodes(critbit_node_t* cur_node);
	
	/* 序列化与反序列化相关操作，将一棵树序列化到磁盘上 */
	bit_vector* create_bp();
	void create_bp_helper(critbit_node_t* cur_node, bit_vector& bv, uint64_t& pos);
	void print_bp(bit_vector& bv);
	void print_bp(uint64_t* mem, uint64_t g);

	int_vector<>* create_posarray();
	void create_posarray_helper(critbit_node_t* cur_node, int_vector<>& pos, uint64_t& p, uint64_t parentpos);

	int_vector<>* create_suffixarray();
	void create_suffixarray_helper(critbit_node_t* cur_node, int_vector<>& SA, uint64_t& pos);

	int_vector<>* create_full_posarray();
	void create_full_posarray_helper(critbit_node_t* cur_node, int_vector<>& pos, uint64_t& p, uint64_t parentpos);

	/*返回后缀T[i..n-1], T[j..n-j]首个 different 的字符，即LCP信息*/
	int64_t find_diff_char(uint64_t i, uint64_t j);
	/*To vertify the correctness*/
	void insert_suffix(uint64_t* SA, uint64_t* LCP, uint64_t idx);
	/*The praticial function that is fast and correct*/
	void insert_suffix2(uint64_t* SA, uint64_t* LCP, uint64_t idx); 

public:
	uint64_t get_max_poswidth();
	/*inline function*/
	uint64_t get_direction(uint8_t ch, uint64_t diffPos) {
	/* the priority of different sysmbles must be declare. */
		return  ( (ch & (1 << (7- diffPos))) >> (7-diffPos));	 /* a little bug make me noisy */
	}

	/* TODO: check  the __builtin_clz( x), the type of the parameter is uint32_t ?*/
	uint64_t get_critbit_pos(uint8_t x, uint8_t y) {
		return ( __builtin_clz(x^y) - ((sizeof(uint32_t) - sizeof(uint8_t))<<3) );
	}
};
#endif
