%{
#include <stdio.h>
#include "file_liberty.tab.h"

#define yyerror file_libertyerror
#define yylval file_libertylval
#define yylex  file_libertylex
#define TOKEN(t) (yylval.token=t)
#define YY_DECL extern "C" int yylex()

void yyerror(const char *s) {
    fprintf(stderr, "unable to recognize line %d: '%s'\n", yylineno, s);
    exit(1);
}

%}
%option noinput
%option nounput
%option noyywrap
%option yylineno
%s MULTILINE_COMMENT
%s SINGLELINE_COMMENT
%%
<INITIAL>{
"/*"            { BEGIN(MULTILINE_COMMENT); }
"//"            { BEGIN(SINGLELINE_COMMENT); }
[ \t\n]         { ; }
"\\"            { ; }
"\""            { ; }
[\ \t]:[\ \t]   { return ':'; }
[\(\)\{\},;']   { /* cout << yytext[0] << endl; */ return yytext[0]; }
([0-9]+:)+[0-9]+ { yylval.sval = strdup(yytext); return STRING; }
[\+\-]*[0-9]+(\.[0-9]*)*([Ee][\+\-]{0,1}[0-9]*)* { yylval.dval = atof(yytext); return NUMBER; }
\"[\+\-]*[0-9]+(\.[0-9]*)*([Ee][\+\-]{0,1}[0-9]*)*\" { unsigned i = 1; for (; yytext[i] != '\0'; ++i); yytext[i - 1] = '\0'; yylval.dval = atof(yytext + 1); return NUMBER; }
[\+\-\*\/]      { return yytext[0]; }
(([A-Za-z0-9_\.\-!&]+|\(c\))[\ ])*([A-Za-z0-9_\.\-!&]+|\(c\)) { /* cout << string(yytext) << endl; */ yylval.sval = strdup(yytext); return STRING; }
\"([^,;\\\"]|\\.|,\\\n)+\" { unsigned i = 1; for (; yytext[i] != '\0'; ++i); yytext[i - 1] = '\0'; yylval.sval = strdup(yytext + 1); return STRING; }
.               { ; }
}
<SINGLELINE_COMMENT>{
\n              { BEGIN(INITIAL); }
.               { ; }
}
<MULTILINE_COMMENT>{
"*/"            { BEGIN(INITIAL); }
[^*\n]+         { ; }
"*"             { ; }
\n              { ; }
}

%%

