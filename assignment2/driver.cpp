#include <cstring>
#include <cstddef>
#include <istream>
#include <iostream>
#include <fstream>

#include "scanner.hh"
#include "parser.tab.hh"
//#include "parser.tab.cc"

extern symtab* globalst;
extern std::map<std::string,statement_astnode*> astnodes; // (fname,ast)
extern std::string get_str(std::string);

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

  map<string,symtab_entry*> struct_entries,fun_entries;
  for(auto it = globalst->entries.begin(); it != globalst->entries.end(); it++)
  {
    auto name = it->first;
    auto entry = it->second;
    if(entry->entry_type == kstruct)
    {
      struct_entries[name] = entry;
    }
    else if(entry->entry_type == kfun)
    {
      fun_entries[name] = entry;
    }
  }
  cout << "{";
  globalst->print();
  cout << "\n,\n";
  cout << "  \"structs\": [" << endl;

  for(auto it = struct_entries.begin(); it != struct_entries.end(); it++)
  {
    auto name = it->first;
    auto entry = it->second;
      cout << "{\n\"name\": \"" + name + "\",\n";
      entry->localst->print();
      cout << "}";
      if(next(it,1) != struct_entries.end())
      {
        cout <<",";
      }
      cout<<"\n";
  }

  cout << "],\n";

  cout << "  \"functions\": [" << endl;

  for(auto it = fun_entries.begin(); it != fun_entries.end(); it++)
  {
    auto name = it->first;
    auto entry = it->second;
    cout << "{ \"name\": \"" + name + "\",\n";
    entry->localst->print();
    cout << "," << endl;
    cout << get_str("ast") + ": {";
    astnodes[name]->print();
    cout << "}\n";
    cout <<"}";
      if(next(it,1) != fun_entries.end())
      {
        cout <<"\n,";
      }
      cout<<"\n";

  }
  cout << "]\n";

  cout <<"}\n";





}

