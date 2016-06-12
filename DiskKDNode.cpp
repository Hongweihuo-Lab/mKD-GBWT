#include <cassert>

#include "DiskKDNode.h"


using namespace std;

DiskKDNode::DiskKDNode(uint64_t value, int32_t treeShape, bool isDisk) {
	m_treeShape = treeShape;
	m_isDisk = isDisk;
	if(isDisk) {
		// 此节点为根的子树存储在磁盘上，value代表磁盘页的页号。
		m_diskNum = value;
	} else {
		//KD数内部节点，value 表示节点的分裂值。
		m_value = value;
	}
	
	m_leftChild = NULL;
	m_rightChild = NULL;

	m_pointNode = false;
	m_point = NULL;
	m_sa = NULL;
}

DiskKDNode::DiskKDNode(uint64_t nPoint, bool isPointNode) {
	assert(isPointNode == true);
	m_pointNode = isPointNode;
	m_point = new vector<pair<uint64_t, uint64_t> >();
	m_sa = new vector<uint64_t>();

	m_treeShape = -1;
	m_isDisk = false;
	m_diskNum = -1;
	m_value = 0;
	m_leftChild = NULL;
	m_rightChild = NULL;
}

DiskKDNode::~DiskKDNode() {
	//析构函数，回收点集所占用的空间。
	if(m_pointNode && m_point != NULL) {
		delete m_point;
		m_point = NULL;
	}
	if(m_pointNode && m_sa != NULL){
		delete m_sa;
		m_sa = NULL;
	}
}

void DiskKDNode::setRect(Rect r) {
	m_rect = r;
}

Rect DiskKDNode::getRect() {
	return m_rect;
}

void DiskKDNode::setDepth(int32_t depth) {
	m_depth = depth;
	if(m_depth % 2 == 1)
		m_split = true;
	else 
		m_split = false;
}

int32_t DiskKDNode::getDepth() {
	return m_depth;
}

bool DiskKDNode::getSplit() {
	return m_split;
}

void DiskKDNode::setLeftChild(DiskKDNode* left) { m_leftChild = left; }
void DiskKDNode::setRightChild(DiskKDNode* right) { m_rightChild = right; }

DiskKDNode* DiskKDNode::getLeftChild() { return m_leftChild; }
DiskKDNode* DiskKDNode::getRightChild() { return m_rightChild; }

	
int32_t DiskKDNode::getTreeShape() {
	return m_treeShape;
}

int32_t DiskKDNode::setTreeShape(int32_t treeShape) {
	int32_t result = m_treeShape;
	m_treeShape = treeShape;
	return result;
}

bool DiskKDNode::isPointNode() {
	return m_pointNode;
}

uint64_t DiskKDNode::getPointNum() {
	assert(isPointNode() && m_point != NULL);
	return m_point->size();
}

pair<uint64_t, uint64_t> DiskKDNode::getPoint(uint64_t idx) {
	assert(isPointNode() && m_point != NULL);
	return (*m_point)[idx];
}

void DiskKDNode::setPoint(uint64_t idx, uint64_t x, uint64_t y) {
	assert(m_pointNode == true && m_point != NULL);
	m_point->push_back(make_pair(x, y));
}

uint64_t DiskKDNode::getSA(uint64_t idx){
	assert(m_pointNode == true && m_sa != NULL);
	return (*m_sa)[idx];
}

void DiskKDNode::setSA(uint64_t idx, uint64_t saValue){
	assert(m_pointNode == true && m_sa != NULL);
	m_sa->push_back(saValue);
}


bool DiskKDNode::isDiskNode() {
	return m_isDisk;
}

int32_t DiskKDNode::getDiskNum() {
	assert(m_isDisk == true);
	return m_diskNum;
}

//删除以root为根的二叉树。
void DiskKDNode::RemoveTree(DiskKDNode* root) {

	DiskKDNode* left = root->getLeftChild();
	DiskKDNode* right = root->getRightChild();

	if(left != NULL) {
		RemoveTree(left);
	} 
	if(right != NULL) {
		RemoveTree(right);
	}
	delete root;
}
