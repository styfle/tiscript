/* vector.c - 'Array' handler */
/*
        Copyright (c) 2001-2004 Terra Informatica Software, Inc.
        and Andrew Fedoniouk andrew@terrainformatica.com
        All rights reserved
*/

#include <string.h>
#include "cs.h"

namespace tis
{

/* method handlers */
static value CSF_ctor(VM *c);
static value CSF_clone(VM *c);
static value CSF_push(VM *c);
static value CSF_pushFront(VM *c);
static value CSF_pop(VM *c);
static value CSF_popFront(VM *c);
static value CSF_concat(VM *c);
static value CSF_join(VM *c);
static value CSF_reverse(VM *c);
static value CSF_slice(VM *c);
static value CSF_splice(VM *c);
static value CSF_sort(VM *c);
static value CSF_indexOf(VM *c);
static value CSF_lastIndexOf(VM *c);
static value CSF_remove(VM *c);
static value CSF_removeByValue(VM *c);
static value CSF_map(VM *c);
static value CSF_reduce(VM *c);
static value CSF_reduceRight(VM *c);
static value CSF_filter(VM *c);

#define FETCH(c,obj) if( _CsIsPersistent(obj) ) obj = CsFetchVectorData(c, obj);
#define FETCH_P(c,obj,p1) if( _CsIsPersistent(obj) ) { CsPush(c,p1); obj = CsFetchVectorData(c, obj); p1 = CsPop(c); }
#define FETCH_PP(c,obj,p1,p2) if( _CsIsPersistent(obj) ) { CsPush(c,p2); CsPush(c,p1); obj = CsFetchVectorData(c, obj); p1 = CsPop(c); p2 = CsPop(c); }

/* virtual property methods */
static value CSF_length(VM *c,value obj);
static void CSF_set_length(VM *c,value obj,value value);

static value CSF_first(VM *c,value obj);
static void CSF_set_first(VM *c,value obj,value value);
static value CSF_last(VM *c,value obj);
static void CSF_set_last(VM *c,value obj,value value);

/* Vector methods */
static c_method methods[] = {
C_METHOD_ENTRY( "this",      CSF_ctor      ),
C_METHOD_ENTRY( "toLocaleString",   CSF_std_toLocaleString  ),
C_METHOD_ENTRY( "toString",         CSF_join            ),
C_METHOD_ENTRY( "valueOf",          CSF_join            ),
C_METHOD_ENTRY( "clone",            CSF_clone           ),
C_METHOD_ENTRY( "push",             CSF_push            ),
C_METHOD_ENTRY( "unshift",          CSF_pushFront       ),
C_METHOD_ENTRY( "pop",              CSF_pop             ),
C_METHOD_ENTRY( "shift",            CSF_popFront        ),
C_METHOD_ENTRY( "concat",           CSF_concat          ),
C_METHOD_ENTRY( "join",             CSF_join            ),
C_METHOD_ENTRY( "reverse",          CSF_reverse         ),
C_METHOD_ENTRY( "slice",            CSF_slice           ),
C_METHOD_ENTRY( "splice",           CSF_splice          ),
C_METHOD_ENTRY( "sort",             CSF_sort            ),
C_METHOD_ENTRY( "indexOf",          CSF_indexOf         ),
C_METHOD_ENTRY( "lastIndexOf",      CSF_lastIndexOf     ),
C_METHOD_ENTRY( "remove",           CSF_remove          ),
C_METHOD_ENTRY( "removeByValue",    CSF_removeByValue   ),
C_METHOD_ENTRY( "map",              CSF_map             ),
C_METHOD_ENTRY( "reduce",           CSF_reduce          ),
C_METHOD_ENTRY( "reduceRight",      CSF_reduceRight     ),
C_METHOD_ENTRY( "filter",           CSF_filter          ),


C_METHOD_ENTRY( 0,                  0                   )
};

/* Vector properties */
static vp_method properties[] = {
VP_METHOD_ENTRY( "length",         CSF_length,          CSF_set_length  ),
VP_METHOD_ENTRY( "first",          CSF_first,           CSF_set_first       ),
VP_METHOD_ENTRY( "last",           CSF_last,            CSF_set_last        ),
VP_METHOD_ENTRY( 0,                0,         0         )
};

/* prototypes */


/* CsInitVector - initialize the 'Vector' obj */
void CsInitVector(VM *c)
{
    c->vectorObject = CsEnterType(CsGlobalScope(c),"Array",&CsVectorDispatch);
    CsEnterMethods(c,c->vectorObject,methods);
    CsEnterVPMethods(c,c->vectorObject,properties);
}

/* CSF_ctor - built-in method 'initialize' */
static value CSF_ctor(VM *c)
{
    long size = 0;
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsVectorDispatch);
    int n = c->argc - 3 + 1;
    if( n )
    {
      obj = CsResizeVector(c,obj,n);
      if (CsMovedVectorP(obj))
        obj = CsVectorForwardingAddr(obj);
      for( int i = 3; i <= c->argc; ++i )
        CsSetVectorElementI(obj,i-3,CsGetArg(c,i));
    }
    CsCtorRes(c) = obj;
    return obj;
}

/* CSF_Clone - built-in method 'Clone' */
static value CSF_clone(VM *c)
{
    value obj;
    CsParseArguments(c,"V=*",&obj,&CsVectorDispatch);
    FETCH(c, obj);
    return CsCloneVector(c,obj);
}

/* CSF_Push - built-in method 'Push' */
static value CSF_push(VM *c)
{
    value obj;
    int_t size;
    int_t argc = CsArgCnt(c);
    if( argc < 3)  
      CsTooFewArguments(c);
    CsCheckType(c,1,CsVectorP);
    obj = CsGetArg(c,1);
    FETCH(c, obj);
    CsSetModified(obj,true);
    size = CsVectorSize(c,obj);
    int new_size = size + argc - 2;
    obj = CsResizeVector(c,obj,new_size);
    if (CsMovedVectorP(obj))
        obj = CsVectorForwardingAddr(obj);
    value *parr = CsVectorAddressI(obj);
    value v = UNDEFINED_VALUE;
    for( int n = 3; n <= argc; ++n)
      parr[size + n - 3] = (v = CsGetArg(c,n));
    return v;
}

value ThisVector(VM *c)
{
    CsCheckType(c,1,CsVectorP);
    return CsGetArg(c,1);
}


/* CSF_PushFront - built-in method 'pushFront' */
static value CSF_pushFront(VM *c)
{
    value obj;
    int_t size;
    int_t argc = CsArgCnt(c);
    if( argc < 3)  
      CsTooFewArguments(c);
    CsCheckType(c,1,CsVectorP);
    obj = CsGetArg(c,1);
    FETCH(c, obj);
    CsSetModified(obj,true);
    size = CsVectorSize(c,obj);
    int new_size = size + argc - 2;
    obj = CsResizeVector(c,obj,new_size);
    if (CsMovedVectorP(obj))
        obj = CsVectorForwardingAddr(obj);
    value *parr = CsVectorAddressI(obj);
    value *dst = parr + new_size;
    value *src = parr + size;
    while( --size >= 0)
      *--dst = *--src;
    value v = UNDEFINED_VALUE;
    for( int n = 3; n <= argc; ++n)
      parr[n - 3] = (v = CsGetArg(c,n));
    return v;
}


/* CSF_Pop - built-in method 'Pop' */
static value CSF_pop(VM *c)
{
    value vector = ThisVector(c), val;
    FETCH(c, vector);
    CsSetModified(vector,true);

    if (CsMovedVectorP(vector))
        vector = CsVectorForwardingAddr(vector);

    int_t size;
    size = CsVectorSize(c,vector);
    //if (size <= 0)
    //    CsThrowKnownError(c,CsErrStackEmpty,obj);
    if (size <= 0)
      return NOTHING_VALUE;
    val = CsVectorElement(c,vector,--size);
    CsSetVectorSize(vector,size);
    return val;
}

/* CSF_PopFront - built-in method 'PopFront' */
static value CSF_popFront(VM *c)
{
    value vector = ThisVector(c),val,*p;
    FETCH(c, vector);
    CsSetModified(vector,true);

    if (CsMovedVectorP(vector))
        vector = CsVectorForwardingAddr(vector);

    int_t size;
    size = CsVectorSize(c,vector);
    if (size <= 0)
      return NOTHING_VALUE;
    val = CsVectorElement(c,vector,0);
    CsSetVectorSize(vector,--size);
    for (p = CsVectorAddress(c,vector); --size >= 0; ++p)
        *p = p[1];
    return val;
}

static value CSF_remove(VM *c)
{
    value vector, val, *p;
    int_t pos;
    CsParseArguments(c,"V=*i",&vector,&CsVectorDispatch,&pos);
    FETCH(c, vector);
    CsSetModified(vector,true);

    if (CsMovedVectorP(vector))
        vector = CsVectorForwardingAddr(vector);

    int_t size = CsVectorSize(c,vector);

    if( pos < 0 || pos >= size)
      return NOTHING_VALUE;

    val = CsVectorElement(c,vector,pos);
    CsSetVectorSize(vector,--size);
    for (p = CsVectorAddress(c,vector) + pos; --size >= pos; ++p)
        *p = p[1];
    return val;
}

static bool RemoveItem(VM *c, value vector, value idx)
{
    if(!CsIntegerP(idx))
      CsThrowKnownError(c, CsErrUnexpectedTypeError, idx, "only integer as an index");
    int_t pos = CsIntegerValue(idx);
    FETCH(c, vector);
    CsSetModified(vector,true);
    if (CsMovedVectorP(vector))
        vector = CsVectorForwardingAddr(vector);
    int_t size = CsVectorSize(c,vector);
    if( pos < 0 || pos >= size)
      return false;
    CsSetVectorSize(vector,--size);
    for (value* p = CsVectorAddress(c,vector) + pos; --size >= pos; ++p)
        p[0] = p[1];
    return true;
}

static value CSF_removeByValue(VM *c)
{
    value vector = 0, element = 0;
    protect2 _(c,vector, element);

    CsParseArguments(c,"V=*V",&vector,&CsVectorDispatch,&element);
    FETCH(c, vector);
      CsSetModified(vector,true);

    if (CsMovedVectorP(vector))
        vector = CsVectorForwardingAddr(vector);

    int_t size = CsVectorSize(c,vector);
    int n = 0;
    for( ; n < size; ++n )
    {
      if( CsEqualOp(c, CsVectorElement(c,vector,n), element ) ) 
      {
        element = CsVectorElement(c,vector,n);
        break;
      }
    }
    if( n >= size)
      return NOTHING_VALUE;
    CsSetVectorSize(vector,--size);
    value *p = CsVectorAddress(c,vector);
    for (p += n; --size >= n; ++p)
        p[0] = p[1];
    return element;
}


/* CSF_length - built-in property 'length' */
static value CSF_length(VM *c,value obj)
{
    FETCH(c, obj);
    return CsMakeInteger(CsVectorSize(c,obj));
}

/* CSF_set_length - built-in property 'size' */
static void CSF_set_length(VM *c,value obj,value value)
{
    FETCH(c, obj);
    if (!CsIntegerP(value))
        CsTypeError(c,value);
    CsSetModified(obj,true);
    CsResizeVector(c,obj,CsIntegerValue(value));
}

/* CSF_first - built-in property 'first' */
static value CSF_first(VM *c,value obj)
{
    FETCH(c, obj);
    if( CsVectorSize(c,obj) )
      return CsVectorElement(c,obj,0);
    return NOTHING_VALUE;
}

/* CSF_set_first - built-in property 'size' */
static void CSF_set_first(VM *c,value obj,value val)
{
    protect1 _(c,val);
    FETCH(c,obj);
    CsSetModified(obj,true);
    if( CsVectorSize(c,obj) == 0 )
      obj = CsResizeVector(c,obj,1);
    CsSetVectorElement(c, obj,0,val);
}

/* CSF_first - built-in property 'size' */
static value CSF_last(VM *c,value obj)
{
    FETCH(c, obj);
    if( CsVectorSize(c,obj) )
      return CsVectorElement(c,obj,CsVectorSize(c,obj) - 1);
    return NOTHING_VALUE;
}

/* CSF_set_first - built-in property 'size' */
static void CSF_set_last(VM *c,value obj, value val)
{
    protect1 _(c,val);
    FETCH(c, obj);
    CsSetModified(obj,true);
    if( CsVectorSize(c,obj) == 0 )
      obj = CsResizeVector(c,obj,1);
    CsSetVectorElement(c,obj,CsVectorSize(c,obj) - 1,val);
}

static value CSF_concat(VM *c)
{
    value vector = ThisVector(c);
    FETCH(c, vector);
    int n = c->argc - 3 + 1;
    if( n == 0)
      return vector;

    int_t d = CsVectorSize(c,vector);
    int i;

    int_t extra = 0;
    for( i = 3; i <= c->argc; ++i )
    {
      value t = CsGetArg(c,i);
      if( CsVectorP(t) )
        extra += CsVectorSize(c,t); 
      else
        extra ++; 
    }
    value nvector = CsMakeVector(c,d+extra);
    protect2 _(c,vector,nvector);
    n = d;
    for( i = 0; i < d; ++i )
      CsSetVectorElement(c,nvector, i, CsVectorElement(c,vector,i));

    for( i = 3; i <= c->argc; ++i )
    {
      value t = CsGetArg(c,i);
      if( CsVectorP(t) )
      {
        for(int j = 0; j < CsVectorSize(c,t); ++j)
          CsSetVectorElement(c,nvector,n++,CsVectorElement(c,t,j));
      }
      else
        CsSetVectorElement(c,nvector,n++,t);
    }
    return nvector;
}

static value CSF_join(VM *c)
{
    value vector = 0;
    protect1 _(c,vector);
    wchar *str = 0;
    CsParseArguments(c,"V=*|S",&vector,&CsVectorDispatch,&str);
    tool::ustring dlm = str?str:L",";

    FETCH(c, vector);

    int_t n = CsVectorSize(c,vector);

    value r;
    string_stream s(20);

    int i;
    for( i = 0; i < n-1; ++i )
    {
       CsToString(c,CsVectorElement(c,vector,i), s);
       s.put_str(dlm);
    }
    if( i < n )
      CsToString(c,CsVectorElement(c,vector,i), s);

    r = s.string_o(c);
    s.close();
    return r;
}



static value CSF_reverse(VM *c)
{
    value vector = ThisVector(c);

    FETCH(c, vector);
    CsSetModified(vector,true);
    int_t d = CsVectorSize(c,vector);
    value *p1 = CsVectorAddress(c,vector);
    value *p2 = p1 + d - 1;

    while ( p1 < p2 )
    {
      tool::swap(*p1, *p2);
      ++p1; --p2;
    }
    
    return vector;
}

value CsVectorSlice(VM *c, value vector, int start, int end)
{
    FETCH(c, vector);
    int len = CsVectorSize(c,vector);

    /* handle indexing from the left */
    if (start > 0) {
        if (start > len)
            return UNDEFINED_VALUE;
    }

    /* handle indexing from the right */
    else if (start < 0) {
        if ((start = len + start) < 0)
            return UNDEFINED_VALUE;
    }

    /* handle the count */
    if (end < 0)
        end = len + end + 1;
    else if (end > len)
        end = len;

    if( start > end )
      return CsMakeVector(c,0);

    /* return the slice */
    protect1 _(c,vector);
    value vdst = CsMakeVector(c,end - start);
    value* src = CsVectorAddress(c,vector) + start;
    value* dst = CsVectorAddress(c,vdst);
    tool::copy(dst,src, end - start);
    return vdst;

}

static value CSF_slice(VM *c)
{
    int len,start,end = -1;
    value  vector;

    /* parse the arguments */
    CsParseArguments(c,"V=*i|i",&vector,&CsVectorDispatch,&start,&end);
    FETCH(c, vector);

    len = CsVectorSize(c,vector);

    /* handle indexing from the left */
    if (start > 0) {
        if (start > len)
            return UNDEFINED_VALUE;
    }

    /* handle indexing from the right */
    else if (start < 0) {
        if ((start = len + start) < 0)
            return UNDEFINED_VALUE;
    }

    /* handle the count */
    if (end < 0)
        end = len + end + 1;
    else if (end > len)
        end = len;

    if( start > end )
      return CsMakeVector(c,0);

    /* return the slice */
    protect1 _(c,vector);
    value vdst = CsMakeVector(c,end - start);
    value* src = CsVectorAddress(c,vector) + start;
    value* dst = CsVectorAddress(c,vdst);

    tool::copy(dst,src, end - start);

    return vdst;
}

static value CSF_splice(VM *c)
{
    int len, start, cnt = -1;
    value  vector;

    /* parse the arguments */
    CsParseArguments(c,"V=*i|i|",&vector,&CsVectorDispatch,&start,&cnt);
    FETCH(c, vector);

    len = CsVectorSize(c,vector);

    /* handle indexing from the left */
    if (start > 0) {
        if (start > len)
            return UNDEFINED_VALUE;
    }

    /* handle indexing from the right */
    else if (start < 0) {
        if ((start = len + start) < 0)
            return UNDEFINED_VALUE;
    }

    /* handle the count */
    if (cnt < 0)
        cnt = len - start;
    else if (start + cnt > len)
        cnt = len - start;

    if( cnt < 0 )
        return UNDEFINED_VALUE;

    int cnt_to_insert = 0;
    for( int n = 5; n <= CsArgCnt(c); ++n )
    {
      value v = CsGetArg(c,n);
      if( CsVectorP(v) )
        cnt_to_insert += CsVectorSize(c,v);
      else 
        cnt_to_insert += 1;
    }


    /* return the slice */
    protect1 _(c,vector);
    value vdst = CsMakeVector(c,cnt);

    value* p1 = CsVectorAddress(c,vector) + start;
    value* p2 = CsVectorAddress(c,vdst);
    tool::copy(p2,p1,cnt);

    CsSetModified(vector,true);

    if( cnt_to_insert )
    {
      CsResizeVector(c,vector, len - cnt + cnt_to_insert);
      size_t cnt_copy = len - start - cnt;
      value* p1src = CsVectorAddress(c,vector) + start + cnt;
      value* p1dst = CsVectorAddress(c,vector) + start + cnt_to_insert;
      tool::move(p1dst,p1src,cnt_copy );
      value* p3 = CsVectorAddress(c,vector) + start;
      for( int n = 5; n <= CsArgCnt(c); ++n )
      {
        value v = CsGetArg(c,n);
        if( CsVectorP(v) )
        {
          int vsz = CsVectorSize(c,v);
          value *src = CsVectorAddress(c,v);
          for(int k = 0; k < vsz; ++k )
            *p3++ = *src++;  
        }
        else 
          *p3++ = v;
      }
      //size_t cnt_copy = len - start - cnt;
      //CsResizeVector(c,vector, len - cnt);
    }
    else 
    {
      value* p3 = p1 + cnt;
      size_t cnt_copy = len - start - cnt;
      tool::copy(p1,p3,cnt_copy );
      CsResizeVector(c,vector, len - cnt);
    }


    return vdst;
}


  struct cmpValues
  {
    VM *vm;
    cmpValues( VM *pvm ): vm(pvm) {}
    bool less( const value& v1, const value& v2) { return CsCompareObjects(vm, v1,v2,true) < 0; }
  };

  typedef tool::sorter<value,cmpValues> simple_sorter;

  //value CsCallFunction(CsScope *scope,value fun,int argc,...)

  struct cmpValuesProxy
  {
    pvalue fun;
    cmpValuesProxy( VM *pvm, value f ): fun(pvm,f) {}
    bool less( const value& v1, const value& v2)
    {
      value r = CsCallFunction(CsCurrentScope(fun.pvm),fun,2,v1,v2);
      if(CsIntegerP(r))
        return CsIntegerValue(r) < 0;
      return false;
    }
  };


  struct vector_GC_protector: public gc_callback
  {
    tool::array<value>& elements;
    vector_GC_protector(VM* c, tool::array<value>& elems):gc_callback(c),elements(elems) {}
    virtual void on_GC(VM* c)
    {
      for( int n = 0; n < elements.size(); ++n ) 
        elements[n] = CsCopyValue(c,elements[n]);
    }
  };

static value CSF_sort(VM *c)
{
    value vector = 0,cmpf = 0;
    protect2 _(c,vector,cmpf);

    CsParseArguments(c,"V=*|V",&vector,&CsVectorDispatch,&cmpf);
    //vector = CsMovedVectorP(vector)? CsVectorForwardingAddr(vector): vector;
    FETCH(c, vector);
    CsSetModified(vector,true);

    int_t d = CsVectorSize(c,vector);
    value *p = CsVectorAddress(c,vector);
    if(!cmpf)
    {
      simple_sorter::sort(p, d,cmpValues(c));
    }
    else if(CsMethodP(cmpf))
    {
      cmpValuesProxy comparator(c,cmpf);
      tool::array<value> arr = tool::slice<value>(p,d);
      vector_GC_protector _(c,arr);
      tool::sorter<value,cmpValuesProxy>::sort(arr.head(), arr.size(), comparator);
      tool::copy(CsVectorAddress(c,vector),arr.head(),arr.size());
    }
    else
      CsTypeError(c,cmpf);

    return vector;
}

static value CSF_indexOf(VM *c)
{
    value vector = 0, v = 0, dv = CsMakeInteger(-1);
    protect3 _(c,vector, v, dv);

    CsParseArguments(c,"V=*V|V",&vector,&CsVectorDispatch,&v, &dv);

    FETCH(c,vector);

    int_t d = CsVectorSize(c,vector);
    for( int n = 0; n < d; ++n )
    {
      if( CsEqualOp(c, CsVectorElement(c,vector,n), v ) ) 
        return CsMakeInteger(n);
    }
    return dv;
}

static value CSF_lastIndexOf(VM *c)
{
    value vector = 0, v = 0, dv = CsMakeInteger(-1);
    protect3 _(c,vector, v, dv);

    CsParseArguments(c,"V=*V|V",&vector,&CsVectorDispatch,&v, &dv);

    FETCH(c,vector);

    int_t d = CsVectorSize(c,vector);
    for( int n = d - 1; n >= 0; --n )
    {
      if( CsEqualOp(c, CsVectorElement(c,vector,n ), v ) ) return CsMakeInteger(n);
    }
    return dv;
}

static value CSF_map(VM *c)
{
    value vector = 0 ,cmpf = 0, cmpf_this = 0, r_vector = 0;
    protect4 _(c,vector,cmpf,cmpf_this,r_vector);

    CsParseArguments(c,"V=*V=|V",&vector,&CsVectorDispatch,&cmpf,&CsMethodDispatch,&cmpf_this);

    FETCH(c, vector);

    int_t d = CsVectorSize(c,vector);

    r_vector = CsMakeVector(c,d);

    int_t ndst = 0;
    //value *p = CsVectorAddress(c,vector);
    if(cmpf_this)
    {
      for( int n = 0; n < d; ++n )
      {
        value r = CsCallMethod(c,cmpf_this, cmpf, cmpf_this, 3, 
           CsVectorElement(c,vector,n), 
           CsMakeInteger(n),
           vector);
        if( r == NOTHING_VALUE )
          continue;
        CsSetVectorElement(c,r_vector,ndst++,r);
      }
    }
    else
    {
      CsScope* scope = CsCurrentScope(c);
      for( int n = 0; n < d; ++n )
      {
        value r = CsCallFunction(scope,cmpf,3,
          CsVectorElement(c,vector,n), 
          CsMakeInteger(n),
          vector);
        if( r == NOTHING_VALUE )
          continue;
        CsSetVectorElement(c,r_vector,ndst++,r);
      }
    }
    if( ndst != d)
      return CsResizeVector(c,r_vector,ndst);
    return r_vector;
}

static value CSF_reduce(VM *c)
{
    value vector = 0, cmpf = 0, last = 0;
    protect3 _(c,vector, cmpf, last);

    CsParseArguments(c,"V=*V=|V",&vector,&CsVectorDispatch,&cmpf,&CsMethodDispatch,&last);

    FETCH(c, vector);

    int_t d = CsVectorSize(c,vector);

    CsScope* scope = CsCurrentScope(c);
    if( last )
      for( int n = 0; n < d; ++n )
      {
        last = CsCallFunction(scope,cmpf,4, // previousValue, currentValue, index, array
          last,
          CsVectorElement(c,vector,n), 
          CsMakeInteger(n),
          vector);
      }
    else if( d )
    {
      last = CsVectorElement(c,vector,0);
      for( int n = 1; n < d; ++n )
      {
        last = CsCallFunction(scope,cmpf,4, // previousValue, currentValue, index, array
          last,
          CsVectorElement(c,vector,n), 
          CsMakeInteger(n),
          vector);
      }
    }
    return  last;
}

static value CSF_reduceRight(VM *c)
{
    value vector = 0, cmpf = 0, last = 0;
    protect3 _(c,vector, cmpf, last);

    CsParseArguments(c,"V=*V=|V",&vector,&CsVectorDispatch,&cmpf,&CsMethodDispatch,&last);

    FETCH(c, vector);

    int_t d = CsVectorSize(c,vector);

    CsScope* scope = CsCurrentScope(c);
    if( last )
      for( int n = d - 1; n >= 0; --n )
      {
        last = CsCallFunction(scope,cmpf,4, // previousValue, currentValue, index, array
          last,
          CsVectorElement(c,vector,n), 
          CsMakeInteger(n),
          vector);
      }
    else if( d )
    {
      last = CsVectorElement(c,vector,d - 1);
      for( int n = d - 2; n >= 0; --n )
      {
        last = CsCallFunction(scope,cmpf,4, // previousValue, currentValue, index, array
          last,
          CsVectorElement(c,vector,n), 
          CsMakeInteger(n),
          vector);
      }
    }
    return  last;
}

static value CSF_filter(VM *c)
{
    value vector = 0, cmpf = 0, cmpf_this = 0,r_vector = 0;
    protect4 _(c,vector, cmpf, cmpf_this,r_vector);

    CsParseArguments(c,"V=*V=|V",&vector,&CsVectorDispatch,&cmpf,&CsMethodDispatch,&cmpf_this);

    FETCH(c, vector);

    int_t d = CsVectorSize(c,vector);

    r_vector = CsMakeVector(c,d);
    int ndst = 0;
    //value *p = CsVectorAddress(c,vector);
    if(cmpf_this)
    {
      for( int n = 0; n < d; ++n )
      {
        value r = CsCallMethod(c,cmpf_this, cmpf, cmpf_this, 3, 
           CsVectorElement(c,vector,n), 
           CsMakeInteger(n),
           vector);
        if( CsToBoolean(c,r) == TRUE_VALUE)
          CsSetVectorElement(c,r_vector,ndst++,CsVectorElement(c,vector,n));
      }
    }
    else
    {
      CsScope* scope = CsCurrentScope(c);
      for( int n = 0; n < d; ++n )
      {
        value r = CsCallFunction(scope,cmpf,3,
          CsVectorElement(c,vector,n), 
          CsMakeInteger(n),
          vector);
        if( CsToBoolean(c,r) == TRUE_VALUE)
          CsSetVectorElement(c,r_vector,ndst++,CsVectorElement(c,vector,n));
      }
    }
    return CsResizeVector(c,r_vector,ndst);
}

bool CsVectorsEqual(VM *c, value v1, value v2)
{
  if( CsVectorSize(c,v1) != CsVectorSize(c,v2) )
    return false;
  protect2 _(c,v1,v2);
  FETCH(c,v1);
  FETCH(c,v2);
  int d = CsVectorSize(c,v1);
  for( int n = 0; n < d; ++n )
  {
    if(!CsEqualOp(c,CsVectorElement(c,v1,n), CsVectorElement(c,v2,n))) 
      return false;
  }
  return true;
}

int CsCompareVectors(VM* c, value v1, value v2, bool suppressError)
{
  protect2 _(c,v1,v2);
  FETCH(c,v1);
  FETCH(c,v2);
  int d = CsVectorSize(c,v1);
  for( int n = 0; n < d; ++n )
  {
    int r = CsCompareObjects(c, CsVectorElement(c,v1,n), CsVectorElement(c,v2,n), suppressError );
    if( r ) return r;
  }
  return tool::limit(int(CsVectorSize(c,v1) - CsVectorSize(c,v2)),-1,1);
}


/* VECTOR */

/* Vector handlers */
static bool GetVectorProperty(VM *c,value& obj,value tag,value *pValue);
static bool SetVectorProperty(VM *c,value obj,value tag,value value);
static value VectorNewInstance(VM *c,value proto);
static long VectorSize(value obj);
static void VectorScan(VM *c,value obj);

static value CsVectorGetItem(VM *c,value obj,value tag);
static void  CsVectorSetItem(VM *c,value obj,value tag,value value);

bool  VectorPrint(VM *c,value vector,stream *s, bool toLocale)
{
    int_t n = CsVectorSize(c,vector);

    int i;
    for( i = 0; i < n-1; ++i )
    {
       if(!CsPrintValue(c,CsVectorElement(c,vector,i),s,toLocale))
         return false;
       if(!s->put_str(","))
         return false;
    }
    if( i < n )
      return CsPrintValue(c,CsVectorElement(c,vector,i),s,toLocale);

    return true;

}

inline value FindFirstMember(VM *c, value& index, value collection)
{
  if(CsVectorP(collection))
  {
    if(CsVectorSize(c,collection))
    {
      index = CsMakeInteger(0);
      return CsVectorElement(c,collection,0);
    }
  }
  //else if(CsObjectOrMethodP(obj))
  //{
  //  return CsFindFirstSymbol(c,obj);
  //}
  return NOTHING_VALUE;
}

value VectorNextElement(VM *c, value* index, value collection, int nr)
{
    if(*index == NOTHING_VALUE) // first
    {
      FETCH(c,collection);
      if(CsVectorSize(c,collection))
      {
        *index = CsMakeInteger(0);
        CsSetRVal(c,1,*index);
        return CsVectorElement(c,collection,0);
      }
    }
    else if(CsIntegerP(*index))
    {
      int_t i = CsIntegerValue(*index) + 1;
      *index = CsMakeInteger(i);
      if(i < CsVectorSize(c,collection))
      {
        CsSetRVal(c,1,*index);
        return CsVectorElement(c,collection,i);
      }
    }
    else
      assert(false);

    return NOTHING_VALUE;
}


/* Vector pdispatch */
dispatch CsVectorDispatch = {
    "Array",
    &CsVectorDispatch,
    GetVectorProperty,
    SetVectorProperty,
    VectorNewInstance,
    VectorPrint,
    VectorSize,
    CsDefaultCopy,
    VectorScan,
    CsDefaultHash,
    CsVectorGetItem,
    CsVectorSetItem,
    VectorNextElement,
    0,
    RemoveItem,
};

static value CsVectorGetItem(VM *c,value obj,value tag)
{
    FETCH_P(c, obj,tag);
    if (CsIntegerP(tag))
    {
        int_t i;
        if ((i = CsIntegerValue(tag)) < 0 || i >= CsVectorSizeI(obj))
            CsThrowKnownError(c,CsErrIndexOutOfBounds,tag);
        return CsVectorElementI(obj,i);
    }
    return UNDEFINED_VALUE;
}
static void     CsVectorSetItem(VM *c,value obj,value tag,value value)
{
    FETCH_PP(c, obj,tag, value);
    if (CsIntegerP(tag))
    {
        CsSetModified(obj,true);
        int_t i;
        if ((i = CsIntegerValue(tag)) < 0)
            CsThrowKnownError(c,CsErrIndexOutOfBounds,tag);
        else if (i >= CsVectorSizeI(obj)) {
            CsCPush(c,value);
            obj = CsResizeVector(c,obj,i + 1);
            if (CsMovedVectorP(obj))
                obj = CsVectorForwardingAddr(obj);
            value = CsPop(c);
        }
        CsSetVectorElementI(obj,i,value);
    }
    else 
      CsThrowKnownError(c, CsErrUnexpectedTypeError, tag, "only integer as an index");
}

/* GetVectorProperty - Vector get property handler */
static bool GetVectorProperty(VM *c,value& obj,value tag,value *pValue)
{
    return CsGetVirtualProperty(c,obj,c->vectorObject,tag,pValue);
}

/* SetVectorProperty - Vector set property handler */
static bool SetVectorProperty(VM *c,value obj,value tag,value value)
{
    return CsSetVirtualProperty(c,obj,c->vectorObject,tag,value);
}

/* VectorNewInstance - create a new vector */
static value VectorNewInstance(VM *c,value proto)
{
    return CsMakeVector(c,0);
}

/* VectorSize - Vector size handler */
static long VectorSize(value obj)
{
    assert(!CsMovedVectorP(obj));
    return sizeof(vector) + CsVectorMaxSize(obj) * sizeof(value);
}

/* VectorScan - Vector scan handler */
static void VectorScan(VM *c,value obj)
{
    //vector* pv =  ptr<vector>(obj);
    //if (CsMovedVectorP(obj))
    //    obj = CsVectorForwardingAddr(obj);
    assert(!CsMovedVectorP(obj));

    long i, sz = CsVectorSizeI(obj);
    for (i = 0; i < sz; ++i)
        CsSetVectorElementI(obj,i,CsCopyValue(c,CsVectorElementI(obj,i)));
}

/* MOVED VECTOR */

/* MovedVector handlers */
static bool GetMovedVectorProperty(VM *c,value& obj,value tag,value *pValue);
static bool SetMovedVectorProperty(VM *c,value obj,value tag,value value);
static value MovedVectorCopy(VM *c,value obj);
static value CsMovedVectorGetItem(VM *c,value obj,value tag);
static void     CsMovedVectorSetItem(VM *c,value obj,value tag,value value);

/* VectorSize - Vector size handler */
static long MovedVectorSize(value obj)
{
    assert(CsMovedVectorP(obj));
    return sizeof(vector);
}

/* MovedVector pdispatch */
dispatch CsMovedVectorDispatch = {
    "MovedVector",
    &CsVectorDispatch,
    GetMovedVectorProperty,
    SetMovedVectorProperty,
    CsDefaultNewInstance,
    CsDefaultPrint,
    MovedVectorSize,
    MovedVectorCopy,
    CsDefaultScan,
    CsDefaultHash,
    CsMovedVectorGetItem,
    CsMovedVectorSetItem,
    VectorNextElement,
};

static value CsMovedVectorGetItem(VM *c,value obj,value tag)
{
  return CsVectorGetItem(c,CsVectorForwardingAddr(obj),tag);
}
static void     CsMovedVectorSetItem(VM *c,value obj,value tag,value val)
{
    value resizedVector = CsVectorForwardingAddr(obj);
    if (CsIntegerP(tag)) {
        int_t i;
        if ((i = CsIntegerValue(tag)) < 0)
            CsThrowKnownError(c,CsErrIndexOutOfBounds,tag);
        else if (i >= CsVectorSizeI(resizedVector)) {
            CsCPush(c,val);
            resizedVector = CsResizeVector(c,obj,i + 1);
            if (CsMovedVectorP(resizedVector))
                resizedVector = CsVectorForwardingAddr(resizedVector);
            val = CsPop(c);
        }
        CsSetVectorElementI(resizedVector,i,val);
    }
 }

/* GetMovedVectorProperty - MovedVector get property handler */
static bool GetMovedVectorProperty(VM *c,value& obj,value tag,value *pValue)
{
    obj = CsVectorForwardingAddr(obj);
    return GetVectorProperty(c,obj,tag,pValue);
}

/* SetMovedVectorProperty - MovedVector set property handler */
static bool SetMovedVectorProperty(VM *c,value obj,value tag,value value)
{
    return CsSetVirtualProperty(c,obj,c->vectorObject,tag,value);
}

/* MovedVectorCopy - MovedVector scan handler */
static value MovedVectorCopy(VM *c,value obj)
{
    // Moved vector will become plain vector here:
    value newObj = CsCopyValue(c,CsVectorForwardingAddr(obj));
    CsSetDispatch(obj,&CsBrokenHeartDispatch);
    CsBrokenHeartSetForwardingAddr(obj,newObj);
    return newObj;
}

/* CsMakeFixedVectorValue - make a new vector value */
value CsMakeFixedVectorValue(VM *c,dispatch *type,int size)
{
    long allocSize = sizeof(CsFixedVector) + size * sizeof(value);
    value newo = CsAllocate(c,allocSize);
    CsSetDispatch(newo,type);
    value *p = CsFixedVectorAddress(newo);
    while (--size >= 0)
        *p++ = UNDEFINED_VALUE;
    assert(allocSize == ValueSize(newo));
    return newo;
}

/* CsFixedVector - construct a fixed vector */
value CsMakeFixedVector(VM *c, dispatch *type, int argc, value *argv)
{
    value newo;
    int i;
    CsCheck(c,argc);
    for(i = argc - 1; i >= 0; --i)
      CsPush(c,argv[i]);
    newo = CsMakeFixedVectorValue(c,type,argc);
    for(i = 0; i < argc; ++i)
      CsSetFixedVectorElement(newo,i,CsPop(c));
    return newo;
}

/* Property handlers */
static long FixedVectorSize(value obj);
static void FixedVectorScan(VM *c,value obj);
static bool FixedVectorPrint(VM *c,value obj,stream *s, bool toLocale);

/* Property pdispatch */
dispatch CsFixedVectorDispatch = {
    "Structure",
    &CsFixedVectorDispatch,
    CsDefaultGetProperty,
    CsDefaultSetProperty,
    CsDefaultNewInstance,
    FixedVectorPrint,
    FixedVectorSize,
    CsDefaultCopy,
    FixedVectorScan,
    CsDefaultHash,
    CsDefaultGetItem,
    CsDefaultSetItem
};

/* FixedVectorSize - FixedVector size handler */
static long FixedVectorSize(value obj)
{
    dispatch *d = CsQuickGetDispatch(obj);
    return sizeof(CsFixedVector) + d->dataSize * sizeof(value);
}


/* FixedVectorScan - FixedVector scan handler */
static void FixedVectorScan(VM *c,value obj)
{
    long i;
    dispatch *d = CsQuickGetDispatch(obj);
    for (i = 0; i < d->dataSize; ++i)
        CsSetFixedVectorElement(obj,i,CsCopyValue(c,CsFixedVectorElement(obj,i)));
}


/* CsDefaultPrint - print an obj */
bool FixedVectorPrint(VM *c,value obj,stream *s, bool toLocale)
{
    long i;
    dispatch *d = CsQuickGetDispatch(obj);

    s->put('[');
    s->put_str(CsTypeName(obj));
    s->put(' ');

    for(i = 0; i < d->dataSize - 1; ++i)
    {
      CsDisplay(c,CsFixedVectorElement(obj,i),s);
      s->put(',');
    }
    CsDisplay(c,CsFixedVectorElement(obj,d->dataSize - 1),s);

    s->put(']');

    return true;
}

/* CsMakeFixedVectorType - make a new fixed vector type (structure) */
dispatch *CsMakeFixedVectorType(
     VM *c,
     dispatch *proto,
     char *typeName,
     c_method *methods,
     vp_method *properties,
     int size)
{
    dispatch *d;

    /* make and initialize the type pdispatch structure */
    if (!(d = CsMakeDispatch(c,typeName,&CsFixedVectorDispatch)))
        return NULL;
    d->proto = proto;
    d->dataSize = size;

    d->getItem = CsDefaultGetItem;
    d->setItem = CsDefaultSetItem;

    /* make the type obj */
    d->obj = CsMakeCPtrObject(c,c->typeDispatch,d);

    /* enter the methods and properties */
    CsEnterCObjectMethods(c,d,methods,properties);

    /* return the new type */
    return d;
}


/* CsEnterFixedVectorType - add a built-in cobject type to the symbol table */
dispatch *CsEnterFixedVectorType(CsScope *scope,dispatch *proto,char *typeName,c_method *methods,vp_method *properties,int size)
{
    VM *c = scope->c;
    dispatch *d;

    /* make the type */
    if (!(d = CsMakeFixedVectorType(c,proto,typeName,methods,properties,size)))
        return NULL;

  /* add the type symbol */
  CsCPush(c,CsInternCString(c,typeName));
    CsSetGlobalValue(scope,CsTop(c),d->obj);
  CsDrop(c,1);

    /* return the new obj type */
    return d;
}



/* BASIC VECTOR */

/* CsBasicVectorSizeHandler - BasicVector size handler */
long CsBasicVectorSizeHandler(value obj)
{
    return sizeof(CsBasicVector) + CsBasicVectorSize(obj) * sizeof(value);
}

/* CsBasicVectorScanHandler - BasicVector scan handler */
void CsBasicVectorScanHandler(VM *c,value obj)
{
    long i;
    for (i = 0; i < CsBasicVectorSize(obj); ++i)
        CsSetBasicVectorElement(obj,i,CsCopyValue(c,CsBasicVectorElement(obj,i)));
}

/* CsMakeBasicVector - make a new vector value */
value CsMakeBasicVector(VM *c,dispatch *type,int_t size)
{
    long allocSize = sizeof(CsBasicVector) + size * sizeof(value);
    value newo = CsAllocate(c,allocSize);
    value *p = CsBasicVectorAddress(newo);
    CsSetDispatch(newo,type);
    CsSetBasicVectorSize(newo,size);
    while (--size >= 0)
        *p++ = UNDEFINED_VALUE;
    assert(allocSize == ValueSize(newo));
    return newo;
}

/* Tuple pdispatch - fixed vector here */
dispatch CsTupleDispatch = {
    "Tuple",
    &CsTupleDispatch,
    CsDefaultGetProperty,
    CsDefaultSetProperty,
    CsDefaultNewInstance,
    CsDefaultPrint,
    CsBasicVectorSizeHandler,
    CsDefaultCopy,
    CsBasicVectorScanHandler,
    CsDefaultHash,
    CsDefaultGetItem,
    CsDefaultSetItem
};

value CsMakeBasicVector(VM *c, int_t size)
{
  return CsMakeBasicVector(c, &CsTupleDispatch,size);
}

/* CsMakeVector - make a new vector value */
value CsMakeVector(VM *c,int_t size)
{
#ifdef _DEBUG
   if ( size == 256 )
     size = size;
#endif
    long allocSize = sizeof(vector) + size * sizeof(value);
    value *p,newo = CsAllocate(c,allocSize);
    CsSetDispatch(newo,&CsVectorDispatch);
    CsSetVectorSize(newo,size);
    CsSetVectorMaxSize(newo,size);
    p = CsVectorAddress(c,newo);
    while (--size >= 0)
        *p++ = UNDEFINED_VALUE;
    _CsInitPersistent(newo);
    assert(allocSize == ValueSize(newo));
    return newo;
}

/* CsCloneVector - clone an existing vector */
value CsCloneVector(VM *c,value obj)
{
    FETCH(c, obj);
    int_t size = CsVectorSize(c,obj);
    long allocSize = sizeof(vector) + size * sizeof(value);
    value *src,*dst,newo = CsAllocate(c,allocSize);
    CsSetDispatch(newo,&CsVectorDispatch);
    CsSetVectorSize(newo,size);
    CsSetVectorMaxSize(newo,size);
    src = CsVectorAddress(c,obj);
    dst = CsVectorAddress(c,newo);
    while (--size >= 0)
        *dst++ = *src++;
    assert(allocSize == ValueSize(newo));
    return newo;
}

value CsResizeVector(VM *c,value obj,int_t newSize)
{
 FETCH(c, obj);
 return CsResizeVectorNoLoad(c,obj,newSize);
}

/* CsResizeVector - resize a vector */
value CsResizeVectorNoLoad(VM *c,value obj,int_t newSize)
{
    int_t size;
    value resizeVector;

    /* handle a vector that has already been moved */
    if (CsMovedVectorP(obj))
        resizeVector = CsVectorForwardingAddr(obj);
    else
        resizeVector = obj;

    /* make sure the size is really changing */
    if ((size = CsVectorSizeI(resizeVector)) != newSize) {

        /* check for extra existing space */
        if (newSize <= CsVectorMaxSize(resizeVector)) {

            /* fill the extra space with nil */
            if (newSize > size) {
                value *dst = CsVectorAddressI(resizeVector) + size;
                while (++size <= newSize)
                    *dst++ = UNDEFINED_VALUE;
            }

            /* store the new vector size */
            CsSetVectorSize(resizeVector,newSize);
        }

        /* expand the vector */
        else {
            value newVector,*src,*dst;
            int_t allocSize;

            /* try expanding by a fraction of the current size */
            allocSize = size / CsVectorExpandDivisor;

            /* but make sure we expand by at least CsVectorExpandMinimum */
            if (allocSize < CsVectorExpandMinimum)
                allocSize = CsVectorExpandMinimum;

            /* and at most CsVectorExpandMaximum */
            if (allocSize > CsVectorExpandMaximum)
                allocSize = CsVectorExpandMaximum;

            /* but at least what we need */
            if ((allocSize += size) < newSize)
                allocSize = newSize;

            /* make a new vector */
            CsCheck(c,2);
            CsPush(c,obj);
            CsPush(c,resizeVector);
            newVector = CsMakeVector(c,allocSize);
            CsSetVectorSizeI(newVector,newSize);
            resizeVector = CsPop(c);
            obj = CsPop(c);

            /* copy the data from the old to the new vector */
            src = CsVectorAddressI(resizeVector);
            dst = CsVectorAddressI(newVector);
            memcpy(dst, src, size * sizeof(tis::value));

            /* set the forwarding address of the old vector */
            CsSetDispatch(obj,&CsMovedVectorDispatch);
            CsSetVectorForwardingAddr(obj,newVector);
        }
    }

    /* return the resized vector */
    return obj;
}

/* CsVectorSize - get the size of a vector */
int_t CsVectorSize(VM* c, value obj)
{
    FETCH(c,obj);
    if (CsMovedVectorP(obj))
        obj = CsVectorForwardingAddr(obj);
    return CsVectorSizeI(obj);
}

/* CsVectorSize - get the size of a vector */
int_t CsVectorSizeNoLoad(VM* c, value obj)
{
    if (CsMovedVectorP(obj))
        obj = CsVectorForwardingAddr(obj);
    return CsVectorSizeI(obj);
}

/* CsVectorAddress - get the address of the vector data */
value *CsVectorAddress(VM* c, value obj)
{
    FETCH(c,obj);
    if (CsMovedVectorP(obj))
        obj = CsVectorForwardingAddr(obj);
    return CsVectorAddressI(obj);
}

/* CsVectorAddress - get the address of the vector data */
value *CsVectorAddressNoLoad(VM* c, value obj)
{
    if (CsMovedVectorP(obj))
        obj = CsVectorForwardingAddr(obj);
    return CsVectorAddressI(obj);
}


/* CsVectorElement - get a vector element */
value CsVectorElement(VM* c, value obj,int_t i)
{
    FETCH(c,obj);
    if (CsMovedVectorP(obj))
        obj = CsVectorForwardingAddr(obj);
    return CsVectorElementI(obj,i);
}

/* CsVectorElement - get a vector element */
value CsVectorElementNoLoad(VM* c, value obj,int_t i)
{
    if (CsMovedVectorP(obj))
        obj = CsVectorForwardingAddr(obj);
    return CsVectorElementI(obj,i);
}


/* CsSetVectorElement - set a vector element */
void CsSetVectorElement(VM* c, value obj,int_t i,value val)
{
    FETCH(c,obj);
    CsSetModified(obj,true);
    if (CsMovedVectorP(obj))
        obj = CsVectorForwardingAddr(obj);
    CsSetVectorElementI(obj,i,val);
}

void CsSetVectorElementNoLoad(VM* c, value obj,int_t i,value val)
{
    if (CsMovedVectorP(obj))
        obj = CsVectorForwardingAddr(obj);
    CsSetVectorElementI(obj,i,val);
}


}
