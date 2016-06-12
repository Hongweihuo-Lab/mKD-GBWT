#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <math.h>
#include <queue>	// for queue
#include <utility>  // for pair and make_pair
#define __STDC_FORMAT_MACROS // for PRIu64
#include <inttypes.h>

#include "divsufsort64.h"

#include "sb_tree.h"
#include "sbt_util.h"
#include "critbit_tree.h"

#include "sdsl/bitmagic.hpp"

#define SCANBUFSIZE 10000

using namespace std;
using sdsl::bit_magic;


/**
* 1）根据文本文件创建SA.
* 2) 根据SA，创建采样的SA与采样的LCP，并且写入到磁盘文件中.
* 3) 根据上 SA文件，LCP文件，文本T，创建一棵 String B-tree
* 4) 将String B-tree 序列化到磁盘上。
**/
sb_tree::sb_tree(const char* text_file, uint64_t B, uint64_t step) {

	m_text_file = getTextFile(text_file);
	m_step = step;
	m_index_file = getSBTFile(text_file, step);   /*get the outfile*/
	m_blocks = 0;
	m_blocks ++;

	char sa_file[256];
	/* load text file */
	fprintf(stderr, "Begin Loading text.\n");
	this->m_n = sb_read_file((uint8_t*)text_file, &m_T);
	fprintf(stderr, "End Loading text.\n");

	fprintf(stderr, "\nBegin BUILD Suffix Array.\n");
	m_sa_file = getSAFile(m_text_file, m_step);
	fprintf(stderr, "End BUILD Suffix Array\n");


	/* get the max lcp between different suffixes */
	fprintf(stderr, "\nBegin BUILD LCP Array.\n");
	m_lcp_file = getLCPFile(m_text_file, m_step);
	uint64_t max_lcp = get_max_lcp02();
	fprintf(stderr, "End BUILD LCP Array.\n\n");

	this->m_nsa = sb_file_size((uint8_t*)m_sa_file) / 8;
	this->bits_per_pos = max_lcp;
	this->m_B = B;
	

	build();
}

sb_tree::~sb_tree(){
	
	// if(remove(m_lcp_file) != 0){
	// 	fprintf(stderr, "sb_tree::~sb_tree---DELETE FILE `%s` error!\n", m_lcp_file);
	// 	exit(EXIT_FAILURE);
	// }

	free(m_T);   /*free the text*/
	
	delete[] m_index_file; 
	delete[] m_lcp_file;
	delete[] m_text_file;
	delete[] m_sa_file;
}

/* given a sa and text on disk create a SB-tree with disk page size B(in bytes) */
void sb_tree::build() {


	fprintf(stderr, "\n---BUILD String B-tree---\n");
	this->bits_per_suffix = bit_magic::l1BP(this->m_n) + 1;
	this->m_b = calc_branch_factor();
	this->m_height = calc_height();

	fprintf(stderr, "n = %" PRIu64 "\n", this->m_n);
	fprintf(stderr, "bits_per_suffix = %" PRIu64 "\n", this->bits_per_suffix);
	fprintf(stderr, "bits_per_pos = %" PRIu64 "\n", this->bits_per_pos);
	fprintf(stderr, "b = %" PRIu64 "\n", this->m_b);
	fprintf(stderr, "B = %" PRIu64 "\n", this->m_B);
	fprintf(stderr, "height = %" PRIu64 "\n", this->m_height);

	/* open output file */
	FILE* out = fopen(m_index_file, "w");
	if(!out) {
		fprintf(stderr, "cannot not open output file '%s'.\n", m_index_file);
		exit(EXIT_FAILURE);
	}

	/* construct the whole sbt tree */
	FILE* sa_fd = fopen(m_sa_file, "r");   /* read suffix array from sa_file */
	FILE* lcp_fd = fopen(m_lcp_file, "r"); /* read lcp value from lcp_file */
	if(!sa_fd){
		fprintf(stderr, "sb_tree::build(): Open file %s error.\n", m_sa_file);
		exit(EXIT_FAILURE);
	}
	if(!lcp_fd){
		fprintf(stderr, "sb_tree::build(): Open file %s error.\n", m_lcp_file);
		exit(EXIT_FAILURE);
	}

	sbtmpfile* sbtf =  sbtmpfile_read_from_file(sa_fd); 
	sbtmpfile* lcp_sbtf = sbtmpfile_read_from_file(lcp_fd);

	/* write the index header first */
	write_header(out);

	/* now write a dummy root page that are going to overwrite later.
		we do this so the root file is always at the same offset in the index file */
	add_padding(out, m_B);
	create_tree(sbtf, lcp_sbtf, out);     /* create the string B-tree */
	// sbtmpfile_delete(sbtf);

	/*  close the index file */
	fclose(out);
	fprintf(stderr, "---BUILD String B-tree DONE---\n\n");

} // end of member function sb_tree build(sa_file, text_file, outfile, maxlcp, B)


void sb_tree::create_tree(sbtmpfile* suffixes,
						  sbtmpfile* lcps, 
						FILE* sbt_fd) {

	create_tree_helper(suffixes, lcps, sbt_fd, 1, 0, NULL);

} // end of member function sb_tree::create_tree()


/** create the string B-tree, and write its node into disk page.
*@param  suffixes: file descripter, read suffixes from the file.
*@param  lcps    : file descripter, 存储相邻两个采样后缀(SA[i],SA[i+1])之间的lcp的信息。
*@param  sbt_fd  : file descripter, write string B-tree node into the file.
*@param  offset  : 表示到目前为止，已经写入磁盘的 string B-tree 的节点的个数。
*@param  level   : string B-tree的叶子节点那一层为 level 0，level 向上递增。
*@param  cur_off : String B-tree当前层的每个String B-tree节点的首个孩子节点的位置和此节点的孩子的个数。
				   这个值实在下一层次计算好，然后传入到当前层次，保存在每个串B-tree的节点的信息中。
*Description: 				
**/

void sb_tree::create_tree_helper(sbtmpfile* suffixes,
								sbtmpfile* lcps,
								FILE* sbt_fd, 
								uint64_t offset,
								uint64_t level,								
								queue<pair<uint64_t, uint64_t> >* cur_off)
{

	/* we store the suffixes of next level into the file descripter: next_level. */
	sbtmpfile* next_level = sbtmpfile_create_write();
	sbtmpfile* next_lcp_level =  sbtmpfile_create_write();  /*store the lcp data*/

	sbtmpfile_open_read(suffixes);  /* read suffixes of current level */
	sbtmpfile_open_read(lcps);

	uint64_t elem_num;
	if((elem_num = sbtmpfile_elem_num(suffixes)) <= m_b) {
		/* There are less than or equal to m_b suffixes in current level
		 * construt the root node of string B-tree 
		 */
		create_tree_root(suffixes, lcps, sbt_fd, offset, level, cur_off);
		return ;
	}

	if(level == 0){

#ifdef KK_DEBUG	
		fprintf(stderr, "%lu, %lu.\n", elem_num, m_nsa);
#endif		
		assert(elem_num == m_nsa);/*确保文件中的后缀的个数和我们创建的后缀的个数相同*/
	}

	if(cur_off != NULL) {
		/*	string B-tree的当前level的节点个数ceil((double)elem_num/m_b)，和cur_off中元素的个数相同。
			因为cur_off中每个元素存储了其中每个节点的孩子的位置和个数
		 */
		if(cur_off->size() != (uint64_t)ceil((double)elem_num / m_b) ){
			fprintf(stderr, "error! cur_off->size() != elem (%lu, %lu), %lu, %lu.\n", 
									cur_off->size(), 
									elem_num,
									m_b,
									m_n);

			fprintf(stderr, "level = %lu.\n", level);
		}
		assert(cur_off->size() == (uint64_t)ceil((double)elem_num / m_b) );  /* a simple test */
	}

	/* read m_b suffixes and process */
	uint64_t* cur_suf = (uint64_t*)sb_malloc(sizeof(uint64_t) * m_b);
	uint64_t* next_suf = (uint64_t*)sb_malloc(sizeof(uint64_t) * m_b);

	uint64_t* cur_lcp = (uint64_t*)sb_malloc(sizeof(uint64_t) * m_b);
	uint64_t* next_lcp = (uint64_t*)sb_malloc(sizeof(uint64_t) * m_b);
	
	/*next_off需要传递到下一层，存储了下一层中每个节点的孩子.*/
	queue<pair<uint64_t, uint64_t> >* next_off = new queue<pair<uint64_t, uint64_t> >();
	uint64_t j = 0, cnsuf, cnlcp;
	uint64_t block_processed = 0, next_level_block = 0;
	uint64_t prev_lcp_val = 0XFFFFFFFFFFFFFFFF;

	while( (cnsuf = sbtmpfile_read_block(suffixes, cur_suf, m_b)) > 0 ) {
        /*每次读取m_b个后缀，有可能少于m_b个，当cnsuf == 0的时候，说明到了文件的尾部 */

		cnlcp = sbtmpfile_read_block(lcps, cur_lcp, m_b);

		assert(cnlcp == cnsuf);

	#ifdef KK_DEBUG	
		fprintf(stderr, "PROCESSING  %lu suffixes.\n", cnsuf);
		fprintf(stderr, "PROCESSING %lu blocks.\n", offset);
		fprintf(stderr, "creating critbit tree.\n");
	#endif
		
		critbit_tree cbt;  /* 创建critbit_tree */
		cbt.set_text(m_T, m_n);
		cbt.create_from_suffixes2(cur_suf, cur_lcp, cnsuf); /*向critbit_tree中插入数据*/
		/* set current suffixes child's offset and number */
		if(cur_off != NULL){
            /**用来确定此节点的孩子在文件中的位置:
			* set_child_offset: 起始偏移量(在文件中偏移的disk_page的个数)
			* set_child_num: 孩子节点的个数(因为孩子节点是连续存储的)
			**/
			cbt.set_child_offset(cur_off->front().first); 
			cbt.set_child_num(cur_off->front().second);
			cur_off->pop();

		} else {
			cbt.set_child_offset(block_processed * m_b); 
			cbt.set_child_num(0);
		}

		/* TODO: set critbit_tree level and flags */
		uint32_t flags = 0;
		if(level == 0) flags = flags | 0x1; /*表明这是一个叶节点 */
		cbt.set_flag(flags);
		cbt.set_level(level);              /*设置当前节点的level */

		/* write the current cbt to the disk page */
		uint64_t written = cbt.write(sbt_fd); /*将 critbit_tree写入到磁盘中，具体写入什么看critbit_tree*/
		cbt.delete_text();                   /*解除critbit_tree 与文本m_T的关系，防止在释放critbit_Tree的时候，删除这个文本*/
#ifdef KK_DEBUG
		fprintf(stderr, "write %ld bytes to disk.\n", written);
#endif
		if(written > m_B) {
            /*确保写入的磁盘块中的数据不会大于m_B，否则表示出现了错误*/
			fprintf(stderr, "ERROR! critbit tree larger than block size! (%lu, %lu)\n", written, this->m_B);
			exit(EXIT_FAILURE);
		}

		/* add the padding to fill up the disk page */
		add_padding(sbt_fd, m_B - written);

		offset ++; /*表明到如今为止，一共处理了多少个node(or disk page) */
		m_blocks ++;

		block_processed ++; /*在当前的 level中所处理的 block的个数*/

		/* add the first suffix to the next suffix file */
		next_suf[j] = cur_suf[0];
		next_lcp[j] = min(cur_lcp[0], prev_lcp_val);
		j++;
		prev_lcp_val = cnlcp > 1 ? cur_lcp[1] : 0XFFFFFFFFFFFFFFFF;
		for(uint64_t r = 1; r < cnlcp; r++){
			if(prev_lcp_val > cur_lcp[r])
				prev_lcp_val = cur_lcp[r];
		}   
		
		if(j == m_b) {
#ifdef KK_DEBUG			
			fprintf(stderr, "writting %lu suffixes to next level.\n", j);
#endif
			sbtmpfile_write_block(next_level, next_suf, j); /*将m_b个后缀写入到文件中，作为下一轮的中输入的后缀 */
			sbtmpfile_write_block(next_lcp_level, next_lcp, j);

			/* add next level's child offset and child number */
			next_off->push(make_pair(offset - j, j) );
			j = 0;
			next_level_block ++;
		}

	} // end of while()

	if(j > 0) {
#ifdef KK_DEBUG
		fprintf(stderr, "writting %lu suffixes to next level.\n", j);
#endif
		sbtmpfile_write_block(next_level, next_suf, j);
		sbtmpfile_write_block(next_lcp_level, next_lcp, j);
		/* add next level's child offset and child number */
		next_off->push(make_pair(offset - j, j) );
		next_level_block++;
	}

#ifdef KK_DEBUG
	fprintf(stderr, "processed %lu blocks.\n", block_processed);
#endif
	/* free the resources */
	if(cur_off != NULL)
		delete cur_off;

	free(cur_suf);
	free(next_suf);
	free(cur_lcp);
	free(next_lcp);

	sbtmpfile_finish(next_level);  /* finish write and begin read，当前后缀写入完毕，接下来需要读*/
	sbtmpfile_finish(next_lcp_level);
	sbtmpfile_delete(suffixes);    /* delete the suffix file, 清理掉这个suffix file*/
	sbtmpfile_delete(lcps);

	/* process the next level */
	create_tree_helper(next_level, next_lcp_level, sbt_fd, offset, level + 1, next_off);
} // end of member function sb_tree::create_tree_helper

/* TODO: if the root is the only node */

void sb_tree::create_tree_root(sbtmpfile_t* suffixes, 
								sbtmpfile_t* lcps,
								FILE* sbt_fd,
								uint64_t offset, 
								uint64_t level,
								queue<pair<uint64_t, uint64_t> >* cur_off) {

	uint64_t* cur_suf = (uint64_t*) sb_malloc(sizeof(uint64_t) * m_b);
	uint64_t* tmp_suf = (uint64_t*) sb_malloc(sizeof(uint64_t) * m_b);
	uint64_t* cur_lcp = (uint64_t*) sb_malloc(sizeof(uint64_t) * m_b);

	uint64_t j = 0, k = 0, jlcp;

	// sbtmpfile_open_read(suffixes); /* have been done it. */

	uint64_t elem_num;
	if((elem_num = sbtmpfile_elem_num(suffixes)) > m_b){
		/* something is wrong */
		fprintf(stderr, "error sb_tree::create_tree_root: root has more than b suffixes.\n");
		exit(EXIT_FAILURE);
	}

	j = sbtmpfile_read_block(suffixes, cur_suf, m_b);  /* file suffixes has less than or equal to  m_b suffixes */
	jlcp = sbtmpfile_read_block(lcps, cur_lcp, m_b);
	assert(j == jlcp);

	if(cur_off != NULL){
		assert(cur_off->size() == 1);  /* a simple test */
	}

	fprintf(stderr, "processing root: %lu suffixes.\n", j);
	fprintf(stderr, "createing critbit tree.\n");

	/* create the critbit tree of root */
	critbit_tree cbt;
	cbt.set_text(m_T, m_n);
	cbt.create_from_suffixes2(cur_suf, cur_lcp, j);
	if(cur_off != NULL) {
		cbt.set_child_offset(cur_off->front().first);
		cbt.set_child_num(cur_off->front().second);
		cur_off->pop();
	} else {

		cbt.set_child_offset(0);
		cbt.set_child_num(0);
	}
	
	cbt.set_level(level);
	uint64_t flag = 0;
	if(level == 0) flag = flag | 0x1; /* the least significant bit: it is leaf */
	flag = flag | 0x80000000;   /* the most significant bit: it is root */
	cbt.set_flag(flag);
	/* seek the sbt_fd to the write offset，从文件起始位置偏移4096字节处写入根节点 */
	fseek(sbt_fd, 4096, SEEK_SET);
	/* write the critbit_tree to disk */
	uint64_t written  =  cbt.write(sbt_fd);
	fprintf(stderr, "root: writing %lu bytes into disk.\n", written);
	if(written > m_B){
		fprintf(stderr, "ERROR! root crirbit_tree is larger than page block (%lu, %lu).\n", written, m_B);
		exit(EXIT_FAILURE);
	}

	free(cur_suf);
	free(tmp_suf);
	free(cur_lcp);
	sbtmpfile_delete(suffixes);
	if(cur_off != NULL)
		delete cur_off;

	return;
}

/* write the index header + padding */
void sb_tree::write_header(FILE* out) {
	uint64_t written =  0;
	fseek(out, 0, SEEK_SET);

	written += fwrite(&m_n, sizeof(uint64_t), 1, out);
	written += fwrite(&bits_per_suffix, sizeof(uint64_t), 1, out);
	written += fwrite(&bits_per_pos, sizeof(uint64_t), 1, out);
	written += fwrite(&m_b, sizeof(uint64_t), 1, out);
	written += fwrite(&m_B, sizeof(uint64_t), 1, out);
	written += fwrite(&m_height, sizeof(uint64_t), 1, out);

	/* pad up to SBT_ROOT_OFFSET bytes so we have nice alignment */
	add_padding(out, SBT_ROOT_OFFSET - (written * sizeof(uint64_t)));
}


/* add padding to the index file to get nice alignment */
void sb_tree::add_padding(FILE* out, uint64_t bytes) {
	if(bytes) {
		uint8_t* dummy = (uint8_t*) sb_malloc(bytes);
		memset(dummy, 0, bytes);
		fwrite(dummy, 1, bytes, out);
		free(dummy);
	}
}


/* read the index header */
void sb_tree::read_header(FILE* in) {
	uint64_t read = 0;
	read += fread(&m_n, sizeof(uint64_t), 1, in);
	read += fread(&bits_per_suffix, sizeof(uint64_t), 1, in);
	read += fread(&bits_per_pos, sizeof(uint64_t), 1, in);
	read += fread(&m_b, sizeof(uint64_t), 1, in);
	read += fread(&m_B, sizeof(uint64_t), 1, in);
	read += fread(&m_height, sizeof(uint64_t), 1, in);

	if(read != 6) {
		fprintf(stderr, "error reading index file.\n");
		exit(EXIT_FAILURE);
	}
}

uint64_t sb_tree::get_max_lcp02(){
	int64_t max_posarray_width = -1;

	FILE* flcp = fopen(m_lcp_file, "r");
	if(!flcp){
		fprintf(stderr, "sb_tree::get_max_lcp02(): Open file %s error!\n", m_lcp_file);
		exit(EXIT_FAILURE);
	}
	int64_t* buffer = new int64_t[SCANBUFSIZE];  //it should be int64_t not uint64_t
	int k;
	while(true){
		k = fread(buffer, sizeof(uint64_t), SCANBUFSIZE, flcp);
		for(int64_t i = 0; i < k; i++)
			max_posarray_width = max(max_posarray_width, buffer[i]);
		if(feof(flcp))
			break;
	}

	delete []buffer;
	/* write LCP into file */
	return bit_magic::l1BP((uint64_t)max_posarray_width) + 1 + 3;
}


uint64_t sb_tree::get_max_lcp(){
	uint64_t max_posarray_width = 0;

	uint64_t* rank = new uint64_t[m_n]; //reverse suffix array. (rank array)
	for(uint64_t i = 0; i < m_n; i++)
		rank[m_sa[i]] = i;

	uint64_t* lcp = new uint64_t[m_n]; 	//lcp array.

	int64_t l = 0, i = 0;				// The LCP meaning?
	for(i = 0; i < m_n; i++){
		uint64_t k = rank[i];
		if(k == 0)
			continue;
		uint64_t j = m_sa[k-1];
		while(m_T[i+l] == m_T[j+l])
			l++;
		lcp[k] = l;
		if(l > 0)
			l = l - 1;
	}

	for(i = 0; i < m_n; i++)
		// if(m_sa[i] % m_step == 0 && m_sa[i] != 0 && max_posarray_width < lcp[i])
		if(m_sa[i] % m_step == 0 && max_posarray_width < lcp[i])
			max_posarray_width = lcp[i];

	int j;
	bool first = false, prev = true;
	uint64_t tmpVal;

	for(i = 0, j = 0; i < m_n; i++){
		// if(m_sa[i] % m_step == 0 && m_sa[i] != 0){
		if(m_sa[i] % m_step == 0) {
			if(prev == true) {
				//the first time through this.
				tmpVal = lcp[i];
			} else {
				if(tmpVal > lcp[i])
					tmpVal = lcp[i];
			}
			lcp[j++] = tmpVal;
			prev = true;

			if(!first)
				first = true;
		} else {
			if(first == true) {
				if(prev == true){
					tmpVal = lcp[i];
				} else {
					tmpVal = min(tmpVal, lcp[i]);
				}
				prev =false;
			}
		}
	}

	/* write LCP into file */
	FILE* lcp_fd = fopen(m_lcp_file, "w");
	if(lcp_fd == NULL){
		fprintf(stderr, "error! sb_tree::get_max_lcp02(), open lcp file error!\n");
		exit(EXIT_FAILURE);
	}
	int64_t size = fwrite(lcp, sizeof(uint64_t), j, lcp_fd);
	if(size != j){
		fprintf(stderr, "error! sb_tree::get_max_lcp02(), write lcp data error!.\n");
		exit(EXIT_FAILURE);
	}
	fclose(lcp_fd);

	delete []lcp;
	delete []rank;
	return bit_magic::l1BP(max_posarray_width) + 1 + 3;
}

/*** 
*	For a given diskpage size B in bytes and the size of text n,
*	calculate the branching factor b.
*	
*	Calculation as follows: (assume a node contains x suffixes)
*		-- each node in the SB-T contains b <= |x| <= 2b suffixes
*		-- each node must fit into B bytes.
*		-- each ndoe contains:
*			1)	|x| + 1 child pointers (offset into the sb index to the diskpage contains the child)
*				
*				note: as the sb-tree is organized via disk pages, 
*						we can use log(n/(2b)) bits per child pointers.
*			2) a blind trie over all |n| suffixes.
*
*	NOTE: It is okay that we only calculate the largest b.
*			and the m_b = 2*b;
*
*
*	string B-tree child pointer: 
*
*	bp(bit_vector): 
					((2*(m_g + m_g -1) + 63) >> 6) << 3 bytes
*							= (4*m_g -2)
*							= 0.5*m_g + 8bytes  (most occupied)
*
*	suffix array:   	
*					((m_g*log(n) + 63) >> 6 ) << 3 bytes
							= (m_g * log(n)) / 8.0 + 8 bytes  (most occupied)
*
*   pos array:  maxlcp < n
*
*					((m_g*log(maxlcp) + 63) >> 6) << 3 bytes
							= (m_g * log(n)) / 8.0 + 8 bytes   (most occupied)					
*
*	pointer array: (m_g ) pointers
*					 
*					(((m_g)*log(n / (2b-1)) + 63) >> 6) << 3 bytes
*							= (m_g * log(n / (2b-1) )) / 0.8 + 8 bytes (most occupied.)
*
*   m_g:				8byte 		|}
*   pos_width: 			8byte		|}  
*	suffix_width:   	8byte		|} 	
*	pointer_width:		8byte		|}	totally 52 byte
*	slibing pointer:	4byte       |}
*	child offset:		4byte 		|}
*	# of page block 	4byte 		|}	
*	flag:				4byte 		|}
* 	level:				4byte(32bit)|}	
*
*  so the branch factors is (B - 88) / (0.5 + (log(n) + log(n) + _B)/8.0);
*  And I choose to loop to get the proper _B ()
***/

uint64_t sb_tree::calc_branch_factor(){
	uint64_t _B = calc_branch_factor_with_maxlcp();
	return _B;
}


uint64_t sb_tree::calc_branch_factor_with_pointer(){
	uint64_t _B = 0;
	/*每个指向孩子的指针需要32个bit。
	* 为了保存树形，需要2(m_g+m_g-1)，所以每个m_g需要4个bit。
	*/
	uint64_t bitsPerSuffix = 4 + this->bits_per_pos + this->bits_per_suffix + 32;
	_B = (uint64_t)( ((this->m_B - 84) * 8) / bitsPerSuffix);
	fprintf(stderr, "calculating branch factor (b = %lu)\n", _B);
	return _B;
}

uint64_t sb_tree::calc_branch_factor_without_pointer(){
	uint64_t _B = 0;
	uint64_t bitsPerSuffix = 4 + this->bits_per_pos + this->bits_per_suffix;
	_B = (uint64_t)(((this->m_B - 72)*8) / bitsPerSuffix);
	return _B;
}

uint64_t sb_tree::calc_branch_factor_with_maxlcp(){
	uint64_t _B = 0;
	uint64_t bitsPerSuffix = 4 + this->bits_per_pos + this->bits_per_suffix;
	_B = (uint64_t)(((this->m_B - 72)*8)/bitsPerSuffix);
	return _B;
}

uint64_t sb_tree::calc_branch_factor_without_maxlcp(){

	uint64_t _B = 0;
	//bits_per_pos = bits_per_suffix bit.
	uint64_t bitsPerSuffix = 4 + this->bits_per_suffix + this->bits_per_suffix;
	_B = (uint64_t)(((this->m_B - 72)*8)/bitsPerSuffix);
	return _B;
}