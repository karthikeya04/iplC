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

void inc_esp_by_4()
{
    std::cout << "\taddl\t$4, %esp\n";
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
identifier_astnode::identifier_astnode (std::string id,std::string f,datatype dtype) : ref_astnode(kidentifier_astnode),id(id), fname(f)
{
    this->dtype = dtype;
}
void identifier_astnode::print(int blanks)
{
    std::cout << get_str("identifier") + ": "  + get_str(this->id) << "\n";
}
void identifier_astnode::gencode()
{
    symtab* st = globalst->entries[this->fname]->localst;
    auto entry = st->entries[id];
    if(entry)
    {
        std::cout << "\tleal\t" << entry->offset << "(%ebp), %eax\n";
        if(entry->scope == param && entry->type.arr.size() >= 1)
        {
            std::cout << "\tmovl\t(%eax), %eax\n";
        }

        std::cout << "\tpushl\t%eax\n";
    
    }

}

member_astnode::member_astnode () : ref_astnode(kmember_astnode) {}
member_astnode::member_astnode (exp_astnode* exp_node,exp_astnode* id_node) : ref_astnode(kmember_astnode)
{
    this->dtype = id_node->dtype;
    this->children["struct"] = exp_node;
    this->children["field"] = id_node;
    //this->lvalue = exp_node->lvalue;
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
void member_astnode::gencode()
{
    exp_astnode* exp_node = this->children["struct"];
    exp_astnode* id_node = this->children["field"];

    symtab* st = globalst->entries[exp_node->dtype.get_in_string_format()]->localst;

    exp_node->gencode();

    int field_offset = st->entries[static_cast<identifier_astnode*>(id_node)->id]->offset;
    //std::cout << "fo : "<<field_offset << "\n";
    std::cout << "\taddl\t$" << field_offset << ", " << "(%esp)\n";
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
void arrayref_astnode::gencode()
{
    exp_astnode* array = this->children["array"];
    exp_astnode* index = this->children["index"];
    array->gencode();
    index->gencode();

    std::cout << "\tmovl\t(%esp), %ebx\n";
    inc_esp_by_4();
    if(index->lvalue)
    {
        std::cout << "\tmovl\t(%ebx), %ebx\n";
    }  
    std::cout << "\tmovl\t(%esp), %eax\n";
    inc_esp_by_4();

    if(array->dtype.arr.size() == 0)
    {
        std::cout << "\tmovl\t(%eax), %eax\n";
    }
    datatype elem_dtype = array->dtype;
    elem_dtype.deref();
    // eax = eax + size * ebx;
    std::cout << "\timull\t$" <<  elem_dtype.get_size() << ", %ebx, %ebx\n";
    std::cout << "\taddl\t%ebx, %eax\n";
    std::cout << "\tpushl\t%eax\n";

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
void arrow_astnode::gencode()
{
    exp_astnode* pointer = this->children["pointer"];
    exp_astnode* field = this->children["field"];
    pointer->gencode();
    
    datatype pointee_dtype = pointer->dtype;
    pointee_dtype.deref();

    symtab* st = globalst->entries[pointee_dtype.get_in_string_format()]->localst;
    int field_offset = st->entries[static_cast<identifier_astnode*>(field)->id]->offset;
    if(pointer->lvalue)
    {
        std::cout << "\tmovl\t(%esp), %eax\n";
        std::cout << "\tmovl\t(%eax), %eax\n";
        std::cout << "\tmovl\t%eax, (%esp)\n";
    }

    std::cout << "\taddl\t$" << field_offset << ", " << "(%esp)\n";
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
void op_unary_astnode::gencode()
{
    auto op = this->op;
    exp_astnode* exp = this->children["child"];
    exp->gencode();
    if(op == kUMINUS)
    {
        std::cout << "\tmovl\t(%esp), %eax\n";
        inc_esp_by_4();
        if(exp->lvalue)
        {
            std::cout << "\tmovl\t(%eax), %eax\n";
        }  
        std::cout << "\tnegl\t%eax\n";
        std::cout << "\tpushl\t%eax\n";
       
    }
    else if(op == kNOT)
    {
        std::cout << "\tmovl\t(%esp), %eax\n";
        inc_esp_by_4();
        if(exp->lvalue)
        {
            std::cout << "\tmovl\t(%eax), %eax\n";
        }  
        std::cout << "\tcmpl\t$0, %eax\n";
        std::cout << "\tsete\t%al\n";
        std::cout << "\tmovzbl\t%al, %eax\n"; // Now eax = !exp
        std::cout << "\tpushl\t%eax\n";
        
    }
    else if(op == kADDRESS)
    {
        // Address is already at esp
    }
    else if(op == kDEREF)
    {
        if(exp->lvalue)
        {
            std::cout << "\tmovl\t(%esp), %eax\n";
            inc_esp_by_4();
            std::cout << "\tmovl\t(%eax), %eax\n";
            std::cout << "\tpushl\t%eax\n";
        }
        // else the pointer is already on top of the stack
        
      
    }
    else if(op == kPP)
    {
        std::cout << "\tmovl\t(%esp), %eax\n";
        inc_esp_by_4();
        std::cout << "\tmovl\t(%eax), %ebx\n";
        std::cout << "\taddl\t$1, (%eax)\n";
        std::cout << "\tpushl\t%ebx\n";
       

    }
}


op_binary_astnode::op_binary_astnode() :  exp_astnode(kop_binary_astnode) {}
op_binary_astnode::op_binary_astnode (operation_type_binary op,exp_astnode* L,exp_astnode* R,base_dtype bdtype) : exp_astnode(kop_binary_astnode), op(op) 
{
    
    datatype ldtype = L->dtype,rdtype = R->dtype;
    this->lvalue = false;
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
void op_binary_astnode::gencode()
{
    exp_astnode* left = this->children["left"];
    exp_astnode* right = this->children["right"];

    if(op == kOR_OP)
    {
        left->gencode();
        std::cout << "\tmovl\t(%esp), %eax\n";
        inc_esp_by_4();
        if(left->lvalue)
        {
            std::cout << "\tmovl\t(%eax), %eax\n";
        }  
        int idx1 = Lcount++,idx0 = Lcount++,out_idx = Lcount++;
        std::cout << "\tcmpl\t$0, %eax\n";
        std::cout << "\tjne\t.L" << idx1 << "\n";

        right->gencode();
        std::cout << "\tmovl\t(%esp), %eax\n";
        inc_esp_by_4();
        if(right->lvalue)
        {
            std::cout << "\tmovl\t(%eax), %eax\n";
        } 
        std::cout << "\tcmpl\t$0, %eax\n";
        std::cout << "\tje\t.L" << idx0 << "\n";

        std::cout << ".L" << idx1 << ":\n";
        std::cout << "\tmovl\t$1, %eax\n";
        std::cout << "\tjmp\t.L" << out_idx <<"\n";

        std::cout << ".L" << idx0 << ":\n";
        std::cout << "\tmovl\t$0, %eax\n";

        std::cout << ".L" << out_idx << ":\n";
        std::cout << "\tpushl\t%eax\n";

    }   
    else if(op == kAND_OP)
    {
        left->gencode();
        std::cout << "\tmovl\t(%esp), %eax\n";
        inc_esp_by_4();
        if(left->lvalue)
        {
            std::cout << "\tmovl\t(%eax), %eax\n";
        }  
        int idx0 = Lcount++,out_idx = Lcount++;
        std::cout << "\tcmpl\t$0, %eax\n";
        std::cout << "\tje\t.L" << idx0 << "\n";

        right->gencode();
        std::cout << "\tmovl\t(%esp), %eax\n";
        inc_esp_by_4();
        if(right->lvalue)
        {
            std::cout << "\tmovl\t(%eax), %eax\n";
        } 
        std::cout << "\tcmpl\t$0, %eax\n";
        std::cout << "\tje\t.L" << idx0 << "\n";

        std::cout << "\tmovl\t$1, %eax\n";
        std::cout << "\tjmp\t.L" << out_idx << "\n";

        std::cout << ".L" << idx0 << ":\n";
        std::cout << "\tmovl\t$0, %eax\n";

        std::cout << ".L" << out_idx << ":\n";
        std::cout << "\tpushl\t%eax\n";


    }
    else if(op == kEQ_OP)
    {
        left->gencode();
        right->gencode();

        std::cout << "\tmovl\t(%esp), %eax\n";
        inc_esp_by_4();
        if(right->lvalue)
        {
            std::cout << "\tmovl\t(%eax), %eax\n";
        }  
        std::cout << "\tmovl\t(%esp), %ebx\n";
        inc_esp_by_4();
        if(left->lvalue)
        {
            std::cout << "\tmovl\t(%ebx), %ebx\n";
        }  
        std::cout << "\tcmpl\t%eax, %ebx\n";
        std::cout << "\tsete\t%al\n";
        std::cout << "\tmovzbl\t%al, %eax\n";
        std::cout << "\tpushl\t%eax\n";

    }
    else if(op == kNE_OP)
    {
        left->gencode();
        right->gencode();

        std::cout << "\tmovl\t(%esp), %eax\n";
        inc_esp_by_4();
        if(right->lvalue)
        {
            std::cout << "\tmovl\t(%eax), %eax\n";
        }  
        std::cout << "\tmovl\t(%esp), %ebx\n";
        inc_esp_by_4();
        if(left->lvalue)
        {
            std::cout << "\tmovl\t(%ebx), %ebx\n";
        }  
        std::cout << "\tcmpl\t%eax, %ebx\n";
        std::cout << "\tsetne\t%al\n";
        std::cout << "\tmovzbl\t%al, %eax\n";
        std::cout << "\tpushl\t%eax\n";
    }
    else if(op == kLT_OP)
    {
        left->gencode();
        right->gencode();

        std::cout << "\tmovl\t(%esp), %eax\n";
        inc_esp_by_4();
        if(right->lvalue)
        {
            std::cout << "\tmovl\t(%eax), %eax\n";
        }  
        std::cout << "\tmovl\t(%esp), %ebx\n";
        inc_esp_by_4();
        if(left->lvalue)
        {
            std::cout << "\tmovl\t(%ebx), %ebx\n";
        }  
        std::cout << "\tcmpl\t%eax, %ebx\n";
        std::cout << "\tsetl\t%al\n";
        std::cout << "\tmovzbl\t%al, %eax\n";
        std::cout << "\tpushl\t%eax\n";
    }
    else if(op == kGT_OP)
    {
        left->gencode();
        right->gencode();

        std::cout << "\tmovl\t(%esp), %eax\n";
        inc_esp_by_4();
        if(right->lvalue)
        {
            std::cout << "\tmovl\t(%eax), %eax\n";
        }  
        std::cout << "\tmovl\t(%esp), %ebx\n";
        inc_esp_by_4();
        if(left->lvalue)
        {
            std::cout << "\tmovl\t(%ebx), %ebx\n";
        }  
        std::cout << "\tcmpl\t%eax, %ebx\n";
        std::cout << "\tsetg\t%al\n";
        std::cout << "\tmovzbl\t%al, %eax\n";
        std::cout << "\tpushl\t%eax\n";

    }
    else if(op == kLE_OP)
    {
        left->gencode();
        right->gencode();

        std::cout << "\tmovl\t(%esp), %eax\n";
        inc_esp_by_4();
        if(right->lvalue)
        {
            std::cout << "\tmovl\t(%eax), %eax\n";
        }  
        std::cout << "\tmovl\t(%esp), %ebx\n";
        inc_esp_by_4();
        if(left->lvalue)
        {
            std::cout << "\tmovl\t(%ebx), %ebx\n";
        }  
        std::cout << "\tcmpl\t%eax, %ebx\n";
        std::cout << "\tsetle\t%al\n";
        std::cout << "\tmovzbl\t%al, %eax\n";
        std::cout << "\tpushl\t%eax\n";
    }
    else if(op == kGE_OP)
    {
        left->gencode();
        right->gencode();

        std::cout << "\tmovl\t(%esp), %eax\n";
        inc_esp_by_4();
        if(right->lvalue)
        {
            std::cout << "\tmovl\t(%eax), %eax\n";
        }  
        std::cout << "\tmovl\t(%esp), %ebx\n";
        inc_esp_by_4();
        if(left->lvalue)
        {
            std::cout << "\tmovl\t(%ebx), %ebx\n";
        }  
        std::cout << "\tcmpl\t%eax, %ebx\n";
        std::cout << "\tsetge\t%al\n";
        std::cout << "\tmovzbl\t%al, %eax\n";
        std::cout << "\tpushl\t%eax\n";

    }
    else if(op == kPLUS)
    {
        left->gencode();
        right->gencode();

        std::cout << "\tmovl\t(%esp), %eax\n";
        inc_esp_by_4();
        //printf("lval : %d\n",right->lvalue);

        if(right->lvalue)
        {
            std::cout << "\tmovl\t(%eax), %eax\n";
        }  
        std::cout << "\tmovl\t(%esp), %ebx\n";
        inc_esp_by_4();
        if(left->lvalue)
        {
            std::cout << "\tmovl\t(%ebx), %ebx\n";
        }
        if(left->dtype.get_tot_stars())
        {
            datatype pointee_dtype = left->dtype;
            pointee_dtype.deref();
            std::cout << "\timull\t$" << pointee_dtype.get_size() << ", %eax, %eax\n";
        }  
        if(right->dtype.get_tot_stars())
        {
            datatype pointee_dtype = right->dtype;
            pointee_dtype.deref();
            std::cout << "\timull\t$" << pointee_dtype.get_size() << ", %ebx, %ebx\n";

        }
        std::cout << "\taddl\t%ebx, %eax\n";
        std::cout << "\tpushl\t%eax\n";
    }
    else if(op == kMINUS)
    {
        left->gencode();
        right->gencode();

        std::cout << "\tmovl\t(%esp), %eax\n";
        inc_esp_by_4();
        if(right->lvalue)
        {
            std::cout << "\tmovl\t(%eax), %eax\n";
        }  
        std::cout << "\tmovl\t(%esp), %ebx\n";
        inc_esp_by_4();
        if(left->lvalue)
        {
            std::cout << "\tmovl\t(%ebx), %ebx\n";
        }  
        if(left->dtype.get_tot_stars())
        {
            datatype pointee_dtype = left->dtype;
            pointee_dtype.deref();
            std::cout << "\timull\t$" << pointee_dtype.get_size() << ", %eax, %eax\n";
            
        }
        std::cout << "\tsubl\t%eax, %ebx\n";
        std::cout << "\tpushl\t%ebx\n";
    }
    else if(op == kMULT)
    {
        left->gencode();
        right->gencode();

        std::cout << "\tmovl\t(%esp), %eax\n";
        inc_esp_by_4();
        if(right->lvalue)
        {
            std::cout << "\tmovl\t(%eax), %eax\n";
        }  
        std::cout << "\tmovl\t(%esp), %ebx\n";
        inc_esp_by_4();
        if(left->lvalue)
        {
            std::cout << "\tmovl\t(%ebx), %ebx\n";
        }  
        std::cout << "\timull\t%ebx, %eax\n";
        std::cout << "\tpushl\t%eax\n";
    }
    else if(op == kDIV)
    {
        left->gencode();
        right->gencode();

        std::cout << "\tmovl\t(%esp), %ebx\n";
        inc_esp_by_4();
        if(right->lvalue)
        {
            std::cout << "\tmovl\t(%ebx), %ebx\n";
        }  
        std::cout << "\tmovl\t(%esp), %eax\n";
        inc_esp_by_4();
        if(left->lvalue)
        {
            std::cout << "\tmovl\t(%eax), %eax\n";
        }  
        std::cout << "\tcltd\n";
        std::cout << "\tidivl\t%ebx\n";
        std::cout << "\tpushl\t%eax\n";
        
    }
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
void assignE_astnode::gencode()
{
    auto left = this->children["left"];
    auto right = this->children["right"];
    
    right->gencode();
    left->gencode(); 
   
    int sz = left->dtype.get_size();
    std::cout << "\tmovl\t(%esp), %ebx\n";
    std::cout << "\taddl\t$4, %esp\n";

    std::cout << "\tmovl\t(%esp), %eax\n";
    std::cout << "\taddl\t$4, %esp\n"; 

    // eax holds rhs and ebx holds lhs
    for(int i = 0;i < sz; i+=4)
    {
        if(right->lvalue)
        {
            std::cout << "\tmovl\t" << i << "(%eax), %edx\n";
            std::cout << "\tmovl\t%edx, " << i << "(%ebx)\n";     
        }
        else 
        {
            std::cout << "\tmovl\t%eax, (%ebx)\n";
        }
        
    }
}

funcall_astnode::funcall_astnode () : exp_astnode(kfuncall_astnode) {}
funcall_astnode::funcall_astnode (exp_astnode* fname,std::vector<exp_astnode*> params) : exp_astnode(kfuncall_astnode)
{
    this->params = params;
    this->children["fname"] = fname;
    this->dtype = fname->dtype;
    if(this->dtype.ts.bdtype == kSTRUCT)
    {
        this->lvalue = true;
    }
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
void funcall_astnode::gencode()
{
    std::string fname = static_cast<identifier_astnode*>(this->children["fname"])->id;
    int sz = 0;
    //std::cout << fname <<"\n";
    int ret_size = this->dtype.get_size();
    std::cout << "\tsubl\t$" << ret_size << ", %esp\n"; // Making space for return value
    std::cout << "\tmovl\t%esp, %esi\n";
    
    for(int i = (int)this->params.size()-1; i >= 0; i--)
    {
        auto p = this->params[i];
        p->gencode();
        if(p->dtype.ts.bdtype == kSTRING)
        {
            std::cout << "\tpushl\t" << static_cast<stringconst_astnode*>(p)->value <<"\n";
            sz += 4;
        }
        else
        {
            if(p->lvalue)
            {
                
                if(p->dtype.get_tot_stars() == 0 && p->dtype.ts.bdtype == kSTRUCT)
                {
                    std::cout << "\tmovl\t(%esp), %eax\n"; 
                    inc_esp_by_4();
                    for(int i = (p->dtype.get_size() - 4);i >= 0;i-=4)
                    {
                        std::cout << "\tmovl\t" << i << "(%eax), %ebx\n";
                        std::cout << "\tpushl\t%ebx\n"; 
                        sz += 4;
                    }   
                }
                else if(p->dtype.arr.size() >= 1) 
                {
                    // Do nothing since the array pointer is already on top of the stack
                    sz += 4;
                }
                else 
                {
                    
                    std::cout << "\tmovl\t(%esp), %eax\n"; 
                    std::cout << "\tmovl\t(%eax), %eax\n"; 
                    std::cout << "\tmovl\t%eax, (%esp)\n";
                    sz += 4;
                }
            }
            else sz += 4;

        }
    }
    std::cout << "\tpushl\t%ecx\n"; // caller saved register
    std::cout << "\tmovl\t%esi, %ecx\n"; 
    std::cout << "\tcall\t" << fname <<"\n";
    std::cout << "\tmovl\t(%esp), %ecx\n";
    //std::cout << "\tmovl\t%ecx, %esp\n";
    std::cout << "\taddl\t$" << sz + 4 << ", %esp\n"; 
    
    if(this->dtype.ts.bdtype == kSTRUCT)
    {
        this->lvalue = true;
        std::cout << "\tmovl\t%esp, %eax\n";
        std::cout << "\tpushl\t%eax\n";
    }

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
void intconst_astnode::gencode()
{
    std::cout << "\tmovl\t$" << value << ", %eax\n";
    std::cout << "\tpushl\t%eax\n";
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
void seq_astnode::gencode()
{
    for(auto x:this->statements)
    {
        x->gencode();
    }
}


return_astnode::return_astnode (exp_astnode* return_exp,std::string f) : statement_astnode(kreturn_astnode), return_exp(return_exp), fname(f) {}
void return_astnode::print (int blanks)
{
    std::cout << get_str("return") + " : {\n";
    this->return_exp->print();
    std::cout << "}\n";
}
void return_astnode::gencode()
{
    return_exp->gencode();
    if(this->fname == "main")
    {
        std::cout << "\tmovl\t(%esp), %eax\n";
        return;
    }
    if(return_exp->lvalue)
    {
        std::cout << "\tmovl\t(%esp), %eax\n"; 
        for(int i = 0; i < return_exp->dtype.get_size(); i+=4)
        {
            std::cout << "\tmovl\t" << i << "(%eax), %ebx\n";
            std::cout << "\tmovl\t%ebx, " << i << "(%ecx)\n";
        }

    }
    else 
    {
        std::cout << "\tmovl\t(%esp), %eax\n"; 
        std::cout << "\tmovl\t%eax,(%ecx)\n";
    }
    std::cout << "\tjmp\t.L" << ret_idx <<"\n";

    
}

assignS_astnode::assignS_astnode (exp_astnode* ass_exp) : statement_astnode (kassignS_astnode)
{
    this->exp_children["left"] = ass_exp->children["left"];
    this->exp_children["right"] = ass_exp->children["right"];
}
void assignS_astnode::print (int blanks){
    std::cout << get_str("assignS") + " : {\n";
    std::cout << get_str("left") + ": {\n";
    this->exp_children["left"]->print();
    std::cout <<"},\n";
    std::cout << get_str("right") + ": {\n";
    this->exp_children["right"]->print();
    std::cout << "}\n}\n";
}
void assignS_astnode::gencode(){
    // Yet to take care of struct 

    auto left = this->exp_children["left"];
    auto right = this->exp_children["right"];
    right->gencode();
    left->gencode(); 
    int sz = left->dtype.get_size();
    std::cout << "\tmovl\t(%esp), %ebx\n";
    std::cout << "\taddl\t$4, %esp\n";
    std::cout << "\tmovl\t(%esp), %eax\n";
    std::cout << "\taddl\t$4, %esp\n"; 
    // eax holds rhs and ebx holds lhs
    //std::cout << "sz " << sz <<"\n";
    for(int i = 0;i < sz; i+=4)
    {
        if(right->lvalue)
        {
            std::cout << "\tmovl\t" << i << "(%eax), %edx\n";
            std::cout << "\tmovl\t%edx, " << i << "(%ebx)\n";     
        }
        else 
        {
            std::cout << "\tmovl\t%eax, (%ebx)\n";
        }
        
    }
    
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
void proccall_astnode::gencode()
{
    // push paramters, call fun and update esp 
    // Todo : struct params
    std::string fname = static_cast<identifier_astnode*>(this->exp_children["fname"])->id;
    int sz = 0;
    for(int i = (int)this->params.size()-1; i >= 0; i--)
    {
        auto p = this->params[i];
        p->gencode();
        //std::cout << "param : " << i << " "<<p->lvalue <<"\n";

        if(p->dtype.ts.bdtype == kSTRING)
        {
            std::cout << "\tpushl\t" << static_cast<stringconst_astnode*>(p)->value <<"\n";
            sz += 4;
        }
        else
        {
            if(p->lvalue)
            {
                
                if(p->dtype.get_tot_stars() == 0 && p->dtype.ts.bdtype == kSTRUCT)
                {
                    std::cout << "\tmovl\t(%esp), %eax\n"; 
                    inc_esp_by_4();
                    for(int i = (p->dtype.get_size() - 4);i >= 0;i-=4)
                    {
                        std::cout << "\tmovl\t" << i << "(%eax), %ebx\n";
                        std::cout << "\tpushl\t%ebx\n"; 
                        sz += 4;
                    }   
                }
                else if(p->dtype.arr.size() >= 1) 
                {
                    // Do nothing since the array pointer is already on top of the stack
                    sz += 4;
                }
                else 
                {

                    std::cout << "\tmovl\t(%esp), %eax\n"; 
                    std::cout << "\tmovl\t(%eax), %eax\n"; 
                    std::cout << "\tmovl\t%eax, (%esp)\n";
                    sz += 4;
                }
            }
            else sz += 4;

        }
    }
    if(fname == "printf")
    {
        std::cout << "\tmovl\t%ecx, %edi\n";
        std::cout << "\tcall\t" << fname <<"\n";
        std::cout << "\taddl\t$" << sz << ", %esp\n";
        std::cout << "\tmovl\t%edi, %ecx\n";
    }
    else 
    {
        std::cout << "\tpushl\t%ecx\n";
        std::cout << "\tcall\t" << fname <<"\n";
        std::cout << "\tmovl\t(%esp), %ecx\n";
        std::cout << "\taddl\t$" << sz + 4 << ", %esp\n";
    }
    

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
void if_astnode::gencode()
{
    exp_astnode* cond = this->exp_children["cond"];
    statement_astnode* ifblock = this->st_children["then"];
    statement_astnode* elseblock = this->st_children["else"];
    cond->gencode();
    std::cout << "\tmovl\t(%esp), %eax\n";

    if(cond->lvalue)
    {
        std::cout << "\tmovl\t(%eax), %eax\n";
    }
    
    std::cout << "\tcmpl\t$0, %eax\n"; 
    int else_idx = Lcount++,out_idx = Lcount++;
    std::cout << "\tje\t.L" << else_idx << "\n";
    ifblock->gencode();
    std::cout << "\tjmp\t.L" << out_idx << "\n";
    std::cout << ".L" << else_idx << ":\n";
    elseblock->gencode();
    std::cout << ".L" << out_idx << ":\n";
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
void while_astnode::gencode()
{
    exp_astnode* cond = this->exp_children["cond"];
    statement_astnode* stmt = this->st_children["stmt"];
    int out_idx = Lcount++, in_idx = Lcount++;
    std::cout << "\tjmp\t.L" <<  out_idx << "\n";
    std::cout << ".L" << in_idx << ":\n";
    stmt->gencode();
    std::cout << ".L" << out_idx <<":\n";
    cond->gencode();
    
    std::cout << "\tmovl\t(%esp), %eax\n";
    if(cond->lvalue)
    {
        std::cout << "\tmovl\t(%eax), %eax\n";
    }
    
    std::cout << "\tcmpl\t$0, %eax\n";
    std::cout << "\tjne\t.L" << in_idx << "\n";

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
void for_astnode::gencode()
{
    exp_astnode* init = this->exp_children["init"];
    exp_astnode* guard = this->exp_children["guard"];
    exp_astnode* step = this->exp_children["step"];
    statement_astnode* body = this->st_children["body"];
    init->gencode();
    int out_idx = Lcount++, in_idx = Lcount++;

    // body
    std::cout << "\tjmp\t.L" <<  out_idx << "\n";
    std::cout << ".L" << in_idx << ":\n";
    body->gencode();
    step->gencode();

    // out
    std::cout << ".L" << out_idx <<":\n";
    guard->gencode();
    std::cout << "\tmovl\t(%esp), %eax\n";
    if(guard->lvalue)
    {
        std::cout << "\tmovl\t(%eax), %eax\n";
    }
    std::cout << "\tcmpl\t$0, %eax\n";
    std::cout << "\tjne\t.L" << in_idx << "\n";
}








