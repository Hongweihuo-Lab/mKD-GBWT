#ifndef TEST_SBT_H
#define TEST_SBT_H

#include "sb_tree.h"

class test_sbt{

public:

	static void test_01();
	static void test_02();  /*向磁盘中写入sb_tree，并且测试数据 */
	static void test_03();

	/*读取test_02()中写入磁盘中的数据(./raw_data/bible.sbt)，并且进行数据测试*/
	static void test_04();
	/*从磁盘上读取数据，测试我自己的 suffix_range2 函数 */
	static void test_05();

	static void test_sbt_size(char* out_file);

	static void test_06();
};




#endif