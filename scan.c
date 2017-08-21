#include "globals.h"
#include "util.h"
#include "scan.h"
typedef enum
{
	START, INLT, INGT, INEQ, INNOT, INSLASH, INCOMMENT, INCOMMENTSTAR, INNUM, INID, DONE
}
StateType;
char tokenString[MAXTOKENLEN + 1];
#define BUFLEN 256
static char lineBuf[BUFLEN];
static int linepos = 0;
static int bufsize = 0;
static int EOF_flag = FALSE;
static int getNextChar(void)
{
	if (!(linepos < bufsize))
	{
		lineno++;
		if (fgets(lineBuf, BUFLEN - 1, source))
		{
			if (EchoSource) fprintf(listing, "%4d: %s", lineno, lineBuf);
			bufsize = strlen(lineBuf);
			linepos = 0;
			return lineBuf[linepos++];
		}
		else
		{
			EOF_flag = TRUE;
			return EOF;
		}
	}
	else return lineBuf[linepos++];
}
static void ungetNextChar(void)
{
	if (!EOF_flag) linepos--;
}
static struct
{
	char* str;
	TokenType tok;
} reservedWords[MAXRESERVED]
= { {"else",ELSE},
   {"if",IF},
   {"int",INT},
   {"return",RETURN},
   {"void",VOID},
   {"while",WHILE} };
static TokenType reservedLookup(char * s)
{
	int i;
	for (i = 0; i < MAXRESERVED; i++)
		if (!strcmp(s, reservedWords[i].str))
			return reservedWords[i].tok;
	return ID;
}
TokenType getToken(void)
{

	int tokenStringIndex = 0;

	TokenType currentToken;

	StateType state = START;

	int save;
	while (state != DONE)
	{
		int c = getNextChar();
		save = TRUE;
		switch (state)
		{
		case START:
			if (isdigit(c))
				state = INNUM;
			else if (isalpha(c))
				state = INID;
			else if ((c == ' ') || (c == '\t') || (c == '\n'))
				save = FALSE;
			else if (c == '<')
				state = INLT;
			else if (c == '>')
				state = INGT;
			else if (c == '=')
				state = INEQ;
			else if (c == '!')
				state = INNOT;
			else if (c == '/')
			{
				save = FALSE;
				state = INSLASH;
			}
			else
			{
				state = DONE;
				switch (c)
				{
				case EOF:
					save = FALSE;
					currentToken = ENDFILE;
					break;
				case '+':
					currentToken = PLUS;
					break;
				case '-':
					currentToken = MINUS;
					break;
				case '*':
					currentToken = TIMES;
					break;
				case ';':
					currentToken = SEMI;
					break;
				case ',':
					currentToken = COMMA;
					break;
				case '(':
					currentToken = LPAREN;
					break;
				case ')':
					currentToken = RPAREN;
					break;
				case '[':
					currentToken = LBRACK;
					break;
				case ']':
					currentToken = RBRACK;
					break;
				case '{':
					currentToken = LBRACE;
					break;
				case '}':
					currentToken = RBRACE;
					break;
				default:
					currentToken = ERROR;
					break;
				}
			}
			break;
		case INSLASH:
			if (c == '*')
			{
				state = INCOMMENT;
				save = FALSE;
			}
			else
			{
				state = DONE;
				ungetNextChar();
				currentToken = OVER;
			}
			break;
		case INCOMMENT:
			save = FALSE;
			if (c == EOF)
			{
				state = DONE;
				currentToken = ENDFILE;
			}
			else if (c == '*') state = INCOMMENTSTAR;
			break;
		case INCOMMENTSTAR:
			save = FALSE;
			if (c == EOF)
			{
				state = DONE;
				currentToken = ENDFILE;
			}
			else if (c == '/') state = START;
			else state = INCOMMENT;
			break;
		case INLT:
			state = DONE;
			if (c == '=')
				currentToken = LTEQ;
			else
			{
				ungetNextChar();
				currentToken = LT;
			}
			break;
		case INGT:
			state = DONE;
			if (c == '=')
				currentToken = GTEQ;
			else
			{
				ungetNextChar();
				currentToken = GT;
			}
			break;
		case INEQ:
			state = DONE;
			if (c == '=')
				currentToken = EQ;
			else
			{
				ungetNextChar();
				currentToken = ASSIGN;
			}
			break;
		case INNOT:
			state = DONE;
			if (c == '=')
				currentToken = NEQ;
			else
			{
				ungetNextChar();
				save = FALSE;
				currentToken = ERROR;
			}
			break;
		case INNUM:
			if (!isdigit(c))
			{

				ungetNextChar();
				save = FALSE;
				state = DONE;
				currentToken = NUM;
			}
			break;
		case INID:
			if (!isalpha(c))
			{

				ungetNextChar();
				save = FALSE;
				state = DONE;
				currentToken = ID;
			}
			break;
		case DONE:
		default:
			fprintf(listing, "Scanner Bug: state= %d\n", state);
			state = DONE;
			currentToken = ERROR;
			break;
		}
		if ((save) && (tokenStringIndex <= MAXTOKENLEN))
			tokenString[tokenStringIndex++] = (char)c;
		if (state == DONE)
		{
			tokenString[tokenStringIndex] = '\0';
			if (currentToken == ID)
				currentToken = reservedLookup(tokenString);
		}
	}
	if (TraceScan)
	{
		fprintf(listing, "\t%d: ", lineno);
		printToken(currentToken, tokenString);
	}
	return currentToken;
}
