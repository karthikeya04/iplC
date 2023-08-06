#include "symtab.hh"


// class symtab_entry

symtab_entry::symtab_entry (std::string name,st_entry_type entry_type,st_scope scope,int sz,int offset,datatype type,symtab* localst) : name(name), entry_type(entry_type),scope(scope),sz(sz),offset(offset),type(type), localst(localst) {}    
symtab_entry::symtab_entry (std::string name,st_entry_type entry_type,st_scope scope) : name(name), entry_type(entry_type), scope(scope) {}
std::string symtab_entry::get_str(std::string s){
    return "\"" + s + "\"";
}

void symtab_entry::print(){
    std::vector<std::string> attrs;
    attrs.push_back(get_str(name));
    std::string et = "";
    switch(entry_type)
    {        case kfun:
            et = "fun";
            break;
        case kstruct:
            et = "struct";
            break;
        case kvar:
            et = "var";
            break;

    }
    attrs.push_back(get_str(et));
    std::string sc = "";
    switch(scope)
    {
        case global:
            sc = "global";
            break;
        case local:
            sc = "local";
            break;
        case param:
            sc = "param";
    }
    attrs.push_back(get_str(sc));
    attrs.push_back(std::to_string(sz));
    if(entry_type == kstruct) attrs.push_back(get_str("-"));
    else attrs.push_back(std::to_string(offset));
    if(entry_type == kstruct ) attrs.push_back(get_str("-"));
    else attrs.push_back(get_str(type.get_in_string_format()));    
    
    std::cout << "[ ";

    for(int i = 0; i < (int)attrs.size(); i++)
    {
        std::cout << attrs[i];
        if(i != (int)attrs.size() - 1){
            std::cout << ",";
        }
        std::cout <<" ";
    }

    std::cout << "\n]";

}

void symtab_entry::set_entry_type_(st_entry_type et)
{
    this->entry_type = et;
}

void symtab_entry::set_sz_(int sz)
{
    this->sz = sz;

}
void symtab_entry::set_offset_(int offset)
{
    this->offset = offset;
}
void symtab_entry::set_type_(datatype type)
{
    this->type = type;
}
void symtab_entry::set_scope_(st_scope scope)
{
    this->scope = scope;
}

// class symtab 
symtab::symtab (st_scope scope, symtab_type st_type) : scope(scope), st_type(st_type) {}

bool symtab::addEntry(symtab_entry* new_entry){
    std::string key = new_entry->name;
    if(this->entries[key]){
        return false;
    }
    this->entries[key] = new_entry;
    return true;
}

void symtab::print(){
    switch(scope)
    {
        case global:
            std::cout << "\"globalST\": \n";
            break;
        case local:
            std::cout << "\"localST\": \n";
            break;
        default:
            break;
    }
    std::cout<<"[";
    for(auto it = entries.begin(); it != entries.end(); it++){
        auto entry = it->second;
        entry->print();
        if(next(it,1) != entries.end()) 
        {
            std::cout << ",\n";
        }
        else{
            std::cout << "\n";
        }
    }
    std:: cout << "]";
}

void symtab::set_entry_type(std::string key,st_entry_type et)
{
    this->entries[key]->set_entry_type_(et);
}

void symtab::set_sz(std::string key,int sz)
{
    this->entries[key]->set_sz_(sz);
}

void symtab::set_offset(std::string key,int offset)
{
    this->entries[key]->set_offset_(offset);
}

void symtab::set_type(std::string key,datatype type)
{
    this->entries[key]->set_type_(type);
}
void symtab::set_scope(std::string key,st_scope scope)
{
    this->entries[key]->set_scope_(scope);
}

bool symtab::exists(std::string key)
{
    return (entries.count(key) ? true : false);
}