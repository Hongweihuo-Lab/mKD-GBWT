#include "KDTree.h"

#include <cassert>
#include <limits>
#include <utility>
#include <cstddef>
#include <queue>
#include <algorithm>	//max, nth_element
#include <cstdio>
#include <tgmath.h>		//for log2()
#include <math.h>

using namespace std;

uint64_t KDTree::m_step = 0;

int32_t KDTree::m_B = -1;
int32_t KDTree::m_xBitWidth	= -1;
int32_t KDTree::m_yBitWidth = -1;
int32_t KDTree::m_splitValueBitWidth = -1;
int32_t KDTree::m_numPoint = -1;
int32_t KDTree::m_F = -1;

uint64_t KDTree::n_PointNode = 0;
uint64_t KDTree::n_Point = 0;
uint64_t KDTree::n_AllNode = 0;

uint64_t KDTree::n_unFullPointNode = 0;
uint64_t KDTree::n_unFullInternalNode = 0;

void KDTree::SetStep(uint64_t step){
	m_step = step;
	KDNode::m_step = step;
}

void KDTree::Set(int32_t B, int32_t xBitWidth, int32_t yBitWidth, int32_t SABitWidth){
	m_B = B;
	m_xBitWidth = xBitWidth;
	m_yBitWidth = yBitWidth;
	m_splitValueBitWidth = std::max(m_xBitWidth, m_yBitWidth);

	//调用KDPointDisk的静态函数，进行全局设置。
	KDPointDisk::Set(B, xBitWidth, yBitWidth, SABitWidth);
	KDDisk::Set(B, m_splitValueBitWidth);
	//一个disk page中最多可以存储的点的个数。
	m_numPoint = KDPointDisk::GetMaxPoint();
	m_F = KDDisk::GetMaxFanout();
}

//析构函数
KDTree::~KDTree(){
	//递归调用删除整颗树在内存中的节点。
	removeTree(m_root);
}

void KDTree::removeTree(KDNode* root){
	if(root == NULL)
		return;
	if(root->getLeftChild() != NULL)
		removeTree(root->getLeftChild());
	if(root->getRightChild() != NULL)
		removeTree(root->getRightChild());
	
	delete root;
}

KDTree::KDTree(vector<uint64_t>& vx, vector<uint64_t>& vy, uint64_t* sam_sa){
	assert(vx.size() == vy.size());
	if(0 == vx.size()) {  // There is no element in the vector.
		m_root = NULL;  // NULL defined in file <cstddef>
		return;
	}

	m_n = vx.size();
	m_sa = sam_sa;
	// m_numRoot = numOfNodeInDiskRoot();

	//寻找vx，vy中的最大值和最小值，来构造整个KDTree的Rect.
	int64_t n = vx.size();
	uint64_t xmin, xmax, ymin, ymax;
	xmin = xmax = vx[0];
	ymin = ymax = vy[0];
	for(int64_t i = 0; i < n; i++){
		if(vx[i] < xmin)
			xmin = vx[i];
		if(vx[i] > xmax)
			xmax = vx[i];
		if(vy[i] < ymin)
			ymin = vy[i];
		if(vy[i] > ymax)
			ymax = vy[i];
	}
	
	Rect r(xmin, xmax, ymin, ymax);
	m_rect = r;
	//Construct the KDTree recursively
	int32_t depth = 1;
	pair<uint64_t, uint64_t> range;
	range.first = 0;
	range.second = vx.size() - 1;
	
	m_root = construct(vx, vy, range, depth, r);

}// end of member function KDTree(vector<int>& vx, vector<int>& vy) 

KDTree::KDTree(void* point, uint64_t* sam_sa){
	if(0 == pointSize(point, m_step)){
		m_root = NULL;
		return;
	}

	m_n = pointSize(point, m_step);
	m_sa = sam_sa;

	int64_t n = m_n;
	uint64_t xmin, xmax, ymin, ymax;
	xmin = xmax = pointGetX(point, m_step, 0);
	ymin = ymax = pointGetY(point, m_step, 0);
	for(int64_t i = 0; i < n; i++){
		uint64_t x = pointGetX(point, m_step, i);
		uint64_t y = pointGetY(point, m_step, i);

		if(x < xmin)
			xmin = x;
		if(x > xmax)
			xmax = x;
		if(y < ymin)
			ymin = y;
		if(y > ymax)
			ymax = y;
	}
	
	Rect r(xmin, xmax, ymin, ymax);
	m_rect = r;	

	int32_t depth = 1;
	pair<uint64_t, uint64_t> range;
	range.first = 0;
	range.second = m_n - 1;
	
	m_root = construct(point, range, depth, r);	
}

KDNode* KDTree::construct(vector<uint64_t>& vx, 
						vector<uint64_t>& vy,
						pair<uint64_t, uint64_t> range,
						int32_t depth,
						Rect r) {
	
	//如果点的个数可以存放进一个磁盘块中
	if(range.second - range.first + 1 <= this->m_numPoint) {
		//创建一个存放点的block.
		KDNode* root = new KDNode(vx, vy, range, depth, m_sa);
		root->setLevel(-1);
		return root;
	}
	
	// Get the Level of each node.
	double S = range.second - range.first + 1;  // the nubmer of point
	double logF = floor(log(S / (2*m_numPoint)) / log(m_F));
	if(pow(m_F, logF+1) <= (S/(2*m_numPoint))){
		logF = logF + 1;    //对logF的进行矫正，因为采用自然对数进行了计算
	}
	double bF = m_numPoint * pow(m_F, logF);

	double logr = floor(log(S/m_numPoint) / log(m_F));
	if(pow(m_F, logr+1) <= S/m_numPoint){
		logr = logr + 1;
	}
	double bF_r = m_numPoint * pow(m_F, (int64_t)logr);

	int32_t level;
	if(bF_r < S && S < 2*bF_r){
		// level = (int32_t)ceil(log(S / (2*m_numPoint)) / log(m_F));
		level = logF + 1;
	} else {
		// level = (int32_t)floor(log(S / (2*m_numPoint)) / log(m_F));
		level = logF;
	}

	// Get the index of the median number
	uint64_t median = getMedian_nthE(vx, vy, range, depth);
	KDNode* root = new KDNode(vx[median], vy[median], depth, r);
	root->setLevel(level);
	
	// getMedian will  divide the vector into two parts.
	
	// recursively invoke the construct() function
	pair<uint64_t, uint64_t> rangeLeft = std::make_pair(range.first, median);
	pair<uint64_t, uint64_t> rangeRight = std::make_pair(median + 1, range.second);
	
	Rect rLeft 	= r;
	Rect rRight	= r;
	
	if(depth % 2 == 1) {
		rLeft.m_hx = vx[median]; //我写成  = median. 直接错了很多，错误也很难发现。
		rLeft.b_hx = true;
		rRight.m_lx = vx[median];
		rRight.b_lx = true;		//rRight.b_lx = false;
	} else {
		rLeft.m_hy = vy[median];
		rLeft.b_hy = true;
		rRight.m_ly = vy[median];
		rRight.b_ly = true;		//rRight.b_ly = false;
	}

	KDNode* leftChild = construct(vx, vy, rangeLeft, depth + 1, rLeft);
	KDNode* rightChild = construct(vx, vy, rangeRight, depth + 1, rRight);

	// set it's left and right child
	root->setLeftChild(leftChild);
	root->setRightChild(rightChild);
						
	return root;
}// end of member function construct()

bool cmp_x(Point a, Point b){
	return a.m_x < b.m_x;
}

bool cmp_y(Point a, Point b){
	return a.m_y < b.m_y;
}

KDNode* KDTree::construct(void* point,
						pair<uint64_t, uint64_t> range,
						int32_t depth, 
						Rect r) {

	//如果点的个数可以存放进一个磁盘块中
	if(range.second - range.first + 1 <= this->m_numPoint) {
		//创建一个存放点的block.
		KDNode* root = new KDNode(point, range, depth, m_sa);
		root->setLevel(-1);
		return root;
	}
	
	// Get the Level of each node.
	double S = range.second - range.first + 1;  // the nubmer of point
	double logF = floor(log(S / (2*m_numPoint)) / log(m_F));
	if(pow(m_F, logF+1) <= (S/(2*m_numPoint))){
		logF = logF + 1;    //对logF的进行矫正，因为采用自然对数进行了计算
	}
	double bF = m_numPoint * pow(m_F, logF);

	double logr = floor(log(S/m_numPoint) / log(m_F));
	if(pow(m_F, logr+1) <= S/m_numPoint){
		logr = logr + 1;
	}
	double bF_r = m_numPoint * pow(m_F, (int64_t)logr);

	int32_t level;
	if(bF_r < S && S < 2*bF_r){
		// level = (int32_t)ceil(log(S / (2*m_numPoint)) / log(m_F));
		level = logF + 1;
	} else {
		// level = (int32_t)floor(log(S / (2*m_numPoint)) / log(m_F));
		level = logF;
	}

	// Get the index of the median number
	uint64_t median = getMedian_nthE(point, range, depth);
	KDNode* root = new KDNode(pointGetX(point, m_step, median), pointGetY(point, m_step, median), depth, r);
	root->setLevel(level);
	
	// getMedian will  divide the vector into two parts.
	
	// recursively invoke the construct() function
	pair<uint64_t, uint64_t> rangeLeft = std::make_pair(range.first, median);
	pair<uint64_t, uint64_t> rangeRight = std::make_pair(median + 1, range.second);
	
	Rect rLeft 	= r;
	Rect rRight	= r;
	
	if(depth % 2 == 1) {
		rLeft.m_hx = pointGetX(point, m_step, median); //我写成  = median. 直接错了很多，错误也很难发现。
		rLeft.b_hx = true;
		rRight.m_lx = pointGetX(point, m_step, median);
		rRight.b_lx = true;		//rRight.b_lx = false;
	} else {
		rLeft.m_hy = pointGetY(point, m_step, median);
		rLeft.b_hy = true;
		rRight.m_ly = pointGetY(point, m_step, median);
		rRight.b_ly = true;		//rRight.b_ly = false;
	}

	KDNode* leftChild = construct(point, rangeLeft, depth + 1, rLeft);
	KDNode* rightChild = construct(point, rangeRight, depth + 1, rRight);

	// set it's left and right child
	root->setLeftChild(leftChild);
	root->setRightChild(rightChild);
						
	return root;
}

uint64_t KDTree::getMedian_nthE(std::vector<uint64_t>& vx,
							vector<uint64_t>& vy,
							pair<uint64_t, uint64_t>range,
							int32_t depth){

	uint64_t kth = 0;
	double S = range.second - range.first + 1;  // the nubmer of point
	double logF = floor(log(S / (2*m_numPoint)) / log(m_F));
	if(pow(m_F, logF+1) <= S/(2*m_numPoint)){
		logF = logF + 1;
	}
	double bF = m_numPoint * pow(m_F, (int64_t)logF);

	double logr = floor(log(S/m_numPoint) / log(m_F));
	if(pow(m_F, logr+1) <= S/m_numPoint){
		logr = logr + 1;
	}
	double bF_r = m_numPoint * pow(m_F, (int64_t)logr);
	
	//按照公式求得分割的位置
	if(S <= 2*m_numPoint){
		kth = (range.second - range.first + 1 + 1) / 2;
		kth --;
	} else if(bF_r < S && S < 2*bF_r){
		kth = bF_r - 1;
		assert((kth+1)%m_numPoint == 0);
	} else {
		kth = floor(floor(S/(2*bF))*bF)-1;
		assert((kth+1)%m_numPoint == 0);
	}

	uint64_t size = range.second - range.first + 1;
	Point* point = new Point[size];
	for(uint64_t i = range.first, j = 0; i <= range.second; i++, j++){
		point[j].m_x = vx[i];
		point[j].m_y = vy[i];
	}

	//调用nth_element() 进行分割
	if(depth % 2 == 1)
		nth_element(point, point + kth, point + size, cmp_x);
	else
		nth_element(point, point + kth, point + size, cmp_y);

	for(uint64_t i = range.first, j = 0; i <= range.second; i++, j++){
		vx[i] = point[j].m_x;
		vy[i] = point[j].m_y;
	}

	uint64_t idx = range.first + kth;

	//Just for test;
	vector<uint64_t>*  v1 = &vx;
	if(depth % 2 == 0) 
		v1 = &vy;
	vector<uint64_t>& v = *v1;

	uint64_t i = idx;
	for(uint64_t j = range.first; j <= i; j++){
		assert(v[j] <= v[i]);
	}
	for(uint64_t j = i + 1; j <= range.second; j++) {
		assert(v[j] >= v[i]);
	}

	if(i == range.first || i == range.second){
		fprintf(stderr, "this will be something not good.\n");
		assert(false);
	}

	delete []point;

	return idx;
}

uint64_t KDTree::getMedian_nthE(void* point, pair<uint64_t, uint64_t> range, int32_t depth){

	uint64_t kth = 0;
	double S = range.second - range.first + 1;  // the nubmer of point
	double logF = floor(log(S / (2*m_numPoint)) / log(m_F));
	if(pow(m_F, logF+1) <= S/(2*m_numPoint)){
		logF = logF + 1;
	}
	double bF = m_numPoint * pow(m_F, (int64_t)logF);

	double logr = floor(log(S/m_numPoint) / log(m_F));
	if(pow(m_F, logr+1) <= S/m_numPoint){
		logr = logr + 1;
	}
	double bF_r = m_numPoint * pow(m_F, (int64_t)logr);
	
	//按照公式求得分割的位置
	if(S <= 2*m_numPoint){
		kth = (range.second - range.first + 1 + 1) / 2;
		kth --;
	} else if(bF_r < S && S < 2*bF_r){
		kth = bF_r - 1;
		assert((kth+1)%m_numPoint == 0);
	} else {
		kth = floor(floor(S/(2*bF))*bF)-1;
		assert((kth+1)%m_numPoint == 0);
	}

	// uint64_t size = range.second - range.first + 1;

	// //调用nth_element() 进行分割
	// vector<Point>::iterator it = point->begin();
	// vector<Point>::iterator beg;
	// beg = it + range.first;

	// if(depth % 2 == 1)
	// 	nth_element(beg, beg + kth, beg + size, cmp_x);
	// else
	// 	nth_element(beg, beg + kth, beg + size, cmp_y);
	point_nth_element(point, m_step, kth, range, depth);


	uint64_t idx = range.first + kth;

	//Just for test;
	uint64_t i = idx;
	if(depth % 2 == 0){
		for(uint64_t j = range.first; j <= i; j++){
			assert(pointGetY(point, m_step, j) <= pointGetY(point, m_step, i));
		}
		for(uint64_t j = i + 1; j <= range.second; j++) {
			assert(pointGetY(point, m_step, j)>= pointGetY(point, m_step, i));
		}
	} else {
		for(uint64_t j = range.first; j <= i; j++){
			assert(pointGetX(point, m_step, j) <= pointGetX(point, m_step, i));
		}
		for(uint64_t j = i + 1; j <= range.second; j++) {
			assert(pointGetX(point, m_step, j) >= pointGetX(point, m_step, i));
		}		
	}

	if(i == range.first || i == range.second){
		fprintf(stderr, "this will be something not good.\n");
		assert(false);
	}

	return idx;	
}

uint64_t KDTree::getMedian(vector<uint64_t>& vx, 
						vector<uint64_t>& vy, 
						pair<uint64_t, uint64_t>range, 
						int32_t depth){
	// Get the median, and split the vector vx[range.first, range.second], vy[range.first, range.second]
	//into vx[range.first, i] and vx[i+1, range.second]
	//     vy[range.first, i] and vy[i+1, range.second]
	uint64_t kth = (range.second - range.first + 1 + 1) / 2;
	uint64_t idx = quickSelect(vx, vy, range, depth, kth);
	
	//TODO:
	vector<uint64_t>*  v1 = &vx;
	if(depth % 2 == 0) 
		v1 = &vy;
	vector<uint64_t>& v = *v1;

	//处理相等元素，使得和分裂值相等的元素位于一个节点上。
	uint64_t i = idx;
	
	//TODO：这里对于相等元素的处理很重要，因为[range.first, range.second]中可能会有大量的相同元素
	// 如果选择将相同元素放在一边可能得不偿失，所以可能需要牺牲查询性能了。
	// ++i;
	// while(i <= range.second && v[i] == v[idx]) ++i;
	// --i;

	/*TODO: Just For Test*/
	for(uint64_t j = range.first; j <= i; j++){
		assert(v[j] <= v[i]);
	}
	for(uint64_t j = i + 1; j <= range.second; j++) {
		// assert(v[j] > v[i]);
		assert(v[j] >= v[i]);
	}

	if(i == range.first || i == range.second){
		fprintf(stderr, "this will be something not good.\n");
		assert(false);
	}

	return i;
}

/****
* Get the kth value of v[range.first, range.second];  1 <= k and k <= range.second - range.first + 1
* @return idx,  返回下标[range.first, range.second];
* v[range.fist idx-1] <= v[idx]
* v[idx] <= v[idx + 1, v.range.second]
*
* 注意，仅仅通过 quickSelect() , 不能保证 v[idx+1, v.range.second]的值是完全大于v.[idx]的。这种
* 情况在KD-tree中不允许：在这个节点分裂的时候，和分裂值相等的元素只能位于这个节点的一个孩子中，不能
* 同时位于两个孩子中。
******/

uint64_t KDTree::quickSelect(vector<uint64_t>& vx, 
							vector<uint64_t>& vy,
							pair<uint64_t, uint64_t>range, 
							int32_t depth, 
							uint64_t kth) {

	assert(kth >= 1 && kth <= (range.second - range.first + 1));
	
	uint64_t div = partition(vx, vy, range, depth);  //分割符的index; range.first <= div <= range.second
	uint64_t xth = (div - range.first + 1);			//分割符的 rank.

	if(xth < kth) {
		kth -= xth;					// update the value of kth
		range.first = div + 1;		// update the range.first
		return quickSelect(vx, vy, range, depth, kth);
	}

	else if(xth > kth) {
		range.second = div -1;		// update the range.second
		return quickSelect(vx, vy, range, depth, kth);
	}

	else {
		return div;
	}
					
}// end of KDTree::quickSelect(vector<int>&,vector<int>&, pair<int, int>, depth, kth)


/**
* 类似于 快排的一次划分的过程。
* @return index of the partition value.  range.first <= idx <= range.second
* v[range.first, idx-1] <= v[idx]
* v[idx] < v[range.idx + 1, v.second]
**/
uint64_t KDTree::partition(vector<uint64_t>& vx, 
						vector<uint64_t>& vy, 
						pair<uint64_t, uint64_t>range, 
						int32_t depth) {
	
	uint64_t left, right, mid, div, idx;
	vector<uint64_t>* tv1 = &vx;
	vector<uint64_t>* tv2 = &vy;
	if(depth % 2 == 0) {
		tv1 = &vy;
		tv2 = &vx;
	}

	vector<uint64_t>& v1 = *tv1;
	vector<uint64_t>& v2 = *tv2;
	
	left = v1[range.first];
	right = v1[range.second];
	mid = v1[(range.first + range.second) / 2];

	//三分取中方法
	if(left <= mid){
		if	(mid <= right){
			div = mid;  //the value
			idx = (range.first + range.second) / 2; // the index
		}
		else if	(left <= right ) {
			div = right;
			idx = range.second;
		} else {
			div = left;
			idx = range.first;
		}
	}
	else {
	
		if(left <= right) {
			div = left;
			idx = range.first;
		} 
		else if(mid <= right) {
			div = right;
			idx = range.second;
		} else {
			div = mid;
			idx = (range.first + range.second) / 2;
		}
	}
	
	left = mid = right = range.first;
	//对数组进行一次调整
	uint64_t tmp;
	/***
	* 一次完整的划分过程，确保不会出错。
	* left -----> mid ----> right
	* left 指向下一个 等于或者大于 div的元素
	* mid  指向下一个 大于 div的元素
	* right 指向下一个未测试的元素。
	***/
	while(right <= range.second) {
		
		if(v1[right] > div) {
			right ++;
			if(v1[left] < div) {
				left ++;
			}
			if(v1[mid] <= div) {
				mid++;
			}
		}
		else if(v1[right] == div) {
			tmp = v1[mid];
			v1[mid] = v1[right];
			v1[right] = tmp;
			
			tmp = v2[mid];
			v2[mid] = v2[right];
			v2[right] = tmp;

			right++;
			mid++;
		}

		else if (v1[right] < div) {
			if (left < mid && mid < right) {
				tmp = v1[left];
				v1[left] = v1[right];
				v1[right] = v1[mid];
				v1[mid] = tmp;

				tmp = v2[left];
				v2[left] = v2[right];
				v2[right]=v2[mid];
				v2[mid] = tmp;
			}
			else if(left <= mid && mid <= right) {
				tmp = v1[left];
				v1[left] = v1[right];
				v1[right] = tmp;

				tmp = v2[left];
				v2[left] = v2[right];
				v2[right] = tmp;
			}

			left ++;
			mid ++;
			right ++;
		}

	}// end of while()

	/*TODO: Just For Test.*/
	for(uint64_t i = range.first; i <= mid -1 ; i++){
		assert(v1[i] <= v1[mid - 1]);
	}
	for(uint64_t i = mid; i <= range.second; i++) {
		assert(v1[i] > v1[mid -1]);
	}

	return mid - 1;

} // end of member function KDTree::partition()


KDNode* KDTree::getRoot(){
	return m_root;
}

//判断点(x, y)是否在矩形 (px.first, px.second)x(py.first, py.second)
bool KDTree::isInRange(pair<int ,int>& px, pair<int, int>& py, int x, int y) {

	if((x >= px.first && x <= px.second) && (y >= py.first && y <= py.second))
		return true;

	return false;
}


// Range Query, 找到出现在矩形 (px.first, px.second)x(py.first, py.second)
// 中的所有的点
vector<pair<int, int> >* KDTree::locate(pair<int, int>& px, pair<int, int>& py) {

	vector<pair<int ,int> >* result = new vector<pair<int, int> >();
	
	int lx, hx, ly, hy;
	lx = ly = std::numeric_limits<int>::min();
	hx = hy = std::numeric_limits<int>::max();
	Rect node_rect(lx, hx, ly, hy, true, true, true, true); //根节点对应的矩形
	Rect query_rect(px.first, px.second, py.first, py.second, true, true, true, true); //查询矩形

	locate(query_rect, m_root, node_rect, 1, result);  //KD-树的查询

	return result;   //返回找到的所有的点
}


void KDTree::locate(Rect& qrect, 
				KDNode* root,
				Rect nrect,
				int depth, 
				vector<pair<int, int> >* result) {
	
	int x = root->getX();
	int y = root->getY();

	int& val = x;

	if(depth % 2 == 0) {
		val = y;
	}
	
	if(root->isLeaf()){
		if (qrect.isInRange(x, y)) {
			// Add the leaf into the result
			result->push_back(std::make_pair(x, y));
		}
		return;
	}
	
	Rect lrect = nrect;  //左孩子节点对应的矩形
	Rect rrect = nrect;	 //右孩子节点对应的矩形

	// 设定左右孩子节点对应的矩形。
	if(depth % 2 == 1) {
		//split on x
		assert(val >= nrect.getLowX() && val <= nrect.getHighX());
		lrect.setHighX(val, true);  //左孩子包含边界，也就是说在分割数据的时候和val相等的数据分配到了左孩子。
		rrect.setLowX(val, false);
	} else {
		//split on y
		assert(val >= nrect.getLowY() && val <= nrect.getHighY());
		lrect.setHighY(val, true); //左孩子包含边界。
		rrect.setLowY(val, false);
	}

	/**递归查询孩子节点**/

	if (root->getLeftChild() != NULL) {
		//如果查询矩形qrect完全包含左孩子对应的矩形lrect
		if(qrect.isContained(lrect)){
			//递归查询其中所有的子节点
			locateAllChild(root->getLeftChild(), result);
		}

		//如果矩形qrect和lrect相交
		if (qrect.isIntersected(lrect)) {
			locate(qrect, root->getLeftChild(), lrect, depth + 1, result);
		}
	}

	if (root->getRightChild() != NULL) {
		//if qrect fully contained rrect.
		if(qrect.isContained(rrect)){
			//递归查询所有的孩子节点
			locateAllChild(root->getRightChild(), result);
		}

		//if qrect intersects rrect.
		if (qrect.isIntersected(rrect)) {
			locate(qrect, root->getRightChild(), rrect, depth + 1, result);
		}
	}

}// end of KDTree::locate(pair<int ,int>&, pair<int, int>&, KDTree* root, int depth, *result )


void KDTree::locateAllChild(KDNode* root, vector<pair<int, int> >* result){
	if(root == NULL)
		return;

	if(root->isLeaf()){
		//收集叶子节点
		int x = root->getX();
		int y = root->getY();
		result->push_back(std::make_pair(x, y));
		return;
	}

	//递归查询孩子节点
	locateAllChild(root->getLeftChild(), result);
	locateAllChild(root->getRightChild(), result);
}


/**将一颗KDTree存储在磁盘上 **
*
*@param name :   The file to save.
*@param B    :   The size of a disk page in bytes.
*
*方法如下:
*1. 构造KDB-tree的时候，如果切分之后的点的个数若是小于B个，就停止划分。
*2. 按照BFS（Breadth First Search）深度优先的方式访问KDTree。
*3. 当一个磁盘中可以装入的节点的个数已经满，那么就停止装入节点。
*4. 对它的每个孩子节点依次如此访问。
*
* An internal node consist of:
* a) The Shape of the kdtree.
* b) the data in the kdtree node.
* c) the child pointer of the internal node.
******************************/
int32_t KDTree::SaveToDisk(DiskFile* df){

	m_numRoot = numOfNodeInDiskRoot();

	//将根节点写入到磁盘中
	KDVirtualDisk* rootDisk = SaveNodeToDisk(m_root, true, df);
	int32_t rootNum = rootDisk->writeToDisk(df);	//返回根节点写入的磁盘号
	delete rootDisk;
	
	return rootNum;	//返回根节点的磁盘号。
	// int32_t allNum = df->m_diskNum;
	// m_header.set(rootNum, allNum);	
	// m_header.writeToDisk(&df);	

	// fclose(out);
}

void KDTree::SaveToDiskFile(const char* name){
	FILE* out = fopen(name, "w");
	if(!out){
		fprintf(stderr, "Open file '%s' error!\n", name);
		exit(1);
	}
	DiskFile df(out, 0);
	m_header.writeToDisk(&df);

	//将根节点写入到磁盘中
	KDVirtualDisk* rootDisk = SaveNodeToDisk(m_root, true, &df);
	int32_t rootNum = rootDisk->writeToDisk(&df);	//返回根节点写入的磁盘号
	delete rootDisk;
	
	int32_t allNum = df.m_diskNum;
	m_header.set(rootNum, allNum);	
	m_header.writeToDisk(&df);	

	fclose(out);
}



/*****将一个节点为根的树保存到磁盘上
*@param root	数的根
*@param out     file descripter
*@return 		The disk number into which it saves.
************************************/
KDVirtualDisk* KDTree::SaveNodeToDisk(KDNode* root, bool isRoot, DiskFile* diskOut) {
	
	if(NULL == root){
		assert(false);
	}

	if(root->isPointNode()) {
		/**节点保存了点：可以存放在一个磁盘块中**/
		/*将这个node中的point保存到一个磁盘块中，并且返回磁盘号*/	
		int64_t nPoint = root->getPointNum();	// the number of points in a node.
		KDPointDisk* pDisk = new KDPointDisk(nPoint);	
		for(uint64_t i = 0; i < nPoint; i++){
			pair<uint64_t, uint64_t> p = root->getPoint(i);
			uint64_t saValue = root->getSaValue(i);
			pDisk->setPoint(i, p.first, p.second, saValue);
		}
		// Save the KDPointDisk into real disk page.
		// int32_t numDisk = pDisk.writeToDisk(diskOut);

		n_PointNode++;						//统计个数
		n_Point += root->getPointNum();		//统计个数
		n_AllNode ++;

		// return numDisk;
		return pDisk;	//返回构造好的根节点。
	}
	
	/**从这个节点开始层次遍历**/
	queue<KDNode*> qChild;	 //存储孩子节点.(仅需要 Pointer)
	queue<KDNode*> qParent;	 //存储父节点.(仅需要SplitValue)
	queue<KDNode*> qPNode;	 //存储层次遍历中遇到的叶子节点.(需要Pointer)
	queue<KDNode*> qAllNode; //存储所有层次遍历的节点。

	qChild.push(root);
	qAllNode.push(root);

	while(!qChild.empty()){

		KDNode* tmpNode = qChild.front();
		qChild.pop();
	
	/***
	* 这里的写入依然有问题，要好好的思量一下。
	***/
		//这里要保证 叶子节点的 没有左右孩子。
		if(tmpNode->getLeftChild() != NULL){
			qChild.push(tmpNode->getLeftChild());
			qAllNode.push(tmpNode->getLeftChild());
		}	
		if(tmpNode->getRightChild() != NULL){
			qChild.push(tmpNode->getRightChild());
			qAllNode.push(tmpNode->getRightChild());
		}

		//给内部节点的 树形赋值：
		int32_t shapeNode = -1;
		if(tmpNode->getLeftChild() != NULL){
			if(tmpNode->getRightChild() != NULL){
				shapeNode = 0x1;  //11, two child
			} else {
				shapeNode = 0x3;  //01, only left child
				fprintf(stderr, "ERROR! KDTree::SaveNodeToDisk()--- KD-tree node can't have only left child");
				assert(false);
			}
		} else {
			if(tmpNode->getRightChild() != NULL){
				shapeNode = 0x2;  //10; only right child
				fprintf(stderr, "ERROR! KDTree::SaveNodeToDisk()--- KD-tree node can't have only right child");
				assert(false);
			}
			else{
				shapeNode = 0x0;  //00, zero child
			}
		}
		tmpNode->setShapeValue(shapeNode);

		//将这个节点加入这个磁盘页中，并且判断磁盘页的空间。
		if(tmpNode->isPointNode()){
			qPNode.push(tmpNode);
		} else {
			qParent.push(tmpNode);
		}
		
		//计算加入下一个节点(the next node)之后，所有节点占用的空间
		int32_t nc = qChild.size() + qPNode.size() + 1; /*弹出一个节点，最多加入两个孩子节点*/
		int32_t np = qParent.size() + 1;

		if(isRoot){
			if(nc > m_numRoot)
				break;
		}
		else if(KDDisk::OverFlow(np, nc))
			break;
		// else if(nc > m_numDisk)
		// 	break;

	}//end of while()
	
	while(!qChild.empty()){
		qPNode.push(qChild.front());
		qChild.pop();
	}
	while(!qPNode.empty()){
		KDNode* tmpNode = qPNode.front();
		/***
		* 给叶子节点的树形赋值，所有的叶子节点都没有孩子节点所以值都是 0x0。
		* 所谓叶子节点： 存储它的孩子节点为子树的的磁盘页的位置。
		***/
		tmpNode->setShapeValue(0x0);
		qChild.push(tmpNode);
		qPNode.pop();
	}

	// qParent 保存了层次遍历的所有需要保存的父节点。
	// qChild  保存了层次遍历所需要的孩子节点。
	int32_t nc = qChild.size();  //孩子节点.
	int32_t np = qParent.size(); //父亲节点.
	
	int32_t idx = 0, idxNode = 0;
	KDDisk *disk = new KDDisk(root->getRect(), root->getDepth());
	disk->init(np, nc);

	while(!qAllNode.empty()){
		KDNode* tmpNode = qAllNode.front();  
		qAllNode.pop();
		int32_t shapeNode = tmpNode->getShapeValue();
		assert(shapeNode != -1);
		assert(shapeNode != 2);
		assert(shapeNode != 3);
	
		//向disk中写入树形的值。
		disk->writeTreeShape(idx, shapeNode);
		idx++;

		//向disk中写入节点的value的值。
		if(shapeNode != 0) {
			// 是内部节点，不是叶子节点，写入splitValue.
			disk->writeSplitValue(idxNode, tmpNode->getSplitValue());
			idxNode++;
		}
	
	}//end of while()


	/**确定disk中孩子节点的值**/
	queue<KDVirtualDisk*> qVirtual;

	while(!qChild.empty()){
		KDNode* tmpNode = qChild.front();
		qChild.pop();
		tmpNode->setShapeValue(-1);  //重新设置shapeValue的值。
		// int32_t numDisk = SaveNodeToDisk(tmpNode, diskOut);
		KDVirtualDisk* virtualChild = SaveNodeToDisk(tmpNode, false, diskOut);
		qVirtual.push(virtualChild);

		//TODO: 将孩子节点的 numDisk 写入disk中。
		// disk.writeChildPointer(idx, numDisk);
		// idx++;
	}

	idx = 0;	
	while(!qVirtual.empty()){
		KDVirtualDisk* tmpNode = qVirtual.front();
		qVirtual.pop();
		int32_t numDisk = tmpNode->writeToDisk(diskOut);
		disk->writeChildPointer(idx, numDisk);
		idx++;
		delete tmpNode;
	}

	/**TODO: 将disk 写入真正的磁盘中**/
	// int32_t numDisk = disk.writeToDisk(diskOut);
	n_AllNode++;		//统计个数
	
	/**TODO: 返回写入的磁盘号**/
	// return numDisk;

	
	/*返回构造好的节点*/
	return disk;
}


/*****
* TODO:下面这种计算方式有时候可能会出现问题。
* 以后需要进行测试.
* (其实就是一个数学公式而已，为什么我不能计算正确呢？)
*****/
uint64_t KDTree::numOfNodeInDiskRoot(){

	int64_t Htree;	//整颗树的高度。
	int64_t Hdisk;	//一个磁盘页中可以存储的子树的高度。

	int64_t HrootDisk;	//跟节点中树的高度
	int64_t numRoot;	//根节点中所存储的一棵树中叶子节点的个数。

	// Htree = (ceil)(log2((double)(ceil((double)m_n / m_numPoint))));	//求取树的高度
	// uint64_t Htree1 = getTreeHeight();
	Htree = getTreeHeight();

	Hdisk = KDDisk::HeightOfTreeInADisk();
	if(Htree > Hdisk){
		HrootDisk = (Htree - Hdisk + 1) % (Hdisk - 1);
		if(HrootDisk == 0)
			HrootDisk = Hdisk - 1;
		numRoot = pow(2, HrootDisk - 1 + 1);
	}
	else{
		HrootDisk = Hdisk;
		numRoot = pow(2, HrootDisk -1);
	}
	// HrootDisk = (Htree-1) % (Hdisk-1);
	
	m_numDisk = pow(2, Hdisk - 1);
	return numRoot;
}


uint64_t KDTree::getTreeHeight(){

	getTreeHeightHelper(m_root);
}

uint64_t KDTree::getTreeHeightHelper(KDNode* root){

	if(root == NULL)
		return 0;

	uint64_t left = getTreeHeightHelper(root->getLeftChild())  + 1;
	uint64_t right = getTreeHeightHelper(root->getRightChild()) + 1;

	return left > right ? left : right;
}



void KDTree::SaveToDiskFile_2(const char* name){
	FILE* out = fopen(name, "w");
	if(!out){
		fprintf(stderr, "Open file '%s' error!\n", name);
		exit(1);
	}
	DiskFile df(out, 0);
	m_header.writeToDisk(&df);

	//将根节点写入到磁盘中
	KDVirtualDisk* rootDisk = SaveNodeToDisk_2(m_root, true, &df);
	int32_t rootNum = rootDisk->writeToDisk(&df);	//返回根节点写入的磁盘号
	delete rootDisk;
	
	int32_t allNum = df.m_diskNum;
	m_header.set(rootNum, allNum);	
	m_header.writeToDisk(&df);	

	fclose(out);
}

/**将一颗KDTree存储在磁盘上 **
*
*@param name :   The file to save.
*@param B    :   The size of a disk page in bytes.
*
*方法如下:
*1. 构造一棵内存kd-tree时，如果切分之后的点的个数若是小于B个，就停止划分。
*2. 对每个节点分配一个Level值。
*3. 将和根节点的level值相同的节点打包到同一个磁盘页中。
*4. 对它的每个孩子节点依次如此访问。
*
* An internal node consist of:
* a) The Shape of the kdtree.
* b) the data in the kdtree node.
* c) the child pointer of the internal node.
******************************/
int32_t KDTree::SaveToDisk_2(DiskFile* df){

	m_numRoot = numOfNodeInDiskRoot();

	//将根节点写入到磁盘中
	KDVirtualDisk* rootDisk = SaveNodeToDisk_2(m_root, true, df);
	int32_t rootNum = rootDisk->writeToDisk(df);	//返回根节点写入的磁盘号
	delete rootDisk;
	
	return rootNum;	//返回根节点的磁盘号。
}


/**
* 将和root的level值相同的节点打包到同一个磁盘页中，形成一一棵小子树
**/
KDVirtualDisk* KDTree::SaveNodeToDisk_2(KDNode* root, bool isRoot, DiskFile* diskOut){

	if(NULL == root){
		assert(false);
	}

	if(root->isPointNode()){
		/**节点保存了点：可以存放在一个磁盘块中**/
		/*将这个node中的point保存到一个磁盘块中，并且返回磁盘号*/	
		int64_t nPoint = root->getPointNum();  // the number of points in a node.
		KDPointDisk* pDisk = new KDPointDisk(nPoint);
		for(uint64_t i = 0; i < nPoint; i++){
			pair<uint64_t, uint64_t> p = root->getPoint(i);
			uint64_t saValue = root->getSaValue(i);
			pDisk->setPoint(i, p.first, p.second, saValue);
		}		

		n_PointNode++;                        //统计存储点的磁盘页的个数
		n_Point += root->getPointNum();       //统计存储了多少个点
		n_AllNode++;                          //统计磁盘页的个数

		if(root->getPointNum() < m_numPoint){
			n_unFullPointNode ++;
		}

		return pDisk; //返回构造好的根节点(KDVirtualDisk*类型)。
	}

	/**从节点root开始层次遍历**/
	queue<KDNode*> qChild;                   //用来进行层次遍历(取出队头节点，并将这个节点的两个孩子节点依据情况加入到qChild中)
	queue<KDNode*> qParent;                  //保存和root->getLevel()值相同的节点
	queue<KDNode*> qPNode;                   //保存层次遍历过程中的(和root->getLevel()不相同的节点，存储point的节点)
	queue<KDNode*> qAllNode;                 //保存层次遍历过程中的所有的节点

	qChild.push(root);                       
	qAllNode.push(root);                     

	while(!qChild.empty()){

		KDNode* tmpNode = qChild.front();
		qChild.pop();
		//如果tmpNode不是point节点，并且level值和root的level值相同
		if(!tmpNode->isPointNode() && tmpNode->getLevel() == root->getLevel()){
			if(tmpNode->getLeftChild() != NULL){
				qChild.push(tmpNode->getLeftChild());
				qAllNode.push(tmpNode->getLeftChild());
			}
			if(tmpNode->getRightChild() != NULL){
				qChild.push(tmpNode->getRightChild());
				qAllNode.push(tmpNode->getRightChild());
			}
		}
		
		//给tmpNode节点 赋 树形值
		int32_t shapeNode = -1;
		if(tmpNode->isPointNode()){
			shapeNode = 0x0;  //作为层次遍历所形成的一棵子树的叶子节点
		} else if(tmpNode->getLevel() != root->getLevel()){
			shapeNode = 0x0;  //作为层次遍历所形成的一棵子树的叶子节点
		} else {
			//层次遍历所形成的一棵子树的内部节点，必定有且只有两个孩子节点.
			if(tmpNode->getLeftChild() != NULL){
				if(tmpNode->getRightChild() != NULL){
					shapeNode = 0x1;   //11, two child
				} else {
					shapeNode = 0x3;   //01, only left child
					fprintf(stderr, "ERROR! KDTree::SaveNodeToDisk() --- KD-tree internal node can't have left child\n");
					assert(false);
				}
			} else {
				if(tmpNode->getRightChild() != NULL){
					shapeNode = 0x2;   //10; only right child
					fprintf(stderr, "Error! KDTree::SaveNodeToDisk() --- KD-tree internal node can't have only right child\n");
					assert(false);
				}
				else {
					shapeNode = 0x0;	// 00, zero child
					fprintf(stderr, "Error! KDTree::SaveNodeToDisk() --- KD-tree internal node can't have only no child\n");
					assert(false);
				}
			}
		}
		tmpNode->setShapeValue(shapeNode);

		if(tmpNode->isPointNode()){
			qPNode.push(tmpNode);
		} else if(tmpNode->getLevel() != root->getLevel()){
			assert(tmpNode->getLevel() < root->getLevel());
			qPNode.push(tmpNode);
		} else {
			qParent.push(tmpNode);
		}
	}

	int32_t np = qParent.size();
	int32_t nc = qPNode.size();
	assert(np+nc == qAllNode.size());

	if(nc > m_F){
		fprintf(stderr, "KDTree::SaveNodeToDisk_2()-- np = %d, nc = %d, m_F = %d\n", np, nc, m_F);
		assert(nc <= m_F);
	}

	int32_t idx = 0, idxNode = 0;
	KDDisk *disk = new KDDisk(root->getRect(), root->getDepth());
	disk->init(np, nc);

	//存储层次遍历所得的子树的树形与内部节点的splitValue值
	while(!qAllNode.empty()){
		KDNode* tmpNode = qAllNode.front();
		qAllNode.pop();
		int32_t shapeNode = tmpNode->getShapeValue();
		assert(shapeNode != -1);
		assert(shapeNode != 2);
		assert(shapeNode != 3);

		disk->writeTreeShape(idx, shapeNode);  //存储树形
		idx++;

		if(shapeNode != 0){
			disk->writeSplitValue(idxNode, tmpNode->getSplitValue()); //存储splitValue值
			idxNode++;
		}
	}

	queue<KDVirtualDisk*> qVirtual;
	
	//首先递归存储它的 nc 个叶子节点(可能为另一棵子树，可能为直接存储点集的磁盘页)
	while(!qPNode.empty()){
		KDNode* tmpNode = qPNode.front();
		qPNode.pop();
		tmpNode->setShapeValue(-1);
		KDVirtualDisk* virtualChild = SaveNodeToDisk_2(tmpNode, false, diskOut); //先将这nc个孩子节点所形成的VirtualNode保存起来
		qVirtual.push(virtualChild);
	}

	idx = 0;
	while(!qVirtual.empty()){
		KDVirtualDisk* tmpNode = qVirtual.front();
		qVirtual.pop();
		int32_t numDisk = tmpNode->writeToDisk(diskOut);  //将nc个Fanout节点在磁盘上连续存储
		disk->writeChildPointer(idx, numDisk);            //保存nc个Fanout节点的磁盘页位置
		idx++;
		delete tmpNode;
	}

	n_AllNode++;

	if(nc < m_F){
		n_unFullInternalNode++;
	}

	return disk;
}
