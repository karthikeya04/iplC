#include <cstring>
#include <cstddef>
#include <istream>
#include <iostream>
#include <fstream>

#include "scanner.hh"
#include "parser.tab.hh"
//#include "parser.tab.cc"

extern symtab* globalst;
extern std::vector<std::string> ordered_funcs;
extern std::map<std::string,std::vector<std::pair<std::string,std::string>>> rodata;
extern std::map<std::string,statement_astnode*> astnodes; // (fname,ast)
extern std::string get_str(std::string);
extern int Lcount;
int ret_idx;

int main(const int argc, const char **argv)
{

  using namespace std;
  fstream in_file;

  in_file.open(argv[1], ios::in);
  // Generate a scanner
  IPL::Scanner scanner(in_file);
  // Generate a Parser, passing the scanner as an argument.
  // Remember %parse-param { Scanner  &scanner  }
  IPL::Parser parser(scanner);


  #ifdef YYDEBUG
   parser.set_debug_level(1);
  #endif 
  parser.parse();

// header
  cout << "\t.file\t" << "\"" << argv[1] << "\"\n";
  cout << "\t.text\n";

  for(string func:ordered_funcs)
  {
    // rodata
    if(rodata[func].size())
    {
      cout << "\t.section\t.rodata\n";
      for(auto p:rodata[func])
      {
        cout << "." << p.first << ":\n";
        cout << "\t.string\t"+p.second+"\n";
      }
    }
    cout << "\t.text\n";
    cout << "\t.globl\t" << func << "\n";
    cout << "\t.type\t" << func << ", @function" << "\n";
    cout << func <<":\n";
    cout << "\tpushl\t%ebp\n";
    cout << "\tmovl\t%esp, %ebp\n";
    int locals_sz = 0;
    auto st = globalst->entries[func]->localst;
    for(auto x : st->entries)
    {
      if(x.second->scope == local)
      {
        locals_sz += x.second->sz;
      } 
    }
    if(locals_sz)
    {
      cout << "\tsubl\t$" << locals_sz << ", %esp\n";
    }
    ret_idx = Lcount++;
    astnodes[func]->gencode();
    // if(func == "main")
    // {
    //   cout << "\tmovl\t$0, %eax\n";
    // }
    std::cout << ".L" << ret_idx << ":\n";
    cout << "\tleave\n\tret\n";
    
    cout << "\t.size\t" << func <<", .-" << func << "\n";

  }




// footer
  cout << "\t.ident\t\"GCC: (Ubuntu 8.1.0-9ubuntu1~16.04.york1) 8.1.0\"\n\t.section\t.note.GNU-stack,\"\",@progbits\n";



  // map<string,symtab_entry*> struct_entries,fun_entries;
  // for(auto it = globalst->entries.begin(); it != globalst->entries.end(); it++)
  // {
  //   auto name = it->first;
  //   auto entry = it->second;
  //   if(entry->entry_type == kstruct)
  //   {
  //     struct_entries[name] = entry;
  //   }
  //   else if(entry->entry_type == kfun)
  //   {
  //     fun_entries[name] = entry;
  //   }
  // }
  // cout << "{";
  // globalst->print();
  // cout << "\n,\n";
  // cout << "  \"structs\": [" << endl;

  // for(auto it = struct_entries.begin(); it != struct_entries.end(); it++)
  // {
  //   auto name = it->first;
  //   auto entry = it->second;
  //     cout << "{\n\"name\": \"" + name + "\",\n";
  //     entry->localst->print();
  //     cout << "}";
  //     if(next(it,1) != struct_entries.end())
  //     {
  //       cout <<",";
  //     }
  //     cout<<"\n";
  // }

  // cout << "],\n";

  // cout << "  \"functions\": [" << endl;

  // for(auto it = fun_entries.begin(); it != fun_entries.end(); it++)
  // {
  //   auto name = it->first;
  //   auto entry = it->second;
  //   cout << "{ \"name\": \"" + name + "\",\n";
  //   entry->localst->print();
  //   cout << "," << endl;
  //   cout << get_str("ast") + ": {";
  //   astnodes[name]->print();
  //   cout << "}\n";
  //   cout <<"}";
  //     if(next(it,1) != fun_entries.end())
  //     {
  //       cout <<"\n,";
  //     }
  //     cout<<"\n";

  // }
  // cout << "]\n";

  // cout <<"}\n";





}

