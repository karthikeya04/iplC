%skeleton "lalr1.cc"
%require  "3.0.1"

%defines 
%define api.namespace {IPL}
%define api.parser.class {Parser}

%define parse.trace

%code requires{
   #include "ast.hh"
   #include "location.hh"
   #include "symtab.hh"
   #include "type.hh"
   #include <vector>


   namespace IPL {
      class Scanner;
   }

  // # ifndef YY_NULLPTR
  // #  if defined __cplusplus && 201103L <= __cplusplus
  // #   define YY_NULLPTR nullptr
  // #  else
  // #   define YY_NULLPTR 0
  // #  endif
  // # endif

}

%printer { std::cerr << $$; } IDENTIFIER
%printer { std::cerr << $$; } WHILE
%printer { std::cerr << $$; } FOR
%printer { std::cerr << $$; } STRUCT
%printer { std::cerr << $$; } IF
%printer { std::cerr << $$; } ELSE
%printer { std::cerr << $$; } PTR_OP
%printer { std::cerr << $$; } INC_OP
%printer { std::cerr << $$; } LE_OP
%printer { std::cerr << $$; } GE_OP
%printer { std::cerr << $$; } NE_OP
%printer { std::cerr << $$; } EQ_OP
%printer { std::cerr << $$; } AND_OP
%printer { std::cerr << $$; } OR_OP
%printer { std::cerr << $$; } VOID
%printer { std::cerr << $$; } INT
%printer { std::cerr << $$; } FLOAT
%printer { std::cerr << $$; } FLOAT_CONSTANT
%printer { std::cerr << $$; } INT_CONSTANT
%printer { std::cerr << $$; } STRING_LITERAL
%printer { std::cerr << $$; } RETURN


%parse-param { Scanner  &scanner  }
%locations
%code{
   #include <iostream>
   #include <cstdlib>
   #include <fstream>
   #include <string>
   #include <set>
   
   
   #include "scanner.hh"

   std::map<std::string,statement_astnode*> astnodes; // (fname,ast)
   std::set<std::string> pred_procedures({"printf"});
   symtab* globalst = new symtab(global);
   symtab* localst = new symtab(local);
   std::vector<std::string> params_list;
   std::vector<std::string> struct_fields;
   ctype_specifier* ltype_spec = new ctype_specifier();
   datatype* ret_dtype;
   std::vector<cdecl*> decl_list;
   cdecl_inh* decl_inh;
   int local_offset = 0;

   std::map<std::string,std::vector<datatype>> map_params; // (fname,params_dtypes)
   std::vector<datatype> params_dtypes;

   std::vector<std::string> ordered_funcs;
   std::map<std::string,std::vector<std::pair<std::string,std::string>>> rodata;
   std::string curr_func;
   int lc_count = 0;
   int Lcount = 4;
   std::map<std::string,datatype> ret_type;

   std::pair<bool,std::string> cmp_operators_check(exp_astnode* L,exp_astnode* R)
   {
      datatype ldtype = L->dtype,rdtype = R->dtype;
      int lstars = ldtype.get_tot_stars(),rstars = rdtype.get_tot_stars();
      std::string ltype = ldtype.get_type_string(),rtype = rdtype.get_type_string();
      std::string err_msg = "Invalid operands for binary operator "+get_str("<")+", "+get_str(ltype)+" "+get_str(rtype);
      std::pair<bool,std::string> err = {false,err_msg}, ok = {true,""};                
      /*
         Relational operators may only be used on pointers to elements of the same array or struct

      */
      if(lstars)
      {
         if(!(R->astnode_type == kintconst_astnode && static_cast<intconst_astnode*>(R)->value == 0))
         {
            if(ltype != rtype)
               return err;
         }

      }
      else if(rstars)
      {
         if(!(L->astnode_type == kintconst_astnode && static_cast<intconst_astnode*>(L)->value == 0))
         {
            if(ltype != rtype)
               return err;

         }

      }
      else
      {
         if(!(rdtype.ts.bdtype == kINT || rdtype.ts.bdtype == kFLOAT || ldtype.ts.bdtype == kINT || ldtype.ts.bdtype == kFLOAT))
            return err;
      }
      return ok;
   }
   std::pair<bool,std::string> and_or_check(exp_astnode* exp)
   {
      int stars = exp->dtype.get_tot_stars();
      base_dtype bdtype = exp->dtype.ts.bdtype;
      std::string err_msg = "Invalid operand of "+get_str("&&")+", "+get_str(exp->dtype.get_type_string());
      std::pair<bool,std::string> err = {false,err_msg}, ok = {true,""};                
      if(stars == 0 && (bdtype == kVOID || bdtype == kSTRUCT || bdtype == kSTRING))
         return err;
      return ok;
      

   }
   std::pair<bool,std::string> assign_type_check(datatype ldt,exp_astnode* exp) // ldt = exp
   {
      // lvalue check has already been done
      datatype rdt = exp->dtype;
      std::pair<bool,std::string> err = {false,"Incompatible assignment when assigning to type "+ get_str(ldt.get_in_string_format()) + "from type "+get_str(rdt.get_in_string_format())}, ok =  {true,""};
      std::string ltype = ldt.get_in_string_format(), rtype = rdt.get_in_string_format();
      std::string lbtype = ldt.ts.get_in_string_format(), rbtype = rdt.ts.get_in_string_format();
      int lstars = ldt.get_tot_stars(), rstars = rdt.get_tot_stars();
      int larr_sz = ldt.arr.size(),rarr_sz = rdt.arr.size();
      
      if(larr_sz)
         return {false,"Assignment to expression with array type"};
      
      // now lhs's type can only be of kind : int***
      // handle void* (only case where *'s might not match)

      if(exp->astnode_type == kintconst_astnode && static_cast<intconst_astnode*>(exp)->value == 0 && lstars)
         return ok;

      if(ltype == "void*")
         return rstars ? ok : err;
      if(rtype == "void*")
         return lstars ? ok : err;

      /*
         type specifiers: void, int, float, struct <id>
      */
      if(lstars != rstars)
      {
         return err;
      }
      if(lstars)
      {
         if(lbtype != rbtype)
               return err;
         if(ldt.stars_top)
               return rarr_sz ? err : ok;
         else 
               return rarr_sz > 1 ? err : ok;
      }   
      else{
         if(ltype == rtype)
               return ok;
         
         if(ltype == "void" || rtype == "void")
               return err;
         if(ldt.ts.bdtype == kSTRUCT || rdt.ts.bdtype == kSTRUCT || ldt.ts.bdtype == kSTRING || rdt.ts.bdtype == kSTRING)
               return ltype == rtype ? ok : err;
         return ok;
      }
      
   }

   bool arg_type_check(datatype pdtype,exp_astnode* arg)
   {
      datatype adtype = arg->dtype;
      if(assign_type_check(pdtype,arg).first) 
         return true;
      // Now cases where pdtype is (*)[3] like are left
      return (pdtype.get_type_string() == adtype.get_type_string());
      
   }
   std::pair<exp_astnode*,std::string> funcall(std::string id,std::vector<exp_astnode*> params = std::vector<exp_astnode*>(0))
   {
      exp_astnode* id_node;
      int p_sz = params.size();
      if(pred_procedures.count(id))
      {
         datatype dtype = datatype();
         dtype.ts.bdtype = kVOID;
         id_node = new identifier_astnode(id,curr_func,dtype);
      }
      else
      {       
         if(!globalst->exists(id))
            return {NULL,"Function "+get_str(id)+" is not declared"};
         // Arguments semantic check
         auto exp_params = map_params[id];
         int exp_sz = exp_params.size();
         if(p_sz < exp_sz)
            return {NULL,"Function "+get_str(id)+" is called with too few arguments"};
         if(p_sz > exp_sz)
            return {NULL,"Function "+get_str(id)+" is called with too many arguments"};
         for(int i = 0; i < p_sz; i++)
         {
            if(!arg_type_check(exp_params[i],params[i]))
               return {NULL,"Incompatible type for argument "+std::to_string(i+1)+" of "+get_str(id)+" : expected "+get_str(exp_params[i].get_type_string())+" but argument is of type "+get_str(params[i]->dtype.get_type_string())};
            base_dtype ebd = exp_params[i].ts.bdtype,abd  = params[i]->dtype.ts.bdtype;
            if(ebd == kINT && abd == kFLOAT)
            {
               params[i] = new op_unary_astnode(kTO_INT,params[i]);
            }
            if(ebd == kFLOAT && abd == kINT)
            {
               params[i] = new op_unary_astnode(kTO_FLOAT,params[i]);
            }
         }

         datatype res_dtype = globalst->entries[id]->type;             
         id_node = new identifier_astnode(id,curr_func,res_dtype);
      }
      exp_astnode *res;
      if(p_sz == 0)
         res = new funcall_astnode(id_node);
      else
         res = new funcall_astnode(id_node,params);
      return {res,""};

   }

   std::pair<statement_astnode*,std::string> proccall(std::string id,std::vector<exp_astnode*> params = std::vector<exp_astnode*>(0))
   {
      exp_astnode* id_node;
      int p_sz = params.size();
      if(pred_procedures.count(id))
      {
         datatype dtype = datatype();
         dtype.ts.bdtype = kVOID;
         id_node = new identifier_astnode(id,curr_func,dtype);
      }
      else
      {       
         if(!globalst->exists(id))
            return {NULL,"Procedure "+get_str(id)+" is not declared"};

         // Arguments semantic check
         auto exp_params = map_params[id];
         int exp_sz = exp_params.size();
         if(p_sz < exp_sz)
            return {NULL,"Procedure "+get_str(id)+" is called with too few arguments"};
         if(p_sz > exp_sz)
            return {NULL,"Procedure "+get_str(id)+" is called with too many arguments"};
         for(int i = 0; i < p_sz; i++)
         {
            if(!arg_type_check(exp_params[i],params[i]))
               return {NULL,"Incompatible type for argument "+std::to_string(i+1)+" of "+get_str(id)+" : expected "+get_str(exp_params[i].get_type_string())+" but argument is of type "+get_str(params[i]->dtype.get_type_string())};
            base_dtype ebd = exp_params[i].ts.bdtype,abd  = params[i]->dtype.ts.bdtype;
            if(ebd == kINT && abd == kFLOAT)
            {
               params[i] = new op_unary_astnode(kTO_INT,params[i]);
            }
            if(ebd == kFLOAT && abd == kINT)
            {
               params[i] = new op_unary_astnode(kTO_FLOAT,params[i]);
            }
         }
         datatype res_dtype = globalst->entries[id]->type;             
         id_node = new identifier_astnode(id,curr_func,res_dtype);
      }
      statement_astnode *res;
      if(p_sz == 0)
         res = new proccall_astnode(id_node);
      else
         res = new proccall_astnode(id_node,params);
      return {res,""};
      
   }


#undef yylex
#define yylex IPL::Parser::scanner.yylex

}





%define api.value.type variant
%define parse.assert

//%start translation_unit
%start translation_unit



%token <std::string> WHILE FOR STRUCT IF ELSE INT VOID FLOAT RETURN
%token <std::string> FLOAT_CONSTANT
%token <std::string> INT_CONSTANT
%token <std::string> IDENTIFIER
%token <std::string> STRING_LITERAL
%token <std::string> GE_OP LE_OP EQ_OP AND_OP OR_OP NE_OP INC_OP PTR_OP
%token '>' '<' '=' '&' '!' '{' '}' '.' '[' ']' ';' ',' '(' ')' ':'
%left '+' '-'
%left '*' '/' 
%token OTHERS

%nterm <int> translation_unit struct_specifier function_definition parameter_list declarator_list declaration_list declaration

%nterm <statement_astnode*> statement selection_statement iteration_statement assignment_statement procedure_call statement_list compound_statement 
%nterm <std::vector<exp_astnode*>> expression_list
%nterm <operation_type_unary> unary_operator
%nterm <exp_astnode*> expression assignment_expression unary_expression logical_and_expression equality_expression relational_expression additive_expression multiplicative_expression postfix_expression primary_expression 
%nterm <ctype_specifier*> type_specifier
%nterm <std::string> fun_declarator
%nterm <cdecl*> declarator_arr declarator parameter_declaration

%%
     translation_unit: struct_specifier
                     | function_definition
                     | translation_unit struct_specifier
                     | translation_unit function_definition

      struct_specifier: STRUCT IDENTIFIER '{' 
                     {
                        localst = new symtab(local);
                        localst->st_type = st_kstruct;
                        struct_fields.clear();
                        if(!globalst->addEntry((symtab_entry*)new symtab_entry($1+" "+$2,kstruct,global,0,0))){
                           error(@2,get_str($1+" "+$2)+ " has a previous definition");   
                        }
                     }
                     declaration_list '}' ';'
                     {
                        // offset calculation
                        int offset_ = 0;
                        for(int i = 0; i < (int) struct_fields.size(); i++)
                        {
                           std::string key = struct_fields[i];
                           localst->set_offset(key,offset_);
                           offset_ += localst->entries[key]->sz;
                        }
                        ctype_specifier* struct_ts = new ctype_specifier(kSTRUCT,abs(offset_),$1 + " " + $2);
                        datatype *struct_dtype = new datatype(*struct_ts);
                        // size
                        symtab_entry* curr_entry = globalst->entries[$1+" "+$2];
                        curr_entry->sz = struct_ts->sz;
                        curr_entry->type = *struct_dtype;
                        curr_entry->localst = localst;

                     }

 
      function_definition: type_specifier fun_declarator  
                           {

                              datatype *fun_type = new datatype(*$1);
                              if(!globalst->addEntry((symtab_entry*) new symtab_entry($2,kfun,global,0,0,*fun_type,localst)))
                                 error(@2, "The function "+get_str($2)+" has a previous definition");
                              ordered_funcs.push_back($2);
                              curr_func = $2;
                              ret_dtype = new datatype(*$1);

                           } compound_statement 
                           {
                              astnodes[$2] = $4;
                           }

      type_specifier: VOID {$$ = new ctype_specifier(kVOID); ltype_spec = $$;}
                  | INT {$$ = new ctype_specifier(kINT,INT_SIZE); ltype_spec = $$;}
                  | FLOAT  {$$ = new ctype_specifier(kFLOAT,FLOAT_SIZE); ltype_spec = $$;}
                  | STRUCT IDENTIFIER 
                  {
                     std::string tname = $1 + " " + $2;
                     if(!globalst->exists(tname))
                        error(@2,get_str(tname) + " is not defined");
                     int st_sz = globalst->entries[tname]->sz;
                     $$ = new ctype_specifier(kSTRUCT,st_sz,$2);
                     ltype_spec = $$;
                  }

     fun_declarator: IDENTIFIER '(' 
                     {
                        localst = new symtab(local,st_kfun);
                        local_offset = 0;
                        params_list.clear();
                        params_dtypes.clear();

                     } parameter_list ')'
                     {
                        int offset_ = PARAMS_OFFSET;
                        map_params[$1] = params_dtypes;
                        // for(int i = (int)params_list.size() - 1; i >= 0; i--){
                        //    std::string key = params_list[i];
                        //    localst->set_offset(key,offset_);
                        //    offset_ += localst->entries[key]->sz;
                        // }
                        for(int i = 0; i <= (int)params_list.size() - 1; i++){
                           std::string key = params_list[i];
                           localst->set_offset(key,offset_);
                           offset_ += localst->entries[key]->sz;
                        }
                        $$ = $1;

                     }
                    | 
                    IDENTIFIER '(' ')'
                    {
                       localst = new symtab(local,st_kfun);
                       local_offset = 0;
                       params_list.clear();
                       params_dtypes.clear();
                       $$ = $1;
                    }

     parameter_list: parameter_declaration
                     {
                        params_list.push_back($1->id);
                        params_dtypes.push_back($1->dtype);
                     }
                    | parameter_list ',' parameter_declaration 
                    {
                       params_list.push_back($3->id);
                       params_dtypes.push_back($3->dtype);
                    }

     parameter_declaration: type_specifier 
                           declarator 
                           {
                              if($1->bdtype==kVOID && $2->dtype.stars == 0)
                                 error(@1,"Cannot declare the type of a parameter as "+get_str("void"));
                              $2->dtype.ts = *$1;
                              localst->set_type($2->id,$2->dtype);
                              localst->set_scope($2->id,param);
                              if($2->dtype.arr.size())
                              {
                                 localst->set_sz($2->id,4);
                              }
                              else
                              {
                                 localst->set_sz($2->id,$2->dtype.get_size());
                              }

                              $$ = $2;
                           }
                           

     declarator_arr: IDENTIFIER 
                     {
                        if(!localst->addEntry((symtab_entry*) new symtab_entry($1,kvar)))
                           error(@1,get_str($1)+ " has a previous declaration");
                        $$ = new cdecl($1);
                     }
                    | declarator_arr '[' INT_CONSTANT ']' 
                    {
                       $$ = $1;
                       $$->dtype.add_arr(stoi($3));
                    }

     declarator: 
               declarator_arr 
               {
                  $$ = $1;
               }
               | '*' declarator
               {
                  $$ = $2;
                  $$->dtype.inc_stars();
               }



     compound_statement: '{' '}' {$$ = new seq_astnode();}
                         | '{' statement_list '}' {$$ = $2;}
                         | '{' declaration_list '}' {$$ = new seq_astnode();}
                         | '{' declaration_list statement_list '}' {$$ = $3;}

     statement_list: statement 
                     {
                        $$ = new seq_astnode(); 
                        static_cast<seq_astnode*>($$)->push($1);
                     }
                    | statement_list statement
                    {
                       $$ = $1;
                       static_cast<seq_astnode*>($$)->push($2);
                    }

     statement: ';' {$$ = new empty_astnode();}
               | '{' statement_list '}' 
               {
                  $$ = $2;
               }
               | selection_statement { $$ = $1; }
               | iteration_statement { $$ = $1; }
               | assignment_statement { $$ = $1; }
               | procedure_call  { $$ = $1; }
               | RETURN expression ';' 
               {
                  // semantic checks
                  std::string err_msg = "Incompatible type "+get_str($2->dtype.get_type_string())+" returned, expected "+get_str(ret_dtype->get_type_string());
                  datatype ret = $2->dtype,exp = *ret_dtype;
                  if(ret.get_tot_stars())
                     error(@1,err_msg);
                  base_dtype rbd = ret.ts.bdtype,ebd = exp.ts.bdtype;
                  exp_astnode* ret_exp = $2;
                  if(rbd != ebd)
                  {
                     if(rbd == kINT && ebd == kFLOAT)
                     {
                        ret_exp = new op_unary_astnode(kTO_FLOAT,$2);

                     }
                     else if(rbd == kFLOAT && ebd == kINT)
                     {
                        ret_exp = new op_unary_astnode(kTO_INT,$2);

                     }
                     else
                     { 
                        error(@1,err_msg);
                     }

                  }
                  
                  $$ = new return_astnode(ret_exp,curr_func);
                  ret_type[curr_func] = *ret_dtype;

               }

     assignment_expression: unary_expression '=' expression 
                           { 
                              if(!($1->lvalue))
                                 error(@1,"Left operand of an assignment should have an lvalue");
                              auto check = assign_type_check($1->dtype,$3);
                              if(!(check.first)) 
                                 error(@1,check.second); 
                              
                              $$ = new assignE_astnode($1,$3); 
                           }

     assignment_statement: assignment_expression ';' 
                           {
                              $$ = new assignS_astnode($1);
                           }

     procedure_call: IDENTIFIER '(' ')' ';'
                  {

                     auto res = proccall($1);
                     if(res.first)
                        $$ = res.first;
                     else 
                        error(@1,res.second);

                  }
                  | IDENTIFIER '(' expression_list ')' ';'
                  {
                     
                     auto res = proccall($1,$3);
                     if(res.first)
                        $$ = res.first;
                     else 
                        error(@1,res.second);
                     if($1 == "printf")
                     {
                        for(auto x : $3)
                        {
                           if(x->astnode_type == kstringconst_astnode)
                           {
                              std::string lc_id = "LC"+std::to_string(lc_count);
                              lc_count++;
                              rodata[curr_func].push_back({lc_id,static_cast<stringconst_astnode*>(x)->value});
                              static_cast<stringconst_astnode*>(x)->value = "$."+lc_id;
                           }
                        }
                     }
                  }

     expression: logical_and_expression {$$ = $1;}
               | expression OR_OP logical_and_expression 
               {
                  auto check1 = and_or_check($1);
                  auto check2 = and_or_check($3);
                  if(!check1.first)
                     error(@1,check1.second);
                  if(!check2.first)
                     error(@3,check2.second);
                  $$ = new op_binary_astnode(kOR_OP,$1,$3,kINT);
               }

     logical_and_expression: equality_expression {$$ = $1;}
                           | logical_and_expression AND_OP equality_expression
                           {
                              auto check1 = and_or_check($1);
                              auto check2 = and_or_check($3);
                              if(!check1.first)
                                 error(@1,check1.second);
                              if(!check2.first)
                                 error(@3,check2.second);

                              $$ = new op_binary_astnode(kAND_OP,$1,$3,kINT);
                           }
     
     equality_expression: relational_expression {$$ = $1;}
                         | equality_expression EQ_OP relational_expression
                         {
                           auto check = cmp_operators_check($1,$3);
                           datatype ldtype = $1->dtype,rdtype = $3->dtype;
                           if(!check.first)
                           {
                              if(!((ldtype.get_tot_stars() > 0 && rdtype.get_type_string()=="void*")||(rdtype.get_tot_stars()>0&&ldtype.get_type_string()=="void*")))
                                 error(@2,check.second);
                           }
                           $$ = new op_binary_astnode(kEQ_OP,$1,$3);
                         }
                         | equality_expression NE_OP relational_expression
                         {
                           auto check = cmp_operators_check($1,$3);
                           datatype ldtype = $1->dtype,rdtype = $3->dtype;

                           if(!check.first)
                           {
                              if(!((ldtype.get_tot_stars() > 0 && rdtype.get_type_string()=="void*")||(rdtype.get_tot_stars()>0&&ldtype.get_type_string()=="void*")))
                                 error(@2,check.second);
                           }
                           $$ = new op_binary_astnode(kNE_OP,$1,$3);
                         }                   

     relational_expression: additive_expression {$$ = $1;}
                              | relational_expression '<' additive_expression
                              {
                                 auto check = cmp_operators_check($1,$3);
                                 if(!check.first)
                                    error(@2,check.second);
                                 $$ = new op_binary_astnode(kLT_OP,$1,$3,kINT); 
                              }
                              | relational_expression '>' additive_expression
                              {
                                 auto check = cmp_operators_check($1,$3);
                                 if(!check.first)
                                    error(@2,check.second);
                                 $$ = new op_binary_astnode(kGT_OP,$1,$3,kINT);
                                 
                              }
                              | relational_expression LE_OP additive_expression
                              {
                                 auto check = cmp_operators_check($1,$3);
                                 if(!check.first)
                                    error(@2,check.second);
                                 $$ = new op_binary_astnode(kLE_OP,$1,$3,kINT);
                              }
                              | relational_expression GE_OP additive_expression
                              {
                                 auto check = cmp_operators_check($1,$3);
                                 if(!check.first)
                                    error(@2,check.second);
                                 $$ = new op_binary_astnode(kGE_OP,$1,$3,kINT);
                              }

     additive_expression: multiplicative_expression {$$ = $1;}
                        | additive_expression '+' multiplicative_expression
                        {
                           /*
                           ptr(or an array),int
                           +ves              -ves
                           ptr + int        int-ptr    
                           int + ptr         ptr+ptr
                           ptr-ptr
                           Exception : pointers cannot be added, but can be subtracted
   
                           */
                           
                           datatype ldtype = $1->dtype,rdtype = $3->dtype;
                           int lstars = ldtype.get_tot_stars(),rstars = rdtype.get_tot_stars();
                           std::string ltype = ldtype.get_type_string(),rtype = rdtype.get_type_string();
                           std::string err_msg = "Invalid operands for binary operator "+get_str("+")+", "+get_str(ldtype.get_type_string())+" "+get_str(rdtype.get_type_string());
                           if(lstars)
                           {
                              if(!(rstars == 0 && rdtype.ts.bdtype == kINT))
                                 error(@2,err_msg);
                           }
                           else if(rstars)
                           {
                              if(!(lstars == 0 && ldtype.ts.bdtype == kINT))
                                 error(@2,err_msg);

                           }
                           else{
                              if(ltype == "void" || rtype == "void")
                                 error(@2,err_msg);
                              if(rdtype.ts.bdtype == kSTRUCT || ldtype.ts.bdtype == kSTRUCT || rdtype.ts.bdtype == kSTRING || ldtype.ts.bdtype == kSTRING)
                                 error(@2,err_msg);
                              
                           }
                           $$ = new op_binary_astnode(kPLUS,$1,$3);
                        }
                        | additive_expression '-' multiplicative_expression
                        {
                           /*
                              Pointers can be subtracted
                           */
                           
                           datatype ldtype = $1->dtype,rdtype = $3->dtype;
                           int lstars = ldtype.get_tot_stars(),rstars = rdtype.get_tot_stars();
                           std::string ltype = ldtype.get_type_string(),rtype = rdtype.get_type_string();
                           std::string err_msg = "Invalid operands for binary operator "+get_str("-")+", "+get_str(ldtype.get_type_string())+" "+get_str(rdtype.get_type_string());
                           if(lstars)
                           {
                              if(!((rstars == 0 && rdtype.ts.bdtype == kINT) || 
                              (ltype == rtype)))
                                 error(@2,err_msg);
                           }
                           else if(rstars)
                           {
                              
                              error(@2,err_msg);

                           }
                           else{
                              if(ltype == "void" || rtype == "void")
                                 error(@2,err_msg);
                              if(rdtype.ts.bdtype == kSTRUCT || ldtype.ts.bdtype == kSTRUCT || rdtype.ts.bdtype == kSTRING || ldtype.ts.bdtype == kSTRING)
                                 error(@2,err_msg);
                              
                           }
                           $$ = new op_binary_astnode(kMINUS,$1,$3);
                           if(lstars && ltype == rtype)   // pointer, pointer subtraction
                           {
                              $$->dtype = datatype();
                              $$->dtype.ts.bdtype = kINT;
                           }
                        }

     unary_expression: postfix_expression {$$ = $1;}
                        | unary_operator unary_expression 
                        {
                           std::string un_op = "";
                           datatype dtype = $2->dtype;

                           if($1 == kUMINUS)
                           {
                              un_op = "-";
                              if(!(
                                 (dtype.get_tot_stars() == 0 && (dtype.ts.bdtype == kINT || dtype.ts.bdtype == kFLOAT))
                              ))
                                 error(@2,"Invalid operand type for unary operator "+get_str(un_op)+", "+get_str(dtype.get_in_string_format()));
                           }
                           else if($1 == kNOT)
                           {
                              un_op = "!";
                              if(dtype.get_tot_stars() == 0 && (dtype.ts.bdtype == kSTRUCT || dtype.ts.bdtype == kVOID))    // anything is ok except a struct/void type
                                 error(@2,"Invalid operand type for unary operator "+get_str(un_op)+", "+get_str(dtype.get_in_string_format()));
                           }
                           else if($1 == kADDRESS)
                           {
                              un_op = "&";
                              if(!$2->lvalue)
                                 error(@2,"Operand of unary operator "+get_str(un_op)+" should have lvalue");
                           }
                           else if($1 == kDEREF)
                           {
                              un_op = "*";
                              if(dtype.get_tot_stars() == 0 || (dtype.get_tot_stars() == 1 && dtype.ts.bdtype == kVOID)) 
                                 error(@2,"Invalid operand type for unary operator "+get_str(un_op)+", "+get_str(dtype.get_in_string_format()));
                           }
                           $$ = new op_unary_astnode($1,$2);
                        }

     multiplicative_expression: unary_expression {$$ = $1;}
                              | multiplicative_expression '*' unary_expression
                              {
                                 if(!(
                                    ($1->dtype.get_tot_stars() == 0 && ($1->dtype.ts.bdtype == kINT || $1->dtype.ts.bdtype == kFLOAT)) &&
                                 ($3->dtype.get_tot_stars() == 0 && ($3->dtype.ts.bdtype == kINT || $3->dtype.ts.bdtype == kFLOAT))
                                 ))
                                    error(@2,"Invalid operand types for binary operator "+get_str("*")+", "+get_str($1->dtype.get_type_string())+" and "+get_str($3->dtype.get_type_string()));
                                 $$ = new op_binary_astnode(kMULT,$1,$3);
                              }

                              | multiplicative_expression '/' unary_expression
                              {
                                 if(!(
                                    ($1->dtype.get_tot_stars() == 0 && ($1->dtype.ts.bdtype == kINT || $1->dtype.ts.bdtype == kFLOAT)) &&
                                    ($3->dtype.get_tot_stars() == 0 && ($3->dtype.ts.bdtype == kINT || $3->dtype.ts.bdtype == kFLOAT))
                                 ))
                                    error(@2,"Invalid operand types for binary operator "+get_str("/")+", "+get_str($1->dtype.get_type_string())+" and "+get_str($3->dtype.get_type_string()));
                                 $$ = new op_binary_astnode(kDIV,$1,$3);
                              }

     postfix_expression: primary_expression { $$ = $1; }
                         | postfix_expression '[' expression ']'
                         {
                           if($1->dtype.get_tot_stars() == 0)
                              error(@1,"Subscriped value is neither an array nor a pointer");
                           if($1->dtype.get_type_string()=="void*")
                              error(@1,"Cannot dereference void*");
                           if(!($3->dtype.ts.bdtype == kINT && $3->dtype.get_tot_stars()==0))
                              error(@3,"Array subscript is not an integer");
         
                            $$ = new arrayref_astnode($1,$3);                           
                         }
                         | IDENTIFIER '(' ')'
                         {
                           auto res = funcall($1);
                           if(res.first)
                              $$ = res.first;
                           else
                              error(@1,res.second);
                         }
                         | IDENTIFIER '(' expression_list ')'
                         {
                           auto res = funcall($1,$3);
                           if(res.first)
                              $$ = res.first;
                           else 
                              error(@1,res.second);

                         }
                         | postfix_expression '.' IDENTIFIER
                         {
                            // check if $1's datatype is a struct
                            if(!($1->dtype.ts.bdtype == kSTRUCT && $1->dtype.get_tot_stars() == 0))
                              error(@1,"Left operand of "+get_str(".")+" is not a structure");                        

                            std::string id = $3;
                            symtab* struct_st =  globalst->entries[$1->dtype.get_in_string_format()]->localst;
                            if(!struct_st->exists($3))
                              error(@3,"Structure "+get_str($1->dtype.ts.get_in_string_format())+" has no member named "+get_str($3));
                            datatype res_dtype = struct_st->entries[id]->type;
                            exp_astnode* id_node = new identifier_astnode(id,curr_func,res_dtype);
                            $$ = new member_astnode($1,id_node);
                         }
                         | postfix_expression PTR_OP IDENTIFIER
                         {
                            if(!($1->dtype.ts.bdtype == kSTRUCT && $1->dtype.get_tot_stars() == 1))
                              error(@1,"Left operand of "+get_str("->")+" is not a pointer to a structure");
                         
                            symtab* struct_st = globalst->entries[$1->dtype.ts.get_in_string_format()]->localst;
                            std::string id = $3;
                            if(!struct_st->exists(id))
                              error(@1,"Structure "+get_str($1->dtype.ts.get_in_string_format())+" has no member named "+get_str($3));
                            datatype res_dtype = struct_st->entries[id]->type;       
                            exp_astnode* id_node = new identifier_astnode(id,curr_func,res_dtype);
                            $$ = new arrow_astnode($1,id_node);

                         }
                         | postfix_expression INC_OP 
                         {
                            if(!($1->lvalue))
                              error(@1,"Operand of "+get_str("++")+" should have lvalue");
                            base_dtype bdtype = $1->dtype.ts.bdtype;
                            if(!(
                                 ($1->dtype.arr.size() == 0 && $1->dtype.get_tot_stars() >= 1) || // pointers
                                 ($1->dtype.arr.size() == 0 && (bdtype == kINT || bdtype == kFLOAT)) // int, float
                               ))
                               error(@1,"Operand of "+get_str("++")+" should be int/float/pointer");
                            $$ = new op_unary_astnode(kPP,$1);
                         }

     primary_expression: IDENTIFIER 
                        {
                           std::string id = $1;
                           if(!localst->exists(id))
                              error(@1,"Variable "+get_str(id)+" is not declared");
                           datatype res_dtype = localst->entries[id]->type;
                           $$ = new identifier_astnode(id,curr_func,res_dtype);
                        }
                         | INT_CONSTANT
                         {
                            $$ = new intconst_astnode(std::stoi($1));

                         }
                         | FLOAT_CONSTANT
                         {
                            $$ = new floatconst_astnode(std::stof($1));

                         }
                         | STRING_LITERAL
                         {
                           $$ = new stringconst_astnode($1);
                         }
                         | '(' expression ')' 
                         {
                           $$ = $2;
                         }
                         

     expression_list: expression 
                     {
                        $$.push_back($1);
                     }
                    | expression_list ',' expression
                    {
                       $$ = $1;
                       $$.push_back($3);
                    }

     unary_operator: '-'   { $$ = kUMINUS; }
                    | '!'  { $$ = kNOT; }
                    | '&'  { $$ = kADDRESS; }
                    | '*'  { $$ = kDEREF; }

     selection_statement: IF '(' expression ')' statement ELSE statement 
                        {
                           datatype dtype = $3->dtype;
                           if(!(dtype.get_tot_stars()>0||dtype.ts.bdtype==kINT||dtype.ts.bdtype==kFLOAT))
                              error(@3,"conditional expression can only be int/float/pointer");
                           $$ = new if_astnode($3,$5,$7);
                        }

     iteration_statement: WHILE '(' expression ')' statement
                        {
                            datatype dtype = $3->dtype;
                           if(!(dtype.get_tot_stars()>0||dtype.ts.bdtype==kINT||dtype.ts.bdtype==kFLOAT))
                              error(@3,"conditional expression can only be int/float/pointer");
                           $$ = new while_astnode($3,$5);
                        }
                         | FOR '(' assignment_expression ';' expression ';' assignment_expression ')' statement
                         {
                             datatype dtype = $5->dtype;
                           if(!(dtype.get_tot_stars()>0||dtype.ts.bdtype==kINT||dtype.ts.bdtype==kFLOAT))
                              error(@5,"conditional expression can only be int/float/pointer");
                            $$ = new for_astnode($3,$5,$7,$9);
                         }

     declaration_list: declaration 
                     | declaration_list declaration 

     declaration: type_specifier declarator_list ';'
                  {

                     for(auto decl: decl_list){
                        if($1->bdtype==kVOID && decl->dtype.stars == 0)
                           error(@1,"Cannot declare variable of type "+get_str("void"));
                        decl->dtype.ts = *$1;
                        localst->set_type(decl->id,decl->dtype);
                        localst->set_sz(decl->id,decl->dtype.get_size());
                        if(localst->st_type == st_kfun)
                        {
                           local_offset -= decl->dtype.get_size();
                           localst->set_offset(decl->id,local_offset);
                        }
                        else{ //kstruct
                           struct_fields.push_back(decl->id);
                        }
                     }
                     decl_list.clear();
                  }

     declarator_list:
                     declarator 
                     {
                        decl_list.push_back($1);
                     }

                     | 

                     declarator_list ',' 
                     declarator
                     {
                        decl_list.push_back($3);
                     }
%%
void IPL::Parser::error( const location_type &l, const std::string &err_message )
{
   std::cout << "Error at line " << l.begin.line << ": " << err_message << "\n";
   exit(1);
}


