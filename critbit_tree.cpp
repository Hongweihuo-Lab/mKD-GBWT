#define __STDC_FORMAT_MACROS // for PRIu64
#include <stdio.h>
#include <stdlib.h>
#include <stack>
#include <assert.h>
#include <stdint.h>
#include <inttypes.h>


#include "critbit_tree.h"
#include "sbt_util.h"

using std::stack;
using std::pair;
using namespace sdsl;

critbit_tree::critbit_tree(uint64_t step){
	this->m_root = NULL;
	this->m_g = 0;
	this->T = NULL;
	this->n = 0;
	this->m_max_posarray_width = 0;
	this->m_step = step;
}

critbit_tree::critbit_tree(uint8_t* T, uint64_t n, uint64_t* SA, uint64_t nsuf, uint64_t step){
	this->m_root = NULL;
	this->m_g = 0;
	this->T = T;
	this->n = n;
	this->m_step = step;
	create_from_suffixes(SA, nsuf);
}

/* deconstructor */
critbit_tree::~critbit_tree(){
	clear();
}

void critbit_tree::clear(){
	if(m_root != NULL)
		delete_nodes(m_root);
	else{
		m_root = NULL;
	}
	m_g = 0;
}

void critbit_tree::delete_nodes(critbit_node_t* cur_node){
	if(cur_node != NULL){
		if(cur_node->is_leaf()) {
			/* if the current node is a leaf node, just free it */
			free(cur_node);
		}else {
			/* it is a internal nodes */
			delete_nodes(cur_node->get_left_child());
			delete_nodes(cur_node->get_right_child());
			free(cur_node);
		}
	}
}

void critbit_tree::create_from_suffixes(uint64_t* SA, uint64_t nsuf) {
	for(uint64_t i = 0; i < nsuf; i++){
		insert_suffix(SA[i]);
	}

}

void critbit_tree::create_from_suffixes(uint64_t* SA, uint64_t* LCP, uint64_t nsuf) {
	for(uint64_t i = 0; i < nsuf; i++){
		insert_suffix(SA, LCP, i);
	}
}

void critbit_tree::create_from_suffixes2(uint64_t* SA, uint64_t* LCP, uint64_t nsuf) {
	for(uint64_t i = 0; i < nsuf; i++){
		insert_suffix2(SA, LCP, i);
	}
}


void critbit_tree::insert_suffix(uint64_t sufpos){
	if(m_root == NULL) {

		m_root = (critbit_node_t*)malloc(sizeof(critbit_node_t));
		m_root->set_critpos(0);
		m_root->set_suffix(sufpos);   /* set the root be the suffix (leaf node)*/
		m_g++;
		return;
	}

	/* find if the suffix is already in the critbit tree */
	critbit_node_t* cur_node = m_root;
	uint64_t newdirection;

	while(!cur_node->is_leaf()){
		/* traverse until we find a leaf to see if the suffifx T[sufpos..n-1] has been in there 
		   IF the suffix has been in the critbit tree, we must can find a leaf node is the suffix */
		uint64_t charLen = cur_node->get_char_len();
		uint64_t diffPos = cur_node->get_diff_pos();
		uint8_t  diffChar = 0;
		/* if the sufpos + charLen >= n, then the diffChar = 0, so the newdirection == 0 */
		if(sufpos + charLen < n) diffChar = T[sufpos + charLen];
		/* if the diffChar's diffPos bit is 1 the direction is 1, else the direction is 0 */
		newdirection = get_direction(diffChar, diffPos);
		cur_node = cur_node->get_child(newdirection);

	}// end of while()

	/* the cur_node is now a leaf node */
	uint64_t suffix = cur_node->get_suffix();

	if(suffix == sufpos){
		/* the suffix has always been in the critbit_tree */
		return;
	}

	/* The suffix doesn't in the critbit_tree. So insert suffix into the Critbit Tree.*/

	/* we need insert two node, an internal node and a leaf node. */ 
	critbit_node_t* insert_node = (critbit_node_t*)malloc(sizeof(critbit_node_t));
	if(!insert_node) {
		fprintf(stderr, "error allocating critbit node.\n");
		exit(EXIT_FAILURE);
	}

	critbit_node_t* insert_leaf_node = (critbit_node_t*) malloc(sizeof(critbit_node_t));
	if(!insert_leaf_node){
		fprintf(stderr, "error critbit_tree::insert_suffix(): allocating critbit node.\n");
		exit(EXIT_FAILURE);
	}	

	uint64_t i = suffix, j = sufpos, k = 0;
	/* find the position of first character that the two suffixes differs */
	//for(; ((i+k) < n) && ((j+k)<n) && T[i+k] == T[j+k]; k++){}  //It is time wasting.
	k = find_diff_char(i, j);

	uint64_t charLen1 = k;  /* the LCP between the suffix sufpos and critbit tree */
	uint8_t char1, char2;
	if( (i+k) == n) char1 = 0; else char1 = T[i+k];
	if( (j+k) == n) char2 = 0; else char2 = T[j+k]; 

	uint64_t diffPos1 = get_critbit_pos(char1, char2);
	newdirection  = get_direction(char2, diffPos1);

	insert_node->set_critpos((charLen1 << 3) | (diffPos1));
	insert_node->set_child(newdirection, insert_leaf_node);

	insert_leaf_node->set_suffix(sufpos);  /* set the inserting leaf node */
	insert_leaf_node->set_critpos(0);	   /* set critpos of leaf node */

	uint64_t direction;
	critbit_node* parent = NULL;
	cur_node = m_root;

	while(!cur_node->is_leaf()) {
		/* traverse the critbit tree find the crrect internal node */
		uint64_t charLen = cur_node->get_char_len();
		uint64_t diffPos = cur_node->get_diff_pos();

		uint64_t diffChar = 0;
		if(sufpos + charLen < n) diffChar = T[sufpos + charLen];

		/* because we have to find the locus position and insert it */
		if(insert_node->get_critpos() <  cur_node->get_critpos()) break;  /* this is the important place!!! */

		direction = get_direction(diffChar, diffPos);
		parent = cur_node;
		cur_node = cur_node->get_child(direction);
	}	

	if(parent == NULL) {
		m_root = insert_node;
		insert_node->set_child(1-newdirection, cur_node);
	} else {
		parent->set_child(direction, insert_node);
		insert_node->set_child(1 - newdirection, cur_node);
	}

	m_g++;

} // critbit_tree::insert_suffix()


void critbit_tree::insert_suffix(uint64_t* SA, uint64_t* LCP, uint64_t idx){

	uint64_t sufpos = SA[idx];

	if(m_root == NULL) {
		m_root = (critbit_node_t*)malloc(sizeof(critbit_node_t));
		m_root->set_suffix(sufpos);   /* set the root be the suffix (leaf node)*/
		m_root->set_critpos(0);		  /* the critpos of leaf node is zero */
		m_g++;
		return ;
	}

	/* find if the suffix is already in the critbit tree */
	critbit_node_t* cur_node = m_root;
	uint64_t newdirection;

	while(!cur_node->is_leaf()){
		/* traverse until we find a leaf to see if the leaf node has been in there 
		   IF the node has been in the critbit tree, we must can find a leaf node is the suffix */
		uint64_t charLen = cur_node->get_char_len();
		uint64_t diffPos = cur_node->get_diff_pos();
		uint8_t  diffChar = 0;
		/* if the sufpos + charLen >= n, then the diffChar = 0, so the newdirection == 0 */
		if(sufpos + charLen < n) diffChar = T[sufpos + charLen];
		/* if the diffChar's diffPos bit is 1 the direction is 1, else the direction is 0 */
		newdirection = get_direction(diffChar, diffPos);
		cur_node = cur_node->get_child(newdirection);

	}// end of while()

	/* the cur_node is now a leaf */
	uint64_t suffix = cur_node->get_suffix();
	if(suffix == sufpos){
		/* the suffix has always been in the critbit_tree */
		return;
	}

	/* The suffix doesn't in the critbit_tree. So insert suffix into the Critbit Tree. */

	/* we need insert two node, an internal node and a leaf node. */ 
	critbit_node_t* insert_node = (critbit_node_t*)malloc(sizeof(critbit_node_t));
	if(!insert_node) {
		fprintf(stderr, "error allocating critbit node.\n");
		exit(EXIT_FAILURE);
	}

	critbit_node_t* insert_leaf_node = (critbit_node_t*) malloc(sizeof(critbit_node_t));
	if(!insert_leaf_node){
		fprintf(stderr, "error critbit_tree::insert_suffix(): allocating critbit node.\n");
		exit(EXIT_FAILURE);
	}	


	uint64_t i = suffix, j = sufpos, k = 0;
	/* find the position of first character that the two suffixes differs */
	k = find_diff_char(i, j);

	uint64_t charLen1 = k;  /* the LCP between the suffix sufpos and critbit tree */
	uint8_t char1, char2;
	if( (i+k) == n) char1 = 0; else char1 = T[i+k];
	if( (j+k) == n) char2 = 0; else char2 = T[j+k]; 

	uint64_t diffPos1 = get_critbit_pos(char1, char2);
	newdirection  = get_direction(char2, diffPos1);

	/******************** Vertify the LCP method is correct ***************************************************/
	/* What we need is the critpos: different character and different position in the character */
	uint64_t k1 = LCP[idx];
	if( (i+k1) == n ) char1 = 0; else char1 = T[i+k1];
	if( (j+k1) == n ) char2 = 0; else char2 = T[j+k1];
	uint64_t diffPos2 = get_critbit_pos(char1, char2);
	uint64_t newdirection2 = get_direction(char2, diffPos2);
	if(k != k1) {
		fprintf(stderr, "error critbit_tree::insert_suffix(), k != k1. k = %lu, k1 = %lu.\n", k, k1);
		exit(EXIT_FAILURE);
	}
	if(newdirection != newdirection2){
		fprintf(stderr, "error critbit_tree::insert_suffix(), newdirection != newdirection2. newdirection = %lu, newdirection2 = %lu.\n", newdirection, newdirection2);
		exit(EXIT_FAILURE);
	}
	/***********************************************************************/

	insert_node->set_critpos(charLen1, diffPos1);
	insert_node->set_child(newdirection, insert_leaf_node);

	insert_leaf_node->set_suffix(sufpos);  	/* set the inserting leaf node */
	insert_leaf_node->set_critpos(0);		/* critpos of leaf node is zero */

	uint64_t direction;
	critbit_node* parent = NULL;
	cur_node = m_root;

	while(!cur_node->is_leaf()) {
		/* traverse the critbit tree find the crrect internal node */
		uint64_t charLen = cur_node->get_char_len();
		uint64_t diffPos = cur_node->get_diff_pos();

		uint64_t diffChar = 0;
		if(sufpos + charLen < n) diffChar = T[sufpos + charLen];

		/* because we have to find the locus position and insert it */
		if(insert_node->get_critpos() < cur_node->get_critpos()) break;  /* this is the important place!!! */

		direction = get_direction(diffChar, diffPos);
		parent = cur_node;
		cur_node = cur_node->get_child(direction);
	}	

	if(parent == NULL) {
		m_root = insert_node;
		insert_node->set_child(1-newdirection, cur_node);
	} else {

		parent->set_child(direction, insert_node);
		insert_node->set_child(1 - newdirection, cur_node);
	}

	m_g++;

} // critbit_tree::insert_suffix(SA, LCP, idx)


void critbit_tree::insert_suffix2(uint64_t* SA, uint64_t* LCP, uint64_t idx){

	uint64_t sufpos = SA[idx];

	if(m_root == NULL) {
		/*If m_root == NULL, idx = 0*/
		m_root = (critbit_node_t*)malloc(sizeof(critbit_node_t));
		m_root->set_suffix(sufpos);   	/* set the root be the suffix (leaf node)*/		
		m_root->set_critpos(0);			/* critpos of leaf node is zero */
		m_g++;
		return;
	}
	/* The suffix T[sufpos..n-1]doesn't in the critbit_tree. So insert suffix into the Critbit Tree. */

	/* we need insert two nodes, an internal node and a leaf node. */ 
	critbit_node_t* insert_node = (critbit_node_t*)malloc(sizeof(critbit_node_t));
	if(!insert_node) {
		fprintf(stderr, "error allocating critbit node.\n");
		exit(EXIT_FAILURE);
	}

	critbit_node_t* insert_leaf_node = (critbit_node_t*) malloc(sizeof(critbit_node_t));
	if(!insert_leaf_node){
		fprintf(stderr, "error critbit_tree::insert_suffix(): allocating critbit node.\n");
		exit(EXIT_FAILURE);
	}	

	uint64_t i , j , k;
	/* find the position of first character that the two suffixes differs */
	k = LCP[idx];
	i = SA[idx-1]; /*idx >= 1, because m_root != NULL*/
	j = sufpos;

	uint64_t charLen = k;  /* the LCP between the suffix sufpos and critbit tree */
	uint8_t char1, char2, diffChar;
	if( (i+k) == n) char1 = 0; else char1 = T[i+k];
	if( (j+k) == n) char2 = 0; else char2 = T[j+k]; 

	uint64_t diffPos = get_critbit_pos(char1, char2);
	uint64_t newdirection  = get_direction(char2, diffPos);

	insert_node->set_critpos(charLen, diffPos);
	insert_node->set_child(newdirection, insert_leaf_node);

	insert_leaf_node->set_suffix(sufpos);  /* set the inserting leaf node */
	insert_leaf_node->set_critpos(0);

	/* insert the two nodes into critbit-tree */
	uint64_t direction;
	critbit_node_t* parent = NULL;
	critbit_node_t* cur_node = m_root;

	while(!cur_node->is_leaf()) {
		/* traverse the critbit tree find the crrect internal node */
		charLen = cur_node->get_char_len();
		diffPos = cur_node->get_diff_pos();

		diffChar = 0;
		if(sufpos + charLen < n) diffChar = T[sufpos + charLen];

		/* because we have to find the locus position and insert it */
		if(insert_node->get_critpos() < cur_node->get_critpos()) break;  /* IMPORTANT!!! this is the important place!!!*/

		direction = get_direction(diffChar, diffPos);
		parent = cur_node;
		cur_node = cur_node->get_child(direction);
	}	

	if(parent == NULL) {
		m_root = insert_node;
		insert_node->set_child(1-newdirection, cur_node);
	} else {
		parent->set_child(direction, insert_node);
		insert_node->set_child(1 - newdirection, cur_node);
	}

	m_g++;
} // critbit_tree::insert_suffix2(SA, LCP, idx)






/** A critbit_tree of  g suffixes has:
* 		1) there are g leaf nodes in the tree. 
*
*		2) so there are g-1 internal nodes, and the g leaf nodes is in the pointers of internal nodes.
*
*	So the bytes used by the tree is:  (2*g-1)*sizeof(node size)
**/
uint64_t critbit_tree::size_in_bytes(){
	uint64_t bytes;
	if(m_g == 0){
		/* there is no nodes in the suffix tree */
		bytes = 0;
	} else {
		bytes = (m_g - 1 + m_g)*sizeof(critbit_node_t);
	}

	return bytes;
}

void critbit_tree::set_text(uint8_t* T, uint64_t n){
	this->T = T;
	this->n = n;
}

void critbit_tree::delete_text(){
	if(T != NULL){
		T = NULL;
	}
	n = 0;
}

/**
* if P is a prefix of some suffix in the critbit tree return 1 , otherwise return 0.
**/
bool critbit_tree::contains(const uint8_t* P, uint64_t m) {

	if(m_root == NULL){
		return 0;
	}

	critbit_node_t* cur_node = m_root;
	uint64_t direction;

	while(!cur_node->is_leaf()) {
		/* traverse till we reach a leaf node */
		uint64_t charLen = cur_node->get_char_len();
		uint64_t diffPos = cur_node->get_diff_pos();
		uint8_t diffChar = 0;
		if(charLen < m) diffChar = P[charLen];
		else {
			/* all the leaf node of cur_node will satisfy  and we choose the leftmost leaf node */
			cur_node = cur_node->get_child(0);
			while(!cur_node->is_leaf()) cur_node = cur_node->get_child(0);
			break;
		}

		direction = get_direction(diffChar, diffPos);
		cur_node = cur_node->get_child(direction);
	} // end of while

	/* cur_node is the leaf node */
	uint64_t suffix = cur_node->get_suffix();
	test_T();
	uint64_t i = suffix,  k = 0;
	for(;(i+k < n) && (k < m) && (T[i+k] == P[k]); k++) {}

	fprintf(stderr, "%lu, %lu.\n",k , m);

	if(k == m)
		return true;
	else 
		return false;

} // end of member function critbit_tree::contains()


void  critbit_tree::test_T(){
	if(T == NULL){
		fprintf(stderr, "error testing T.\n");
		exit(EXIT_FAILURE);
	}
}

/* Delete Suffxi T[sufpos..n-1] */
void critbit_tree::delete_suffix(uint64_t sufpos) {

	if(m_root == NULL) {
		return ;
	}

	critbit_node_t* cur_node = m_root;
	critbit_node_t* parent = NULL;
	critbit_node_t* grandparent = NULL;

	uint64_t direction, gp_dire;

	while(!cur_node->is_leaf()){
		/* traverse untill we reach a leaf node */
		uint64_t charLen = cur_node->get_char_len();
		uint64_t diffPos = cur_node->get_diff_pos();
		uint64_t diffChar = 0;

		if(charLen + sufpos < n) diffChar = T[charLen + sufpos];

		gp_dire = direction;
		direction = get_direction(diffChar, diffPos);

		grandparent = parent;
		parent = cur_node;
		cur_node = cur_node->get_child(direction);

	}

	uint64_t suffix = cur_node->get_suffix();
	if(suffix != sufpos){
		/*  the suffix is not in the critbit tree */
		return ;
	}

	if(parent == NULL){
		/* the root is the leaf node */
		free(m_root);
		m_root = NULL;
		--m_g;
		return;
	}

	if(grandparent == NULL) {
		/* there is only one internal node in the critbit tree
			and set the root be the leaf node */
		m_root = parent->get_child(1-direction);
		free(parent);
		free(cur_node);
		--m_g;
		return;
	}

	grandparent->set_child(gp_dire, parent->get_child(1-direction));
	free(parent);
	free(cur_node);
	--m_g;
	return;
} // end of critbit_tree::delete_suffix


int suf_compare(const void* p1, const void* p2){
	uint64_t v1 = *((uint64_t*)p1);
	uint64_t v2 = *((uint64_t*)p2);
	if(v1 < v2) return -1;
	else if(v1 == v2) return 0;
	else return 1;
}

/* collect all the suffixes matching the prefix P of length m. */
uint64_t critbit_tree::suffixes(const uint8_t* P, uint64_t m, uint64_t** result) {

	if(m_root == NULL) {
		return 0;
	}

	critbit_node_t* cur_node = m_root;
	critbit_node_t* locus = NULL;
	uint64_t direction;

	while(!cur_node->is_leaf()){

		uint64_t charLen = cur_node->get_char_len();
		uint64_t diffPos = cur_node->get_diff_pos();
		uint8_t diffChar = 0;
		if(charLen < m) diffChar = P[charLen];
		else {
			locus = cur_node;
			/* get the leftmost child leaf node */
			while(!cur_node->is_leaf()) cur_node = cur_node->get_child(0);
			break;
		}

		direction = get_direction(diffChar, diffPos);
		cur_node = cur_node->get_child(direction);

	}// end of while()

	/* check if the suffix matching the prefix of P */
	uint64_t suffix = cur_node->get_suffix();
	uint64_t i = 0;
	for(;(i+suffix < n)&& (i<m) && T[i+suffix] == P[i]; i++ ) {}

	if(i != m) return 0;

	/* all the child leaf node of locus  will match the prefix of P.*/

	if(locus == NULL){
		/* there is only the T[suffix .. n] matches */
		(*result) = (uint64_t*)malloc(sizeof(uint64_t));
		(*result)[0] = suffix;
		return 1;
	} else {
		/* collect all the child leaf node of  locus */
		uint64_t rsize = 512;
		uint64_t count = 0;

		(*result) = (uint64_t*)malloc(sizeof(uint64_t)*rsize);
		collect_suffixes(locus, result, &rsize, &count);

		(*result) = (uint64_t*)realloc(*result, count * sizeof(uint64_t));

		if(*result == NULL) {
			fprintf(stderr, "error allocating memory.\n");
			exit(EXIT_FAILURE);
		}
		/* sort the array */
		//qsort(*result, count, sizeof(uint64_t), suf_compare);
		return count;
	}

}// end of member function critbit_tree::suffixes()

void critbit_tree::collect_suffixes(critbit_node_t* locus, 
								uint64_t** result, 
								uint64_t*  rsize, 
								uint64_t*  count) {

	if(!locus) {
		fprintf(stderr, "error collecting suffixes.\n");
		exit(EXIT_FAILURE);
	}

	if(locus->is_leaf()) {
		uint64_t suffix = locus->get_suffix();
		if(*count == *rsize){
			/* there is no space to store the element. reallocate memory. */
			*rsize = (*rsize) * 2;  /* allocate more space */
			*result = (uint64_t*)realloc(*result, (*rsize) * sizeof(uint64_t));  /* realloc memory */
			if(*result == NULL){
				fprintf(stderr, "error reallocating memroy.\n");
				exit(EXIT_FAILURE);
			}
		}
		(*result)[*count] = suffix;
		(*count)++;
	}
	else {
		collect_suffixes(locus->get_left_child(), result, rsize, count);
		collect_suffixes(locus->get_right_child(), result, rsize, count);
	}

} // end of member function critbit_tree::collect_suffixes()

bit_vector* critbit_tree::create_bp(){

	if(m_root == NULL) {
		bit_vector* bv = new bit_vector(0);
		return bv;
	}

	bit_vector* bv = new bit_vector(2* (m_g + m_g -1));
	uint64_t pos = 0;
	critbit_node_t* cur_node = m_root;
	create_bp_helper(cur_node, *bv, pos);

	if(pos != 2*(m_g + m_g - 1)){
		fprintf(stderr, "error creating bp (%lu, %lu).\n", pos, 2*(m_g + m_g -1));
		exit(EXIT_FAILURE);
	}

	return bv;
}

void critbit_tree::create_bp_helper(critbit_node_t* cur_node, bit_vector& bv, uint64_t& pos){

	if(!cur_node->is_leaf()){
		bv[pos] = 1; pos++;
		create_bp_helper(cur_node->get_left_child(), bv, pos);
		create_bp_helper(cur_node->get_right_child(), bv, pos);
		bv[pos] = 0; pos++;	
	}
	else{
		bv[pos] = 1; pos++;
		bv[pos] = 0; pos++;		
	}
}

int_vector<>* critbit_tree::create_posarray(){

	/* there are (m_g-1) internal nodes in the critbit tree 
		make sure the width of the int_vector<> array. */
	if(m_root == NULL || m_root->is_leaf()){
		/* there is no internal node */
		int_vector<>* pos = new int_vector<>(0);
		return pos;
	}

	int_vector<>* pos = new int_vector<>(m_g-1); /* m_g - 1 internal nodes */

	uint64_t p = 0;
	critbit_node_t* cur_node = m_root;
	uint64_t parentpos = cur_node->get_critpos();

	(*pos)[p] = cur_node->get_critpos();  /* I have written pos[p], the data type error.! */
	++p;

	create_posarray_helper(cur_node->get_left_child(), *pos, p, parentpos);
	create_posarray_helper(cur_node->get_right_child(), *pos, p, parentpos);

	if(p != m_g -1){
		fprintf(stderr, "error creating posarray, (%lu, %lu).\n", p, m_g-1);
		exit(EXIT_FAILURE);
	}
	return pos;
}


/* we difference encode the position here to get smaller numbers */
void critbit_tree::create_posarray_helper(critbit_node_t* cur_node, 
											int_vector<>& pos, 
											uint64_t& p,
											uint64_t parentpos ) {
	if(!cur_node->is_leaf()){
		uint64_t result = cur_node->get_critpos() - parentpos;
		pos[p] = result;
		p++;
		create_posarray_helper(cur_node->get_left_child(), pos, p, cur_node->get_critpos());
		create_posarray_helper(cur_node->get_right_child(), pos, p, cur_node->get_critpos());
	}
} 


int_vector<>* critbit_tree::create_suffixarray(){

	/* there are m_g suffixes in the critbit tree
		make sure the width of the int_vector<> array */
	if(m_root == NULL) {
		/* there is no suffix in the critbit tree */
		int_vector<>* SA = new int_vector<>((uint64_t)0);
		return SA;
	}

	int_vector<>* SA = new int_vector<>(m_g);
	critbit_node_t* cur_node = m_root;
	uint64_t p = 0;

	create_suffixarray_helper(cur_node, *SA, p);

	if(p != m_g){
		fprintf(stderr, "error create suffix array, (%lu, %lu).\n", p, m_g);
		exit(EXIT_FAILURE);
	}

	return SA;
}

void critbit_tree::create_suffixarray_helper(critbit_node_t* cur_node, 
											int_vector<>& SA,
											uint64_t& p) {
	if(cur_node->is_leaf()){
		/*在序列化的时候，需要收集critbit-tree上所有的suffix，
		* 这个时候将每个suffix除以m_step，因为每个suffix都是m_step的倍数.
		*/
		uint64_t sufpos = cur_node->get_suffix();
		assert(sufpos % m_step == 0);
		SA[p] = sufpos / m_step;
		p++;
	} else{
		create_suffixarray_helper(cur_node->get_left_child(), SA, p);
		create_suffixarray_helper(cur_node->get_right_child(), SA, p);
	}

}

uint64_t critbit_tree::write(FILE* out){

#ifdef KK_DEBUG
	fprintf(stderr, "m_g = %lu\n", m_g);
#endif
	
	/* create the bp seqence of 2*(m_g-1) bits*/
	bit_vector* bp = create_bp();

	/* create pos array */
	int_vector<>* posarray = create_posarray();	/* create suffix array */
	int_vector<>* sa = create_suffixarray();

	uint64_t written = 0;
	/* write g */
	written += fwrite(&m_g, 1, sizeof(uint64_t), out);

	/* compress pos array and suffix array */
	util::bit_compress(*posarray);
	util::bit_compress(*sa);

	/* write len of data */
	uint64_t pos_width = posarray->get_int_width();  /* should be 64 */
	
	if(m_max_posarray_width < pos_width)
		m_max_posarray_width = pos_width;

	uint64_t suffix_width = sa->get_int_width();		/* should be 64 */
	
	written += fwrite(&pos_width, 1, sizeof(uint64_t), out);
	written += fwrite(&suffix_width, 1, sizeof(uint64_t), out);
	written += fwrite(&m_level, 1, sizeof(uint32_t), out);
	written += fwrite(&m_flag, 1, sizeof(uint32_t), out);
	written += fwrite(&m_child_off, 1, sizeof(uint32_t), out);
	written += fwrite(&m_child_num, 1, sizeof(uint32_t), out);

#ifdef KK_DEBUG
	fprintf(stderr, "critbit_tree::write: posarray_width = %lu.\n", pos_width);
	fprintf(stderr, "critbit_tree::write: suffix_width = %lu.\n", suffix_width);
	fprintf(stderr, "critbit_tree::write: levle = %u.\n", m_level);
	fprintf(stderr, "critbit_tree::write: flag = %u.\n", m_flag);
	fprintf(stderr, "critbit_tree::write: child_off = %u.\n", m_child_off);
	fprintf(stderr, "critbit_tree::write: child_num = %u.\n", m_child_num);
#endif

	/* write bp */
	const uint64_t* bp_data = bp->data();
	uint64_t data_len = bp->capacity() >> 3; /* convert bits to byte */
	written += fwrite(bp_data, 1, data_len, out);

	/* write pos array */
	const uint64_t* pos_data = posarray->data();
	data_len = posarray->capacity() >> 3;  /* convert bits to bytes */

	written += fwrite(pos_data, 1, data_len, out);

	/* write suffix array */
	const uint64_t* sa_data = sa->data();
	data_len = sa->capacity() >> 3;
	written += fwrite(sa_data, 1, data_len, out);

	/* free the resources. */
	delete bp;
	delete posarray;
	delete sa;

	return written;
} // end of member function critbit_tree::write()


uint64_t getelem(const uint64_t* mem, uint64_t idx, uint64_t width){
	uint64_t i = idx * width;
	return bit_magic::read_int(mem + (i >> 6), i&0x3F, width);
}

/* reconstruct the critbti tree from memory */
void critbit_tree::load_from_mem(uint64_t* mem, uint64_t size) {

	/* cacl starting positions of all data elements */
	m_g = mem[0];                  /* the number of suffixes */
	uint64_t pos_width = mem[1];
	uint64_t suffix_width = mem[2];
	m_level = ((uint32_t*)mem)[6];
	m_flag = ((uint32_t*)mem)[7];
	m_child_off  = ((uint32_t*)mem)[8];
	m_child_num = ((uint32_t*)mem)[9];

#define BP_START 5

	uint64_t* bp = &mem[BP_START];

	uint64_t pos_offset = BP_START + ((((m_g + m_g-1)*2)+63) >> 6);
	uint64_t* pos = &mem[pos_offset];

	uint64_t pos_len_in_u64 = (((m_g-1)*pos_width)+63) >> 6;
	uint64_t suffix_offset = pos_len_in_u64 + pos_offset;
	uint64_t* suffixes = &mem[suffix_offset];

	if(m_g == 0) {
		/* there is no element in the critbit tree */
		return;
	}

	if(m_g == 1){
		m_root = (critbit_node_t*)malloc(sizeof(critbit_node_t));
		/*在反序列化的时候，需要将每个suffix乘以m_step */
		m_root->set_suffix(getelem(suffixes, 0, suffix_width)*m_step); 
		m_root->set_critpos(0);		
		return;
	}

	/* reconstruct the tree */
	std::stack<critbit_node_t*> stack;

	/* add root to the tree */
	m_root = (critbit_node_t*) malloc(sizeof(critbit_node_t));
	if(!m_root){
		fprintf(stderr, "error mallocing critbit node memory.\n");
		exit(EXIT_FAILURE);	
	}	

	m_root->set_critpos(getelem(pos, 0, pos_width));
	m_root->set_left_child(NULL);
	m_root->set_right_child(NULL);

	stack.push(m_root);

	/* process the bp */
	critbit_node_t* parent = NULL;

	uint64_t idx_bp = 1;
	uint64_t idx_pos = 1;
	uint64_t idx_sa = 0;
	uint64_t idx_child = 0;

	while(!stack.empty()) { /* if there is element in the stack */

		if(getelem(bp, idx_bp, 1) == 1 && getelem(bp, idx_bp+1, 1) == 0){
			/* it is a leaf node and we should add it to its parent's child */
			/* if getelem(bp, idx_bp -1, 1) == 0, the leaf node is the right child of
				its parent, otherwise the left child */

			parent = stack.top();
			uint64_t direction = CRITBIT_LEFTCHILD;
			if(parent->get_left_child() != NULL)
				direction = CRITBIT_RIGHTCHILD;

			critbit_node_t* tmpnode = (critbit_node_t*)malloc(sizeof(critbit_node_t));
			tmpnode->set_critpos(0);
			/*在反序列化的时候，需要将每个suffix乘以m_step */
			tmpnode->set_suffix(getelem(suffixes, idx_sa, suffix_width)*m_step);
			tmpnode->set_index(idx_child);

			parent->set_child(direction, tmpnode);

			idx_sa++;
			idx_bp += 2;
			idx_child ++;

		}
		else if(getelem(bp, idx_bp, 1) == 1 && getelem(bp, idx_bp+1, 1) == 1) {
			/* it is a internal node */

			critbit_node_t* anode = (critbit_node_t*)malloc(sizeof(critbit_node_t));
			parent = stack.top();
			uint64_t direction = CRITBIT_LEFTCHILD;
			if(parent->get_left_child() != NULL)
				direction = CRITBIT_RIGHTCHILD;

			parent->set_child(direction, anode);
			anode->set_critpos(getelem(pos, idx_pos, pos_width) + parent->get_critpos());			
			anode->set_left_child(NULL);
			anode->set_right_child(NULL);
			stack.push(anode);
			idx_bp++;
			idx_pos++;

		}
		else if(getelem(bp, idx_bp, 1) == 0) {
			/* we have traversed a subtree */
			parent = stack.top();
			stack.pop();
			idx_bp++;
		}

	} //end of while

	return;
} // end of member function critbit_tree::load_from_mem()


void critbit_tree::set_child_offset(uint32_t offset) {
	m_child_off = offset;
}

void critbit_tree::set_child_num(uint32_t num) {
	m_child_num = num;
}

void critbit_tree::set_flag(uint32_t flag) {
	m_flag = flag;
}

void critbit_tree::set_level(uint32_t level) {
	m_level = level;

}

int64_t critbit_tree::get_left_position(uint8_t* P, 
									uint64_t m, 
									const uint8_t * T_filename,
									uint64_t B,
									uint64_t* m_LCP,
									uint64_t* m_io){
	
	/*首先找到 当前critbit_tree中和P具有最长公共前缀的一个后缀LCP：很简单, 这就是critbit_tree的作用 */
	if(m_root == NULL){
		fprintf(stderr, "error!! critbit_tree::get_left_position(): m_root can't be null!\n");
		exit(EXIT_FAILURE);
	}		
	critbit_node_t* cur_node = m_root;
	while(!cur_node->is_leaf()){
		uint64_t charLen = cur_node->get_char_len();
		uint64_t diffPos = cur_node->get_diff_pos();
		uint8_t diffChar = 0;
		if(charLen < m) diffChar = P[charLen];
		else{
			while(!cur_node->is_leaf()) cur_node = cur_node->get_left_child();
			break;
		}
		uint64_t direction = get_direction(diffChar, diffPos);
		cur_node = cur_node->get_child(direction);
	}

	uint64_t j, k = 0;
	j = cur_node->get_suffix();

	uint8_t* text;

	/*需要从文件中读取出来m个字符，虽然m个字符要比B小很多，但是却依旧可能导致两次磁盘I/O
	* 也有可能读取到的字符数量少于m个，这中情况当然也有可能发生。
	* 返回值 text_m 表示读取到 text 中的字符个数。
	* text 保存读取到的字符。
	*/
	uint64_t text_m = sb_read_subfile(T_filename, &text, j, m, B, m_io); 

	/* get the longest common prefix */
	for(k = 0; k < m && k < text_m && P[k] == text[k]; k++){}

	uint64_t LCP = k;
	*m_LCP = k;
	
	uint8_t char1, char2;
	if(LCP == m) char1 = 0; else char1 = P[LCP];
	if(LCP == text_m) char2 = 0; else char2 = text[LCP];
	uint64_t diffPos2 = get_critbit_pos(char1, char2);		
	uint64_t critpos2 = (LCP << 3) | (diffPos2);
	
	/* 找到一个节点 x, 以及它的父亲节点y=parent(x)，charLen(x)>=LCP 并且charLen(y)<LCP */	
	critbit_node_t*  locus_node = m_root;
	critbit_node_t*  parent_node = NULL;

	uint64_t charLen, diffPos, critpos, direction;
	uint8_t diffChar;

	while(!locus_node->is_leaf()){
		charLen = locus_node->get_char_len(); 
		diffPos = locus_node->get_diff_pos();
		critpos = locus_node->get_critpos();

		diffChar = 0;
		if(charLen < m) diffChar = P[charLen];
		direction = get_direction(diffChar, diffPos);
		if(critpos > critpos2){
			break;
		}
	
		parent_node = locus_node;
		locus_node = locus_node->get_child(direction);
	}

	bool less;  /*less = true, 表示P的下一个字符小于 text的下一个字符，否则大于，二者不可能相等*/
	if(LCP < m){
		if(LCP < text_m){
			less = (P[LCP] < text[LCP]) ? true : false;
		} else {
			/*表示text中的字符个数少于 LCP个，这个时候明显 P大于 text */
			less = false;
		}
	}

	int64_t position;

	if(locus_node->is_leaf()){
		/* locus_node 是一个叶子节点, 故仅仅只有locus_node具有和P的最长公共前缀LCP
		locus_node的左兄弟一定是小于P,locus_node的右兄弟一定是大于P，所以需要判断当前节点的情形*/
		position = locus_node->get_index();
		if(LCP == m){
			/*如果LCP == m, 当前节点的左兄弟即为所求的位置（需要判断左兄弟是否存在的情况）*/
			if(position == 0){
				return -3;
			}
			else {
				return position - 1;
			}
		} else if( LCP < m && less){ /* p < text */
			/*当前节点的左兄弟 即为所求*/	
			if(position == 0)
				return -2;  /* P小于所有的后缀 */
			else		
				return position - 1;

		}else if(LCP < m && !less){ /* P > text */
			/*当前节点的即为所求 */
			if(position == m_g -1)
				return -1;  /*P大于所有的后缀 */
			else 
				return position;
		}
	}// end of if

	/*locus_node是一个内部节点，并且它的charLen > LCP && 它的节点parent_node的charLen <= LCP */

	if(LCP == m){
	/* 如果charLen(x) 等于 LCP，那么找到最左边的孩子的 左兄弟 即为所求 */
		while(!locus_node->is_leaf()) locus_node = locus_node->get_left_child();
		position = locus_node->get_index();
		if(position == 0)
			return -3; /*第一个后缀就是以P为前缀*/
		else return position - 1;
	}
	else if(LCP < m && less ){
	/* 如果charLen(x) >LCP, next_char(x)>next_char(y)，找到其子树中最左边的孩子lx 的左兄弟 即为所求
	* 因为 lx的左边的兄弟可以保证是小于P的，并且locus的所有的孩子都要大于P */
		while(!locus_node->is_leaf()) locus_node = locus_node->get_left_child();
		position = locus_node->get_index();
		if(position == 0)
			return -2;  /* P小于所有的后缀 */
		else 
			return position - 1;
	}
	else if(LCP < m && !less ){
	/* 如果charLen(x) >LCP, next_char(x) < next_char(y), 找到其最右边的孩子rx (注意：和上一个的不同)  即为所求
	* 因为这个rx的右边的兄弟可以保证是大于 P的，并且locus 的所有孩子都要小于P **/
		while(!locus_node->is_leaf()) locus_node = locus_node->get_right_child();
		position = locus_node->get_index();
		if(position == m_g -1)
			return -1;        /* P 大于所有的后缀 */
		else
			return position; 
	}

	free(text);
}// end of critbit_tree::get_left_position()


int64_t critbit_tree::get_right_position(uint8_t* P, 
										uint64_t m, 
										const uint8_t* T_filename, 
										uint64_t B,
										uint64_t* m_LCP,
										uint64_t* m_io){

	if(m_root == NULL){
		fprintf(stderr, "error!! critbit_tree::get_right_direction(): m_root can't be NULL.\n");
		exit(EXIT_FAILURE);
	}
	
	/*找到 当前critbit_tree中和P具有最长公共前缀的一个后缀：这就是 critbit_tree的作用*/
	critbit_node_t* cur_node = m_root;
	while(!cur_node->is_leaf()){
		uint64_t charLen = cur_node->get_char_len();
		uint64_t diffPos = cur_node->get_diff_pos();
		uint8_t diffChar = 0;
		
		if(charLen < m) diffChar = P[charLen];
		else{
			while(!cur_node->is_leaf()) cur_node = cur_node->get_right_child();
			break;
		}

		uint64_t direction = get_direction(diffChar, diffPos);
		cur_node = cur_node->get_child(direction);
	}// end of while	

	uint64_t j, k = 0;
	j = cur_node->get_suffix();  /*j 就是了具有和P最长公共前缀的后缀 */
	
	uint8_t* text;
	/*
	* 需要从文本中读取出来m个字符，虽然m个字符要比B小很多，但是却依旧可能导致两次磁盘I/O.
	* 当然这个后缀到文本结束的字符数量少于m个，这种情况很有可能发生。
	* 返回值 text_m 表示读取到的 text 中的字符的个数。
	* text 保存读取到的文本内容。
	**/

	uint64_t text_m = sb_read_subfile(T_filename, &text, j, m, B, m_io);
	
	/*得到最长的公共前缀*/
	for(k = 0; k < m && k < text_m && P[k] == text[k]; k++) {}
	
	uint64_t LCP = k;  /*这里不是 k + 1*/
	*m_LCP = LCP;


	uint8_t char1, char2;
	if(LCP == m) char1 = 0; else char1 = P[LCP];
	if(LCP == text_m) char2 = 0; else char2 = text[LCP];
	uint64_t diffPos2 = get_critbit_pos(char1, char2);		
	uint64_t critpos2 = (LCP << 3) | (diffPos2);


	/*找到一个节点locus_node, 以及它的父亲节点 parent_node,
	 保证 charLen(locus_node)>LCP, charLen(parent_node)<=LCP */
	critbit_node_t*  locus_node = m_root;
	critbit_node_t*  parent_node = NULL;

	uint64_t charLen, diffPos, critpos, direction;
	uint8_t diffChar;

	while(!locus_node->is_leaf()){
		charLen = locus_node->get_char_len();
		diffPos = locus_node->get_diff_pos();
		critpos = locus_node->get_critpos();
		diffChar = 0xFF;  /*这个非常重要，具体分析，写完程序就要写下来。*/
		if(charLen < m) diffChar = P[charLen];
		direction = get_direction(diffChar, diffPos);	

		if(critpos > critpos2)
			break;	

		parent_node = locus_node;		
		locus_node = locus_node->get_child(direction);
	}//end of while

	bool less; /*less = true, 表示P的下一个字符小于 text的下一个字符，否则大于，二者不可能相等.*/
	if(LCP < m){
		if(LCP < text_m) {	
			assert(P[LCP] != text[LCP]);
			less = (P[LCP] < text[LCP]) ? true : false;
		} else {
			less = false;
		}
	}

	int64_t position;
	if(locus_node->is_leaf()){
		/*此时，locus_node是一个叶子节点，可以知道仅仅只有 locus_node具有和P的最长公共前缀LCP
		  locus_node 的左兄弟一定是小于P, locus_node的右兄弟一定是大于P, 需要判断当前节点的情况*/
		position = locus_node->get_index();
		if(LCP == m){
			/*此时LCP == m,当前节点即为所求*/
			return position;			
		} else if(LCP < m && less ) {
			/*此时 P 小于当前的后缀, locus_node的左兄弟即为所求，需要判断左兄弟是否存在*/
			if(position == 0)
				return -2;   /*P 小于所有的后缀*/
			return position -1;
		} else if(LCP < m && !less) {
			/*此时 P 大于当前的后缀，如果locus_node的右兄弟存在，locus_node的右兄弟大于P，
			所以当前节点即为所求 */
			return position;
		}
	}

	/*locus_node是一个内部节点，并且它的childLen > LCP && 它的节点 parent_node 的childLen < LCP*/
	
	if(LCP == m) {
	/*此时locus_node的所有孩子都是以P为前缀，因此locus_node的子树最右边的叶子节点即为所求 */
		while(!locus_node->is_leaf()) locus_node = locus_node->get_right_child();
		position = locus_node->get_index();
		return position;
	}
	
	else if(LCP < m && less) {
	/*此时locus的所有的叶子节点的后缀都大于P，因此locus_node的子树的最左孩子的 “左兄弟即为所求” */
		while(!locus_node->is_leaf()) locus_node = locus_node->get_left_child();
		position = locus_node->get_index();
		if(position == 0) return -2;  /*P 小于所有的后缀*/
		return position -1;
	}

	else if(LCP < m && !less) {
	/*此时locus_node的所有的叶子节点的后缀都小于p, 因此locus_node的子树的最右边的孩子即为所求*/
		while(!locus_node->is_leaf()) locus_node = locus_node->get_right_child();
		position = locus_node->get_index();
		return position;
	}

	free(text);
}// end of member funtion critbit_tree::get_left_position()


/**
* 同时在这个节点中求取 left 和 right position.
***/
pair<int64_t, int64_t> critbit_tree::get_left_and_right_position(uint8_t* P, 
													uint64_t m, 
													const uint8_t* T_filename,
													uint64_t B,
													uint64_t* m_LCP,
													uint64_t* m_io){
	if(m_root == NULL){
		fprintf(stderr, "error!! critbit_tree::get_right_direction(): m_root can't be null.\n");
		exit(EXIT_FAILURE);
	}
	
	pair<int64_t, int64_t> result;
	/*在这个critbit_tree中找到和P具有最长公共前缀的一个后缀：这就是 critbit_tree的功能 */
	critbit_node_t* cur_node = m_root;
	while(!cur_node->is_leaf()){
		uint64_t charLen = cur_node->get_char_len();
		uint64_t diffPos = cur_node->get_diff_pos();
		uint8_t diffChar = 0;

		if(charLen < m) diffChar = P[charLen];		
		else{
			while(!cur_node->is_leaf()) cur_node = cur_node->get_left_child();
			break;
		}
		
		uint64_t direction = get_direction(diffChar, diffPos);
		cur_node = cur_node->get_child(direction);
	}

	uint64_t j, k = 0;	
	j = cur_node->get_suffix();   /*j是具有和P具有最长公共前缀的后缀*/
	
	
	uint8_t* text;
	/**
	* 需要从文本中读取出来m个字符，虽然m个字符要比B小的很多，但是却依旧可能导致两次磁盘I/O
	* 当然这个后缀到文本结束的字符数量可能少于m个，这种情况很有可能发生。
	* 返回值 text_m 表示要读取到的 text 中字符的个数。
	* text 保存读取到的文本中的内容。
	***/
	uint64_t text_m = sb_read_subfile(T_filename, &text, j, m, B, m_io);
	/*计算最长的公共前缀 */
	for(k = 0; k < m && k < text_m && P[k] == text[k]; k++) {}
	uint64_t LCP = k;
	*m_LCP = LCP;
/**=====================================left suffix range=========================================================**/
	/*找到一个节点locus_node，以及它的父亲节点 parent_node，
	 保证 charLen(locus_node) > LCP, charLen(parent_node) <= LCP */
	critbit_node_t* locus_node = m_root;
	critbit_node_t* parent_node = NULL;

	uint8_t char1, char2;
	if(LCP < m) char1 = P[LCP]; else char1 = 0;
	if(LCP < text_m) char2 = text[LCP]; else char2 = 0;
	uint64_t diffPos2 = get_critbit_pos(char1, char2);
	uint64_t critpos2 = (LCP << 3) | (diffPos2);

	uint64_t charLen, diffPos, direction, critpos;
	uint64_t diffChar;
	
	while(!locus_node->is_leaf()){
		charLen = locus_node->get_char_len();
		critpos = locus_node->get_critpos();
		diffPos = locus_node->get_diff_pos();
		diffChar = 0;
		if(charLen < m) diffChar = P[charLen];

		if(critpos > critpos2) break;

		direction = get_direction(diffChar, diffPos);	
		parent_node = locus_node;		
		locus_node = locus_node->get_child(direction);

	}// end of while()
	
	bool less;   /*less == true, 表示P的下一个字符小于 text的下一个字符，否则大于，二者不可能相等*/

	if(LCP < m){
		if(LCP < text_m){
			assert(P[LCP] != text[LCP]);
			less = (P[LCP] < text[LCP]) ? true : false;
		} else {
			less = false;
		}
	}		

	int64_t position;
	
	if(locus_node->is_leaf()){
		/*此时，locus_node是一个叶子节点，可以知道仅仅只有locus_node是具有和P的最长公共前缀LCP 
		locus_node 的左兄弟一定是小于P，locus_node的右兄弟一定是大于P，需要判断当前节点的情况*/
		position = locus_node->get_index();
		if(LCP == m){
		/*此时 LCP == m,当前节点的左兄弟即为所求的位置（需要判断左兄弟是否存在的情况)*/
			if(position == 0)
				result.first = -3;
			else result.first = position - 1;
		} else if(LCP < m && less){
			/*此时P小于当前的后缀，当前节点的左兄弟即为所求*/
			if(position == 0)
				result.first = -2; /*P 小于所有的后缀*/
			else result.first = position -1;
		} else if(LCP < m && !less){
			/*P大于当前的后缀，当前的位置即为所求*/
			if(position == m_g - 1)
				result.first = -1; /*P 大于所有的后缀*/
			else
				result.first = position;   
		}

	} 
	else { // begin locus_node 是一个内部节点
	
		/*locus_node是一个内部节点，并且它的charLen > LCP && 它的节点的 parent_node的charLen <= LCP*/
		if(LCP == m) {
			/*如果charLen(x)等于LCP, 那么找到最左边的孩子的 左兄弟 即为所求*/
			while(!locus_node->is_leaf()) locus_node = locus_node->get_left_child();
			position = locus_node->get_index();
			if(position == 0)
				result.first= -3;   /*第一个后缀是以P为前缀*/
			else 
				result.first = position -1;

		} else if(LCP < m && less) {
			/*locus_node的所有的孩子节点都大于P, locus_node最左边的孩子的左节点即为所求 */
			while(!locus_node->is_leaf()) locus_node = locus_node->get_left_child();
			position = locus_node->get_index();
			if(position == 0)
				result.first = -2;   /*P小于所有的后缀*/
			else
				result.first = position -1;
		} else if(LCP < m && !less) {
			/*locus_node的所有的孩子都小于P，locus_node最右边的孩子即为所求*/
			while(!locus_node->is_leaf()) locus_node = locus_node->get_right_child();
			position = locus_node->get_index();	
			if(position == m_g -1)
				result.first = -1;   /*P 大于所有的后缀*/
			else
				result.first = position;
		}

	} // end locus_node 是一个内部节点

/**====================================right suffix range===========================================================**/
	locus_node = m_root;
	parent_node = NULL;
	/** 找到一个节点locus_node, 以及它的父亲节点 parent_node。
	保证 charLen(locus_node) > LCP, charLen(parent_node) <= LCP **/
	while(!locus_node->is_leaf()){
		charLen = locus_node->get_char_len();
		critpos = locus_node->get_critpos();
		diffChar = 0xFF;
		if(charLen < m) diffChar = P[charLen];
		diffPos = locus_node->get_diff_pos();

		if(critpos > critpos2) break;

		direction = get_direction(diffChar, diffPos);
		parent_node = locus_node;
		locus_node = locus_node->get_child(direction);
	}

	if(locus_node->is_leaf())	{
		/*此时,locus_node是一个叶子节点，可以知道仅仅只有 locus_node具有和P的最长公共前缀LCP
		 locus_node的左兄弟一定是小于P，locus_node的右兄弟一定是大于P，需要判断当前节点的情况*/
		position = locus_node->get_index();
		if(LCP == m) {
			/*LCP == m，当前节点即为所求*/
			result.second = position;
		} else if(LCP < m && less) {
			if(position == 0)
				result.second = -2;  /*P小于所有的后缀*/
			else
				result.second = position - 1;
		} else if(LCP < m && !less) {
			result.second = position;
		}
	} 
	else {
	/*Locus_node 是一个内部节点，并且它的childLen > LCP && 它的父亲节点 parent_node的 childLen <= LCP*/
		if(LCP == m) {
			/*此时locus_node的所有孩子节点都是以P为前缀，因此locus_node的子树最右边的叶子节点即为所求*/
			while(!locus_node->is_leaf()) locus_node = locus_node->get_right_child();
			position = locus_node->get_index();
			result.second = position;
		} else if(LCP < m && less) {
			/*此时P小于locus_node的所有的孩子节点，因此locus_node的子树的最左边孩子的 “左兄弟”即为所求 */
			while(!locus_node->is_leaf()) locus_node = locus_node->get_left_child();
			position = locus_node->get_index();
			if(position == 0) position = -2; /*P 小于所有的后缀*/
			else  position = position -1;
			result.second = position;
		} else if(LCP < m && !less) {
			/*此时P大于locus_node的所有的孩子节点，因此locus_node的子树的最右边的孩子节点即为所求*/
			while(!locus_node->is_leaf()) locus_node = locus_node->get_right_child();
			position = locus_node->get_index();
			result.second = position;
		}		
	}

	free(text);
	return result;
}// end of member function critbit_tree::get_left_and_right_position()

int64_t critbit_tree::find_diff_char(uint64_t i, uint64_t j){
		int64_t k = 0;
		for(; ((i+k) < n) && ((j+k)<n) && T[i+k] == T[j+k]; k++){}  //It is time wasting.
		return k;
}


/*===================================================================================================*/

/** Useless and For debug. **/
void critbit_tree::print_bp(bit_vector& bv){

    fprintf(stderr, "bp size = %lu\n", bv.size());
    for(uint64_t i = 0; i < bv.size(); i++){
        if(bv[i])
        	fprintf(stderr, "[");
        else
        	fprintf(stderr, "]");
    }

    fprintf(stderr, "\n\n");
}

void critbit_tree::print_bp(uint64_t* mem, uint64_t g) {

	uint64_t size = (g+g-1)*2;
	for(uint64_t i = 0; i < size; i++){

		int x = getelem(mem, i, 1);
		if(x)
			fprintf(stderr, "[");
		else 
			fprintf(stderr, "]");
	}

	fprintf(stderr, "\n\n");
}

/* 完整的保存所有的critpos的值，
*  目的是为了计算critpos 所占据的最大的bit.
*/
int_vector<>* critbit_tree::create_full_posarray(){
	
	if(m_root == NULL || m_root->is_leaf()){
		/*there is no internal node */
		int_vector<>* pos = new int_vector<>(0);
		return pos;
	}
	int_vector<>* pos = new int_vector<>(m_g-1);
	uint64_t p = 0;
	critbit_node_t* cur_node = m_root;
	uint64_t parentpos = cur_node->get_critpos();

	(*pos)[p] = cur_node->get_critpos();
	++p;

	create_full_posarray_helper(cur_node->get_left_child(),*pos, p, parentpos);
	create_full_posarray_helper(cur_node->get_right_child(), *pos, p, parentpos);
	if(p != m_g -1){
		fprintf(stderr, "error creating posarray, (%lu, %lu).\n", p, m_g-1);
		exit(EXIT_FAILURE);
	}
	return pos;
}

void critbit_tree::create_full_posarray_helper(critbit_node_t* cur_node,
											int_vector<>& pos,
											uint64_t& p,
											uint64_t parentpos ){
	if(!cur_node->is_leaf()){
		uint64_t result = cur_node->get_critpos();
		pos[p] = result;
		p++;
		create_full_posarray_helper(cur_node->get_left_child(), pos, p, cur_node->get_critpos());
		create_full_posarray_helper(cur_node->get_right_child(), pos, p, cur_node->get_critpos());
	}

}

uint64_t critbit_tree::get_max_poswidth(){
	int_vector<>* posarray = create_full_posarray();
	util::bit_compress(*posarray);
	uint64_t pos_width = posarray->get_int_width();
	m_max_posarray_width = pos_width;
	delete posarray;
	return pos_width;
}


void critbit_tree::getAllSuffixes(vector<uint64_t>& result){

	critbit_node_t* locus = m_root;
	critbit_tree::getAllSuffixes_helper(result, locus);
}

void critbit_tree::getAllSuffixes_helper(vector<uint64_t>& result, 
											critbit_node_t* locus){

	if(!locus) {
		fprintf(stderr, "critbit_Tree::getAllSuffixes_helper---error collecting suffixes.\n");
		exit(EXIT_FAILURE);
	}

	if(locus->is_leaf()){
		uint64_t suffix = locus->get_suffix();
		result.push_back(suffix);
	} else {
		getAllSuffixes_helper(result, locus->get_left_child());
		getAllSuffixes_helper(result, locus->get_right_child());
	}
}