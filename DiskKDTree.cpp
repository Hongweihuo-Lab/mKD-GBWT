#include <cstring>
#include <vector>	//std::vector
#include <utility> //std::pair
#include <algorithm>
#include <cassert>
#include <cstdio>

#include "DiskKDTree.h"
#include "KDDisk.h"
#include "KDPointDisk.h"
#include "DiskGBWT.h"

using namespace std;

//构造函数
DiskKDTree::DiskKDTree(FILE* in, int32_t rootNum, int32_t B){
	
	m_B = B;
	m_rootNum = rootNum;

	//open the file.
	m_in = in;
	
}

//析构函数
DiskKDTree::~DiskKDTree(){


}


//读取KD-树的文件头
void DiskKDTree::readHeaderFromDisk(){
	int32_t rootNum;
	fseek(m_in, 0, SEEK_SET);
	int32_t bytes = fread(&rootNum, 1, sizeof(int32_t), m_in);
	assert(bytes == sizeof(int32_t));
	m_rootNum = rootNum;
}


DiskKDNode* DiskKDTree::getNodeFromDisk(int32_t diskNum){
	//首先将光标定位到相应的磁盘快: KDTreeDiskHeader
	int64_t position = DiskGBWT::GetPosition(diskNum, m_B);
	fseek(m_in, position, SEEK_SET);
	
	//读取这个磁盘块的内容，并且分析是属于内部节点还是point 节点
	uint8_t* buf = new uint8_t[m_B];
	int32_t bytes = fread(buf, 1, m_B, m_in);
	assert(bytes == m_B);

	uint8_t flag = buf[0];
	DiskKDNode* node;
	if(flag == 0){	
		//如果是内部节点，调用KDDisk恢复成 DiskKDNode
		node = KDDisk::GetNodeFromMem(buf);
	}
	else if(flag == 1) {
		//如果是叶子节点，调用KDPointDisk恢复成 DiskKDNode
		node = KDPointDisk::GetNodeFromMem(buf);
	} else {
		assert(false);
	}

	delete[] buf;
	return node;
}


/*进行正交范围查找*/
vector<Point_XYSA >* DiskKDTree::Locate(Rect r){
	
	n_PointNode = 0;
	n_Point = 0;
	m_IOCounts = 0;

	m_root = getNodeFromDisk(m_rootNum);
	m_IOCounts ++;

	//从根节点进行正交范围查找。
	vector<Point_XYSA>* result = new vector<Point_XYSA>();
	locate(r, m_root, result); //需要在locate中释放资源.
	
	return result;
}

/***************************************
*@root: 	是从一个磁盘页恢复出来的一颗子树 的根节点。
*@r	  : 	正交范围查找的查询矩形。
*@result:	存储结果的点(x, y, sa)。
*
*思路2： 如果仅仅存储叶子节点的矩形，访问的时候仅仅需要查询叶子节点的矩形即可？
*        这样子会不会占据更少的空间？（我觉得这是一个非常好的思路）
***************************************/
void DiskKDTree::locate(Rect r, DiskKDNode* root, vector<Point_XYSA>* result) {
	
	assert(root != NULL);
	if(root->isPointNode()) {
		/*如果root中存储的是<点集>，判断每个点是否在rect中，并且加入到result中*/
		uint64_t nPoint = root->getPointNum();
		pair<uint64_t, uint64_t> p;
		uint64_t saVal;
		for(uint64_t i = 0; i < nPoint; i++){
			p = root->getPoint(i);
			saVal = root->getSA(i);
			if(r.isInRange(p.first, p.second)){
				result->push_back(Point_XYSA(p.first, p.second, saVal));
			} 
		}
		DiskKDNode::RemoveTree(root); //释放资源。
		
		n_PointNode++;	//进行统计
		n_Point += nPoint;

		return;
	}

	//对以root为根节点的子树进行正交范围查找
	locate_helper(r, root, result);
	DiskKDNode::RemoveTree(root);    //释放资源。		
}

void DiskKDTree::locate_helper(Rect r, DiskKDNode* root, vector<Point_XYSA>* result) {
	
	Rect rNode = root->getRect();  //需要知道每个节点所代表的矩形。
	
	if(r.isIntersected(rNode)) {
		//两个矩形有交集，分别查询root的孩子节点。
		if(root->isDiskNode()) {
			/*这个节点存储在磁盘上，从磁盘上加载这个节点*/
			DiskKDNode* aroot = getNodeFromDisk(root->getDiskNum());
			m_IOCounts ++;
			locate(r, aroot, result);
			return;
		}

		DiskKDNode *left, *right;
		left = root->getLeftChild();
		right = root->getRightChild();
		if(left != NULL){
			locate_helper(r, left, result);
		}
		if(right != NULL){
			locate_helper(r, right, result);
		}
	} 
}


uint64_t DiskKDTree::getIOCounts(){
	return m_IOCounts;
}


uint64_t DiskKDTree::sizeInBytes(){
	
	FILE* fd = fopen(m_fileName, "r");
	if(!fd){
		fprintf(stderr, "DiskKDTree::sizeInBytes--Open File '%s' Error!\n",m_fileName);
		exit(EXIT_FAILURE);
	}
	fseek(fd, 0, SEEK_END);
	uint64_t size =  ftell(fd);
	fclose(fd);
	return size;
}