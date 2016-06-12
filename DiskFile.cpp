#include "DiskFile.h"

DiskFile::DiskFile(FILE* out, int32_t diskNum) {

	m_out = out;
	m_diskNum = diskNum;
}

DiskFile::~DiskFile(){
	//Do nothing.
}