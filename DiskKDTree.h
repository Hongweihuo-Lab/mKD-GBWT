#ifndef INCLUDED_DISKKDTREE_H
#define INCLUDED_DISKKDTREE_H

#include "rect.h"
#include "DiskKDNode.h"
#include "KDTreeDiskHeader.h"

#include <utility>  //pair
#include <cstdint>  //int32_t
#include <vector>	//std::vector

/**************
* 从磁盘上加载KDTree的节点，支持正交范围查找，返回找到的所有的点。
**************/

struct Point_XYSA{
public:	
	uint64_t x;
	uint64_t y;
	uint64_t sa;

	Point_XYSA(uint64_t a, uint64_t b, uint64_t c): x(a), y(b), sa(c){}

};

class DiskKDTree{

public:
	
	char*				m_fileName;	// name of the file storing the KDTree
	int32_t 			m_B;		// size of a disk page(in bytes)
	int32_t				m_rootNum;	// the number of disk page that stores the root node.
	
	KDTreeDiskHeader	m_header;	// Read Header.
	
	DiskKDNode*			m_root;		// the root.
	bool				m_getRoot;	// if persist root in memory

	FILE*				m_in;		// IO Stream

	uint64_t			n_PointNode;  //每一次进行Locate,统计访问的PointNode的个数。
	uint64_t 			n_Point;

	uint64_t 			m_IOCounts;	//统计进行一次Locate 所需要的I/O次数。

public:

	/********构造函数******
	*@param getRoot		getRoot为真表示根节点长久驻留在内存中。否则每次正交范围查找都要加载根节点。
	*@param B			the size of disk page (in bytes)
	*@param sbtFileName	存储 KDTree的文件名。
	***********************/
	DiskKDTree(FILE* in, int32_t rootNum, int32_t B);

	//析构函数
	~DiskKDTree();

	/*******正交范围查找*************
	* 返回查找到的所有的点，并且统计I/O次数。
	* @return: 查找到的所有的点。
	*********************/
	std::vector<Point_XYSA>* Locate(Rect r);
	void locate(Rect r, DiskKDNode* root, std::vector<Point_XYSA>* result);
	void locate_helper(Rect r, DiskKDNode* root, std::vector<Point_XYSA>* result);

	/***从磁盘加载一个节点，并且恢复成DiskKDNode****
	*
	*******************************************/
	DiskKDNode* getNodeFromDisk(int32_t diskNum);

	void readHeaderFromDisk();

	uint64_t getIOCounts();

	uint64_t sizeInBytes();
};


#endif
