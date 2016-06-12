#ifndef TEST_CRITBIT_TREE_H
#define TEST_CRITBIT_TREE_H

#include "critbit_tree.h"
#include "sbt_util.h"

class test_critbit_tree{

public:

	static void test_01();
	static void test_02();
	static void test_03();
	static void test_04(); /* test file I/O, write critbit_tree into file*/
	static void test_05(); /* test file I/O, read crtibit_tree from file(.cbt) */

	static void test_help_contains(critbit_tree& cbt, uint8_t* P, uint64_t m);
	static void test_help_suffixes(critbit_tree& cbt, uint8_t* P, uint64_t m, uint64_t** results);

	static void test_bible();
	static void test_kernel();

	static void test_cbt_write();
	static void test_cbt_read();
};

#endif //TEST_CRITBIT_TREE_H