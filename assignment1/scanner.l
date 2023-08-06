%{
// This part goes directly into the generated parser

#include "scanner.hh"
#include "parser.tab.hh"

// YY_DECL is bound to the type of the scanner. The default is 
// #define YY_DECL int Scanner::yylex()
// which is generated anyway.

//If you want the type of the generated scanner to be different, do this

#undef  YY_DECL
#define YY_DECL int IPL::Scanner::yylex( IPL::Parser::semantic_type * const lval, IPL::Parser::location_type *location )

//We want these to be the first action after a token is recognized. 

#define YY_USER_ACTION loc->step(); loc->columns(yyleng);

// token_type is a short hand for IPL::Parser::token

using token = IPL::Parser::token;

%}

DIGIT [0-9]
EXPONENT_PART [eE][+-]?{DIGIT}+
INT_CONSTANT [1-9]{DIGIT}*|0[bB][01]*|0[0-7]*|0[xX][0-9a-fA-F]*
FLOAT_CONSTANT ({DIGIT}+\.{DIGIT}*|{DIGIT}*\.{DIGIT}+)({EXPONENT_PART})?
STRING_LITERAL \"((\\\\)|(\\\")|[^\"\\]|(\\[tn]))*\"
IDENTIFIER [a-zA-Z_][a-zA-Z_0-9]*
COMMENTS \/\*([^*]|(\*[^\/]))*(\*)+\/



/* %option debug */
%option nodefault
%option yyclass="Scanner"
%option noyywrap
%option c++

%%

%{ /** Code executed at the beginning of yylex **/

      yylval = lval;

%}

[ \t\n]                            //skip
{COMMENTS}                         //skip

"while" {
       yylval->emplace< std::string > ( YYText() );
       return token::WHILE;
}
"for" {
       yylval->emplace< std::string > ( YYText() );
       return token::FOR;
}
"struct" {
       yylval->emplace< std::string > ( YYText() );
       return token::STRUCT;
}
"if" {
       yylval->emplace< std::string > ( YYText() );
       return token::IF;
}
"else" {
       yylval->emplace< std::string > ( YYText() );
       return token::ELSE;
}
"int" {
       yylval->emplace< std::string > ( YYText() );
       return token::INT;
}
"void" {
       yylval->emplace< std::string > ( YYText() );
       return token::VOID;
}
"float" {
       yylval->emplace< std::string > ( YYText() );
       return token::FLOAT;
}
"return" {
       yylval->emplace< std::string > ( YYText() );
       return token::RETURN;
}

{FLOAT_CONSTANT} {
       yylval->emplace< std::string > ( YYText() );
       return token::FLOAT_CONSTANT;
}
{INT_CONSTANT} {
       yylval->emplace< std::string > ( YYText() );
       return token::INT_CONSTANT;
}
{IDENTIFIER} {
       yylval->emplace< std::string > ( YYText() );
       return token::IDENTIFIER;
}
{STRING_LITERAL} {
       yylval->emplace< std::string > ( YYText() );
       return token::STRING_LITERAL;
}
">=" {
       yylval->emplace< std::string > ( YYText() );
       return token::GE_OP;
}
"<=" {
       yylval->emplace< std::string > ( YYText() );
       return token::LE_OP;
}
"->" {
       yylval->emplace< std::string > ( YYText() );
       return token::PTR_OP;
}
"++" {
       yylval->emplace< std::string > ( YYText() );
       return token::INC_OP;
}
"!=" {
       yylval->emplace< std::string > ( YYText() );
       return token::NE_OP;
}
"==" {
       yylval->emplace< std::string > ( YYText() );
       return token::EQ_OP;
}
"&&" {
       yylval->emplace< std::string > ( YYText() );
       return token::AND_OP;
}
"||" {
       yylval->emplace< std::string > ( YYText() );
       return token::OR_OP;
}

">"                                         return '>';
"<"                                         return '<';
"="                                         return '=';

"&"                                         return '&';
"!"                                         return '!';
"{"                                         return '{';
"}"                                         return '}';
"."                                         return '.';

"+"                                         return '+';
"-"                                         return '-';
"*"                                         return '*';
"/"                                         return '/';

"["                                         return '[';
"]"                                         return ']';
";"                                         return ';';
","                                         return ',';
"("                                         return '(';
")"                                         return ')';
":"                                         return ':';    
"\n"                                        return '\n';
<<EOF>>                                     yyterminate();
.				              return token::OTHERS;


%%

