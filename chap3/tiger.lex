%{
#include <string.h>
#include "util.h"
#include "y.tab.h"
#include "errormsg.h"

int charPos=1;

int yywrap(void)
{
 charPos=1;
 return 1;
}
int i = 0;
char str[1024];
int strSize = 0;
void append(char c){
	str[strSize] = c;
	strSize += 1;
}


void adjust(void)
{
 EM_tokPos=charPos;
 charPos+=yyleng;
}

%}
digits [0-9]+
whitespace [ \f\v\r\t]
%start  COMMENT STR STRESCAPE
%%
<INITIAL>{whitespace}	 {adjust();continue;}
<INITIAL>\n	 {adjust(); EM_newline(); continue;}
<INITIAL>for  	 {adjust(); return   FOR  	; }
<INITIAL>while    {adjust(); return   WHILE ;    }
<INITIAL>to		{adjust(); return   TO		;}
<INITIAL>break	{adjust(); return   BREAK	;}
<INITIAL>let		{adjust(); return   LET	;	}
<INITIAL>in		{adjust(); return   IN		;}
<INITIAL>end		{adjust(); return   END	;	}
<INITIAL>function	{adjust(); return  FUNCTION; }
<INITIAL>var		{adjust(); return   VAR	;	}
<INITIAL>type		{adjust(); return  TYPE ;}
<INITIAL>array	{adjust(); return   ARRAY	;}
<INITIAL>if		{adjust(); return   IF	;}
<INITIAL>else		{adjust(); return  ELSE	; }
<INITIAL>do		{adjust(); return   DO		;}
<INITIAL>of		{adjust(); return   OF		;}
<INITIAL>then	{adjust(); return   THEN	;}
<INITIAL>nil		{adjust(); return   NIL	;	}
<INITIAL>","        {adjust();   return COMMA;}
<INITIAL>":"        {adjust();   return COLON;}
<INITIAL>";"        {adjust();   return SEMICOLON;  }
<INITIAL>"("        {adjust();   return LPAREN; }
<INITIAL>")"        {adjust();   return RPAREN; }
<INITIAL>"["        {adjust();   return LBRACK; }
<INITIAL>"]"        {adjust();   return RBRACK; }
<INITIAL>"{"        {adjust();   return LBRACE; }
<INITIAL>"}"        {adjust();   return RBRACE; }
<INITIAL>"."        {adjust();   return DOT ;}
<INITIAL>"+"        {adjust();   return PLUS; }
<INITIAL>"-"        {adjust();   return MINUS ;}
<INITIAL>"*"        {adjust();   return TIMES ;}
<INITIAL>"/"        {adjust();   return DIVIDE; }
<INITIAL>"="        {adjust();   return EQ ;}
<INITIAL>"<>"        {adjust();  return NEQ; }
<INITIAL>"<"        {adjust();   return LT ;}
<INITIAL>"<="        {adjust();  return LE ;}
<INITIAL>">"        {adjust();   return GT ;}
<INITIAL>">="       {adjust();   return GE ;}
<INITIAL>"&"        {adjust();   return AND; }
<INITIAL>"|"        {adjust();   return OR ;}
<INITIAL>":="        {adjust();  return ASSIGN; }

<INITIAL>[a-zA-Z][a-zA-Z_0-9]* {
	adjust()	;
	yylval.sval = String(yytext);
	return ID;
}
<INITIAL>{digits}	 {adjust(); yylval.ival=atoi(yytext); return INT;}
<INITIAL>"\""		{adjust(); BEGIN STR;}
<INITIAL>"/*"	 {adjust();i++; BEGIN COMMENT;}
<INITIAL>.	 {adjust(); EM_error(EM_tokPos,"illegal token");}

<COMMENT>"*/"	{adjust();
	i -= 1;
	if (i == 0){
	 BEGIN INITIAL;
	}
}
<COMMENT>"/*"	{adjust();
	i += 1;
}
<COMMENT>\n	{adjust(); EM_newline(); }
<COMMENT><<EOF>>	{adjust(); EM_error(EM_tokPos,"comment eof error");return 0;}
<COMMENT>.	{adjust(); }

<STR>"\"" {adjust(); BEGIN INITIAL;
	append('\0');
	yylval.sval = String(str);
	return STRING;
;}
<STR>"\\t" {adjust();
	append('\t');
}
<STR>"\\\"" {adjust();
	append('\"');
}
<STR>"\\n" {adjust();
	append('\n');
}
<STR>"\\\\" {adjust();
	append('\\');
}
<STR>"\\" {
	adjust();
	BEGIN  STRESCAPE;

}
<STR><<EOF>>	{adjust(); EM_error(EM_tokPos,"string eof error");return 0;}

<STR>. {adjust();
	append(yytext[0]);
}


<STRESCAPE>{whitespace} {
	adjust();
}
<STRESCAPE>\n {
	adjust(); EM_newline();
}
<STRESCAPE>"\\" {
    adjust();
	BEGIN STR;
}
<STRESCAPE><<EOF>>	{adjust(); EM_error(EM_tokPos,"STRESCAPE eof error");return 0;}

<STRESCAPE>. {
	adjust(); EM_error(EM_tokPos,"illegal token");
}
