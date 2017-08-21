#ifndef _GLOBALS_H_
#define _GLOBALS_H_
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>#ifndef YYPARSER
#include "y.tab.h"
#define ENDFILE 0
#endif#ifndef FALSE
#define FALSE 0
#endif#ifndef TRUE
#define TRUE 1
#endif
#define MAXRESERVED 6
typedef int TokenType;
extern FILE* source;
extern FILE* listing;
extern FILE* code;
extern int lineno;
typedef enum { StmtK, ExpK, DeclK, ParamK, TypeK } NodeKind;
typedef enum { CompK, IfK, IterK, RetK } StmtKind;
typedef enum { AssignK, OpK, ConstK, IdK, ArrIdK, CallK } ExpKind;
typedef enum { FuncK, VarK, ArrVarK } DeclKind;
typedef enum { ArrParamK, NonArrParamK } ParamKind;
typedef enum { TypeNameK } TypeKind;
typedef struct arrayAttr {
	TokenType type;
	char * name;
	int size;
} ArrayAttr;
typedef enum { Void, Integer, Boolean, IntegerArray } ExpType;
#define MAXCHILDREN 3
struct ScopeRec;
typedef struct treeNode
{
	struct treeNode * child[MAXCHILDREN];
	struct treeNode * sibling;
	int lineno;
	NodeKind nodekind;
	union {
		StmtKind stmt;
		ExpKind exp;
		DeclKind decl;
		ParamKind param;
		TypeKind type;
	} kind;
	union {
		TokenType op;
		TokenType type;
		int val;
		char * name;
		ArrayAttr arr;
		struct ScopeRec * scope;
	} attr;
	ExpType type;
} TreeNode;
extern int EchoSource;
extern int TraceScan;
extern int TraceParse;
extern int TraceAnalyze;
extern int TraceCode;
extern int Error;
#endif
