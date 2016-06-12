#ifndef INCLUDED_TEST_KDTREE
#define INCLUDED_TEST_KDTREE

#include <iostream>
#include <vector>
#include <algorithm> /*sort, nth_element, shuffle*/
/* clock_t, clock, CLOCKS_PER_SEC*/
#include <ctime>  // measure time.
#include <random>	//std::default_random_engine
#include <chrono>	//std::chrono::system_clock
#include <cstdio>

#include "KDNode.h"
#include "KDTree.h"
#include "DiskKDTree.h"

class test_kdtree{

public:
	static void Set(int32_t B, int32_t xBitWidth, int32_t yBitWidth, int32_t SABitWidth);

	static void test_ReadFromDisk();
	static void test_SaveToDisk();
	static void test_quickSort();
	static void test_nth();

	static vector<pair<uint64_t, uint64_t> >* readPointsFromFile(const char* pointName);
	static void writePointsToFile(const char* pointFile, uint64_t n, uint64_t* x_arr, uint64_t* y_arr);

};










#endif