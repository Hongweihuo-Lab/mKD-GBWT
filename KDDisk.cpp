
#include "KDDisk.h"
#include <queue>

using namespace sdsl;

using namespace std;

/*define the static variable.*/
int32_t KDDisk::m_B = -1;
int32_t KDDisk::m_bits = -1;


KDDisk::KDDisk(Rect r, int32_t depth){

	m_rootRect = r;
	m_depth = depth;

	treeShape = NULL;
	splitValue = NULL;
	childPointer = NULL;
}

KDDisk::~KDDisk() {
	delete treeShape;
	delete splitValue;
	delete childPointer;
}

void KDDisk::init(int32_t nparent, int32_t nchild){
	
	m_np = nparent;
	m_nc = nchild;
	treeShape = new int_vector<1>(nparent + nchild);
	splitValue = new int_vector<0>(nparent, 0, m_bits);
	childPointer = new int_vector<32>(nchild);
}

 void KDDisk::Set(int32_t B, int32_t bits){
	m_B = B;
	//assert(bits == 32 || bits == 64);
	m_bits = bits;
}

bool KDDisk::OverFlow(int32_t np, int32_t nc){

	int32_t totalBits = 0;  // in bits.
	totalBits += (((np + nc) + 63) / 64) * 64;  	// the shape of tree.
	totalBits += ((np * m_bits + 63) / 64) * 64;  		// the internal value (4bytes/8bytes).
	// totalBits += ((nc * 32 + 63) / 64) * 64;  		// the child pointer.
	totalBits += (Rect::BytesInDisk() + 4 + 4)*8;  		// Rect + deepth + diskNum.
	totalBits += (4+4)*8; 						// nparent and nchild.
	totalBits += (1+1)*8;						// width of each int_vector.	
	totalBits += 1*8;							// the flag of node.
	totalBits += 4*8;							// disk Number of first child.
	totalBits += (4+4)*8;						// the capacity of each int_vector.

	if(totalBits  > m_B*8)
		return true;

	return false;
}

int32_t KDDisk::GetMaxFanout(){
	// ((2*np+1) + 64):   			tree-shape     vector
	// (np*m_bits + 64):    			interval-value vector
	// (Rect::BytesInDisk() + 4 + 4)*8	Rect + deepth + diskNum
	// ((4+4)*8)      					nparent and nchild
	// (1+1)*8							width of each int_vector
	// 1*8								the flag of node
	// 4*8								disk Number of first child
	// (4+4)*8							capacity of each int_vector

	int32_t t_B = 0;
	t_B += (4+4)*8 + 4*8 + 1*8 + (1+1)*8 + (4+4)*8 + (Rect::BytesInDisk()+4+4)*8;
	t_B += 64 + 64 + 1;
	t_B = m_B * 8 - t_B;
	assert(t_B > 0);

	int32_t np = t_B / (m_bits + 2);
	int32_t Fanout = np + 1;
	return Fanout;
}

int32_t KDDisk::HeightOfTreeInADisk(){

	int32_t height = 1;
	int32_t np = 0, nc = 1;
	while(true){
		height++;
		nc *= 2;
		np = nc - 1;
		if(OverFlow(np, nc)){
			return (height - 1);
		}
	}
}

/***********************************************
* int_vector 很方便，预计要在两天的时间内掌握它。
***********************************************/
void KDDisk::writeTreeShape(uint64_t idx, uint64_t value){
	(*treeShape)[idx] = value; 
}

void KDDisk::writeSplitValue(uint64_t idx, uint64_t value){
	(*splitValue)[idx] = value;
}

void KDDisk::writeChildPointer(uint64_t idx, uint64_t value){
	(*childPointer)[idx] = value;
}

/*****************************
* 将一个disk page 写入到磁盘中。
* @return  返回需要写入的磁盘号。
* 
******************************/
int32_t KDDisk::writeToDisk(DiskFile* diskOut){
	
	/*TODO: Just For Vertify.*/
	for(int32_t i = 1; i < childPointer->size(); i++){
		if((*childPointer)[i] - (*childPointer)[i-1] != 1){
			int k1 = childPointer->size();
			int p1 = (*childPointer)[i];
			int p2 = (*childPointer)[i-1];
			fprintf(stderr, "KDDisk::writeToDisk-- childPointer->size() = %d, i = %d, childPointer[i]= %d, childPointer[i-1] = %d\n", k1, i, p1, p2);
		}
		assert((*childPointer)[i] - (*childPointer)[i-1] == 1);
	}

	FILE*	out = diskOut->m_out;  //写入文件的数据流, out 已经指向需要写入的磁盘号。
	m_diskNum = diskOut->m_diskNum;	//需要写入的磁盘号。
	
	/**calculate the total bytes that need to write **/
	int32_t totalBytes = 0;
	totalBytes += 1;							  // the flag of node.
	totalBytes += (Rect::BytesInDisk()) + 4 + 4;  // bounding box + deepth + diskNum;
	totalBytes += 4 + 4;	 	// number of parent  + number of child.
	totalBytes += 4;			// the disk number of first child.
	totalBytes += 1 + 1 ;   	// the width of  int_vector.
	totalBytes += 4 + 4 ;		// the length of each int_vector.(in bytes);

	totalBytes += treeShape->capacity() >> 3;	// the bytes of treeShape
	totalBytes += splitValue->capacity() >> 3;  // the bytes of the splitValue
	// totalBytes += childPointer->capacity() >> 3;  // the bytes of the childPointer
	
	assert(totalBytes <= m_B);

	uint8_t width = 0, flag = 0;
	/**** 将所有需要的数据写入磁盘中 ******/
	int32_t written = 0;
	written += fwrite(&flag, 1, sizeof(uint8_t), out);
	written += m_rootRect.writeToDisk(out);  // write Rect
	written += fwrite(&m_depth, 1, sizeof(int32_t), out);  // write deepth
	written += fwrite(&m_diskNum, 1, sizeof(int32_t), out);
	written += fwrite(&m_np, 1, sizeof(int32_t), out);
	written += fwrite(&m_nc, 1, sizeof(int32_t), out);		
	int32_t firstChild = (*childPointer)[0];				//disk number of first child.
	written += fwrite(&firstChild, 1, sizeof(int32_t), out);

	width = treeShape->get_int_width();						// write the width of the int_vector
	written += fwrite(&width, 1, sizeof(uint8_t), out);
	width = splitValue->get_int_width();
	written += fwrite(&width, 1, sizeof(uint8_t), out);
	// width = childPointer->get_int_width();
	// written += fwrite(&width, 1, sizeof(uint8_t), out);
	
	uint32_t data_len = 0;
	const uint64_t* data  = NULL;

	data_len = treeShape->capacity() >> 3;  //treeShape的数据的长度
	written += fwrite(&data_len, 1, sizeof(uint32_t), out);
	data_len = splitValue->capacity() >> 3; // splitValue数据长度(in bytes)
	written += fwrite(&data_len, 1, sizeof(uint32_t), out);
	// data_len = childPointer->capacity() >> 3; //childPointer数据的长度(in bytes)
	// written += fwrite(&data_len, 1, sizeof(uint32_t), out);


	data_len = treeShape->capacity() >> 3;  // trun bits into bytes.
	data = treeShape->data();
	written += fwrite(data, 1, data_len, out);	//write treeShape

	data_len = splitValue->capacity() >> 3; 
	data 	 = splitValue->data();
	written += fwrite(data, 1, data_len, out); // write splitValue
	
	// data_len = childPointer->capacity() >> 3;
	// data	 = childPointer->data();
	// written += fwrite(data, 1, data_len, out); // write childPointer
	
	assert(written == totalBytes);
	written += writePadding(m_B - written, out);
	assert(written == m_B);

	diskOut->m_diskNum++;  //下一个写入的磁盘号+1.

	return m_diskNum;      //返回写入的磁盘编号。
}	
	

/*************************
* write padding
**************************/
int64_t KDDisk::writePadding(uint64_t bytes, FILE* out){
	assert(bytes >= 0);
	if(bytes){
		uint8_t* dummy = (uint8_t*)malloc(sizeof(uint8_t)*bytes);	
		memset(dummy, 0, bytes);
		int64_t written = fwrite(dummy, 1, bytes, out);  //write dummy
		free(dummy);
		return written;
	}
	return 0;
}


DiskKDNode* KDDisk::GetNodeFromMem(uint8_t* buf){
	uint8_t flag = buf[0];
	assert(flag == 0);

	Rect rootRect;			//根节点的矩形
	int32_t depth;			//根节点所在的深度
	int32_t diskNum;		//根节点的diskNum
	int32_t nParent;		//父节点的个数
	int32_t nChild;			//孩子节点的个数

	uint8_t treeWidth, splitWidth, pointerWidth; //int_vector's bitwise width.
	int32_t treeCap, splitCap, pointerCap;		 // the int_vector's size in bytes.

	int32_t firstChild;

	rootRect 	= Rect::GetRectFromMem(buf+1);
	depth		= *((int32_t*)(buf+Rect::BytesInDisk()+1));
	diskNum		= *((int32_t*)(buf+Rect::BytesInDisk()+5));
	nParent		= *((int32_t*)(buf+Rect::BytesInDisk()+9));
	nChild		= *((int32_t*)(buf+Rect::BytesInDisk()+13));
	firstChild 	= *((int32_t*)(buf+Rect::BytesInDisk()+17));

	treeWidth	= *((uint8_t*)(buf+Rect::BytesInDisk()+21));
	splitWidth	= *((uint8_t*)(buf+Rect::BytesInDisk()+22));
	// pointerWidth = *((uint8_t*)(buf+Rect::BytesInDisk()+19));
	pointerWidth = 32;
	treeCap		= *((int32_t*)(buf+Rect::BytesInDisk()+23)); //是以8bit(一个字节)来衡量的
	splitCap	= *((int32_t*)(buf+Rect::BytesInDisk()+27));
	// pointerCap	= *((int32_t*)(buf+Rect::BytesInDisk()+31));
	
	int32_t dataPos = Rect::BytesInDisk() + 31;
	uint8_t* treeData = buf + dataPos;
	uint8_t* splitData = treeData + treeCap;
	// uint8_t* pointerData = splitData + splitCap;
	
	int_vector<0>* tmpTree = new int_vector<0>(treeCap, 0, 8);   	//取得树形。
	int_vector<0>* tmpSplit = new int_vector<0>(splitCap, 0, 8); 	//分裂的值。
	int_vector<0>* tmpPointer = new int_vector<0>(nChild, 0, pointerWidth);//孩子节点的指针。

	for(int32_t i = 0; i < treeCap; i++){
		(*tmpTree)[i] = treeData[i];		
	}
	for(int32_t i = 0; i < splitCap; i++){
		(*tmpSplit)[i] = splitData[i];
	}

	(*tmpPointer)[0] = firstChild;
	for(int32_t i = 1; i < nChild; i++){
		// (*tmpPointer)[i] = pointerData[i];
		(*tmpPointer)[i] = (*tmpPointer)[i-1] + 1;
	}

	assert(treeWidth == 1);			//每个节点的左右孩子需要3个比特。
	assert(pointerWidth == 32);		//孩子节点的指针需要32个比特。
	
	tmpTree->set_int_width(treeWidth);
	tmpSplit->set_int_width(splitWidth);
	tmpPointer->set_int_width(pointerWidth);

	assert(tmpTree->get_int_width() == 1);
	assert(tmpPointer->get_int_width() == 32);

	/*现在解析每个 tmpTree和tmpSplit，构造出来一颗树*/
	queue<DiskKDNode*> qParent;

	int32_t idxTree = 0, idxSplit = 0, idxPointer = 0;
	
	DiskKDNode* root = getDiskNode(tmpTree, tmpSplit, tmpPointer, idxTree, idxSplit, idxPointer);
		
	qParent.push(root);  //加入队列中。

	while(!qParent.empty()) {
		DiskKDNode* anode = qParent.front(); //取队首元素
		qParent.pop();
		int32_t treeShape = anode->getTreeShape();

		if(treeShape == 0) {
			anode->setLeftChild(NULL);
			anode->setRightChild(NULL);
		}
		else if(treeShape == 1) {
			/*内部节点, has two children*/
			DiskKDNode *left = getDiskNode(tmpTree, tmpSplit, tmpPointer, idxTree, idxSplit, idxPointer);
			DiskKDNode *right = getDiskNode(tmpTree, tmpSplit, tmpPointer, idxTree, idxSplit, idxPointer);
			anode->setLeftChild(left);
			anode->setRightChild(right);
			qParent.push(left);
			qParent.push(right);
			
		} else if(treeShape == 2) {
			/*不可能出现treeShape == 2的情况*/
			fprintf(stderr, "ERROR!! KDDisk::GetNodeFromMem() --- KDDiskNode can't have only right child\n");
			assert(false);
			DiskKDNode *right = getDiskNode(tmpTree, tmpSplit, tmpPointer, idxTree, idxSplit, idxPointer);
			anode->setLeftChild(NULL);
			anode->setRightChild(right);
			qParent.push(right);

		} else if(treeShape == 3) {
			/*不可能出现 treeShape == 3的情况*/	
            fprintf(stderr, "ERROR!! KDDisk::GetNodeFromMem()---- KDDiskNode can't have only left child.\n"); 			
            assert(false);
			DiskKDNode *left = getDiskNode(tmpTree, tmpSplit, tmpPointer, idxTree, idxSplit, idxPointer);
			// DiskKDNode *right = getDiskNode(tmpTree, tmpSplit, tmpPointer, idxTree, idxSplit, idxPointer);
			anode->setLeftChild(left);
			anode->setRightChild(NULL);
			qParent.push(left);
			// qParent.push(right);
		}	
	}//end of while

	//现在要设置每个DiskKDNode的矩形，这对于查询过程十分关键:
	root->setRect(rootRect);
	root->setDepth(depth);
	setAllRect(root);

	delete tmpTree;
	delete tmpSplit;
	delete tmpPointer;

	return root;
}//end of KDDisk::GetNodeFromMem()


DiskKDNode* KDDisk::getDiskNode(int_vector<0>* tmpTree, 
							int_vector<0>* tmpSplit, 
							int_vector<0>* tmpPointer,
							int32_t& idxTree,
							int32_t& idxSplit,
							int32_t& idxPointer) {
	DiskKDNode* anode;
	int32_t treeShape = (*tmpTree)[idxTree];
	if(treeShape != 0) {
		assert(treeShape == 1);
		uint64_t splitValue = (*tmpSplit)[idxSplit];
		idxSplit ++;
		anode = new DiskKDNode(splitValue, treeShape);
	} else {
		/*是一个叶子节点*/
		int32_t diskNum = (*tmpPointer)[idxPointer];
		idxPointer++;	
		anode = new DiskKDNode(diskNum, treeShape, true);
	}
	idxTree++;
	return anode;
}


void KDDisk::setAllRect(DiskKDNode* root) {
	
	int32_t depth = root->getDepth(); //深度。
	Rect r = root->getRect();		  //这个节点所代表的矩形。

	Rect leftRect, rightRect;
	
	leftRect = rightRect = r;	      

	uint64_t value = root->getSplitValue();

	if(root->getLeftChild() == NULL && root->getRightChild() == NULL)
		return;

	if(depth % 2 == 1) {
		assert(r.m_lx <= value && value <= r.m_hx);
		leftRect.m_hx = value;
		leftRect.b_hx = true;
		rightRect.m_lx = value;
		rightRect.b_lx = true;	//rightRect.b_lx = false;
		
	} else {
		assert(r.m_ly <= value && value <= r.m_hy);
		leftRect.m_hy = value;
		leftRect.b_hy = true;
		rightRect.m_ly = value;
		rightRect.b_ly = true;	//rightRect.b_ly = false;
	}

	depth++;

	//这里要保证叶子节点的左右孩子都是NULL。这样这个递归算法才行的通。
	if(root->getLeftChild() != NULL) {
		DiskKDNode* lchild = root->getLeftChild();
		lchild->setDepth(depth);
		lchild->setRect(leftRect);
		setAllRect(lchild);
	}

	if(root->getRightChild() != NULL) {
		DiskKDNode* rchild = root->getRightChild();
		rchild->setDepth(depth);
		rchild->setRect(rightRect);
		setAllRect(rchild);
	}
}