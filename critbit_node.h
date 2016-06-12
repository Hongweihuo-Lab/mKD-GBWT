/**
* Author: kkzone
* Date: 10/18/2015
* Description: critbit_node_t, 
**/

#ifndef CRITBIT_NODE_H
#define CRITBIT_NODE_H

#include <stdint.h>
#include <assert.h>


/*** NOTE: 
*			1.) the significant(highest) position of child[0](left child) decide if the node is leaf node.
*				If the significant position of node r's left child(child[0]) is 1, it is leaf node. otherwise is not.
*				
*			2.) the child[]: in the internal node, it is its two children. 
*							 对于critbit-tree的叶子节点，  child[0] 表示一个后缀值(但是最高位标识为1，表明是一个叶子节点).
*															child[1] 表示这个叶子节点的index值。
*
*			3.) For internal node, child[0] and child[1] is it's left and right child.
*				For leaf     node, child[0] is suffix, child[1]即右孩子表示这个叶子节点在这颗critbit-tree的所有叶子节点中的index值,
				不用因为critbit-tree的所有的叶子节点都指向一个后缀，而且都是有序的。我们需要知道一个叶子节点在这些所有的叶子节点的中的
				位置，这样在串B-tree中进行查找的时候通过这个index值可以知道，孩子节点的位置，以及后缀范围，是至关重要的。
*
*			4.) The critpos: (critpos >> 3)表示需要比较的那个字符，(critpos & 0x7)表示这个字符中的第几个bit(从最高位开始数)。
							 当在critbit-tree中进行查找的时候，需要求模式P的某个字符ch的critpos值，如果这个值是0，走左孩子这条
							 道路，如果这个值是1，需要走右孩子这条道路。
							 The critpos of leaf node is zero.
*
*			5.) collect the leaf node from left to right is the suffix array. So the leaf ndoe are 
*				lexocically order sorted.
***/

#define CRITBIT_LEFTCHILD 0
#define CRITBIT_RIGHTCHILD 1

class critbit_node;
typedef critbit_node critbit_node_t;

class critbit_node {
private:
	uint64_t critpos;				 /* Position of the critibit bit */
	critbit_node* child[2];  /* Its two child For internal node*/	

public:
	void set_suffix(uint64_t sufpos){
		this->child[CRITBIT_LEFTCHILD] = (critbit_node_t*)(sufpos | 0x8000000000000000);
	}

	void set_index(uint64_t index){
		this->child[CRITBIT_RIGHTCHILD] = (critbit_node_t*)index;
	}

	uint64_t get_suffix(){
		assert(is_leaf());
		return (((uint64_t)(this->child[CRITBIT_LEFTCHILD])) & (~0x8000000000000000));
	}

	uint64_t get_index(){
		assert(is_leaf());
		return (uint64_t)(this->child[CRITBIT_RIGHTCHILD]);
	}

	bool is_leaf(){
		return ((uint64_t)(this->child[CRITBIT_LEFTCHILD]) & 0x8000000000000000) >> 63;
	}

	void set_critpos(uint64_t pos){
		this->critpos = pos;
	}

	void set_critpos(uint64_t diffChar, uint64_t diffPos) {
		this->critpos = ((diffChar << 3) | diffPos);
	}

	uint64_t get_critpos(){
		return this->critpos;
	}

	uint64_t get_char_len(){
		return (this->critpos >> 3);
	}

	uint64_t get_diff_pos(){
		return (this->critpos & 0x7);
	}

	critbit_node_t* get_left_child(){
		assert(!is_leaf());
		return this->child[CRITBIT_LEFTCHILD];
	}

	critbit_node_t* get_right_child(){
		assert(!is_leaf());
		return this->child[CRITBIT_RIGHTCHILD];
	}

	critbit_node_t* get_child(uint64_t direction){
		assert(!is_leaf());
		return this->child[direction];
	}

	void set_left_child(critbit_node_t* lchild){
		this->child[CRITBIT_LEFTCHILD] = lchild;
	}

	void set_right_child(critbit_node_t* rchild){
		this->child[CRITBIT_RIGHTCHILD] = rchild;
	}

	void set_child(uint64_t direction, critbit_node_t* val){
		this->child[direction] = val;
	}

};






















#endif