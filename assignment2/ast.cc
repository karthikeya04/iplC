#include "ast.hh"

std::string get_str(std::string s)
{
    return "\"" + s + "\"";
}

void print_exp(std::string node_name,exp_astnode* exp)
{
    std::cout << get_str(node_name) << " : {\n";
    std::cout << std::endl;
    exp->print();
    std::cout << "}";
}
void print_st(std::string node_name,statement_astnode* st) // print statement node
{
    std::cout << get_str(node_name) << " : ";
    if(st->astnode_type != kempty_astnode)
        std::cout << "{\n";
    st->print();
    if(st->astnode_type != kempty_astnode)
        std::cout << "}";
}
void print_util(exp_astnode* node, std::string par, std::vector<std::string> children)
{
    std::cout << get_str(par) + " : {\n";
    for(int i = 0; i < (int)children.size(); i++)
    {
        std::string child = children[i];
        if(child == "op")
        {
            std::string op_string = (node->astnode_type == kop_binary_astnode) ? static_cast<op_binary_astnode*>(node)->get_bop_str() : static_cast<op_unary_astnode*>(node)->get_uop_str();
            std::cout << get_str("op") + " : " + get_str(op_string) + ",\n";
            continue;

        }
        assert(node->children.count(child));
        print_exp(child,node->children[child]);
        if(i != (int)children.size() - 1)
        {
            std::cout << ",";
        }
        std::cout << "\n";
    }
    std::cout << "}\n";   
}
void print_exp_list(std::string par,std::vector<exp_astnode*> children)
{
    std::cout << get_str(par) + " : [\n";
    for(int i = 0; i < (int) children.size(); i++)
    {
        auto child = children[i];
        std::cout << "{\n";
        child->print();
        std::cout << "}";
        if(i != (int)children.size() - 1)
        {
            std::cout << ",";
        }
        std::cout << "\n";

    }
    std::cout << "]\n";

}
void print_st_list(std::string par,std::vector<statement_astnode*> children)
{
    std::cout << get_str(par) + ": [\n";
    for(int i = 0; i < (int) children.size(); i++)
    {
        auto child = children[i];
        if(child->astnode_type != kempty_astnode) std::cout << "{\n";
        child->print();
        if(child->astnode_type != kempty_astnode) std::cout << "}";
        if(i != (int)children.size() - 1)
        {
            std::cout << ",";
        }
        std::cout << "\n";

    }
    std::cout << "]\n";
}


exp_astnode::exp_astnode (typeExp astnode_type){
    this->astnode_type = astnode_type;
}
exp_astnode::exp_astnode (exp_astnode& exp)
{
    this->children = exp.children;
    this->dtype = exp.dtype;
    this->astnode_type = exp.astnode_type;

}

ref_astnode::ref_astnode (typeExp astnode_type)
{
    this->lvalue = true;
    this->astnode_type = astnode_type;
}
ref_astnode::ref_astnode (ref_astnode& exp) : exp_astnode(exp)
{
    this->lvalue = true;
    this->children = exp.children;
}
// ref_astnodes

identifier_astnode::identifier_astnode () : ref_astnode(kidentifier_astnode) {}
identifier_astnode::identifier_astnode (std::string id,datatype dtype) : ref_astnode(kidentifier_astnode),id(id)
{
    this->dtype = dtype;
}
void identifier_astnode::print(int blanks)
{
    std::cout << get_str("identifier") + ": "  + get_str(this->id) << "\n";
}

member_astnode::member_astnode () : ref_astnode(kmember_astnode) {}
member_astnode::member_astnode (exp_astnode* exp_node,exp_astnode* id_node) : ref_astnode(kmember_astnode)
{
    this->dtype = id_node->dtype;
    this->children["struct"] = exp_node;
    this->children["field"] = id_node;
}
void member_astnode::print(int blanks)
{
    std::cout << get_str("member") + " : {\n";
    std::cout << get_str("struct") + " : {\n";
    this->children["struct"]->print();
    std::cout << "},\n";
    std::cout << get_str("field") + " : {\n";
    this->children["field"]->print();
    std::cout << "}\n}\n";
}

arrayref_astnode::arrayref_astnode () : ref_astnode(karrayref_astnode) {}
arrayref_astnode::arrayref_astnode (arrayref_astnode& exp) : ref_astnode(exp) {}
arrayref_astnode::arrayref_astnode (exp_astnode* array,exp_astnode* index) : ref_astnode(karrayref_astnode)
{
    this->children["array"] = array;
    this->children["index"] = index;
    this->dtype = array->dtype;
    this->dtype.deref();

}
void arrayref_astnode::print(int blanks)
{
    std::vector<std::string> children  = {"array","index"};
    print_util(this,"arrayref",children);
}

arrow_astnode::arrow_astnode () : ref_astnode(karrow_astnode) {}
arrow_astnode::arrow_astnode (exp_astnode* pointer,exp_astnode* field) : ref_astnode(karrow_astnode)
{
    this->children["pointer"] = pointer;
    this->children["field"] = field;
    this->dtype = field->dtype;
}
void arrow_astnode::print(int blanks)
{
    std::vector<std::string> children  = {"pointer","field"};
    print_util(this,"arrow",children);

}

// exp_astnodes 
op_unary_astnode::op_unary_astnode() :  exp_astnode(kop_unary_astnode) {}
op_unary_astnode::op_unary_astnode(operation_type_unary op,exp_astnode* child) : exp_astnode(kop_unary_astnode), op(op) 
{
    if(op == kDEREF)
        this->lvalue = true;
    else 
        this->lvalue = false;
    this->children["child"] = child;
    this->dtype = child->dtype;
    switch (op)
    {
    case kTO_FLOAT :
        this->dtype.ts.bdtype = kFLOAT;
        break;
    case kTO_INT :
        this->dtype.ts.bdtype = kINT;
        break;
    case kNOT :
        this->dtype = datatype();
        this->dtype.ts.bdtype = kINT;
        break;
    case kADDRESS :
        this->dtype.inc_stars_top();
        break;
    case kDEREF :
        this->dtype.deref();
        break;
    default:
        break;
    }
}
std::string op_unary_astnode::get_uop_str ()
{
    switch (op)
    {
    case kTO_FLOAT :
        return "TO_FLOAT";
    case kTO_INT :
        return "TO_INT";
    case kUMINUS :
        return "UMINUS";
    case kNOT :
        return "NOT";
    case kADDRESS :
        return "ADDRESS";
    case kDEREF :
        return "DEREF";
    case kPP : 
        return "PP";
    }
    return "";

}
void op_unary_astnode::print(int blanks)
{
    std::vector<std::string> children  = {"op","child"};
    print_util(this,"op_unary",children);
}

op_binary_astnode::op_binary_astnode() :  exp_astnode(kop_binary_astnode) {}
op_binary_astnode::op_binary_astnode (operation_type_binary op,exp_astnode* L,exp_astnode* R,base_dtype bdtype) : exp_astnode(kop_binary_astnode), op(op) 
{
    
    datatype ldtype = L->dtype,rdtype = R->dtype;
    int lstars = ldtype.get_tot_stars(),rstars = rdtype.get_tot_stars();
    this->op_type = kINT;
    this->children["left"] = L; // These will be changes if any sort of casting is used in last else block
    this->children["right"] = R;
    if(lstars)
    {
        std::set<operation_type_binary> S({kPLUS,kMULT,kMINUS,kDIV});
        if(S.count(this->op))
            this->dtype = ldtype;
        else
        {
            this->dtype.ts.bdtype = kINT;
        }
    }
    else if(rstars)
    {
        std::set<operation_type_binary> S({kPLUS,kMULT,kMINUS,kDIV});
        if(S.count(this->op))
            this->dtype = rdtype;
        else
        {
            this->dtype.ts.bdtype = kINT;
        }

    }
    else
    {

        // for +/-
        this->dtype.ts.bdtype = bdtype;
    
        if(this->op == kAND_OP || this->op == kOR_OP)
        {
            this->children["left"] = L;
            this->children["right"] = R;  
            this->dtype.ts.bdtype = kINT;
            return;
        }
        base_dtype opdtype = kINT;
        if(L->dtype.ts.bdtype == kFLOAT || R->dtype.ts.bdtype == kFLOAT)
        {
            opdtype = kFLOAT;
        }
        op_type = opdtype;
        if(this->dtype.ts.bdtype == kANY)
        {
            this->dtype.ts.bdtype = opdtype;
            
        }
        std::set<operation_type_binary> aops({kDIV,kMULT,kMINUS,kPLUS});
        if(aops.count(this->op) == 0)
        {
            this->dtype.ts.bdtype = kINT;
        }

        exp_astnode *left = L,*right = R;
        if(opdtype == kFLOAT)
        {  
            // typecasting
            if(L->dtype.ts.bdtype == kINT)
            {
                left = new op_unary_astnode(kTO_FLOAT,L);
            }
            if(R->dtype.ts.bdtype == kINT)
            {
                right = new op_unary_astnode(kTO_FLOAT,R);
            }
        }
        this->children["left"] = left;
        this->children["right"] = right;  
    }

} 
std::string op_binary_astnode::get_bop_str()
{
    std::string res = "";
    switch (op)
    {
    case kOR_OP:
        res = "OR_OP";
        break;
    case kAND_OP:
        res = "AND_OP";
        break;
    case kEQ_OP:
        res = "EQ_OP";
        break;
    case kNE_OP:
        res = "NE_OP";
        break;
    case kLT_OP:
        res = "LT_OP";
        break;
    case kGT_OP:
        res = "GT_OP";
        break;
    case kLE_OP:
        res = "LE_OP";
        break;
    case kGE_OP:
        res = "GE_OP";
        break;
    case kPLUS:
        res = "PLUS";
        break;
    case kMINUS:
        res = "MINUS";
        break;
    case kMULT:
        res = "MULT";
        break;
    case kDIV:
        res = "DIV";
        break;
    default:
        break;
    }
    if(op!=kAND_OP&&op!=kOR_OP)
    {
        switch (op_type)
        {
        case kINT:
            res += "_INT";
            break;
        case kFLOAT:
            res += "_FLOAT";
            break;
        default:
            break;
        }
    }
    return res;
}
void op_binary_astnode::print(int blanks)
{
    std::vector<std::string> children  = {"op","left","right"};
    print_util(this,"op_binary",children);
}

assignE_astnode::assignE_astnode () : exp_astnode(kassignE_astnode) {}
assignE_astnode::assignE_astnode (exp_astnode* L, exp_astnode* R) : exp_astnode(kassignE_astnode)
{
    base_dtype opdtype = kINT;
    if(L->dtype.ts.bdtype == kFLOAT || R->dtype.ts.bdtype == kFLOAT)
    {
        opdtype = kFLOAT;
    }
    exp_astnode *left = L,*right = R;
    if(opdtype == kFLOAT)
    {  
        // typecasting
        if(L->dtype.ts.bdtype == kINT)
        {
            left = new op_unary_astnode(kTO_FLOAT,L);
        }
        if(R->dtype.ts.bdtype == kINT)
        {
            right = new op_unary_astnode(kTO_FLOAT,R);
        }
    }
    this->children["left"] = left;
    this->children["right"] = right; 
    this->dtype.ts.bdtype = kINT;
}
void assignE_astnode::print(int blanks)
{
    std::vector<std::string> children  = {"left","right"};
    print_util(this,"assignE",children);
}

funcall_astnode::funcall_astnode () : exp_astnode(kfuncall_astnode) {}
funcall_astnode::funcall_astnode (exp_astnode* fname,std::vector<exp_astnode*> params) : exp_astnode(kfuncall_astnode)
{
    this->params = params;
    this->children["fname"] = fname;
    this->dtype = fname->dtype;
}
void funcall_astnode::print(int blanks)
{
    std::cout << get_str("funcall") + " : {\n";
    std::cout << get_str("fname") + ": {\n";
    this->children["fname"]->print();
    std::cout << "},\n";
    print_exp_list("params",this->params);
    std::cout << "}\n";   
}

intconst_astnode::intconst_astnode () : exp_astnode(kintconst_astnode) {}
intconst_astnode::intconst_astnode (int value) : exp_astnode(kintconst_astnode),value(value)
{
    this->dtype.ts.bdtype = kINT;
}
void intconst_astnode::print(int blanks)
{
    std::cout << get_str("intconst") + " : " + std::to_string(value) + "\n";
}

floatconst_astnode::floatconst_astnode () : exp_astnode(kfloatconst_astnode) {}
floatconst_astnode::floatconst_astnode (float value) : exp_astnode(kfloatconst_astnode),value(value)
{
    this->dtype.ts.bdtype = kFLOAT;
}
void floatconst_astnode::print(int blanks)
{
    std::cout << get_str("floatconst") + " : " << value << "\n";
}

stringconst_astnode::stringconst_astnode () : exp_astnode(kstringconst_astnode) {}
stringconst_astnode::stringconst_astnode (std::string value) : exp_astnode(kstringconst_astnode),value(value)
{
    this->dtype.ts.bdtype = kSTRING;
}
void stringconst_astnode::print(int blanks)
{
    std::cout << get_str("stringconst") + " : " + value + "\n";
}


// statement_astnodes

statement_astnode::statement_astnode(typeExp astnode_type)
{
    this->astnode_type = astnode_type;
}

empty_astnode::empty_astnode() : statement_astnode(kempty_astnode) {}
void empty_astnode::print(int blanks)
{
    std::cout << get_str("empty")+"\n";
}

seq_astnode::seq_astnode() : statement_astnode(kseq_astnode) {}
seq_astnode::seq_astnode(seq_astnode& st)
{
    this->statements = st.statements;
}
seq_astnode::seq_astnode(std::vector<statement_astnode*> statements) : statement_astnode(kseq_astnode), statements(statements) {}
void seq_astnode::print(int)
{
    print_st_list("seq",this->statements);
}
void seq_astnode::push(statement_astnode* st)
{
    this->statements.push_back(st);
}

return_astnode::return_astnode (exp_astnode* return_exp) : statement_astnode(kreturn_astnode), return_exp(return_exp) {}
void return_astnode::print (int blanks)
{
    std::cout << get_str("return") + " : {\n";
    this->return_exp->print();
    std::cout << "}\n";
}

assignS_astnode::assignS_astnode (exp_astnode* ass_exp) : statement_astnode (kassignS_astnode)
{
    this->exp_children["left"] = ass_exp->children["left"];
    this->exp_children["right"] = ass_exp->children["right"];
}
void assignS_astnode::print (int blanks)
{
    std::cout << get_str("assignS") + " : {\n";
    std::cout << get_str("left") + ": {\n";
    this->exp_children["left"]->print();
    std::cout <<"},\n";
    std::cout << get_str("right") + ": {\n";
    this->exp_children["right"]->print();
    std::cout << "}\n}\n";
}

proccall_astnode::proccall_astnode (exp_astnode* fname,std::vector<exp_astnode*> params) : statement_astnode(kproccall_astnode)
{
    this->exp_children["fname"] = fname;
    this->params = params;
    
}
void proccall_astnode::print (int blanks)
{
    std::cout << get_str("proccall") + " : {\n";
    std::cout << get_str("fname") + ": {\n";
    this->exp_children["fname"]->print();
    std::cout << "},\n";
    print_exp_list("params",this->params);
    std::cout << "}\n";      
}

if_astnode::if_astnode (exp_astnode* cond,statement_astnode* then_st,statement_astnode* else_st) : statement_astnode(kif_astnode)
{
    this->exp_children["cond"] = cond;
    this->st_children["then"] = then_st;
    this->st_children["else"] = else_st;
}
void if_astnode::print (int blanks)
{
    std::cout << get_str("if") + ": {\n";
    print_exp("cond",this->exp_children["cond"]);
    std::cout << ",\n";
    print_st("then",this->st_children["then"]);
    std::cout << ",\n";
    print_st("else",this->st_children["else"]);
    std::cout << "}\n";

}

while_astnode::while_astnode (exp_astnode* cond,statement_astnode* stmt) : statement_astnode(kwhile_astnode)
{
    this->exp_children["cond"] = cond;
    this->st_children["stmt"] = stmt;
}
void while_astnode::print (int blanks)
{
    std::cout << get_str("while") + ": {\n";
    print_exp("cond",this->exp_children["cond"]);
    std::cout << ",\n";
    print_st("stmt",this->st_children["stmt"]);
    std::cout << "}\n";
}

for_astnode::for_astnode (exp_astnode* init,exp_astnode* guard,exp_astnode* step,statement_astnode* body) : statement_astnode(kfor_astnode)
{
    this->exp_children["init"] = init;
    this->exp_children["guard"] = guard;
    this->exp_children["step"] = step;
    this->st_children["body"] = body;
}
void for_astnode::print (int blanks)
{
    std::cout << get_str("for") + ": {\n";
    print_exp("init",this->exp_children["init"]);
    std::cout << ",\n";
    print_exp("guard",this->exp_children["guard"]);
    std::cout << ",\n";
    print_exp("step",this->exp_children["step"]);
    std::cout << ",\n";
    print_st("body",this->st_children["body"]);
    std::cout << "}\n";
}










