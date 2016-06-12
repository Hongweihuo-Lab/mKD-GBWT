
#include "KDTreeDiskHeader.h"
#include <cstdio>

KDTreeDiskHeader::KDTreeDiskHeader(int32_t rootNum, int32_t allNum) {

	m_rootNum = rootNum;
	m_allNum = allNum;
}

void KDTreeDiskHeader::set(int32_t rootNum, int32_t allNum) {
	m_rootNum = rootNum;
	m_allNum = allNum;
}

int32_t KDTreeDiskHeader::writeToDisk(DiskFile* df){

	FILE* out = df->m_out;
	fseek(out, 0, SEEK_SET); // in the beginning of the file

	int32_t totalBytes = 0;
	totalBytes += fwrite(&m_rootNum, 1, sizeof(int32_t), out);
	totalBytes += fwrite(&m_allNum, 1, sizeof(int32_t), out);

	return totalBytes;
}

int64_t KDTreeDiskHeader::GetPosition(int64_t diskNum, int32_t B){
	return (4 + 4) + (diskNum * B);
}