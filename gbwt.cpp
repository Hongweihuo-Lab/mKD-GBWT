#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <ctime>
#include <algorithm>

#include "sbt_util.h"
#include "sdsl/bitmagic.hpp"
#include "gbwt.h"

using namespace std;
// using namespace sdsl;


GBWT::GBWT(const char* textFile, uint64_t B, uint64_t step) {

	m_textFile = new char[strlen(textFile) + 1];
	strcpy(m_textFile, textFile);
	m_step = step;
	m_B = B;

	m_sbtFile = getSBTFile(textFile, step);
	m_kdtFile = getKDTFile(textFile, step);
	
	m_resFile = getTestFile(textFile);;
	m_out = fopen(m_resFile, "a");
	if(!m_out) {
		fprintf(stderr, "GBWT::GBWT()--Open file %s error!.\n", m_resFile);
		exit(EXIT_FAILURE);
	}

	time_t begin, end;
	double sbt_time, kdt_time;

	begin = clock();
	/*创建String B-tree，串B-树已经存储到了磁盘上*/
	m_sbt = new sb_tree(textFile, B, step);
	end = clock();
	sbt_time = (end - begin) / CLOCKS_PER_SEC;

	if(step == 1)
		return;		/*create only an string B-tree*/

	/*创建 suffix array file，从中读取在后缀树过程中建立的后缀数组*/
	const char* outFile = getSBTFile(textFile, step);
	strcpy(m_sa_file, outFile);
	strcat(m_sa_file, ".saraw");  //现在m_sa_file是存储后缀数组的文件名称。
	delete []outFile;			  //删除临时文件名。

	/*读取文本 into m_T，创建点集*/
	m_n = sb_read_file((const uint8_t*)m_textFile, &m_T);

	fprintf(stderr, "\nRead sample suffix array: %s\n", m_sa_file);
	m_g = sa_count(m_sa_file);		//采样的后缀数组大小。
	m_sa = sa_create(m_sa_file); 	//将采样的后缀数组读取到内存中。
	suf_create();
	fprintf(stderr, "\nRead sample suffix array done! size = %.2fGB\n",(double)(m_g*8) / (1024.0*1024.0*1024.0));					
	//创建m_suf[256]

	// TODO: 删除采样的后缀数组文件
	// if(remove(m_sa_file) != 0){
	// 	fprintf(stderr, "GBWT::GBWT -- DELETE file '%s' error!\n", m_sa_file);
	// 	exit(EXIT_FAILURE);
	// }

	//TODO:简单的测试
	for(uint64_t i = 0; i < m_g; i++){
		// assert(m_sa[i] % m_step == 0 && m_sa[i] != 0);
		assert(m_sa[i] % m_step == 0);
	}
	
	/*创建点集S*/
	// vector<uint64_t> *coord_x = new vector<uint64_t>();
	// vector<uint64_t> *coord_y = new vector<uint64_t>();
	// vector<Point>* point = new vector<Point>();
	fprintf(stderr, "\nCreate point Begin\n");
	uint64_t memSize;
	void* point = newPointVector(m_step, m_g + 1, memSize);
	create_point(point);
	fprintf(stderr, "\nCreate point Done. size = %.2f\n",(double)memSize / (1024.0*1024.0*1024.0));
	
	m_points = pointSize(point, m_step);

	delete m_sbt;
		
	/* write the point into the file */
	//save_point(coord_x, coord_y);

/****************创建 KD-树***************************/	

	FILE* out = fopen(m_kdtFile, "w");
	m_diskOut = new DiskFile(out, 0);

	fprintf(stderr, "---BUILD KDTree: Creating KDTree %d points,", m_points);
	begin = clock();
	writeMultiKDTHeader();
	create_MultiKDT(point);
	writeMultiKDTHeader();
	end = clock();
	kdt_time = (end - begin) / CLOCKS_PER_SEC;
	fprintf(stderr, "---BUILD KDTree DONE---\n\n");
	
	// delete point;

//Counting
	fprintf(m_out, "\n\nMKD-GBWT(Build):\n");
	fprintf(m_out, "B = %luKB\n", m_B);
	fprintf(m_out, "step(blocking-factor) = %lu\n", m_step);
	fprintf(m_out, "data: %s\n", textFile);
	fprintf(m_out, "data-size: %dbytes = %.2fMB\n", m_n, (m_n / (1024.0*1024.0)));
	fprintf(m_out, "Build-SBT-time: %.2fs\n", sbt_time);
	fprintf(m_out, "Build-KDT-time: %.2fs\n", kdt_time);
	fprintf(m_out, "Number of All points: %lu\n", m_points);
	fprintf(m_out, "Max points number of every kd-tree: %lu\n\n", m_maxPoints);

	fclose(m_out);
	delete [] m_resFile;

}// end of GBWT();

GBWT::~GBWT() {

	fclose(m_diskOut->m_out);
	delete m_diskOut;

	free(m_T);	//创建点集之后就可以释放文本文件和后缀数组文件了。
	free(m_sa);

	delete [] m_textFile;
	delete [] m_sbtFile;
	delete [] m_kdtFile;

}

uint64_t GBWT::sa_count(const char* sa_file){
	uint64_t bytes = sb_file_size((const uint8_t*)sa_file);
	assert(bytes % 8 == 0);
	uint64_t count = bytes / 8;
	return count;
}

uint64_t* GBWT::sa_create(const char* sa_file){
	uint64_t count = sa_count(sa_file);
	uint64_t* sa = (uint64_t*)malloc(sizeof(uint64_t)*count);
	FILE* sa_fd = fopen(sa_file, "r");
	if(sa_fd == NULL){
		fprintf(stderr, "GBWT::sa_create(): open sa_file '%s' error!!.\n", sa_file);
		exit(EXIT_FAILURE);
	}
	uint64_t m = fread(sa, sizeof(uint64_t), count, sa_fd);
	if(m != count){
		fprintf(stderr, "GBWT::sa_create(): read sa_fle '%s' error!!.\n", sa_file);
		exit(EXIT_FAILURE);
	}
	fclose(sa_fd);
	return sa;
}

void GBWT::create_point(void* point) {
	int64_t i = 0;
	int64_t k = 0;
	uint8_t* gbwt = (uint8_t*)malloc(sizeof(uint8_t)*m_step);	//需要保存字符的逆序。
	uint64_t y;  			//存储y轴对应的值。

	/*m_g：后缀数组的长度*/
	for(i = 0; i < m_g; i++){

		//assert(m_sa[i] != 0);
		if(m_sa[i] == 0)
			continue;
			
		// coord_x->push_back(i);	/*点横坐标为：0,1,2,...m_g-1*/
		assert((int64_t)(m_sa[i]) - (int64_t)m_step >= 0);
		for(k = 1; k <= m_step; k++){
			gbwt[k-1] = m_T[m_sa[i]-k];	/*逆序收集 m_step个字符*/
		}
		
		y = 0;	/*计算相应的y轴坐标*/
		y = y | gbwt[0];
		for(k = 1; k < m_step; k++){
			y = y << 8;
			y = y | gbwt[k];
		}
		
		/*将y轴坐标存储到 coord_y中*/
		// coord_y->push_back(y);
		// point->push_back(Point(i, y));
		pointPushBack(point, m_step, i, y);

	}//end of for
	
	free(gbwt);
}//end of GBWT::create_point()




/* save point into file for vertify corretness*/
void GBWT::save_point(vector<uint64_t>* coord_x, vector<uint64_t>* coord_y){

	char xpoint_file[256];
	char ypoint_file[256];

	char point_file[256];
	strcpy(point_file, m_textFile);
	int64_t len = strlen(point_file);
	assert(len > 0);
	int64_t i, j;
	for(i = len -1; i >= 0; i--){
		if(point_file[i] == '/')
			break;
	}
	if(i != -1){
		/*get the file name*/
		for(j = 0, ++i; i < len; i++, j++)
			point_file[j] = point_file[i];
		point_file[j] = '\0';
	} 
	
	strcpy(xpoint_file, "./raw_data/");
	strcpy(ypoint_file, "./raw_data/");
	strcat(xpoint_file, point_file);
	strcat(ypoint_file, point_file);
	strcat(xpoint_file, "._");
	strcat(ypoint_file, "._");
	point_file[0] = (m_step + '0');
	point_file[1] = '\0';
	strcat(xpoint_file, point_file);
	strcat(ypoint_file, point_file);
	strcat(xpoint_file, "x.point");
	strcat(ypoint_file, "y.point");


	uint64_t n1 = coord_x->size();
	uint64_t n2 = coord_y->size();
	assert(n1 == n2);
	uint64_t* x = (uint64_t*)malloc(sizeof(uint64_t)*n1);
	uint64_t* y = (uint64_t*)malloc(sizeof(uint64_t)*n2);
	if(x == NULL || y == NULL){
		fprintf(stderr, "GBWT::save_point(): alloc memory error.\n");
		exit(EXIT_FAILURE);
	}

	for(uint64_t i = 0; i < n1; i++){
		x[i] = coord_x->at(i);
		y[i] = coord_y->at(i);
	}


	FILE *f1, *f2;
	f1 = fopen(xpoint_file, "w");
	if(f1 == NULL){
		fprintf(stderr, "GBWT::save_point(): open file '%s' error.\n", xpoint_file);
		exit(EXIT_FAILURE);
	}
	f2 = fopen(ypoint_file, "w");
	if(f2 == NULL) {
		fprintf(stderr, "GBWT::save_point(): open file '%s' error.\n", ypoint_file);
		exit(EXIT_FAILURE);
	}

	if(fwrite(x, sizeof(uint64_t), n1, f1) != n1){
		fprintf(stderr, "GBWT::save_point(): write x point error.\n");
		exit(EXIT_FAILURE);
	}
	if(fwrite(y, sizeof(uint64_t), n2, f2) != n2){
		fprintf(stderr, "GBWT::save_point(): write y point error.\n");
		exit(EXIT_FAILURE);
	}

	free(x);
	free(y);		
}


void GBWT::suf_create(){

	uint8_t ch = m_T[m_sa[0]];
	int64_t left = -1;
	int64_t right = -1;
	//初始化操作

	for(int64_t i = 0; i < m_g; i++){
		
		if(i == 0){
			ch = m_T[m_sa[i]];
			left = i;
			right = -1;
			continue;
		}
		
		uint8_t chNext = m_T[m_sa[i]];
		if(chNext != ch){
			m_suf[ch].firstChara = ch;
			m_suf[ch].exist = true;
			m_suf[ch].left  = left;
			m_suf[ch].right = i - 1;

			left = i;
			right = -1;
			ch = chNext;
		}
		
	}//end of for
	
	m_suf[ch].firstChara = ch;
	m_suf[ch].exist = true;
	m_suf[ch].left = left;
	m_suf[ch].right = m_g - 1;

	this->m_nChar = 0;		//the number of different character in m_T;

	for(int64_t i = 0; i < 256; i++)
		m_char[i] = 0;

	for(int64_t i = 0, j = 0; i < 256; i++)
		if(m_suf[i].exist){
			this->m_nChar++;
			m_char[j] = i;
			m_CharIdx[j] = m_suf[i].left;
			j++;
		}
}// end of suf_create()


void GBWT::create_MultiKDT(void* point){
	//要把点分为不同的部分 [m_nChar]x[m_nChar]
	uint64_t count = 0;	//统计创建的KD-tree的个数。
	uint64_t maxPoints = 0;  //统计所有KD-tree中包含的点的个数的最大值

	void* mp[256][256];
	for(int64_t i = 0; i < 256; i++){
		for(int64_t j = 0; j < 256; j++){
			mp[i][j] = NULL;
		}
	}
	
	get_point(point, mp);
	// delete point;
	deletePoint(point, m_step);

	//store smaller m_sa[i] value
	for(uint64_t i = 0; i < m_g; i++){
		assert(m_sa[i] % m_step == 0);
		m_sa[i] = m_sa[i] / m_step;
	}

	for(int64_t i = 0; i < m_nChar; i++){
		for(int64_t j = 0; j < m_nChar; j++){
			
			uint8_t charx = m_char[i]; //取得这两个字符
			uint8_t chary = m_char[j];

			void* sp = mp[charx][chary];

			//将点集按照字符 charx 和 字符 chary 进行划分。
			if(!sp)
				continue;
			assert(sp != NULL);	
			
			if(maxPoints < pointSize(sp, m_step))
				maxPoints = pointSize(sp, m_step);
			
			//将点集插入到一颗KD-tree中
			uint64_t xWidth = m_suf[charx].right - m_suf[charx].left;
			int32_t xBitWidth = sdsl::bit_magic::l1BP(xWidth) + 1;
			int32_t yBitWidth = 8*(m_step-1);
			int32_t SABitWidth = sdsl::bit_magic::l1BP(m_g) + 1;
			KDTree::Set(m_B, xBitWidth, yBitWidth, SABitWidth);
			KDTree::SetStep(m_step);
			
			KDTree *kdt = new KDTree(sp, m_sa);
			int32_t diskNum = kdt->SaveToDisk_2(m_diskOut);	//将KD-tree存储到磁盘上，并且返回磁盘页号。
			m_suf[charx].diskNum[chary] = diskNum;
			
			count++;
			// delete sp;
			deletePoint(sp, m_step);
			delete kdt;
			
		}// end of for.
		
	}//end of for

	fprintf(stderr, "Totoally %d characters.\n", m_nChar);
	fprintf(stderr, "Creating %ld KDTrees.\n", count);
	fprintf(stderr, "The max number of Points of each KD-tree is %ld.\n", maxPoints);

}

void GBWT::create_MultiKDT(vector<uint64_t>* coord_x, 
							vector<uint64_t>* coord_y){

	//要把点分为不同的部分 [m_nChar]x[m_nChar]
	uint64_t count = 0;	//统计创建的KD-tree的个数。
	uint64_t maxPoints = 0;  //统计所有KD-tree中包含的点的个数的最大值

	vector<uint64_t>* vx[256][256] ;
	vector<uint64_t>* vy[256][256] ;
	for(int64_t i = 0; i < 256; i++){
		for(int64_t j = 0; j < 256; j++){
			vx[i][j] = NULL;
			vy[i][j] = NULL;
		}
	}

	get_point(coord_x, coord_y, vx, vy);

	//store smaller m_sa[i] value
	for(uint64_t i = 0; i < m_g; i++){
		assert(m_sa[i] % m_step == 0);
		m_sa[i] = m_sa[i] / m_step;
	}
	
	for(int64_t i = 0; i < m_nChar; i++){
		for(int64_t j = 0; j < m_nChar; j++){
			
			uint8_t charx = m_char[i]; //取得这两个字符
			uint8_t chary = m_char[j];

			vector<uint64_t>* px = vx[charx][chary];
			vector<uint64_t>* py = vy[charx][chary];

			//将点集按照字符 charx 和 字符 chary 进行划分。
			if(!px)
				continue;
			assert(px != NULL && py!=NULL);	
			assert(px->size() == py->size());
			
			if(maxPoints < px->size())
				maxPoints = px->size();
			
			//将点集插入到一颗KD-tree中
			uint64_t xWidth = m_suf[charx].right - m_suf[charx].left;
			int32_t xBitWidth = sdsl::bit_magic::l1BP(xWidth) + 1;
			int32_t yBitWidth = 8*(m_step-1);
			int32_t SABitWidth = sdsl::bit_magic::l1BP(m_g) + 1;
			KDTree::Set(m_B, xBitWidth, yBitWidth, SABitWidth);
			//int fi = i, fj = j;
			//fprintf(stderr, "-----kdt[%d][%d], point = %d, m_B`= %d, m_F = %d----\n",fi,fj,px->size(),KDTree::m_numPoint, KDTree::m_F);
			KDTree *kdt = new KDTree(*px, *py, m_sa);
			int32_t diskNum = kdt->SaveToDisk_2(m_diskOut);	//将KD-tree存储到磁盘上，并且返回磁盘页号。
			m_suf[charx].diskNum[chary] = diskNum;
			
			count++;
			delete px;
			delete py;
			delete kdt;
			
		}// end of for.
		
	}//end of for

	fprintf(stderr, "Totoally %d characters.\n", m_nChar);
	fprintf(stderr, "Creating %ld KDTrees.\n", count);
	fprintf(stderr, "The max number of Points of each KD-tree is %ld.\n", maxPoints);
}// end of create_MultiKDT


void GBWT::get_point(void* point, void* mp[][256]){
//我们需要根据每个点，判断这个点所属于的范围
	assert(m_sa != NULL);
	assert(m_T != NULL);
	
	uint64_t** pNum = new uint64_t*[256];
	for(uint64_t i = 0; i < 256; i++){
		pNum[i] = new uint64_t[256];
	} 

	for(uint64_t i = 0; i< 256; i++){
		for(uint64_t j = 0; j< 256; j++){
			pNum[i][j] = 0;
		}
	}

	for(uint64_t k = 0; k < pointSize(point, m_step); k++){
		uint64_t x = pointGetX(point, m_step, k);
		uint64_t y = pointGetY(point, m_step, k);
		uint64_t sufpos = m_sa[x];	
		uint8_t i, j;   //i, j表示这个点所对应的首字符。

		i = m_T[sufpos];	
		assert(sufpos > 0);
		j = m_T[sufpos - 1];

		pNum[i][j]++;					
	}

	fprintf(stderr, "\nCreate multi-kdtree Begin:\n");
	uint64_t allMemSize = 0;
	uint64_t memSize = 0;
	for(uint64_t i = 0; i < 256; i++){
		for(uint64_t j = 0; j < 256; j++){
			if(pNum[i][j] != 0){
				mp[i][j] = newPointVector(m_step, pNum[i][j]+1, memSize);
				allMemSize += memSize;
			} else {
				mp[i][j] = NULL;
			}
		}
	}
	fprintf(stderr, "\nCreate multi-kdtree-End size = %.2f\n", (double)allMemSize / (1024.0*1024.0*1024.0));

	//依次判断每个点。
	for(uint64_t k = 0; k < pointSize(point, m_step); k++){

		// uint64_t x = (*point)[k].m_x;	//x, y分别表示每个点。
		// uint64_t y = (*point)[k].m_y;
		uint64_t x = pointGetX(point, m_step, k);
		uint64_t y = pointGetY(point, m_step, k);
		
		/*根据这个点的横坐标，找到它对应的后缀，
		* 这个后缀的首字母，就是这个点的横坐标所对应的首字母
		* 这个后缀的前一个字符，就是这个点的纵坐标所对应的首字母。
		*/
		uint64_t sufpos = m_sa[x];
		
		uint8_t i, j;   //i, j表示这个点所对应的首字符。

		i = m_T[sufpos];	
		assert(sufpos > 0);
		j = m_T[sufpos - 1];

		assert(mp[i][j] != NULL);
		
		/*
		* 我们需要对这些点进行处理：
		* 1. 存储每个x轴坐标可以使用更少的空间。
		* 2. y轴的范围要去掉首字母，也就是使用更少的空间。
		*/
		assert(x >= m_suf[i].left);
		x = x - m_suf[i].left;
		assert((y >> ((m_step-1)*8) == j));
		y &= ~(0xFFULL << ((m_step-1)*8));
		// mp[i][j]->push_back(Point(x, y));
		pointPushBack(mp[i][j], m_step, x, y);
		
	}//end of for()

//counting
	m_maxPoints = 0;
	for(int i = 0; i < 256; i++){
		for(int j = 0; j < 256; j++){
			if(mp[i][j] != NULL){
				m_maxPoints = std::max(m_maxPoints, pointSize(mp[i][j], m_step));
			}
		}
	}

	for(uint64_t i = 0; i < 256; i++){
		delete []pNum[i];
	}
	delete []pNum;
}


/*根接两个字符将点集(vx, vy)进行划分。
* 并且对每个点进行处理。
*/
void GBWT::get_point(vector<uint64_t>* vx, vector<uint64_t>* vy,
				vector<uint64_t>* px[][256], vector<uint64_t>* py[][256]){
	
	//我们需要根据每个点，判断这个点所属于的范围
	assert(m_sa != NULL);
	assert(m_T != NULL);
	
	//依次判断每个点。
	for(uint64_t k = 0; k < vx->size(); k++){

		uint64_t x = (*vx)[k];	//x, y分别表示每个点。
		uint64_t y = (*vy)[k];
		
		/*根据这个点的横坐标，找到它对应的后缀，
		* 这个后缀的首字母，就是这个点的横坐标所对应的首字母
		* 这个后缀的前一个字符，就是这个点的纵坐标所对应的首字母。
		*/
		uint64_t sufpos = m_sa[x];
		
		uint8_t i, j;   //i, j表示这个点所对应的首字符。

		i = m_T[sufpos];	
		assert(sufpos > 0);
		j = m_T[sufpos - 1];

		if(px[i][j] == NULL){
			assert(py[i][j] == NULL);
			px[i][j] = new vector<uint64_t>();
			py[i][j] = new vector<uint64_t>();
		}

		
		/*
		* 我们需要对这些点进行处理：
		* 1. 存储每个x轴坐标可以使用更少的空间。
		* 2. y轴的范围要去掉首字母，也就是使用更少的空间。
		*/
		assert(x >= m_suf[i].left);
		px[i][j]->push_back(x - m_suf[i].left);
		assert((y >> ((m_step-1)*8) == j));
		y &= ~(0xFFULL << ((m_step-1)*8));
		py[i][j]->push_back(y);
		
	}//end of for()

//counting
	m_maxPoints = 0;
	for(int i = 0; i < 256; i++){
		for(int j = 0; j < 256; j++){
			if(px[i][j] != NULL){
				m_maxPoints = std::max(m_maxPoints, px[i][j]->size());
			}
		}
	}

	
}//end of get_point();



/*********************************
* 向存储KDTree的文件，写入磁盘页的头。
* 第0号磁盘页: [字符个数4B] [256个字符 256B] [256 charIdx, 256*4B]
* 接下来: 每个字符对应有：[每个字符所对应的根节点的磁盘页256*4B=1KB]
***********************************/
int32_t GBWT::writeMultiKDTHeader(){
	
	FILE* out = m_diskOut->m_out;
	fseek(out, 0, SEEK_SET);	/*定位到文件头*/
	uint64_t bytes = 0;			//所写入的字节数。
	
	int32_t numChar = m_nChar;
	//第0号磁盘页的内容。 [字符个数4B] [256个字符 256B] [256 charIdx, 256*4B]
	bytes += fwrite(&numChar, 1, sizeof(int32_t), out);
	bytes += fwrite(m_char, 1, sizeof(uint8_t)*256, out);
	bytes += fwrite(m_CharIdx, 1, sizeof(uint32_t)*256, out);
	bytes += writePadding(m_B-bytes, out);
	m_diskOut->m_diskNum++;

	bytes = 0;
	//接下来磁盘页的内容。
	for(int32_t i = 0; i < m_nChar; i++){
		uint8_t ch = m_char[i];	//取得相应的字符。
		//然后写入1KB的内容。
		bytes += fwrite(m_suf[ch].diskNum, 1, sizeof(int32_t)*256, out);
		// fprintf(stdout, "ch = %d\n", (int32_t)ch);
		// for(int32_t k = 0; k < 256; k++)
		// 	fprintf(stderr, "%d ", m_suf[ch].diskNum[k]);
		// fprintf(stderr, "\n");
	}
	
	//增加磁盘页的个数。
	m_diskOut->m_diskNum += ceil((double)bytes / m_B);

	bytes = bytes % m_B;	//有可能为0.当为0的时候不用写。
	if(bytes != 0){
		bytes += writePadding(m_B - bytes, out);
		assert(bytes == m_B);
	}
		
}


int64_t GBWT::writePadding(uint64_t bytes, FILE* out){
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