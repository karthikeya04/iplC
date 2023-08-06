#ifndef SYMTAB_H
#define SYMTAB_H


#include <vector>
#include <string>
#include <map>
#include "type.hh"
#include <iostream>
#include <string>

#define PARAMS_OFFSET 12




class symtab;

class symtab_entry {
    public: 
    std::string name ;
    st_entry_type entry_type;
    st_scope scope;
    int sz;
    int offset;
    datatype type;
    symtab* localst; // Not null where entry_type is fun/struct
    symtab_entry () {}
    symtab_entry (std::string,st_entry_type,st_scope,int,int,datatype = datatype(),symtab* = NULL);
    symtab_entry (std::string,st_entry_type,st_scope = local);
    std::string get_str(std::string);
    void print();
    void set_entry_type_(st_entry_type);
    void set_sz_(int);
    void set_offset_(int);
    void set_type_(datatype);
    void set_scope_(st_scope);

};

class symtab {
    public:
    st_scope scope;
    symtab_type st_type; 
    std::map< std::string, symtab_entry* > entries;
    symtab () {}
    symtab (st_scope,symtab_type = st_kglobal);
    bool addEntry(symtab_entry*);
    void print();
    void set_entry_type(std::string,st_entry_type);
    void set_sz(std::string,int);
    void set_offset(std::string,int);
    void set_type(std::string,datatype);
    void set_scope(std::string,st_scope);
    bool exists(std::string);

};




#endif