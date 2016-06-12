#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "DiskGBWT.h"
#include "sbt_util.h"



DiskGBWT::DiskGBWT(const char* textFile, uint64_t B, uint64_t step){
	
	m_B = B;
	m_step = step;

	m_textFile = new char[(strlen(textFile)+1)];
	if(!m_textFile){
		fprintf(stderr, "DiskGBWT::DiskGBWT--malloc memory error.\n");
		exit(EXIT_FAILURE);
	}
	strcpy(m_textFile, textFile);

	/*需要由文本文件:m_textFile，得到存储String B-tree和KD-tree的文件*/
	m_sbtFile = getSBTFile(textFile, step);	/*#include "sbt_uitl.h"*/
	m_kdtFile = getKDTFile(textFile, step);
	
	initSuf();
	
	m_sbt = new DiskSBT(m_textFile, m_sbtFile);	//创建String B-tree
	// m_kdt = new DiskKDTree(m_kdtFile, B, true);	//创建 KD-tree

	m_in = fopen(m_kdtFile, "r");
	if(!m_in){
		fprintf(stderr, "DiskGBWT::DiskGBWT()--Open File '%s' error!!\n", m_kdtFile);
		assert(false);
	}
	readMultiKDTHeader();
}


DiskGBWT::~DiskGBWT(){

	if(remove(m_sbtFile) != 0){
		fprintf(stderr, "DiskGBWT::DiskGBWT -- DELETE file '%s' error!\n", m_sbtFile);
		exit(EXIT_FAILURE);
	}

	if(remove(m_kdtFile) != 0){
		fprintf(stderr, "DiskGBWT::DiskGBWT -- DELETE file '%s' error!\n", m_kdtFile);
		exit(EXIT_FAILURE);
	}	

	for(int i = 0; i < 256; i++)
		delete m_suf[i];	

	delete []m_suf;

	delete []m_textFile;	//释放文件名所占用的内存。
	delete []m_sbtFile;
	delete []m_kdtFile;

	fclose(m_in);

	delete m_sbt;		//释放磁盘上的串B-树
	// delete m_kdt;		//释放磁盘上的KD-Tree。
}

/********
* 给定模式P，找到文本T中模式P出现的所有的位置。
* 1. 在进行查询的时候需要进行 (m_step)次 suffix_range查询。
* 2. 需要进行 (m_step -1)次正交范围查找。
*
* 3. 第一次进行Suffix Range查询的时候，首先需要找到完整的模式P，这个时候，需要收集String B-tree的叶子节点.(找到模式P在文本T中出现的所有的位置)。
*
* 4. 随后进行d-1次后缀范围查找，首先找到P的 offset-k(2<=k<=m_step) P[m_step-k+1, m-1]
*		长度为{m-(m_step-k+1)}的后缀范围[left, right]，然后进行后缀范围查找{[left, right]x[Pmin, Pmax]}
*		其中Pmin = P[0..m_step-k]的逆序 + [k-1]个字符 0x00（相当于其中最小的字符）
*			Pmax = P[0..m_step-k]的逆序 + [k-1]个字符 0xFF (也就是最大的字符).
*
*5. 在 d-1 次后缀范围查找之后，我们只是找到了对应的后缀的下标(也就是第几个后缀)，我们还需要根据后缀的下标找到下标对应的后缀的值。
*	所以这个时候需要通过收集 String B-tree的叶子节点，相当与收集后缀的值。
*
*6. 改进的点在于 直接在KD-tree 中存储相应的后缀的值，这样在查询的时候就避免了再去访问String B-tree的叶子节点。
*	（对于出现次数比较少的后缀，这种改进的方式是得不偿失的，但是对于出现次数比较多的后缀，这种方式
*	  还可以。所以这个改进以后再说，首先将 KD-tree的最主要的部分给改进了，需要减少空间，并且提高查询效率）
********/

uint64_t DiskGBWT::Locate(uint8_t* P, uint64_t m, vector<uint64_t>* location) {

	m_sbt_io = 0;
	m_kdt_io = 0;
	
	uint64_t result = 0;

	uint8_t *P1, *P2;	/*P1是需要查询的模式，由P2构造查询范围中的矩形*/
	uint64_t n1, n2;	/*字串 P1, P2的长度*/
	int64_t i, j;

	pair<int64_t, int64_t> sr;	/*查询模式的后缀范围*/
	sr = m_sbt->suffix_range2(P, m);

	m_sbt_io += m_sbt->getIOCounts();
	if(sr.first <= sr.second)
		m_sbt_io += get_sbt_leaf_node_io_counts(Rect(sr.first, sr.second, 0, 0));

	
	/*TODO:根据这个后缀范围 sr, 收集String B-tree中的后缀，看来还需要在String B-tree中在加上一个功能了.
	* And Count the IOs.
	*/
	m_sbt->collectLocation(sr, location);

	/*TODO:将这个String B-tree中的后缀，恢复成文本T中的后缀，存储到结果中*/
	if(sr.first <= sr.second){
		result += sr.second - sr.first + 1;
	}

#ifdef 	KK_DEBUG
	fprintf(stderr, "\n--------------------------------------------\n");
	fprintf(stderr, "k = 1, '%s' sparse suffix range = %lu\n", P, result);
	fprintf(stderr, "----------------------------------------------\n");
#endif
	

	/**===========(m_step-1)次Suffix Range查询 + (m_step-1)次正交范围查找======================**/
	
	for(int64_t k = 2;  k <= this->m_step; k++){
		n2 = m_step - k + 1;
		n1 = m - n2;
		P1 = (uint8_t*)malloc(sizeof(uint8_t)*(n1+1));
		P2 = (uint8_t*)malloc(sizeof(uint8_t)*(n2+1));

		for(i = 0; i < n2; i++)
			P2[i] = P[i];	/*我们需要P2的逆序*/
		P2[i] = '\0';
		for(j = 0; j < n1; j++, i++)
			P1[j] = P[i];
		P1[j] = '\0';
		assert(i == m);		/*写程序的时候这种测试会降低错误*/
		
		/*根据P2, 构造Pmin 和 Pmax*/
		uint64_t Pmin = 0, Pmax = 0;
		uint8_t cmin = 0x00, cmax = 0xFF;
		for(i = n2 - 1; i >=0; i--){ //这里的i不能是uint64_t类型了。
			if(i != n2 - 1) {
				Pmin = Pmin << 8;
				Pmax = Pmax << 8;
			}
			Pmin = Pmin | P2[i];
			Pmax = Pmax | P2[i];
		}
		for(i = 0; i < k-1; i++){
			Pmin = Pmin << 8;
			Pmax = Pmax << 8;
			Pmin = Pmin | cmin;
			Pmax = Pmax | cmax;
		}
		
		/*将求取后缀范围和求取正交范围查找放在locate_helper函数中处理*/
		locate_helper(&result, P1, n1, Pmin, Pmax, k, location);
		
		free(P1); /*释放资源*/
		free(P2);
	}//end of for	

	return result;
}//end Of Locate()


//这些说明真的是非常的有用，对培养我的软件工程的意识真的是太有帮助了。

/**** locate_helper: 用于进行(m_step -1)次模式匹配+正交范围查找。
*@param result: 存储模式P出现的次数。
*@param P:	需要查询的模式。
*@param m:	模式P的长度.
*@param Pmin: 范围查找中，纵坐标最小的值。
*@param Pmax: 范围查找中，纵坐标最大的值。
*@param k:	一个 offset-k 查询。 2 <= k <= m_step;
****/
void DiskGBWT::locate_helper(uint64_t* result,
							uint8_t* P,
							uint64_t m,
							uint64_t Pmin,
							uint64_t Pmax,
							uint64_t k,
							vector<uint64_t>* location) {

	/*首先对P在String B-tree中进行后缀范围查找*/
	pair<uint64_t, uint64_t> sr = m_sbt->suffix_range2(P, m);
	m_sbt_io += m_sbt->getIOCounts();

	uint64_t left = sr.first, right = sr.second;
	int32_t xChar = -1;
	for(int32_t i = 0; i < 256; i ++){
		if(m_suf[i]->exist == false)
			continue;
		if(left >= m_suf[i]->left && left <= m_suf[i]->right){
			assert(right >= m_suf[i]->left && right <= m_suf[i]->right);
			left -= m_suf[i]->left;
			right -= m_suf[i]->left;
			xChar = i;
		}
	}

	int32_t yChar = Pmin >> ((m_step-1)*8);
	assert((Pmin >> ((m_step-1)*8)) == (Pmin >> ((m_step-1)*8)));
	Pmin &= ~(0xFFULL << ((m_step-1)*8));
	Pmax &= ~(0xFFULL << ((m_step-1)*8));

	/*得到KDTree的查询矩形[sr.first, sr.second]x[Pmin, Pmax]*/
	Rect rect(left, right, Pmin, Pmax);
		
	/*进行正交范围查找，找到所有出现的点*/
	vector<Point_XYSA >* point = NULL;
	if(sr.first <= sr.second){	//确保后缀范围不为空。
		if(m_suf[xChar]->diskNum[yChar] != -1){

			int32_t diskNum = m_suf[xChar]->diskNum[yChar];
			setPosition(diskNum);
			m_kdt = new DiskKDTree(m_in, diskNum, m_B);	//create an DiskKDTree.

			point = m_kdt->Locate(rect);
			m_kdt_io += m_kdt->getIOCounts();

			delete m_kdt;
			m_kdt = NULL;

		} else {
			point = new vector<Point_XYSA>(); //new an empty vector.
		}	
	}

	/***
	* 根据 Suffix Range [l, r]收集String B-tree的叶子节点，其实在进行后缀范围查询的时候已经得到两个
	* 叶子节点了(这两个叶子节点中很可能存储有一些后缀)。
	* 
	* 1. 现在我们只是求取count的个数，并不需要收集String B-tree的叶子节点，找到模式P出现的所有位置。
	* 2. 如果能够确定 count 个数正确，我们也只是需要确定在这个查询过程中的I/O次数，其实就写程序来说
	*	 也没有必要收集串B-数的叶子节点。
	***/
	if(sr.first <= sr.second){
		*result += point->size();
	}

	uint64_t loc;
	if(location != NULL && sr.first <= sr.second){
		for(int64_t i = 0; i < point->size(); i++){
			loc = (*point)[i].sa * m_step - (m_step - k + 1);
			location->push_back(loc);
		}
	}

	delete point; /*删除结果*/
}


/**
* 当进行一个Suffix Range 查询之后，如果后缀范围[left, right]不为空，可能需要遍历String B-tree
* 的叶子节点，收集相应的后缀得出结果。
*
* 此函数用来求取，收集后缀的过程中所需要的I/O次数。
* rect.minx: 用来表示最左边的后缀的下标。
* rect.maxx: 用来标识最右边的后缀的下标。
* 
* 根据这两个下标，就可以求出所需要的I/O次数。
* 需要注意：
*		最左侧的disk page可能已经求取出来了.(这里用到了可能，因为在串B-树中有一种情况很特殊
*			并没有加载最左侧的disk page,不过出现的次数非常小，可以忽略)
*		最右侧的disk page 肯定已经求取出来了。
**/
uint64_t DiskGBWT::get_sbt_leaf_node_io_counts(Rect r) {

	// if(this->m_type == 3){
	// 	在KD-tree的叶子节点中保存了相应的后缀
	// 	return 0;
	// }
	assert(r.getLowX() <= r.getHighX());
	uint64_t left = r.getLowX();
	uint64_t right = r.getHighX();

	left = ceil((double)left / (m_sbt->m_b));
	right = ceil((double)right / (m_sbt->m_b));

	if(left == right){
		return 0;
	} else {
		return right - left - 1;
	}
}


//串B-树和KD-树占用的空间,也就是GBWT占用的空间
uint64_t DiskGBWT::all_size_in_bytes(){
	return sbt_size_in_bytes() + kdt_size_in_bytes();
}
//串B树占用的空间
uint64_t DiskGBWT::sbt_size_in_bytes(){
	return m_sbt->sizeInBytes();
}
//KD-树占用的空间
uint64_t DiskGBWT::kdt_size_in_bytes(){
	return sb_file_size((const uint8_t*)m_kdtFile);
}

//进行一次模式匹配GBWT所需要的I/O次数。
uint64_t DiskGBWT::get_all_io_counts(){
	return m_kdt_io + m_sbt_io;
}

//进行一次模式匹配Strint B-tree所需要的I/O次数
uint64_t DiskGBWT::get_sbt_io_counts(){
	return m_sbt_io;
}
//进行一次模式匹配KD-tree所需要的I/O次数。
uint64_t DiskGBWT::get_kdt_io_counts(){
	return m_kdt_io;
}
	

void DiskGBWT::initSuf(){

	m_suf = new Suffix* [256];
	for(int64_t i = 0; i < 256; i++){
		m_suf[i] = new Suffix();
	}
}

void DiskGBWT::readMultiKDTHeader(){

	fseek(m_in, 0, SEEK_SET);	/*set into the heade of file.*/
	int32_t numChar;
	uint8_t	charas[256];
	uint32_t	charIdx[256];

	uint64_t nsize;

	nsize += fread(&numChar, 1, sizeof(int32_t), m_in);
	nsize += fread(charas, 1, sizeof(uint8_t)*256, m_in);
	nsize += fread(charIdx, 1, sizeof(uint32_t)*256, m_in);

	for(int32_t i = 0; i < numChar; i++){
		uint8_t ch = charas[i];
		m_suf[ch]->exist = true;
		m_suf[ch]->firstChara = ch;
		m_suf[ch]->left = charIdx[i];
		if(i != numChar-1){
			m_suf[ch]->right = charIdx[i+1] - 1;
		} else {
			m_suf[ch]->right = std::numeric_limits<int>::max();
		}
	}


	setPosition(1);	/*set m_in be the 1-th disk page*/

	for(int32_t i = 0; i < numChar; i++){
		uint8_t ch = charas[i];
		nsize += fread(m_suf[ch]->diskNum, 1, sizeof(int32_t)*256, m_in);
		// fprintf(stderr, "ch = %d\n",(int32_t)ch);
		// for(int32_t k = 0; k < 256; k++)
		// 	fprintf(stderr, "%d ", m_suf[ch]->diskNum[k]);
		// fprintf("\n");
	}
}

void DiskGBWT::setPosition(int32_t diskNum){
	fseek(m_in, diskNum * m_B, SEEK_SET);
}

int64_t DiskGBWT::GetPosition(int32_t diskNum, int32_t B){

	return (int64_t)diskNum * (int64_t)B;
}