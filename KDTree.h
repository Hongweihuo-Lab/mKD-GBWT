#ifndef INCLUDED_KDTREE_H
#define INCLUDED_KDTREE_H

#include "KDNode.h"
#include "rect.h"
#include "KDDisk.h"
#include "KDPointDisk.h"
#include "KDTreeDiskHeader.h"
#include "KDVirtualDisk.h"

#include "point.h"

#include <vector>
#include <utility>
#include <stdint.h>

using std::vector;
using std::pair;

/**
构建一颗KD树的时间开销是 O(nlogn)，主要的时间耗费在了查找中位数上面。
采用中位数将空间切割，可以把KDtree构建为一颗平衡树
**/

class KDTree{

public:

	static uint64_t	m_step;

	static int32_t	m_B;			// the size the disk page in bytes. 
	static int32_t	m_xBitWidth;	// the bitwise width of x point.
	static int32_t	m_yBitWidth;	// the bitwise width of y point.

	static int32_t	m_splitValueBitWidth;	// the bitwise width of a split value.
	static int32_t	m_numPoint;				// the number of points that can be store in a diks page.
	static int32_t  m_F;					// the number of out degeree of a disk.


	KDNode*			m_root;					//KD-树的跟节点
	Rect 			m_rect;					//KD-tree's Bounding Box.

	KDTreeDiskHeader	m_header;

	uint64_t 		m_n;			//the number of point;
	uint64_t 		m_numRoot;		//根节点中所存储的一棵树中叶子节点的个数。	
	uint64_t 		m_numDisk;		//一个磁盘页中可以存储的根节点个数.

	uint64_t		*m_sa;			//采样的后缀数组	

public:

	static uint64_t  n_PointNode;  //磁盘上的KD-树上的存储点的磁盘页的个数
	static uint64_t  n_Point;	   //存储到磁盘上的点的个数。
	static uint64_t  n_AllNode;	   //所有的磁盘页的个数。

	static uint64_t  n_unFullPointNode;  	//非满的叶子节点磁盘页个数
	static uint64_t  n_unFullInternalNode;  //非满的内部节点磁盘页个数

public:
	//构造函数
	KDTree(vector<uint64_t>& vx, vector<uint64_t>& vy, uint64_t* sam_sa = NULL);  
	KDTree(void* point, uint64_t* sam_sa = NULL);

	//析构函数
	~KDTree();
	void removeTree(KDNode* root);

	/*****
	*范围查找：
	*	找到所有的点(x,y)位于(px.first, px.second, py.first, py.second)
	*@param px   x轴的范围是[px.first, px.second]
	*@param py   y轴的范围是[py.first, py.second]
	*******/
	vector<pair<int, int> >* locate(pair<int, int>& px, pair<int, int>& py);

	/****
	* Count 查询
	* 	返回出现在[px.first, px.second], [py.first, py.second]之间的点的个数
	*	@param px  x轴的范围是[px.first, px.second]
	*   @param py  y轴的范围是[py.first, py.second]
	***/
	int count(pair<int, int>& px, pair<int, int>& py);

	
	KDNode* getRoot();


public:
	/****
	* Construct a KDTree and return its root node.
	* Note: 点集是(vx[i], vy[i]),for range.first <= i <= range.second
	* @param depth   If depth is odd, the split is x, if the depth is even, the split is y
	***/
	KDNode* construct(vector<uint64_t>& vx, 
					vector<uint64_t>& vy,
					pair<uint64_t, uint64_t> range,
					int32_t depth,
					Rect r);

	KDNode* construct(void* point,
					pair<uint64_t, uint64_t> range,
					int32_t depth,
					Rect r);

	/*****
	* Description: Get the index of the median in vector v[range.first, rane.second]
	* Note: we need to split the vector v[range.first, range.second] into two parts.
	*		Let i be the split position, and the median is x.
	*		The first part of v[range.first, range.second] is v[range.first, i] of which
	*		all value of it is less or equal than x.
	*		
	*		The second part is v[i+1, range.second] whose all the values is larger than x.
	*		
	*		When depth is odd, choose vx be the splited vector, otherwise choose vy.
	*
	*@return   The index i in the range [range.first, range.second] which split the vector into two
	*			parts. The value in the v[range.first, i] is less than or equal to the median x.
	*			The value in the v[i+1, range.second] is larger than the median x.
	*****/
	uint64_t getMedian(vector<uint64_t>& vx, vector<uint64_t>& vy, pair<uint64_t, uint64_t> range, int32_t depth);
	
	uint64_t getMedian_nthE(vector<uint64_t>& vx, vector<uint64_t>& vy, pair<uint64_t, uint64_t> range, int32_t depth);
	uint64_t getMedian_nthE(void* point, pair<uint64_t, uint64_t> range, int32_t depth);	

	// There are many methods to get the median.
	// So realize them.
	uint64_t partition(vector<uint64_t>& vx, vector<uint64_t>& vy, pair<uint64_t, uint64_t>range, int32_t depth);

	/****
	* @param kth  The value of kth is in the range[1 .. range.second-range.first+1]
	* @return the index in the range, that the range is the kth smallest value.
	****/
	uint64_t quickSelect(vector<uint64_t>& vx, vector<uint64_t>& vy, pair<uint64_t, uint64_t>range, int32_t depth, uint64_t kth);


	void locate(Rect& qrect, 
				KDNode* root, 
				Rect nrect,
				int depth, 
				vector<pair<int, int> >* result);

	//访问以给定节点为根的所有孩子节点。
	void locateAllChild(KDNode* root, vector<pair<int, int> >* result);   

	bool isInRange(pair<int, int>& px, pair<int, int>& py, int x, int y);
	
	// 建立好KD-树之后将 KD-数存储到磁盘上。
	int32_t SaveToDisk(DiskFile* df);	

	void SaveToDiskFile(const char* name);
	
	KDVirtualDisk* SaveNodeToDisk(KDNode* root, bool isRoot, DiskFile* diskOut);


	/**在调用KDTree的构造函数之前，调用这个函数**************
	* @param B			: The size of disk page in bytes. 
	* @param xBitWidth	: The bitwise width of a x point value
	* @param yBitWidth  : The bitwise width of a y point value
	**********************************************************/
	static void Set(int32_t B, int32_t xBitWidth, int32_t yBitWidth, int32_t SABitWdith);
	static void SetStep(uint64_t step);

	uint64_t numOfNodeInDiskRoot();

	uint64_t getTreeHeight();
	uint64_t getTreeHeightHelper(KDNode* root);

	int32_t SaveToDisk_2(DiskFile* df);
	void SaveToDiskFile_2(const char* name);
	KDVirtualDisk* SaveNodeToDisk_2(KDNode* root, bool isRoot, DiskFile* diskOut);
};



#endif