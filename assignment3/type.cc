#include "type.hh"

// ctype_specifier 
ctype_specifier::ctype_specifier() {sname = ""; bdtype = kANY;}
ctype_specifier::ctype_specifier (base_dtype bdtype,int sz,std::string Sname)
{
    this->bdtype = bdtype;
    this->sname = Sname;
    this->sz = sz;
}
int ctype_specifier::get_size()
{
    if(bdtype != kSTRUCT) return 4;
    return sz;
}
bool ctype_specifier::operator==(ctype_specifier ts)
{
    return (this->bdtype == ts.bdtype) && (this->sname == ts.sname);
}
std::string ctype_specifier::get_in_string_format()
{
    std::string res = "";
    switch (bdtype)
    {
    case kINT:
        res = "int";
        break;
    case kFLOAT:
        res = "float";
        break;
    case kVOID:
        res = "void";
        break;
    case kSTRUCT:
        res = "struct ";    
    default:
        break;
    }
    if(bdtype == kSTRUCT) res += sname;
    return res;
}
// datatype
datatype::datatype (ctype_specifier ts) : ts(ts) {}
std::string datatype::get_in_string_format()
{
    std::string dtype = "";
    switch(ts.bdtype)
    {
        case kINT:
            dtype = "int";
            break;
        case kFLOAT:
            dtype = "float";
            break;
        case kSTRUCT:
            dtype = "struct " + ts.sname;
            break;
        case kVOID:
            dtype = "void";
            break;
        case kSTRING:
            dtype = "string";
        case kANY:
            break;
    }
    int temp = stars;
    while(temp--){
        dtype+="*";
    }
    if(stars_top)
    {
        temp = stars_top;
        dtype+="(";
        while(temp--){
            dtype+="*";
        }
        dtype+=")";
    }
    for(int x:arr){
        dtype+="[";
        dtype+=std::to_string(x);
        dtype+="]";
    }
    return dtype;
}
void datatype::inc_stars(int x)
{
    stars+=x;
}
int datatype::get_size() 
{
    sz = stars ? PTR_SIZE : ts.get_size();
    for(int x:arr)
    {
        sz *= x;
    }
    return sz;
}
void datatype::add_arr(int x)
{
    arr.push_back(x);
}
int datatype::get_tot_stars()
{
    return (int)(arr.size()) + stars + stars_top;
}
void datatype::deref()
{
    if(stars_top)
    {
        stars_top--;
    }
    else if((int)arr.size())
    {
        std::vector<int> new_arr;
        for(int i = 1; i < (int) arr.size(); i++)
        {
            new_arr.push_back(arr[i]);
        }
        arr = new_arr;
    }
    else
    {
        stars--;
    }
}
void datatype::inc_stars_top(int x)
{
    stars_top += x;
}
bool datatype::operator==(datatype dt)
{
    return (this->arr == dt.arr) && 
            (this->stars == dt.stars) && 
            (this->stars_top == dt.stars_top) && 
            (this->ts == dt.ts);
}
std::string datatype::get_type_string()
{
    std::string res = ts.get_in_string_format();
    int temp = stars;
    while(temp--)
        res += "*";
    int start_idx = 0;
    if(stars_top)
    {
        if(arr.size()) res+="(*)";
        else res+="*";
    }
    else{
        if(arr.size())
        {
            if(arr.size() > 1) res += "(*)";
            else res += "*";
            start_idx = 1;
        }
    }
    for(int i = start_idx;i < (int) arr.size(); i++)
    {
        res += "["+std::to_string(arr[i])+"]";
    }
    return res;
}


// cdecl
cdecl::cdecl (std::string id,datatype dtype) : id(id), dtype(dtype) {}
cdecl::cdecl (std::string id) : id(id) {}
cdecl::cdecl (cdecl &dec){
    this->id = dec.id;
    this->dtype = dec.dtype;
}

//cdecl_inh
cdecl_inh::cdecl_inh(ctype_specifier ts,st_scope scope): ts(ts),scope(scope) {}




