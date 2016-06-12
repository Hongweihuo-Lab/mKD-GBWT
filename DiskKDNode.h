	#ifndef INCLUDED_DISKKDNODE_H
#define INCLUDED_DISKKDNODE_H


#include "rect.h"

#include <cstdint>
#include <cstddef>
#include <utility>
#include <vector>

/*************************
* 从磁盘中加载一个磁盘页，恢复成DiskKDNode
* 1. 一个磁盘页中可能只存储了点，恢复成的DiskKDNode也存储点。
* 2. 一个磁盘页中可能存储了一棵树，恢复成的DiskKDNode也是一棵树。
* 3. DiskKDNode 只提供最基本的二叉树的节点操作。
* 4. 之所以不重用 KDNode 是因为二者是有区别的，如果重用KDNode 将使得KDNode中一些关系变得复杂。
* 5. DiskKDNode有三种类型的节点: 内存中的二叉树；存储点；在磁盘上的节点。
*************************/

class DiskKDNode{

public:

	uint64_t		m_value;
	bool			m_split;	// m_split == true 表示以x轴为分割轴。

	Rect			m_rect;		// the bounding box of the node
	int32_t			m_depth;	// the depth of the node.(根节点的depth是1)

	DiskKDNode*		m_leftChild;
	DiskKDNode*		m_rightChild;

	bool			m_pointNode;	//is the point node.(此时需要保证左右孩子为空)
	std::vector<std::pair<uint64_t, uint64_t> >* m_point; //the point of the node.
	std::vector<uint64_t>* m_sa;
	
	/**** This is for disk node that recovry from disk  **
	* 表示以这个节点为根的子树存在于磁盘中，需要从磁盘中加载相应的磁盘也并且恢复。
	******************************************************/
	bool			m_isDisk;		
	int32_t			m_diskNum;		//次节点为根的子树所在的磁盘页的页号。

	int32_t 		m_treeShape;	//用于KDDisk中恢复树形的时候。

public:
	/*构造函数: 内存中的二叉树节点和磁盘上的节点 */
	DiskKDNode(uint64_t value, int32_t treeShape, bool isDisk = false);

	/*构造函数：只存储点集*/
	DiskKDNode(uint64_t nPoint, bool isPointNode = true);

	/*析构函数*/
	~DiskKDNode();
	
	/*得到分裂值*/
	uint64_t 	getSplitValue() { return m_value; }

	/*设置矩形*/
	void 		setRect(Rect r);
	Rect 		getRect();

	void 		setDepth(int32_t depth);
	int32_t 	getDepth();

	bool 		getSplit();

	void 		setLeftChild(DiskKDNode* left);
	void 		setRightChild(DiskKDNode* right);

	DiskKDNode* getLeftChild();
	DiskKDNode* getRightChild();

	/*设置树形*/
	int32_t 	getTreeShape();
	int32_t 	setTreeShape(int32_t treeShape);

	/*存储点集*/
	bool 		isPointNode();
	uint64_t 	getPointNum();

	std::pair<uint64_t, uint64_t> getPoint(uint64_t idx);
	void 		setPoint(uint64_t idx, uint64_t x, uint64_t y);
	
	uint64_t 	getSA(uint64_t idx);
	void		setSA(uint64_t idx, uint64_t saValue);

	/*磁盘上的点*/
	bool 		isDiskNode();
	int32_t 	getDiskNum();

	/*释放掉以root为根的整棵二叉树*/
	static void RemoveTree(DiskKDNode* root);
};



#endif
