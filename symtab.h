#ifndef _SYMTAB_H_
#define _SYMTAB_H_
#include "globals.h"
#define SIZE 211
typedef struct LineListRec
{
	int lineno;
	struct LineListRec * next;
} *LineList;
typedef struct BucketListRec
{
	char * name;
	LineList lines;
	TreeNode *treeNode;
	int memloc;
	struct BucketListRec * next;
} *BucketList;
typedef struct ScopeRec
{
	char * funcName;
	int nestedLevel;
	struct ScopeRec * parent;
	BucketList hashTable[SIZE];
} *Scope;
Scope globalScope;
void st_insert(char * name, int lineno, int loc, TreeNode * treeNode);
int st_lookup(char * name);
int st_add_lineno(char * name, int lineno);
BucketList st_bucket(char * name);
int st_lookup_top(char * name);
Scope sc_create(char *funcName);
Scope sc_top(void);
void sc_pop(void);
void sc_push(Scope scope);
int addLocation(void);
void printSymTab(FILE * listing);
#endif
