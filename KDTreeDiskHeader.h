#ifndef INCLUDED_KDTREEDISKHEADER_H
#define INCLUDED_KDTREEDISKHEADER_H

#include "DiskFile.h"
#include <cstddef>
#include <cstdint>

class KDTreeDiskHeader{

public:
	int32_t 		m_rootNum;	// the nubmer of the root's disk page.
	int32_t			m_allNum;	// the size of disk page it writes into

public:
	KDTreeDiskHeader(int32_t rootNum = 0, int32_t allNum = 0);

	void set(int32_t rootNum, int32_t allNum);

	int32_t writeToDisk(DiskFile * df);

	static int64_t GetPosition(int64_t diskNum, int32_t B);
};





#endif