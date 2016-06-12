#ifndef	INCLUDED_TEST_GBWT_H
#define INCLUDED_TEST_GBWT_H


#include <cstdint>
#include "gbwt.h"

class test_gbwt{

public:

	static void test_01(const char* text_file, uint64_t B, uint64_t step);
	static void test_02();
	static void test_03();

};














#endif