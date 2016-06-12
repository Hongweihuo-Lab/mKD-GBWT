#include "KDNode.h"

#include <iostream>
#include <cassert>

using std::make_pair;
using namespace std;


uint64_t KDNode::m_step = 0;

//构造函数
KDNode::KDNode(uint64_t x, uint64_t y, int32_t depth, Rect r) : m_rect(r){
	m_x = x;
	m_y = y;
	m_split = (depth % 2 == 1) ? true :false;
	m_depth = depth;
	m_PointNode = false;		// It is not a point node.
	m_leftChild = NULL;
	m_rightChild = NULL;

	m_shapeValue = -1;
}



/***** Create a KDNode storing the points.**
*
********************************************/
KDNode::KDNode(vector<uint64_t>& vx, 
			vector<uint64_t>& vy, 
			pair<uint64_t, uint64_t> range, 
			int32_t depth,
			uint64_t* sam_sa): m_rect(0, 0, 0, 0){

	m_split = (depth % 2 == 1) ? true : false;
	m_depth = depth;
	m_leftChild = NULL;
	m_rightChild = NULL;

	m_shapeValue = -1;

	m_PointNode = true;
	m_point = new vector<pair<uint64_t, uint64_t> >();
	m_samSA = new vector<uint64_t>();

	for(uint64_t i = range.first; i <= range.second; i++){
		m_point->push_back(make_pair(vx[i], vy[i]));	//push back point
		m_samSA->push_back(sam_sa[vx[i]]);				//push back SA value of point
	}
}

KDNode::KDNode(void* point,
			pair<uint64_t, uint64_t> range,
			int32_t	depth,
			uint64_t* sam_sa): m_point2(point), m_rect(0, 0, 0, 0){

	m_split = (depth % 2 == 1) ? true : false;
	m_depth = depth;
	m_leftChild = NULL;
	m_rightChild = NULL;

	m_shapeValue = -1;

	m_PointNode = true;
	m_point = NULL;
	m_samSA = NULL;

	m_samSA2 = sam_sa;
	m_range = range;	

}

//deconstructor.
KDNode::~KDNode(){
	if(isPointNode()){
		if(m_point != NULL)
			delete m_point;
		if(m_samSA != NULL)
			delete m_samSA;
	}
}

uint64_t KDNode::getX(){return m_x;}
uint64_t KDNode::getY(){return m_y;}

void KDNode::setX(uint64_t x) { m_x = x;}
void KDNode::setY(uint64_t y) { m_y = y;}
void KDNode::set(uint64_t x, uint64_t y) { m_x = x; m_y = y;}

bool KDNode::getSplit() {return m_split;}
void KDNode::setSplit(bool split) { m_split = split;}

bool KDNode::isLeaf() {return ((m_leftChild == NULL) && (m_rightChild == NULL));}

KDNode* KDNode::getLeftChild() {return m_leftChild;}
KDNode* KDNode::getRightChild() {return m_rightChild;}

void KDNode::setLeftChild(KDNode* left) { m_leftChild = left;}
void KDNode::setRightChild(KDNode* right) {m_rightChild = right;}


Rect 		KDNode::getRect() { return this->m_rect;}
int32_t 	KDNode::getDepth(){ return this->m_depth; }

uint64_t KDNode::getSplitValue() {
	assert(!isPointNode());
	if(m_split) {
		return m_x;
	} else {
		return m_y;
	}
}


bool KDNode::isPointNode() {return m_PointNode;}

uint64_t KDNode::getPointNum() {
	if(!isPointNode()){
		assert(false);
	}
	return m_range.second - m_range.first + 1;
}

pair<uint64_t, uint64_t> KDNode::getPoint(uint64_t i){
	if(!isPointNode() || i >= getPointNum()){
		assert(false);
	}
	uint64_t idx = m_range.first + i;
	uint64_t x = pointGetX(m_point2, m_step, idx);
	uint64_t y = pointGetY(m_point2, m_step, idx);
	pair<uint64_t, uint64_t> p(x, y);

	return p;
}

uint64_t KDNode::getSaValue(uint64_t i){
	if(m_samSA2 == NULL || i >= getPointNum())
		assert(false);

	pair<uint64_t, uint64_t> p = getPoint(i);
	uint64_t res = m_samSA2[p.first];

	return res;
}

int32_t KDNode::getShapeValue() { return m_shapeValue;}

int32_t KDNode::setShapeValue(int32_t value) {
	int32_t result = m_shapeValue;
	m_shapeValue = value;
	return result;
}

int32_t KDNode::getLevel() {return m_level;}
void KDNode::setLevel(int32_t level){
	this->m_level = level;
}

/******
1. 当我想设定一个成员函数为 inline的时候，inline应该放在哪个地方。
2. 如何判断叶子节点：
	这里有两种方式，第一种是设定一个标志位，表示它是否是叶子节点
	第二种：若左右子树同时为空，那么它也是叶子节点。

如果采用标志位的方法，那么会出现一致性的问题：如果你把一个标志位设置成了true，表示这是一个叶子节点，但是同时左右孩子不为空，应该怎么办?

这就是一致性问题，因为冗余才会引起一致性的问题。
在分布式的环境中，冗余是必须的，比如说每个node上都有一个 cache data。如何保证这些data是一致性的呢？

*******/