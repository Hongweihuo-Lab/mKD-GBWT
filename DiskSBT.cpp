#include "DiskSBT.h"

#include <cstring>

using namespace std;

DiskSBT::DiskSBT(const char* textFile, const char* sbtFile){

	uint64_t n = strlen(textFile);
	m_textFile = (char*)malloc(n+1);
	strcpy(m_textFile, textFile);
	n = strlen(sbtFile);
	m_sbtFile = (char*)malloc(n+1);
	strcpy(m_sbtFile, sbtFile);

	/*Read Index File*/
	FILE* in = fopen(m_sbtFile, "r");
	if(!in) {
		fprintf(stderr, "DiskSBT: can't open output file '%s'\n", m_sbtFile);
		exit(EXIT_FAILURE);
	}

	/* Read Header of m_sbtFile */
	readHeader(in);
	fclose(in);

	/* open the file so we can use the sbt right away */
	this->m_sbtFd = fopen(m_sbtFile, "r");
	if(!m_sbtFd){
		fprintf(stderr, "DiskSBT: can't open file %s\n", m_sbtFile);
		exit(EXIT_FAILURE);
	}

	/* read the root page */
	m_root = loadRootNode();
}

//deconstructor
DiskSBT::~DiskSBT(){
	if(m_root != NULL){
		delete m_root;	//删除其在内存中的跟节点
	}
	
	if(!m_sbtFd){
		fclose(m_sbtFd);	//关闭打开的文件。
	}

	free(m_textFile);		//释放文件名所占用的内存。
	free(m_sbtFile);		
}


void DiskSBT::readHeader(FILE* in) {
	uint64_t read = 0;
	fseek(in, 0, SEEK_SET);
	read += fread(&m_n, sizeof(uint64_t), 1, in);
	read += fread(&bits_per_suffix, sizeof(uint64_t), 1, in);
	read += fread(&bits_per_pos, sizeof(uint64_t), 1, in);
	read += fread(&m_b, sizeof(uint64_t), 1, in);
	read += fread(&m_B, sizeof(uint64_t), 1, in);
	read += fread(&m_height, sizeof(uint64_t), 1, in);

	if(read != 6) {
		fprintf(stderr, "DiskSBT::readHeader--error reading index file %d.\n",read);
		exit(EXIT_FAILURE);
	}
}


critbit_tree* DiskSBT::loadDiskPage(uint64_t diskNum) {
	//从磁盘上读出相应的内容
	uint8_t* mem = getMemFromDisk(diskNum);
	critbit_tree* cbt = new critbit_tree();
	cbt->load_from_mem((uint64_t*)mem, m_B/8);  //从磁盘中恢复出一刻critbit_tree.
	free(mem);						 //释放资源。
	return cbt;
}

critbit_tree* DiskSBT::loadRootNode(){
	uint8_t* mem = getMemFromDisk(0);	//串B-树的根节点的磁盘页的页号是0.
	critbit_tree* cbt = new critbit_tree();
	cbt->load_from_mem((uint64_t*)mem, m_B/8);
	free(mem);
	return cbt;
}

/*给定一个磁盘页号，得到对应的偏移。
* 跟节点对应的磁盘页号为0.
*TODO: 需要修改一下create_tree中的内容。
****/
uint64_t DiskSBT::getOffset(uint64_t diskNum) {
	uint64_t offset = SBT_ROOT_OFFSET + diskNum * m_B;
	return offset;
}

//给定一个磁盘页号，从磁盘中读出对应的磁盘页。
uint8_t* DiskSBT::getMemFromDisk(uint64_t diskNum) {
	uint64_t offset = getOffset(diskNum);
	fseek(m_sbtFd, offset, SEEK_SET);
	uint8_t* mem = (uint8_t*)malloc(m_B * sizeof(uint8_t));
	uint64_t bytes = fread(mem, 1, m_B, m_sbtFd);
	if(bytes != m_B) {
		fprintf(stderr, "DiskSBT::getMemFromDisk -- read memory error.\n");
		exit(EXIT_FAILURE);
	}
	return mem;
}




/***
*return the suffix range [left, right] of pattern P.
*so that for every value left <= i <= right, T[SA[i]..n] has the prefix of P.
*if(right == -1 or left == -1), so the suffix range is 0.(there is no occurance of pattern P).
***/
pair<int64_t, int64_t> DiskSBT::suffix_range(uint8_t* P, uint64_t m){
	
	this->m_io = 0;  //初始化为0，计算I/O次数。
	int64_t	left = left_suffix_range(P, m);
	int64_t right = right_suffix_range(P, m);
	return make_pair(left, right);
}


/***
* return the index i, that SA[i] is the first suffix that has the prefix of P.
* If there is no sucn suffix, return -1.
***/

int64_t DiskSBT::left_suffix_range(uint8_t* P, uint64_t m) {
	/*查看根节点中的最左边和最右边的两个孩子suf1 and suf2，相当于整个后缀中的最小的和最大的两个后缀 */

	/*加载相应的文本 t1 and t2，并且和模式P进行比较 */

	/*如果 (t2 < P) 表示所有的后缀都比P小，返回-1*/
	/*如果 (t1 > P) 表示所有的后缀都比P大，返回-1*/	
	/*如果 t1 == P 返回0*/
	
	/*
	* 1) 如果 t1 < P(此时 t2 >= P)，此时需要在根节点中的 critbit_tree树中找到 相应的位置i
    *    用cur_SA 表示当前string B-tree节点中收集的后缀, 那么位置i为
	*    T[cur_SA[i]..n] < P, T[cur_SA[i+1]..n] <= P。
	* 
	* 2）加载第i个孩子，然后在第i个孩子中进行相同的查找P 的位置i.
	*	 此时，如果在此孩子中，所有的后缀都要比P小，那么可以直接找到相应的后缀。
	*
	* 3) 依次递归进行查找，直到找到叶子节点为止。
	*
	* 4) 
	**/

	int64_t cur_pos;
	critbit_tree* cbt = NULL;
	critbit_tree* next_cbt = NULL;
	uint64_t LCP;
	bool prefix_of_P = false;

	cur_pos = m_root->get_left_position(P, m, (const uint8_t*)m_textFile, m_B, &LCP, &this->m_io);
	if(LCP == m)
		prefix_of_P = true;

	//接下来所有的判断语句都是用来判断 根节点的情况的。
	if(cur_pos == -2){
		/*在m_root 中最小的后缀都要比P 大，表明string B-tree中的所有后缀都比P大 */
		return -1;
	}	
	else if(cur_pos == -3){
		/*在m_root中最小的后缀是以P 为前缀的，表明在String B-tree中最小的后缀是以P为前缀的,返回0即可，0即为left suffix range.*/
		assert(LCP == m);
		return 0;
	}else if(cur_pos == -1 && isLeaf(m_root)){
	/*如果在m_root中最大的后缀都要比P小并且m_root是叶子节点表明String B-tree中所有的后缀都要比P小（重要） */
		return -1;
	}
	else if(cur_pos == -1 && !isLeaf(m_root)){
	/*如果在m_root中最大的后缀都要比P小，并不能表明String B-tree中的所有的后缀都要比P小*/
		cur_pos = m_root->m_g-1;
	}
	else if(isLeaf(m_root)){
	/*如果当前节点是String B-tree的叶子节点，此时cur_pos表示小于P，cur_pos+1（必定存在）大于等于模式P，
	如果此时LCP = m，那么可以确定cur_pos+1即为所求。如果LCP < m, 表示不存在一个后缀以P为前缀，返回-1. */
		assert(cur_pos < m_root->m_g-1);
		assert(LCP <= m);
		if(LCP < m) return -1;
		else if(LCP == m) {
			return m_root->m_child_off + cur_pos + 1;	
		}
	}

	assert(cur_pos >= 0 && cur_pos < m_root->m_child_num);
	cbt = loadDiskPage(m_root->m_child_off + cur_pos);
	this->m_io++; /*increase IO times*/

	while(true) {

		cur_pos = cbt->get_left_position(P, m, (const uint8_t*)m_textFile, m_B, &LCP, &this->m_io);
		if(LCP == m)
			prefix_of_P = true;

		if(cur_pos == -1){
			/*表明当前critbit_tree中, 所有的后缀都要比P小 */
			if(isLeaf(cbt)) {
				cur_pos = cbt->m_g - 1;
				if(prefix_of_P) cur_pos = cbt->m_child_off + cur_pos + 1;
				else cur_pos = -1;
				delete cbt;
				return cur_pos;
			} else {
				/*如果不是叶子节点，需要加载它的最后一个孩子*/
				cur_pos = cbt->m_child_num -1;
			}
		}else if(cur_pos == -2){
			/* critbit_tree中所有的后缀都要大于P，这种情况不可能发生，因为其中必有一个后缀严格小于P*/
			delete cbt; cbt = NULL;
			fprintf(stderr, "ERROR!!! DiskSBT::left_suffix_range(): something is wrong cur_pos = -2.\n");
			exit(EXIT_FAILURE);			
		}else if(cur_pos == -3){
			/*同理这种情况也不可能发生，因为其中必有一个后缀严格小于P */
			delete cbt; cbt = NULL;
			fprintf(stderr, "ERROR!!! DiskSBT::left_suffix_range(): something is wrong cur_pos = -3.\n");
			exit(EXIT_FAILURE);
		}

		if(isLeaf(cbt)){
			/*需要判断是否有过LCP的与m相等，如果不想等，表示没有出现以P为前缀的后缀，返回-1 */
			assert(LCP <= m);
			assert(cur_pos >= 0 && cur_pos < cbt->m_g - 1);

			if(prefix_of_P == false) {
				delete cbt; cbt = NULL;
				return -1;
			}
			else{
				cur_pos = cbt->m_child_off + cur_pos +  1;
				delete cbt; cbt = NULL;
				return cur_pos;
			}
		}

		assert(cur_pos >=0 && cur_pos < cbt->m_child_num);
		next_cbt = loadDiskPage(cbt->m_child_off + cur_pos);
		this->m_io++; /*icrease IO times*/

		delete cbt;
		cbt = next_cbt;		

	}//end of while(true)

}// end of DiskSBT::left_suffix_range()


/****
* return the index i, that SA[i] is the last suffix that has the prefix of P.
* If there is no such suffix return -2.
****/
int64_t DiskSBT::right_suffix_range(uint8_t* P, uint64_t m) {

	int64_t cur_pos;
	critbit_tree* cbt = NULL;
	critbit_tree* next_cbt = NULL;
	uint64_t LCP;

	cur_pos = m_root->get_right_position(P, m, (const uint8_t*)m_textFile, m_B, &LCP, &this->m_io);
	/*现在cur_pos这个位置的后缀 小于等于P，cur_pos+1 这个位置的后缀大于模式P*/
	/*如果m_root中的所有后缀都大于P，说明整个String B-tree中的所有后缀都大于P*/
	/*如果m_root中的所有的后缀都小于或者等于P，并不能说明整个String B-tree中的所有后缀都小于P，应该加载m_root中的最后一个孩子*/
	/*否则必定可以找到这样的一个位置 (0<= cur_pos < m_root->m_child_num-1)*/

	if(cur_pos == -2){
	/*表示所有的后缀都大于P，返回-2*/
		return -2;
	}	

	if(isLeaf(m_root)){
	/*当前节点是一个叶子节点，需要判断 LCP 是否与 m 相等。*/
		assert(LCP <= m);
		assert(cur_pos >= 0 && cur_pos < m_root->m_g);

		/*m_root is a leaf node*/
		if(LCP < m) return -2;
		else if (LCP == m) {
			cur_pos = m_root->m_child_off + cur_pos;
			return cur_pos;
		}
	}

	assert(cur_pos >= 0 && cur_pos < m_root->m_child_num);
	cbt = loadDiskPage(m_root->m_child_off + cur_pos);
	this->m_io++;  /*increase IO times*/

	while(true) {
		cur_pos = cbt->get_right_position(P, m, (const uint8_t*)m_textFile, m_B, &LCP, &this->m_io);
		if(cur_pos == -2) {
		/*表示这个Critbit_tree中，所有的后缀都大于P，这中情况不可能发生，因为必定有一个后缀小于等于P。*/
			fprintf(stderr, "ERROR!! DiskSBT::right_suffix_range(): something is wrong cur_pos = -2.\n");
			exit(EXIT_FAILURE);
		}

		if(isLeaf(cbt)) {
			assert(LCP <= m);
			if(LCP < m) {
				delete cbt; cbt = NULL;
				return -2;
			}
			else if(LCP == m) {
				cur_pos = cbt->m_child_off + cur_pos;
				delete cbt; cbt = NULL;
				return cur_pos;				
			}
		}
		assert(cur_pos >= 0 && cur_pos < cbt->m_child_num);
		next_cbt = loadDiskPage(cbt->m_child_off + cur_pos);
		this->m_io++; /*icrease IO times*/

		delete cbt;
		cbt = next_cbt;		
		next_cbt = NULL;
	}// end of while(true)

}// end of DiskSBT::right_suffix_range();



	/************采用一种改进的方法来计算 suffix range******************/

/*首先载入根节点， 然后在这个根节点中计算这个 critbit_tree的 left position
* 和 right position. 
* 如果 这个left_position 和 right position 是同一个位置。那么载入同一个
* 孩子节点即可（然后按照孩子节点进行递归） 。
* 如果 这个left_position 和 right position 不是同一个位置, 那么载入的孩子节点
* 将不是同样的节点，分别求取即可。
*/	

pair<int64_t, int64_t> DiskSBT::suffix_range2(uint8_t* P, uint64_t m) {

	this->m_io = 0;		/*initial it to be zero*/

	pair<int64_t, int64_t> cur_pos, result;
	critbit_tree *cbt = NULL;
	critbit_tree *next_cbt = NULL;
	uint64_t LCP;
	bool prefix_of_P = false;

	cur_pos = m_root->get_left_and_right_position(P, m, (const uint8_t*)m_textFile, m_B, &LCP, &this->m_io);
	if(LCP == m)
		prefix_of_P = true;

	if(cur_pos.first == -2){
	/*在m_root中最小的后缀都要比P 大, 表明string B-tree 中所有的后缀都要比P大*/
		assert(cur_pos.second == -2);
		result.first = -1;
		result.second = -2;
		return result;	
	} 
	else if(cur_pos.first == -3) {
	/*在m_root中最小的后缀是以P 为前缀的, 表明在String B-tree中最小的后缀是以P为前缀的，此时
		left suffix range 为0，right suffix range 还要继续求 */
		result.first = 0;
		uint64_t off2 = m_root->m_child_off + cur_pos.second;
		result.second = suffix_range2_right_position(P, m, off2);
		return result;	
	}
	else if(cur_pos.first == -1 && isLeaf(m_root)) {
	/*表明在m_root中最大的后缀都要比P小,并且m_root又是String B-tree的叶子节点*/
		assert(cur_pos.second == m_root->m_g-1);
		result.first = -1;
		result.second = -2;
		return result;
	}
	else if(cur_pos.first == -1 && !isLeaf(m_root)){
	/*表明在m_root中最大的后缀都要比P小，并不能表明String B-tree中所有的后缀都比 P小(很重要)*/
		cur_pos.first = m_root->m_g -1;
	}

	if(isLeaf(m_root)){
		assert(LCP <= m);		
		assert(cur_pos.first >= 0 && cur_pos.first < m_root->m_g-1);
		assert(cur_pos.second >= 0 && cur_pos.second < m_root->m_g);

		if(LCP < m) {
			result.first = -1;
			result.second = -2;
			return result;
		} else if(LCP == m){
			//这里要记住，left_position()的返回结果不是指向和P相等的第一个位置，是指向
			//和P相等的第一个位置的前一个位置。
			result.first = m_root->m_child_off + cur_pos.first + 1;
			result.second = m_root->m_child_off + cur_pos.second;
			return result;
		}
	}

	assert(cur_pos.first >= 0 && cur_pos.first < m_root->m_g);
	assert(cur_pos.second >= 0 && cur_pos.second < m_root->m_g);

	if(cur_pos.first == cur_pos.second) {
		cbt = loadDiskPage(m_root->m_child_off + cur_pos.first);
		this->m_io++;	/*increase IO times*/
	} else {
		/*TODO: 这个时候 分别加载 不同的孩子节点，查询 Left range 和 suffix range*/
		uint64_t off1 = m_root->m_child_off + cur_pos.first;
		uint64_t off2 = m_root->m_child_off + cur_pos.second;
		result.first = suffix_range2_left_position(P, m, off1, prefix_of_P);
		result.second = suffix_range2_right_position(P, m, off2);
		return result;
	}

	while(true) {
		cur_pos = cbt->get_left_and_right_position(P, m, (const uint8_t*)m_textFile, m_B, &LCP, &this->m_io);
		if(LCP == m)
			prefix_of_P = true;

		if(cur_pos.first == -1) {
		/*表明当前的critbit_tree中，所有的后缀都要比P小*/
			if(isLeaf(cbt)){
				/*当前节点是一个叶子节点，直接返回-1*/
				assert(cur_pos.second == cbt->m_g - 1);
				delete cbt; cbt = NULL;
				result.first = -1; result.second = -2;
				return result;
			} else {
				/*如果不是叶子节点，需要加载它最后一个孩子*/
				assert(cur_pos.second == cbt->m_child_num-1);
				cur_pos.first = cbt->m_child_num -1;
			}			
		} else if(cur_pos.first	== -2){
			/*critbit_tree 中的所有的后缀都要大于P，这种情况不可能发生，因为在后缀之中必定有一个后缀严格小于P*/
			delete cbt; cbt = NULL;
			fprintf(stderr, "ERROR!!! DiskSBT::left_suffix_range(): something is wrong cur_pos = -2.\n");	
			exit(EXIT_FAILURE);

		} else if(cur_pos.second == -3) {
			/*critbit_tree 中的所有的后缀都要大于P，这种情况不可能发生，因为在后缀之中必定有一个后缀严格小于P*/		
			delete cbt; cbt = NULL;
			fprintf(stderr, "ERROR!!! DiskSBT::left_suffix_range(): something is wrong cur_pos = -2.\n");	
			exit(EXIT_FAILURE);			
		}

		if(isLeaf(cbt)){
			/*需要判断LCP是否和m相等,如果不相等, 表示没有出现以P为前缀的后缀(并不一定！！！)
			* 比如说：如果cur_pos.first = cbt->m_g-1。但是 cur_pos.first的下一个后缀（如果存在的话）
			* 可能是以P 为前缀，所以，如果是这种情况的话，需要再次访问一下文本，确定一下它的下一个后缀
			* 是否是以P 为前缀，或者我们可以保存一下上一个LCP，来确定这个，就没有必要访问这次文本了。
			* 所以这里，我们需要使用到prefix_of_P这个值。
			**/
			assert(LCP <= m);
			assert(cur_pos.first >= 0 && cur_pos.first < cbt->m_g - 1);
			assert(cur_pos.second >= 0 && cur_pos.second < cbt->m_g);

			if(LCP < m){
				 result.first = -1;
				 result.second = -2;
				 delete cbt; cbt = NULL;
				 return result;	
			}
			else if(LCP == m){
				result.first = cbt->m_child_off + cur_pos.first + 1;
				result.second = cbt->m_child_off + cur_pos.second;
				delete cbt; cbt = NULL;
				return result;
			}
		}

		// really think about it.
		assert(cur_pos.first >= 0 && cur_pos.first < cbt->m_child_num);
		assert(cur_pos.second >= 0 && cur_pos.second < cbt->m_child_num);				
		if(cur_pos.first == cur_pos.second){
			next_cbt = loadDiskPage(cbt->m_child_off + cur_pos.first);
			this->m_io++;	/**increase IO times**/

			delete cbt;
			cbt = next_cbt;
			next_cbt = NULL;

		}else if(cur_pos.first != cur_pos.second){
			uint64_t off1 = cbt->m_child_off + cur_pos.first;
			uint64_t off2 = cbt->m_child_off + cur_pos.second;
			delete cbt; cbt = NULL;
			result.first = suffix_range2_left_position(P, m, off1, prefix_of_P);
			result.second = suffix_range2_right_position(P, m, off2);
			return result;					
		}

	}//end of while(true)

}//end of DiskSBT::suffix_range2(P, m)


int64_t DiskSBT::suffix_range2_left_position(uint8_t *P,
											uint64_t m,
											uint64_t offset,
											bool prefix_of_P) {

	critbit_tree *cbt = NULL;
	critbit_tree *next_cbt = NULL;
	uint64_t LCP;
	int64_t cur_pos;

	cbt = loadDiskPage(offset);
	this->m_io++;	/*increase IO times*/

	while(true){
		cur_pos = cbt->get_left_position(P, m, (const uint8_t*)m_textFile, m_B, &LCP, &this->m_io);
		if(LCP == m)
			prefix_of_P = true;

		if(cur_pos == -1) {
			if(isLeaf(cbt)){
				cur_pos = cbt->m_g - 1;
				if(prefix_of_P) cur_pos = cbt->m_child_off + cur_pos + 1;
				else cur_pos = -1;
				delete cbt; cbt = NULL;
				return cur_pos;

			} else {
				cur_pos = cbt->m_g - 1;
			}
		}
		else if(cur_pos == -2){
			/* This can not happen! */
			fprintf(stderr, "ERROR!!! DiskSBT::suffix_range2_left_position(): cur_pos==-2.\n");;
			exit(EXIT_FAILURE);
		} else if(cur_pos == -3){
			/* This can not happen neither! */
			fprintf(stderr, "ERROR!! DiskSBT::suffix_range2_left_position(): cur_pos == -3.\n");;
			exit(EXIT_FAILURE);
		}

		if(isLeaf(cbt)){
			/*需要判断LCP是否与m相等,如果不想等，表示没有出现以P为前缀的后缀，返回-1*/
			assert(LCP <= m);
			assert(cur_pos >= 0 && cur_pos < cbt->m_g -1);
			if(prefix_of_P == false) {
				delete cbt; cbt = NULL;
				return -1;
			}
			else {
				cur_pos = cbt->m_child_off + cur_pos + 1;
				delete cbt; cbt = NULL;
				return cur_pos;
			}
		}

		assert(cur_pos>=0 && cur_pos < cbt->m_child_num);
		next_cbt = loadDiskPage(cbt->m_child_off + cur_pos);
		this->m_io++; /*icrease IO times*/

		delete cbt;
		cbt = next_cbt;		
		next_cbt = NULL;
	}// end of while(true)

}// end of DiskSBT::suffix_range2_left_position()

int64_t DiskSBT::suffix_range2_right_position(uint8_t *P,
											uint64_t m,
											uint64_t offset) {
	int64_t cur_pos;
	critbit_tree* cbt = NULL;
	critbit_tree* next_cbt = NULL;
	uint64_t LCP;

	cbt = loadDiskPage(offset);
	this->m_io++;  /*icrease IO times*/	

	while(true) {
		cur_pos = cbt->get_right_position(P, m, (const uint8_t*)m_textFile, m_B, &LCP, &this->m_io);
		if(cur_pos == -2) {
			fprintf(stderr, "ERROR!! DiskSBT::right_suffix_range(): something is wrong cur_pos = -2.\n");
			exit(EXIT_FAILURE);			
		}
		if(isLeaf(cbt)){
			assert(LCP <= m);
			if(LCP < m) {
				delete cbt; cbt = NULL;
				return -2;
			}
			else if(LCP == m){
				cur_pos = cbt->m_child_off + cur_pos;
				delete cbt; cbt = NULL;
				return cur_pos;
			}
		}

		assert(cur_pos >= 0 && cur_pos < cbt->m_child_num);
		next_cbt = loadDiskPage(cbt->m_child_off + cur_pos);
		this->m_io++;  /*increase IO times*/

		delete cbt;
		cbt = next_cbt;		

	}//end of while()

}// end of DiskSBT::suffix_range2_right_position()

uint64_t DiskSBT::getIOCounts(){
	return this->m_io;
}


bool DiskSBT::isLeaf(critbit_tree *cbt){
	return cbt->m_flag &0x1;
}


uint64_t DiskSBT::sizeInBytes(){

	FILE* in = fopen(m_sbtFile, "r");;
	if(!in){
		fprintf(stderr, "DiskSBT::sizeInBytes -- Open File '%s' error!.\n", m_sbtFile);;
		exit(EXIT_FAILURE);
	}
	fseek(in, 0, SEEK_END);
	uint64_t bytes = ftell(in);
	fclose(in);
	return bytes;
}

void DiskSBT::collectLocation(pair<int64_t, int64_t> sr, vector<uint64_t>* location){

	if(location == NULL)
		return;

	int64_t left = sr.first;
	int64_t right = sr.second;

	if(left > right)
		return;

	int64_t leftDiskNum = 1 + left / m_b;
	int64_t rightDiskNum = 1 + right / m_b;

	int64_t leftIdx = left % m_b;
	int64_t rightIdx = right % m_b;

	critbit_tree* cbt = NULL;
	uint64_t loc;
	for(int64_t i = leftDiskNum; i <= rightDiskNum; i++){
		if(i == leftDiskNum && i == rightDiskNum){
			cbt = loadDiskPage(i);
			vector<uint64_t> result;
			cbt->getAllSuffixes(result);
			for(int64_t j = leftIdx; j <= rightIdx; j++){
				loc = result[j]*m_step;
				location->push_back(loc);
			}
			delete cbt;
		}
		else if(i == leftDiskNum){
			cbt = loadDiskPage(i);
			vector<uint64_t> result;
			cbt->getAllSuffixes(result);
			for(int64_t j = leftIdx; j < result.size(); j++){
				loc = result[j]*m_step;
				location->push_back(loc);
			}
			delete cbt;
		}
		else if(i == rightDiskNum){
			cbt = loadDiskPage(i);
			vector<uint64_t> result;
			cbt->getAllSuffixes(result);
			for(int64_t j = 0; j <= rightIdx; j++){
				loc = result[j]*m_step;
				location->push_back(loc);
			}
			delete cbt;
		} else {
			cbt = loadDiskPage(i);
			vector<uint64_t> result;
			cbt->getAllSuffixes(result);
			for(int64_t j = 0; j < result.size(); j++){
				loc = result[j]*m_step;
				location->push_back(loc);
			}
			delete cbt;
		}
	}
}