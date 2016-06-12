#ifndef INCLUDED_KDPOINTDISK_H
#define INCLUDED_KDPOINTDISK_H

#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "sdsl/int_vector.hpp"
#include "rect.h"
#include "DiskFile.h"
#include "DiskKDNode.h"
#include "KDVirtualDisk.h"


class KDPointDisk : public KDVirtualDisk {

public:	
	static int32_t		m_B;			// the size of a disk page in bytes.
	static int32_t		m_xBitWidth;	// the width of x point
	static int32_t		m_yBitWidth;	// the width of y point.	
	static int32_t 		m_SABitWidth;	// the width of SA value;	

	sdsl::int_vector<0>	* m_vx;				// The number of x value.
	sdsl::int_vector<0>	* m_vy;				// The number of y value.
	sdsl::int_vector<0> * m_samSA;			// the saValue of point(x, y)

public:
	//Constructor
	KDPointDisk(uint64_t nPoint);

	/***
	*init the int_vector
	* @param nPoint: the length of the int_vector
	* @param xWidth: the width of m_xv
	* @param yWidth: the width of m_yv
	****/
	//void init(uint64_t nPoint);

	~KDPointDisk();

	void setPoint(uint64_t idx, uint64_t xValue, uint64_t yPoint, uint64_t saValue);
	/****************
	* write the KDPointDisk into really disk.
	* @return  the # of disk which it writes into.
	*****************/
	int32_t writeToDisk(DiskFile *diskOut);

	/***************
	* @return the bytes that it read into.
	****************/
	int32_t writePadding(int64_t bytes, FILE* out);


	// We set these value in the begining of the program.
	static void		Set(int32_t B, int32_t xBitWidth, int32_t yBitWidth, int32_t SABitWidth);
	static int32_t 	GetMaxPoint();	// the max number of points that can be stored in a disk page.
	static DiskKDNode* GetNodeFromMem(uint8_t* buf);
};














#endif