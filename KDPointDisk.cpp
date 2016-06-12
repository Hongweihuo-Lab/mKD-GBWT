#include <cassert>
#include <cstdlib>
#include <iostream>

#include "KDPointDisk.h"

using namespace sdsl;
using namespace std;

int32_t KDPointDisk::m_B = -1;
int32_t KDPointDisk::m_xBitWidth = -1;
int32_t KDPointDisk::m_yBitWidth = -1;
int32_t KDPointDisk::m_SABitWidth = -1;

KDPointDisk::KDPointDisk(uint64_t nPoint){
	m_vx = new int_vector<0>(nPoint, 0, m_xBitWidth);
	m_vy = new int_vector<0>(nPoint, 0, m_yBitWidth);
	m_samSA = new int_vector<0>(nPoint, 0, m_SABitWidth);

	assert(m_vx->get_int_width() == m_xBitWidth);
	assert(m_vy->get_int_width() == m_yBitWidth);
	assert(m_samSA->get_int_width() == m_SABitWidth);
}

KDPointDisk::~KDPointDisk(){
	delete m_vx;
	delete m_vy;
	delete m_samSA;
}

// We set it in the begining of the program.
void KDPointDisk::Set(int32_t B, int32_t xBitWidth, int32_t yBitWidth, int32_t SABitWidth){
	m_B = B;
	m_xBitWidth = xBitWidth;
	m_yBitWidth = yBitWidth;
	m_SABitWidth = SABitWidth;
}

// The maximum of points that can store in a disk page.
int32_t KDPointDisk::GetMaxPoint() {
	int32_t totalBytes = 0;
	totalBytes += 1;		  // node flag.
	totalBytes += 8 + 1 + 1 + 1;  //#points, xBitWidth, yBitWidth, SABitWidth
	totalBytes += 4 + 4 + 4;	  // int_vector's (x, y, sa) capacity.
	int32_t max = 0;
	max = ((m_B - totalBytes)*8 - 64*3) / (m_xBitWidth + m_yBitWidth + m_SABitWidth);
	return max;
}

void KDPointDisk::setPoint(uint64_t idx, uint64_t xValue, uint64_t yValue, uint64_t saValue) {
	(*m_vx)[idx] = xValue;  // I have write m_vx[idx], how c++ can do this?
	(*m_vy)[idx] = yValue;
	(*m_samSA)[idx] = saValue;
}

int32_t KDPointDisk::writeToDisk(DiskFile* diskOut){

	int32_t diskNum = diskOut->m_diskNum;		// the disk nubmer that it need to write into
	FILE*	out 	= diskOut->m_out;		// the file descripter.

	assert(m_vx->size() == m_vy->size());
	uint64_t nPoint = m_vx->size();				//8 byte
	uint8_t  xWidth = m_vx->get_int_width();	//1 byte
	uint8_t	 yWidth = m_vy->get_int_width();	//1 byte
	uint8_t  saWidth = m_samSA->get_int_width(); //1 byte
	uint32_t xCap = m_vx->capacity() >> 3;		//4 byte
	uint32_t yCap = m_vy->capacity() >> 3;		//4 byte
	uint32_t saCap = m_samSA->capacity() >> 3;  //4 byte

	int64_t totalBytes = 0;
	totalBytes += 1;
	totalBytes += 8 + 1 + 1 + 1;
	totalBytes += 4 + 4 + 4;
	totalBytes += m_vx->capacity() >> 3;  	// turn bits into bytes.
	totalBytes += m_vy->capacity() >> 3;  	// turn bits into bytes.
	totalBytes += m_samSA->capacity() >> 3; // turn bits into bytes 
	assert(totalBytes <= m_B);

	uint8_t flag = 1;

	int32_t written = 0;
	written += fwrite(&flag, 1, sizeof(uint8_t), out);
	written += fwrite(&nPoint, 1, sizeof(uint64_t), out);
	written += fwrite(&xWidth, 1, sizeof(uint8_t), out);
	written += fwrite(&yWidth, 1, sizeof(uint8_t), out);
	written += fwrite(&saWidth, 1, sizeof(uint8_t), out);
	written += fwrite(&xCap, 1, sizeof(uint32_t), out);
	written += fwrite(&yCap, 1, sizeof(uint32_t), out);
	written += fwrite(&saCap, 1, sizeof(uint32_t), out);

	const uint64_t* data = NULL;
	uint64_t dataLen = 0;

	data = m_vx->data();
	dataLen = m_vx->capacity() >> 3;
	written += fwrite(data, 1, dataLen, out);	// write m_vx

	data = m_vy->data();
	dataLen = m_vy->capacity() >> 3;
	written += fwrite(data, 1, dataLen, out); 	// write m_vy

	data = m_samSA->data();
	dataLen = m_samSA->capacity() >> 3;
	written += fwrite(data, 1, dataLen, out);    // write m_sa

	assert(written == totalBytes);
	assert(written <= m_B);
	written += writePadding(m_B - written, out);
	assert(written == m_B);

	diskOut->m_diskNum++;
	return diskNum;			/*return the diskNum that it writes into*/
}

int32_t KDPointDisk::writePadding(int64_t bytes, FILE* out){
	assert(bytes >= 0);
	if(bytes){
		uint8_t * dummy = (uint8_t*)malloc(sizeof(uint8_t) * bytes);
		memset(dummy, 0, bytes);
		int32_t written = fwrite(dummy, 1, bytes, out);
		free(dummy);
		return written;
	}
	return 0;
}

//从buf中恢复出一个存放点集的叶子节点。
DiskKDNode* KDPointDisk::GetNodeFromMem(uint8_t* buf){
	uint8_t flag = buf[0];
	assert(flag == 1);

	uint64_t nPoint = *((uint64_t*)(buf + 1));
	uint8_t  xWidth = *((uint8_t*)(buf + 9));
	uint8_t  yWidth = *((uint8_t*)(buf + 10));
	uint8_t	 saWidth = *((uint8_t*)(buf + 11));
	uint32_t xCap   = *((uint32_t*)(buf + 12));
	uint32_t yCap   = *((uint32_t*)(buf + 16));
	uint32_t saCap  = *((uint32_t*)(buf + 20));

	uint8_t* xData = (buf + 24);
	uint8_t* yData = xData + xCap;
	uint8_t* saData = yData + yCap;

	int_vector<0>* vx = new int_vector<0>(xCap, 0, 8);
	int_vector<0>* vy = new int_vector<0>(yCap, 0, 8);
	int_vector<0>* sa = new int_vector<0>(saCap, 0, 8);

	for(uint32_t i = 0; i < xCap; i++){
		(*vx)[i] = xData[i];
	} 
	for(uint32_t i = 0; i < yCap; i++){
		(*vy)[i] = yData[i];
	}
	for(uint32_t i = 0; i < saCap; i++){
		(*sa)[i] = saData[i];
	}

	vx->set_int_width(xWidth);
	vy->set_int_width(yWidth);
	sa->set_int_width(saWidth);
	assert(vx->get_int_width() == xWidth);
	assert(vy->get_int_width() == yWidth);
	assert(sa->get_int_width() == saWidth);

	DiskKDNode* anode = new DiskKDNode(nPoint, true);
	for(uint32_t i = 0; i < nPoint; i++){
		anode->setPoint(i, (*vx)[i], (*vy)[i]);
		anode->setSA(i, (*sa)[i]);
	}

	delete vx;
	delete vy;
	delete sa;
	
	return anode;
}