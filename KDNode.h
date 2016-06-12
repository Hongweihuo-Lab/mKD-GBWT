#ifndef INCLUDED_KDNODE_H
#define INCLUDED_KDNODE_H

#include "rect.h"

#include <cstdint>
#include <cstddef>
#include <utility>		// make_pair(), pair<>
#include <vector>		// vector

#include "point.h"

using std::pair;
using std::vector;

/**
*KD-树的一个节点
**/

class KDNode {

public:

	static uint64_t 	m_step;

	uint64_t 			m_x;
	uint64_t 			m_y;
	bool 				m_split;  // m_split= ture分割轴为x轴，否则为y轴

	Rect 				m_rect;   // the bounding box of the node.
	int32_t				m_depth;  // the depth of the node.

	KDNode*				m_leftChild;
	KDNode* 			m_rightChild;

	bool				m_PointNode;			// is the point node.
	vector<pair<uint64_t, uint64_t> >* m_point;	// the point of the Node.
	vector<uint64_t>* m_samSA;					// the sa value of the point

	void*						m_point2;
	uint64_t*					m_samSA2;
	pair<uint64_t, uint64_t> 	m_range;

	/** This is for KDNode that recovery from disk **/
	bool   				m_disk;			// 判断是否位于磁盘边界，如果位于边界那么孩子节点是磁盘号。 
	int32_t 			m_leftNum;		// the disk number of left 	child
	int32_t 			m_rightNum;		// the disk number of right child

	int32_t 			m_shapeValue;

	int32_t				m_level;

public:
	
	//构造函数, create a internal node.
	KDNode(uint64_t x, uint64_t y, int32_t depth, Rect r);
	//construct a point node.
	KDNode(vector<uint64_t>& vx, vector<uint64_t>& vy, pair<uint64_t, uint64_t> range, int32_t depth, uint64_t* sa);

	KDNode(void* point, pair<uint64_t, uint64_t> range, int32_t depth, uint64_t* sa);

	~KDNode();

	uint64_t getX();
	uint64_t getY();
	void setX(uint64_t x);
	void setY(uint64_t y);
	void set(uint64_t x, uint64_t y);

	bool getSplit();
	void setSplit(bool split);
	bool isLeaf();  //判断是否为叶子节点
	// void setLeaf(bool leaf);
	
	KDNode* getLeftChild();
	KDNode* getRightChild();
	void 	setLeftChild(KDNode* left);
	void 	setRightChild(KDNode* right);

	Rect 		getRect();
	int32_t 	getDepth();
	uint64_t	getSplitValue();

	bool 						isPointNode();  // return the m_PointNode
	uint64_t					getPointNum();
	pair<uint64_t, uint64_t> 	getPoint(uint64_t i);

	uint64_t 	getSaValue(uint64_t i);

	int32_t		getShapeValue();
	int32_t		setShapeValue(int32_t value);

	int32_t		getLevel();
	void 		setLevel(int32_t level);

};



#endif

