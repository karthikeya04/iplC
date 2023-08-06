
#ifndef AST_H
#define AST_H

#include "type.hh"
#include "symtab.hh"
#include <vector>
#include <map>
#include <iostream>
#include <cassert>
#include <set>

extern symtab* globalst;
extern std::string curr_func;
extern std::map<std::string,datatype> ret_type;

class exp_astnode;
class statement_astnode;
extern int Lcount;
extern int ret_idx;

std::string get_str(std::string);

class abstract_astnode 
{
    public:
    virtual void print(int = 0) = 0;
    virtual void gencode() = 0;
    typeExp astnode_type;

};

// Abstract classes

class statement_astnode : public abstract_astnode
{   
    public:
    std::map <std::string, exp_astnode* > exp_children;
    std::map <std::string, statement_astnode*> st_children;
    statement_astnode () {}
    statement_astnode (typeExp);

};

class exp_astnode : public abstract_astnode
{
    public:
    std::map <std::string, exp_astnode* > children;
    datatype  dtype; // datatype of the node
    bool lvalue;
    exp_astnode () {}
    exp_astnode (exp_astnode&);
    exp_astnode (typeExp);
};

class ref_astnode : public exp_astnode
{
    public:

    ref_astnode () {}
    ref_astnode (ref_astnode&);
    ref_astnode (typeExp);
};

class identifier_astnode : public ref_astnode
{
    public:
    std::string id;
    std::string fname;
    identifier_astnode();
    //identifier_astnode()
    identifier_astnode(std::string,std::string,datatype = datatype()); // id, dtype
    void print(int = 0);
    void gencode();
};
class member_astnode : public ref_astnode
{
    public:
    member_astnode();
    member_astnode(exp_astnode*,exp_astnode*); // Last param points to identifier_astnode
    void print(int = 0);
    void gencode();

};
class arrayref_astnode : public ref_astnode
{
    public:
    arrayref_astnode ();
    arrayref_astnode (arrayref_astnode&);
    arrayref_astnode (exp_astnode*, exp_astnode*); // array, index
    void print(int = 0);
    void gencode();

};
class arrow_astnode : public ref_astnode
{
    public:
    arrow_astnode ();
    arrow_astnode (exp_astnode*,exp_astnode*); // struct*, id_node
    void print(int);
    void gencode();
};
// exp_astnode

class op_unary_astnode : public exp_astnode
{
    public:
    operation_type_unary op;
    op_unary_astnode ();
    op_unary_astnode (operation_type_unary,exp_astnode*);
    std::string get_uop_str();
    void print(int);
    void gencode();
};

class op_binary_astnode : public exp_astnode
{
    public:
    operation_type_binary op;
    base_dtype op_type;
    op_binary_astnode ();
    op_binary_astnode (operation_type_binary,exp_astnode*,exp_astnode*,base_dtype = kANY);
    std::string get_bop_str();
    void print(int);
    void gencode();
};



class assignE_astnode : public exp_astnode
{
    public:
    assignE_astnode ();
    assignE_astnode (exp_astnode*,exp_astnode*);
    void print(int);
    void gencode();
};

class funcall_astnode : public exp_astnode
{
    public:
    std::vector<exp_astnode*> params;
    funcall_astnode ();
    funcall_astnode (exp_astnode*,std::vector<exp_astnode*> = std::vector<exp_astnode*>(0)); // fname, params
    void print(int);
    void gencode();
};

class intconst_astnode : public exp_astnode
{
    public: 
    int value;
    intconst_astnode ();
    intconst_astnode (int);
    void print(int = 0);
    void gencode();
};

class floatconst_astnode : public exp_astnode
{
    public:
    float value;
    floatconst_astnode ();
    floatconst_astnode (float);
    void print(int = 0);
    void gencode() {};

};

class stringconst_astnode : public exp_astnode
{
    public:
    std::string value;
    stringconst_astnode ();
    stringconst_astnode (std::string);
    void print(int = 0);
    void gencode() {};

};


// statement_astnode

class empty_astnode : public statement_astnode
{
    public:
    empty_astnode();
    void print(int = 0);
    void gencode() {};

};

class seq_astnode : public statement_astnode
{
    public:
    std::vector<statement_astnode*> statements;
    seq_astnode ();
    seq_astnode (seq_astnode&);
    seq_astnode (std::vector<statement_astnode*>);
    void print(int = 0);
    void push(statement_astnode*);
    void gencode();
};

class assignS_astnode : public statement_astnode
{
    public:
    assignS_astnode () {}
    assignS_astnode (exp_astnode*);
    void print(int = 0);
    void gencode();
};

class return_astnode : public statement_astnode
{
    public:
    exp_astnode* return_exp;
    std::string fname;
    return_astnode () {}
    return_astnode (exp_astnode*,std::string);
    void print(int = 0);
    void gencode();
};

class if_astnode : public statement_astnode
{
    public:
    if_astnode () {}
    if_astnode (exp_astnode*,statement_astnode*,statement_astnode*);
    void print(int = 0);
    void gencode();
};

class while_astnode : public statement_astnode
{
    public:
    while_astnode () {}
    while_astnode (exp_astnode*,statement_astnode*);
    void print(int = 0);
    void gencode();
};

class for_astnode : public statement_astnode
{
    public:
    for_astnode() {}
    for_astnode (exp_astnode*,exp_astnode*,exp_astnode*,statement_astnode*);
    void print(int = 0);
    void gencode();
};

class proccall_astnode : public statement_astnode
{
    public:
    std::vector<exp_astnode*> params;
    proccall_astnode() {}
    proccall_astnode(exp_astnode*,std::vector<exp_astnode*> = std::vector<exp_astnode*>(0)); 
    void print(int = 0);
    void gencode();
};

#endif