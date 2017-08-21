#include "globals.h"
#include "symtab.h"
#include "code.h"
#include "cgen.h"
static char buffer[1000];
#define ofpFO 0
#define retFO -1
#define initFO -2
static int globalOffset = 0;
static int localOffset = initFO;
static int numOfParams = 0;
static int isInFunc = FALSE;
static int mainFuncLoc = 0;
static void cGen(TreeNode * tree);
static int getBlockOffset(TreeNode *list)
{
	int offset = 0;
	if (list == NULL)
	{
	}
	else if (list->nodekind == DeclK)
	{

		TreeNode *node = list;
		while (node != NULL)
		{
			switch (node->kind.decl)
			{
			case VarK:
				++offset;
				break;
			case ArrVarK:
				offset += node->attr.arr.size;
				break;
			default:
				break;
			}
			node = node->sibling;
		}
	}
	else if (list->nodekind == ParamK)
	{

		TreeNode *node = list;
		while (node != NULL)
		{
			++offset;
			node = node->sibling;
		}
	}
	return offset;
}
static void genStmt(TreeNode * tree)
{
	TreeNode * p1, *p2, *p3;
	int savedLoc1, savedLoc2, currentLoc;
	int loc;
	int offset;
	switch (tree->kind.stmt)
	{
	case CompK:
		if (TraceCode) emitComment("-> compound");
		p1 = tree->child[0];
		p2 = tree->child[1];

		offset = getBlockOffset(p1);
		localOffset -= offset;

		sc_push(tree->attr.scope);

		cGen(p2);

		sc_pop();

		localOffset -= offset;
		if (TraceCode) emitComment("<- compound");
		break;
	case IfK:
		if (TraceCode) emitComment("-> if");
		p1 = tree->child[0];
		p2 = tree->child[1];
		p3 = tree->child[2];

		cGen(p1);
		savedLoc1 = emitSkip(1);
		emitComment("if: jump to else belongs here");

		cGen(p2);
		savedLoc2 = emitSkip(1);
		emitComment("if: jump to end belongs here");
		currentLoc = emitSkip(0);
		emitBackup(savedLoc1);
		emitRM_Abs("JEQ", ac, currentLoc, "if: jmp to else");
		emitRestore();

		cGen(p3);
		currentLoc = emitSkip(0);
		emitBackup(savedLoc2);
		emitRM_Abs("LDA", pc, currentLoc, "jmp to end");
		emitRestore();
		if (TraceCode)  emitComment("<- if");
		break;
	case IterK:
		if (TraceCode) emitComment("-> iter.");
		p1 = tree->child[0];
		p2 = tree->child[1];
		savedLoc1 = emitSkip(0);
		emitComment("while: jump after body comes back here");

		cGen(p1);
		savedLoc2 = emitSkip(1);
		emitComment("while: jump to end belongs here");

		cGen(p2);
		emitRM_Abs("LDA", pc, savedLoc1, "while: jmp back to test");

		currentLoc = emitSkip(0);
		emitBackup(savedLoc2);
		emitRM_Abs("JEQ", ac, currentLoc, "while: jmp to end");
		emitRestore();
		if (TraceCode)  emitComment("<- iter.");
		break;
	case RetK:
		if (TraceCode) emitComment("-> return");
		p1 = tree->child[0];

		cGen(p1);
		emitRM("LD", pc, retFO, mp, "return: to caller");
		if (TraceCode) emitComment("<- return");
		break;
	default:
		break;
	}
}
static void genExp(TreeNode * tree, int lhs)
{
	int loc;
	int varOffset, baseReg;
	int numOfArgs;
	TreeNode * p1, *p2;
	switch (tree->kind.exp)
	{
	case AssignK:
		if (TraceCode) emitComment("-> assign");
		p1 = tree->child[0];
		p2 = tree->child[1];

		genExp(p1, TRUE);

		emitRM("ST", ac, localOffset--, mp, "assign: push left (address)");

		cGen(p2);

		emitRM("LD", ac1, ++localOffset, mp, "assign: load left (address)");
		emitRM("ST", ac, 0, ac1, "assign: store value");
		if (TraceCode) emitComment("<- assign");
		break;
	case OpK:
		if (TraceCode) emitComment("-> Op");
		p1 = tree->child[0];
		p2 = tree->child[1];

		cGen(p1);

		emitRM("ST", ac, localOffset--, mp, "op: push left");

		cGen(p2);

		emitRM("LD", ac1, ++localOffset, mp, "op: load left");
		switch (tree->attr.op)
		{
		case PLUS:
			emitRO("ADD", ac, ac1, ac, "op +");
			break;
		case MINUS:
			emitRO("SUB", ac, ac1, ac, "op -");
			break;
		case TIMES:
			emitRO("MUL", ac, ac1, ac, "op *");
			break;
		case OVER:
			emitRO("DIV", ac, ac1, ac, "op /");
			break;
		case LT:
			emitRO("SUB", ac, ac1, ac, "op <");
			emitRM("JLT", ac, 2, pc, "br if true");
			emitRM("LDC", ac, 0, ac, "false case");
			emitRM("LDA", pc, 1, pc, "unconditional jmp");
			emitRM("LDC", ac, 1, ac, "true case");
			break;
		case LTEQ:
			emitRO("SUB", ac, ac1, ac, "op <=");
			emitRM("JLE", ac, 2, pc, "br if true");
			emitRM("LDC", ac, 0, ac, "false case");
			emitRM("LDA", pc, 1, pc, "unconditional jmp");
			emitRM("LDC", ac, 1, ac, "true case");
			break;
		case GT:
			emitRO("SUB", ac, ac1, ac, "op >");
			emitRM("JGT", ac, 2, pc, "br if true");
			emitRM("LDC", ac, 0, ac, "false case");
			emitRM("LDA", pc, 1, pc, "unconditional jmp");
			emitRM("LDC", ac, 1, ac, "true case");
			break;
		case GTEQ:
			emitRO("SUB", ac, ac1, ac, "op >=");
			emitRM("JGE", ac, 2, pc, "br if true");
			emitRM("LDC", ac, 0, ac, "false case");
			emitRM("LDA", pc, 1, pc, "unconditional jmp");
			emitRM("LDC", ac, 1, ac, "true case");
			break;
		case EQ:
			emitRO("SUB", ac, ac1, ac, "op  == ");
			emitRM("JEQ", ac, 2, pc, "br if true");
			emitRM("LDC", ac, 0, ac, "false case");
			emitRM("LDA", pc, 1, pc, "unconditional jmp");
			emitRM("LDC", ac, 1, ac, "true case");
			break;
		case NEQ:
			emitRO("SUB", ac, ac1, ac, "op !=");
			emitRM("JNE", ac, 2, pc, "br if true");
			emitRM("LDC", ac, 0, ac, "false case");
			emitRM("LDA", pc, 1, pc, "unconditional jmp");
			emitRM("LDC", ac, 1, ac, "true case");
			break;
		default:
			emitComment("BUG: Unknown operator");
			break;
		}
		if (TraceCode)  emitComment("<- Op");
		break;
	case ConstK:
		if (TraceCode) emitComment("-> Const");

		emitRM("LDC", ac, tree->attr.val, 0, "load const");
		if (TraceCode)  emitComment("<- Const");
		break;
	case IdK:
	case ArrIdK:
		if (TraceCode)
		{
			sprintf(buffer, "-> Id (%s)", tree->attr.name);
			emitComment(buffer);
		}
		loc = st_lookup_top(tree->attr.name);
		if (loc >= 0)
			varOffset = initFO - loc;
		else
			varOffset = -(st_lookup(tree->attr.name));

		emitRM("LDC", ac, varOffset, 0, "id: load varOffset");
		if (tree->kind.exp == ArrIdK)
		{

			if (loc >= 0 && loc < numOfParams)
			{

				emitRO("ADD", ac, mp, ac, "id: load the memory address of base address of array to ac");
				emitRO("LD", ac, 0, ac, "id: load the base address of array to ac");
			}
			else
			{


				if (loc >= 0)

					emitRO("ADD", ac, mp, ac, "id: calculate the address");
				else

					emitRO("ADD", ac, gp, ac, "id: calculate the address");
			}

			emitRM("ST", ac, localOffset--, mp, "id: push base address");

			p1 = tree->child[0];
			cGen(p1);

			emitRM("LD", ac1, ++localOffset, mp, "id: pop base address");
			emitRO("SUB", ac, ac1, ac, "id: calculate element address with index");
		}
		else
		{


			if (loc >= 0)

				emitRO("ADD", ac, mp, ac, "id: calculate the address");
			else

				emitRO("ADD", ac, gp, ac, "id: calculate the address");
		}
		if (lhs)
		{
			emitRM("LDA", ac, 0, ac, "load id address");
		}
		else
		{
			emitRM("LD", ac, 0, ac, "load id value");
		}
		if (TraceCode)  emitComment("<- Id");
		break;
	case CallK:
		if (TraceCode) emitComment("-> Call");

		numOfArgs = 0;
		p1 = tree->child[0];

		while (p1 != NULL)
		{

			if (p1->type == IntegerArray)
				genExp(p1, TRUE);
			else
				genExp(p1, FALSE);

			emitRM("ST", ac, localOffset + initFO - (numOfArgs++), mp,
				"call: push argument");
			p1 = p1->sibling;
		}
		if (strcmp(tree->attr.name, "input") == 0)
		{

			emitRO("IN", ac, 0, 0, "read integer value");
		}
		else if (strcmp(tree->attr.name, "output") == 0)
		{


			emitRM("LD", ac, localOffset + initFO, mp, "load arg to ac");

			emitRO("OUT", ac, 0, 0, "write ac");
		}
		else
		{

			emitRM("ST", mp, localOffset + ofpFO, mp, "call: store current mp");

			emitRM("LDA", mp, localOffset, mp, "call: push new frame");

			emitRM("LDA", ac, 1, pc, "call: save return in ac");

			loc = -(st_lookup(tree->attr.name));
			emitRM("LD", pc, loc, gp, "call: relative jump to function entry");

			emitRM("LD", mp, ofpFO, mp, "call: pop current frame");
		}
		if (TraceCode)  emitComment("<- Call");
		break;
	default:
		break;
	}
}
static void genDecl(TreeNode * tree)
{
	TreeNode * p1, *p2;
	int loadFuncLoc, jmpLoc, funcBodyLoc, nextDeclLoc;
	int loc;
	int size;
	switch (tree->kind.decl)
	{
	case FuncK:
		if (TraceCode)
		{
			sprintf(buffer, "-> Function (%s)", tree->attr.name);
			emitComment(buffer);
		}
		p1 = tree->child[1];
		p2 = tree->child[2];
		isInFunc = TRUE;

		loc = -(st_lookup(tree->attr.name));
		loadFuncLoc = emitSkip(1);
		emitRM("ST", ac1, loc, gp, "func: store the location of func. entry");

		--globalOffset;

		jmpLoc = emitSkip(1);
		emitComment(
			"func: unconditional jump to next declaration belongs here");
		funcBodyLoc = emitSkip(0);
		emitComment("func: function body starts here");

		emitBackup(loadFuncLoc);
		emitRM("LDC", ac1, funcBodyLoc, 0, "func: load function location");
		emitRestore();

		emitRM("ST", ac, retFO, mp, "func: store return address");

		localOffset = initFO;
		numOfParams = 0;
		cGen(p1);

		if (strcmp(tree->attr.name, "main") == 0)
			mainFuncLoc = funcBodyLoc;
		cGen(p2);

		emitRM("LD", pc, retFO, mp, "func: load pc with return address");

		nextDeclLoc = emitSkip(0);
		emitBackup(jmpLoc);
		emitRM_Abs("LDA", pc, nextDeclLoc,
			"func: unconditional jump to next declaration");
		emitRestore();
		isInFunc = FALSE;
		if (TraceCode)
		{
			sprintf(buffer, "-> Function (%s)", tree->attr.name);
			emitComment(buffer);
		}
		break;
	case VarK:
	case ArrVarK:
		if (TraceCode) emitComment("-> var. decl.");
		if (tree->kind.decl == ArrVarK)
			size = tree->attr.arr.size;
		else
			size = 1;
		if (isInFunc == TRUE)
			localOffset -= size;
		else
			globalOffset -= size;
		if (TraceCode) emitComment("<- var. decl.");
		break;
	default:
		break;
	}
}
static void genParam(TreeNode * tree)
{
	switch (tree->kind.stmt)
	{
	case ArrParamK:
	case NonArrParamK:
		if (TraceCode) emitComment("-> param");
		emitComment(tree->attr.name);
		--localOffset;
		++numOfParams;
		if (TraceCode) emitComment("<- param");
		break;
	default:
		break;
	}
}
static void cGen(TreeNode * tree)
{
	if (tree != NULL)
	{
		switch (tree->nodekind)
		{
		case StmtK:
			genStmt(tree);
			break;
		case ExpK:
			genExp(tree, FALSE);
			break;
		case DeclK:
			genDecl(tree);
			break;
		case ParamK:
			genParam(tree);
			break;
		default:
			break;
		}
		cGen(tree->sibling);
	}
}
void genMainCall()
{
	emitRM("LDC", ac, globalOffset, 0, "init: load globalOffset");
	emitRO("ADD", mp, mp, ac, "init: initialize mp with globalOffset");
	if (TraceCode) emitComment("-> Call");

	emitRM("ST", mp, ofpFO, mp, "call: store current mp");

	emitRM("LDA", mp, 0, mp, "call: push new frame");

	emitRM("LDA", ac, 1, pc, "call: save return in ac");

	emitRM("LDC", pc, mainFuncLoc, 0, "call: unconditional jump to main() entry");

	emitRM("LD", mp, ofpFO, mp, "call: pop current frame");
	if (TraceCode) emitComment("<- Call");
}
void codeGen(TreeNode * syntaxTree, char * codefile)
{
	char * s = malloc(strlen(codefile) + 7);
	strcpy(s, "File: ");
	strcat(s, codefile);
	emitComment("TINY Compilation to TM Code");
	emitComment(s);

	emitComment("Standard prelude:");
	emitRM("LD", gp, 0, ac, "load gp with maxaddress");
	emitRM("LDA", mp, 0, gp, "copy gp to mp");
	emitRM("ST", ac, 0, ac, "clear location 0");
	emitComment("End of standard prelude.");

	sc_push(globalScope);

	cGen(syntaxTree);

	sc_pop();

	genMainCall();

	emitComment("End of execution.");
	emitRO("HALT", 0, 0, 0, "");
}
