
#ifndef INCLUDED_KDVIRTUALDISK_H
#define INCLUDED_KDVIRTUALDISK_H

#include <cstdint>
#include "DiskFile.h"

class KDVirtualDisk{

public:

	KDVirtualDisk();
	~KDVirtualDisk();
	virtual int32_t writeToDisk(DiskFile *diskOut) = 0;

};



#endif