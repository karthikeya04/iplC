#ifndef TYPE_H
#define TYPE_H

#include <string>
#include <vector>
#include <iostream>

#define INT_SIZE 4
#define FLOAT_SIZE 4
#define PTR_SIZE 4

extern std::string get_str(std::string);


typedef enum {
    global,
    local,
    param
} st_scope;

typedef enum {
    kempty_astnode,
    kseq_astnode,
    kassignS_astnode,
    kreturn_astnode,
    kif_astnode,
    kwhile_astnode,
    kfor_astnode,
    kproccall_astnode,
    kop_binary_astnode,
    kop_unary_astnode,
    kassignE_astnode,
    kfuncall_astnode,
    kintconst_astnode,
    kfloatconst_astnode,
    kstringconst_astnode,
    kmember_astnode,
    karrow_astnode,
    kidentifier_astnode,
    karrayref_astnode

} typeExp;

typedef enum {
    kOR_OP,
    kAND_OP,
    kEQ_OP,
    kNE_OP,
    kLT_OP,
    kGT_OP,
    kLE_OP,
    kGE_OP,
    kPLUS,
    kMINUS,
    kMULT,
    kDIV

} operation_type_binary;

typedef enum {
    kTO_FLOAT,
    kTO_INT,
    kUMINUS,
    kNOT,
    kADDRESS,
    kDEREF,
    kPP
} operation_type_unary;

typedef enum {
    kINT,
    kFLOAT,
    kVOID,
    kSTRUCT,
    kSTRING,
    kANY
} base_dtype;

typedef enum {
    kPARAMS,
    kLOCALS
} decl_type;


typedef enum {
    st_kfun,
    st_kstruct,
    st_kglobal
} symtab_type;

typedef enum {
    kfun,
    kvar,
    kstruct
} st_entry_type;

class ctype_specifier
{
    public:
    base_dtype bdtype;
    std::string sname; // format : struct var_name, NA when bdtype is not kSTRUCT
    int sz;
    ctype_specifier();
    ctype_specifier (base_dtype,int = 0,std::string ="");
    int get_size();
    std::string get_in_string_format();
    bool operator==(ctype_specifier);
};
class datatype {
    public:
    int sz = 0;
    std::vector<int> arr;
    int stars = 0;
    int stars_top = 0; // only relevant to & operator. 
    ctype_specifier ts;
    datatype () {ts.bdtype = kANY;};
    datatype (ctype_specifier);
    std::string get_in_string_format(); // raw type, ex: a[4][5] => [4][5]
    std::string get_type_string(); // Actual type, ex: a[4][5] => (*)[5]
    void add_arr(int);
    void inc_stars(int = 1);
    void inc_stars_top(int = 1);
    int get_size();
    int get_tot_stars();
    void deref(); 
    bool operator==(datatype);

};
class cdecl
{
    public:
    std::string id;
    datatype dtype;
    cdecl() {}
    cdecl (std::string,datatype);
    cdecl (std::string);
    cdecl (cdecl&);
};
class cdecl_inh
{
    public:
    ctype_specifier ts;
    st_scope scope;
    cdecl_inh() {}
    cdecl_inh(ctype_specifier,st_scope);
};

#endif