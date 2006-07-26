/*
 * sand_table.c
 *
 * $Author$
 * $Date$
 *
 * Copyright (C) 2006 why the lucky stiff
 */
#include "sand_table.h"

#define SAND_REV_ID "$Rev$"

static VALUE Qimport, Qinit, Qload;

static void Init_kit _((sandkit *));
static void Init_kit_load _((sandkit *));

static void
mark_sandbox(kit)
  sandkit *kit;
{
  if (kit->banished != NULL)
    mark_sandbox(kit->banished);
  rb_mark_tbl(kit->tbl);
  rb_gc_mark_maybe(kit->cObject);
  rb_gc_mark_maybe(kit->cModule);
  rb_gc_mark_maybe(kit->cClass);
  rb_gc_mark_maybe(kit->mKernel);
  rb_gc_mark_maybe(kit->oMain);
  rb_gc_mark_maybe(kit->cArray);
  rb_gc_mark_maybe(kit->cBignum);
  rb_gc_mark_maybe(kit->mComparable);
  rb_gc_mark_maybe(kit->cData);
  rb_gc_mark_maybe(kit->mEnumerable);
  rb_gc_mark_maybe(kit->eException);
  rb_gc_mark_maybe(kit->cFalseClass);
  rb_gc_mark_maybe(kit->cFixnum);
  rb_gc_mark_maybe(kit->cFloat);
  rb_gc_mark_maybe(kit->cHash);
  rb_gc_mark_maybe(kit->cInteger);
  rb_gc_mark_maybe(kit->cMatch);
  rb_gc_mark_maybe(kit->cMethod);
  rb_gc_mark_maybe(kit->cNilClass);
  rb_gc_mark_maybe(kit->cNumeric);
  rb_gc_mark_maybe(kit->mPrecision);
  rb_gc_mark_maybe(kit->cProc);
  rb_gc_mark_maybe(kit->cRange);
  rb_gc_mark_maybe(kit->cRegexp);
  rb_gc_mark_maybe(kit->cString);
  rb_gc_mark_maybe(kit->cStruct);
  rb_gc_mark_maybe(kit->cSymbol);
  rb_gc_mark_maybe(kit->cTrueClass);
  rb_gc_mark_maybe(kit->cUnboundMethod);
  rb_gc_mark_maybe(kit->eStandardError);
  rb_gc_mark_maybe(kit->eSystemExit);
  rb_gc_mark_maybe(kit->eInterrupt);
  rb_gc_mark_maybe(kit->eSignal);
  rb_gc_mark_maybe(kit->eFatal);
  rb_gc_mark_maybe(kit->eArgError);
  rb_gc_mark_maybe(kit->eEOFError);
  rb_gc_mark_maybe(kit->eIndexError);
  rb_gc_mark_maybe(kit->eRangeError);
  rb_gc_mark_maybe(kit->eIOError);
  rb_gc_mark_maybe(kit->eRuntimeError);
  rb_gc_mark_maybe(kit->eSecurityError);
  rb_gc_mark_maybe(kit->eSystemCallError);
  rb_gc_mark_maybe(kit->eSysStackError);
  rb_gc_mark_maybe(kit->eTypeError);
  rb_gc_mark_maybe(kit->eZeroDivError);
  rb_gc_mark_maybe(kit->eNotImpError);
  rb_gc_mark_maybe(kit->eNoMemError);
  rb_gc_mark_maybe(kit->eNoMethodError);
  rb_gc_mark_maybe(kit->eFloatDomainError);
  rb_gc_mark_maybe(kit->eScriptError);
  rb_gc_mark_maybe(kit->eNameError);
  rb_gc_mark_maybe(kit->cNameErrorMesg);
  rb_gc_mark_maybe(kit->eSyntaxError);
  rb_gc_mark_maybe(kit->eLoadError);
  rb_gc_mark_maybe(kit->eLocalJumpError);
  rb_gc_mark_maybe(kit->mErrno);
#ifdef FFSAFE
  if (kit->globals != NULL)
    sandbox_mark_globals(kit->globals);
#endif
}

void
free_sandbox(kit)
  sandkit *kit;
{   
  st_free_table(kit->tbl);
#ifdef FFSAFE
  st_free_table(kit->globals);
#endif
  MEMZERO(kit, sandkit, 1);
  free(kit);
}

static VALUE sandbox_alloc_obj _((VALUE));
static VALUE
sandbox_alloc_obj(klass)
  VALUE klass; 
{   
  NEWOBJ(obj, struct RObject);
  OBJSETUP(obj, klass, T_OBJECT);
  return (VALUE)obj;
}

VALUE sandbox_alloc _((VALUE));
VALUE 
sandbox_alloc(class)
    VALUE class;
{
  sandkit *kit = ALLOC(sandkit);
  MEMZERO(kit, sandkit, 1);
  Init_kit(kit);

  NEWOBJ(_scope, struct SCOPE);
  OBJSETUP(_scope, 0, T_SCOPE);
  _scope->local_tbl = 0;
  _scope->local_vars = 0;
  _scope->flags = 0;
  kit->scope = _scope;

  return Data_Wrap_Struct( class, mark_sandbox, free_sandbox, kit );
}

static VALUE
sandbox_initialize(argc, argv, self)
  int argc;
  VALUE *argv;
  VALUE self;
{
  VALUE opts, import, init;
  if (rb_scan_args(argc, argv, "01", &opts) == 0)
  {
    opts = rb_hash_new();
  }
  else
  {
    Check_Type(opts, T_HASH);
  }

  init = rb_hash_aref(opts, Qinit);
  if (!NIL_P(init))
  {
    int i;
    sandkit *kit;
    Check_Type(init, T_ARRAY);
    Data_Get_Struct( self, sandkit, kit );
    for ( i = 0; i < RARRAY(init)->len; i++ )
    {
      VALUE mod = rb_ary_entry(init, i);
      if ( mod == Qload )
      {
        Init_kit_load(kit);
      }
      else
      {
        rb_raise(rb_eArgError, "no %s module for the sandbox", mod);
      }
    }
  }

  import = rb_hash_aref(opts, Qimport);
  if (!NIL_P(import))
  {
    int i;
    Check_Type(import, T_ARRAY);
    for ( i = 0; i < RARRAY(import)->len; i++ )
    {
      rb_funcall(self, rb_intern("import"), 1, rb_ary_entry(import, i));
    }
  }
  return self;
}

typedef struct {
  VALUE *argv;
  sandkit *kit;
  sandkit *norm;
} go_cart;

VALUE
sandbox_whoa_whoa_whoa(go)
  go_cart *go;
{
  /* okay, move it all back */
  sandkit *norm = go->norm;
  rb_class_tbl = norm->tbl;
  rb_cObject = norm->cObject;
  rb_cModule = norm->cModule;
  rb_cClass = norm->cClass;
  rb_mKernel = norm->mKernel;
  rb_cArray = norm->cArray;
  rb_cBignum = norm->cBignum;
  rb_mComparable = norm->mComparable;
  rb_cData = norm->cData;
  rb_mEnumerable = norm->mEnumerable;
  rb_cFalseClass = norm->cFalseClass;
  rb_cFixnum = norm->cFixnum;
  rb_cFloat = norm->cFloat;
  rb_cHash = norm->cHash;
  rb_cInteger = norm->cInteger;
  rb_cNilClass = norm->cNilClass;
  rb_cNumeric = norm->cNumeric;
  rb_mPrecision = norm->mPrecision;
  rb_cProc = norm->cProc;
  rb_cRange = norm->cRange;
  rb_cRegexp = norm->cRegexp;
  rb_cString = norm->cString;
  rb_cStruct = norm->cStruct;
  rb_cSymbol = norm->cSymbol;
  rb_cTrueClass = norm->cTrueClass;
  rb_eException = norm->eException;
  rb_eStandardError = norm->eStandardError;
  rb_eSystemExit = norm->eSystemExit;
  rb_eInterrupt = norm->eInterrupt;
  rb_eSignal = norm->eSignal;
  rb_eFatal = norm->eFatal;
  rb_eArgError = norm->eArgError;
  rb_eEOFError = norm->eEOFError;
  rb_eIndexError = norm->eIndexError;
  rb_eRangeError = norm->eRangeError;
  rb_eIOError = norm->eIOError;
  rb_eRuntimeError = norm->eRuntimeError;
  rb_eSecurityError = norm->eSecurityError;
  rb_eSystemCallError = norm->eSystemCallError;
  rb_eTypeError = norm->eTypeError;
  rb_eZeroDivError = norm->eZeroDivError;
  rb_eNotImpError = norm->eNotImpError;
  rb_eNoMemError = norm->eNoMemError;
  rb_eNoMethodError = norm->eNoMethodError;
  rb_eFloatDomainError = norm->eFloatDomainError;
  rb_eScriptError = norm->eScriptError;
  rb_eNameError = norm->eNameError;
  rb_eSyntaxError = norm->eSyntaxError;
  rb_eLoadError = norm->eLoadError;
  rb_mErrno = norm->mErrno;
  ruby_top_self = norm->oMain;
  ruby_scope = norm->scope;
#ifdef FFSAFE
  rb_global_tbl = norm->globals;
  rb_cMatch = norm->cMatch;
  rb_cMethod = norm->cMethod;
  rb_cUnboundMethod = norm->cUnboundMethod;
  rb_eRegexpError = norm->eRegexpError;
  rb_cNameErrorMesg = norm->cNameErrorMesg;
  rb_eSysStackError = norm->eSysStackError;
  rb_eLocalJumpError = norm->eLocalJumpError;
#endif

  go->kit->banished = NULL;
  free(go->norm);
  free(go);
}

go_cart *
sandbox_swap_in( self )
  VALUE self;
{
  sandkit *kit, *norm;
  go_cart *go;
  Data_Get_Struct( self, sandkit, kit );

  /* save everything */
  norm = ALLOC(sandkit);
  MEMZERO(norm, sandkit, 1);
  norm->tbl = rb_class_tbl;
  norm->cObject = rb_cObject;
  norm->cModule = rb_cModule;
  norm->cClass = rb_cClass;
  norm->mKernel = rb_mKernel;
  norm->cArray = rb_cArray;
  norm->cBignum = rb_cBignum;
  norm->mComparable = rb_mComparable;
  norm->cData = rb_cData;
  norm->mEnumerable = rb_mEnumerable;
  norm->cFalseClass = rb_cFalseClass;
  norm->cFixnum = rb_cFixnum;
  norm->cFloat = rb_cFloat;
  norm->cHash = rb_cHash;
  norm->cInteger = rb_cInteger;
  norm->cNilClass = rb_cNilClass;
  norm->cNumeric = rb_cNumeric;
  norm->mPrecision = rb_mPrecision;
  norm->cProc = rb_cProc;
  norm->cRange = rb_cRange;
  norm->cRegexp = rb_cRegexp;
  norm->cString = rb_cString;
  norm->cStruct = rb_cStruct;
  norm->cSymbol = rb_cSymbol;
  norm->cTrueClass = rb_cTrueClass;
  norm->eException = rb_eException;
  norm->eStandardError = rb_eStandardError;
  norm->eSystemExit = rb_eSystemExit;
  norm->eInterrupt = rb_eInterrupt;
  norm->eSignal = rb_eSignal;
  norm->eFatal = rb_eFatal;
  norm->eArgError = rb_eArgError;
  norm->eEOFError = rb_eEOFError;
  norm->eIndexError = rb_eIndexError;
  norm->eRangeError = rb_eRangeError;
  norm->eIOError = rb_eIOError;
  norm->eRuntimeError = rb_eRuntimeError;
  norm->eSecurityError = rb_eSecurityError;
  norm->eSystemCallError = rb_eSystemCallError;
  norm->eTypeError = rb_eTypeError;
  norm->eZeroDivError = rb_eZeroDivError;
  norm->eNotImpError = rb_eNotImpError;
  norm->eNoMemError = rb_eNoMemError;
  norm->eNoMethodError = rb_eNoMethodError;
  norm->eFloatDomainError = rb_eFloatDomainError;
  norm->eScriptError = rb_eScriptError;
  norm->eNameError = rb_eNameError;
  norm->eSyntaxError = rb_eSyntaxError;
  norm->eLoadError = rb_eLoadError;
  norm->mErrno = rb_mErrno;
  norm->oMain = ruby_top_self;
  norm->scope = ruby_scope;
#ifdef FFSAFE
  norm->globals = rb_global_tbl;
  norm->cMatch = rb_cMatch;
  norm->cMethod = rb_cMethod;
  norm->cUnboundMethod = rb_cUnboundMethod;
  norm->eRegexpError = rb_eRegexpError;
  norm->cNameErrorMesg = rb_cNameErrorMesg;
  norm->eSysStackError = rb_eSysStackError;
  norm->eLocalJumpError = rb_eLocalJumpError;
#endif

  /* replace everything */
  rb_class_tbl = kit->tbl;
  rb_cObject = kit->cObject;
  rb_cModule = kit->cModule;
  rb_cClass = kit->cClass;
  rb_mKernel = kit->mKernel;
  rb_cArray = kit->cArray;
  rb_cBignum = kit->cBignum;
  rb_mComparable = kit->mComparable;
  rb_cData = kit->cData;
  rb_mEnumerable = kit->mEnumerable;
  rb_cFalseClass = kit->cFalseClass;
  rb_cFixnum = kit->cFixnum;
  rb_cFloat = kit->cFloat;
  rb_cHash = kit->cHash;
  rb_cInteger = kit->cInteger;
  rb_cNilClass = kit->cNilClass;
  rb_cNumeric = kit->cNumeric;
  rb_mPrecision = kit->mPrecision;
  rb_cProc = kit->cProc;
  rb_cRange = kit->cRange;
  rb_cRegexp = kit->cRegexp;
  rb_cString = kit->cString;
  rb_cStruct = kit->cStruct;
  rb_cSymbol = kit->cSymbol;
  rb_cTrueClass = kit->cTrueClass;
  rb_eException = kit->eException;
  rb_eStandardError = kit->eStandardError;
  rb_eSystemExit = kit->eSystemExit;
  rb_eInterrupt = kit->eInterrupt;
  rb_eSignal = kit->eSignal;
  rb_eFatal = kit->eFatal;
  rb_eArgError = kit->eArgError;
  rb_eEOFError = kit->eEOFError;
  rb_eIndexError = kit->eIndexError;
  rb_eRangeError = kit->eRangeError;
  rb_eIOError = kit->eIOError;
  rb_eRuntimeError = kit->eRuntimeError;
  rb_eSecurityError = kit->eSecurityError;
  rb_eSystemCallError = kit->eSystemCallError;
  rb_eTypeError = kit->eTypeError;
  rb_eZeroDivError = kit->eZeroDivError;
  rb_eNotImpError = kit->eNotImpError;
  rb_eNoMemError = kit->eNoMemError;
  rb_eNoMethodError = kit->eNoMethodError;
  rb_eFloatDomainError = kit->eFloatDomainError;
  rb_eScriptError = kit->eScriptError;
  rb_eNameError = kit->eNameError;
  rb_eSyntaxError = kit->eSyntaxError;
  rb_eLoadError = kit->eLoadError;
  rb_mErrno = kit->mErrno;
  ruby_top_self = kit->oMain;
  ruby_scope = kit->scope;
#ifdef FFSAFE
  rb_global_tbl = kit->globals;
  rb_cMatch = kit->cMatch;
  rb_cMethod = kit->cMethod;
  rb_cUnboundMethod = kit->cUnboundMethod;
  rb_eRegexpError = kit->eRegexpError;
  rb_cNameErrorMesg = kit->cNameErrorMesg;
  rb_eSysStackError = kit->eSysStackError;
  rb_eLocalJumpError = kit->eLocalJumpError;
#endif

  go = ALLOC(go_cart);
  go->argv = ALLOC(VALUE);
  go->norm = norm;
  go->kit = kit;
  go->kit->banished = go->norm;
  return go;
}

VALUE
sandbox_go_go_go(go)
  go_cart *go;
{
  return rb_mod_module_eval(1, go->argv, go->kit->cObject);
}

VALUE
sandbox_eval( self, str )
  VALUE self, str;
{
  go_cart *go = sandbox_swap_in( self );
  go->argv[0] = str;
  return rb_ensure(sandbox_go_go_go, (VALUE)go, sandbox_whoa_whoa_whoa, (VALUE)go);
}

VALUE
sandbox_load_begin(go)
  go_cart *go;
{
  go->argv[0] = rb_funcall(rb_cFile, rb_intern("read"), 1, go->argv[0]);
  return sandbox_go_go_go(go);
}

VALUE
sandbox_load( self, path )
  VALUE self, path;
{
  // rb_load(path, 0);
  go_cart *go = sandbox_swap_in( self );
  go->argv[0] = path;
  return rb_ensure(sandbox_load_begin, (VALUE)go, sandbox_whoa_whoa_whoa, (VALUE)go);
}

VALUE
sandbox_import( self, klass )
  VALUE self, klass;
{
  VALUE sandklass;
  sandkit *kit;
  Data_Get_Struct( self, sandkit, kit );
  sandklass = sandbox_import_class_path( kit, rb_class2name( klass ) );
  return Qnil;
}

VALUE
sandbox_safe_go_go_go(go)
  go_cart *go;
{
  return rb_marshal_dump(rb_mod_module_eval(1, go->argv, go->kit->cObject), Qnil);
}

VALUE
sandbox_safe_eval( self, str )
  VALUE self, str;
{
  VALUE marshed;
  go_cart *go = sandbox_swap_in( self );
  go->argv[0] = str;
  marshed = rb_ensure(sandbox_safe_go_go_go, (VALUE)go, sandbox_whoa_whoa_whoa, (VALUE)go);
  return rb_marshal_load(marshed);
}

VALUE
sandbox_safe_load_begin(go)
  go_cart *go;
{
  go->argv[0] = rb_funcall(rb_cFile, rb_intern("read"), 1, go->argv[0]);
  return sandbox_safe_go_go_go(go);
}

VALUE
sandbox_safe_load( self, path )
  VALUE self, path;
{
  VALUE marshed;
  go_cart *go = sandbox_swap_in( self );
  go->argv[0] = path;
  marshed = rb_ensure(sandbox_safe_load_begin, (VALUE)go, sandbox_whoa_whoa_whoa, (VALUE)go);
  return rb_marshal_load(marshed);
}

#ifdef FFSAFE
static VALUE cSandboxSafe;

VALUE sandbox_safe( klass )
  VALUE klass;
{
  return rb_funcall( cSandboxSafe, rb_intern("new"), 0 );
}
#endif

static void
Init_kit(kit)
  sandkit *kit;
{
  VALUE metaclass;

  kit->tbl = st_init_numtable();
#ifdef FFSAFE
  kit->globals = st_init_numtable();
#endif
  kit->cObject = 0;

  kit->cObject = sandbox_defclass(kit, "Object", 0);
  kit->cModule = sandbox_defclass(kit, "Module", kit->cObject);
  kit->cClass =  sandbox_defclass(kit, "Class",  kit->cModule);

  metaclass = sandbox_metaclass(kit, kit->cObject, kit->cClass);
  metaclass = sandbox_metaclass(kit, kit->cModule, metaclass);
  metaclass = sandbox_metaclass(kit, kit->cClass, metaclass);

  kit->mKernel = sandbox_defmodule(kit, "Kernel");
  rb_include_module(kit->cObject, kit->mKernel);
  rb_define_alloc_func(kit->cObject, sandbox_alloc_obj);

  rb_define_private_method(kit->cModule, "method_added", sandbox_dummy, 1);
  rb_define_private_method(kit->cObject, "initialize", sandbox_dummy, 0);
  rb_define_private_method(kit->cClass, "inherited", sandbox_dummy, 1);
  rb_define_private_method(kit->cModule, "included", sandbox_dummy, 1);
  rb_define_private_method(kit->cModule, "extended", sandbox_dummy, 1);
  rb_define_private_method(kit->cModule, "method_removed", sandbox_dummy, 1);
  rb_define_private_method(kit->cModule, "method_undefined", sandbox_dummy, 1);

  SAND_COPY(mKernel, "nil?");
  SAND_COPY(mKernel, "==");
  SAND_COPY(mKernel, "equal?");
  SAND_COPY(mKernel, "===");
  SAND_COPY(mKernel, "=~");

  SAND_COPY(mKernel, "eql?");

  SAND_COPY(mKernel, "id");
  SAND_COPY(mKernel, "type");
  SAND_COPY(mKernel, "class");

  SAND_COPY(mKernel, "clone");
  SAND_COPY(mKernel, "dup");
  SAND_COPY(mKernel, "initialize_copy");

  SAND_COPY(mKernel, "taint");
  SAND_COPY(mKernel, "tainted?");
  SAND_COPY(mKernel, "untaint");
  SAND_COPY(mKernel, "freeze");
  SAND_COPY(mKernel, "frozen?");

  SAND_COPY(mKernel, "to_a");
  SAND_COPY(mKernel, "to_s");
  SAND_COPY(mKernel, "inspect");
  SAND_COPY(mKernel, "methods");

  SAND_COPY(mKernel, "singleton_methods");
  SAND_COPY(mKernel, "protected_methods");
  SAND_COPY(mKernel, "private_methods");
  SAND_COPY(mKernel, "public_methods");
  SAND_COPY(mKernel, "instance_variables");
  SAND_COPY(mKernel, "instance_variable_get");
  SAND_COPY(mKernel, "instance_variable_set");
  SAND_COPY(mKernel, "remove_instance_variable");

  SAND_COPY(mKernel, "instance_of?");
  SAND_COPY(mKernel, "kind_of?");
  SAND_COPY(mKernel, "is_a?");

  SAND_COPY(mKernel, "singleton_method_added");
  SAND_COPY(mKernel, "singleton_method_removed");
  SAND_COPY(mKernel, "singleton_method_undefined");

  SAND_COPY_KERNEL("sprintf");
  SAND_COPY_KERNEL("format"); 

  SAND_COPY_KERNEL("Integer");
  SAND_COPY_KERNEL("Float");

  SAND_COPY_KERNEL("String");
  SAND_COPY_KERNEL("Array");

  kit->cNilClass = sandbox_defclass(kit, "NilClass", kit->cObject);
  SAND_COPY(cNilClass, "to_i");
  SAND_COPY(cNilClass, "to_f");
  SAND_COPY(cNilClass, "to_s");
  SAND_COPY(cNilClass, "to_a");
  SAND_COPY(cNilClass, "inspect");
  SAND_COPY(cNilClass, "&");
  SAND_COPY(cNilClass, "|");
  SAND_COPY(cNilClass, "^");

  SAND_COPY(cNilClass, "nil?");
  rb_undef_alloc_func(kit->cNilClass);
  SAND_UNDEF(cNilClass, "new");
  SAND_COPY_CONST(cObject, "NIL");

  kit->cSymbol = sandbox_defclass(kit, "Symbol", kit->cObject);
  SAND_COPY_S(cSymbol, "all_symbols");
  rb_undef_alloc_func(kit->cSymbol);
  SAND_UNDEF(cSymbol, "new");

  SAND_COPY(cSymbol, "to_i");
  SAND_COPY(cSymbol, "to_int");
  SAND_COPY(cSymbol, "inspect");
  SAND_COPY(cSymbol, "to_s");
  SAND_COPY(cSymbol, "id2name");
  SAND_COPY(cSymbol, "to_sym");
  SAND_COPY(cSymbol, "===");

  SAND_COPY(cModule, "freeze");
  SAND_COPY(cModule, "===");
  SAND_COPY(cModule, "==");
  SAND_COPY(cModule, "<=>");
  SAND_COPY(cModule, "<");
  SAND_COPY(cModule, "<=");
  SAND_COPY(cModule, ">");
  SAND_COPY(cModule, ">=");
  SAND_COPY(cModule, "initialize_copy");
  SAND_COPY(cModule, "to_s");
  SAND_COPY(cModule, "included_modules");
  SAND_COPY(cModule, "include?");
  SAND_COPY(cModule, "name");
  SAND_COPY(cModule, "ancestors");

  SAND_COPY(cModule, "attr");
  SAND_COPY(cModule, "attr_reader");
  SAND_COPY(cModule, "attr_writer");
  SAND_COPY(cModule, "attr_accessor");

  SAND_COPY_ALLOC(cModule);
  SAND_COPY(cModule, "initialize");
  SAND_COPY(cModule, "instance_methods");
  SAND_COPY(cModule, "public_instance_methods");
  SAND_COPY(cModule, "protected_instance_methods");
  SAND_COPY(cModule, "private_instance_methods");

  SAND_COPY(cModule, "constants");
  SAND_COPY(cModule, "const_get");
  SAND_COPY(cModule, "const_set");
  SAND_COPY(cModule, "const_defined?");
  SAND_COPY(cModule, "remove_const");
  SAND_COPY(cModule, "const_missing");
  SAND_COPY(cModule, "class_variables");
  SAND_COPY(cModule, "remove_class_variable");
  SAND_COPY(cModule, "class_variable_get");
  SAND_COPY(cModule, "class_variable_set");

  SAND_COPY(cClass, "allocate");
  SAND_COPY(cClass, "new");
  SAND_COPY(cClass, "initialize");
  SAND_COPY(cClass, "initialize_copy");
  SAND_COPY(cClass, "superclass");
  SAND_COPY_ALLOC(cClass);
  rb_undef_method(kit->cClass, "extend_object");
  rb_undef_method(kit->cClass, "append_features");

  kit->cData = sandbox_defclass(kit, "Data", kit->cObject);
  rb_undef_alloc_func(kit->cData);

  rb_global_variable(&kit->oMain);
  kit->oMain = rb_obj_alloc(kit->cObject);
  SAND_COPY_MAIN("to_s");

  kit->cTrueClass = sandbox_defclass(kit, "TrueClass", kit->cObject);
  SAND_COPY(cTrueClass, "to_s");
  SAND_COPY(cTrueClass, "&");
  SAND_COPY(cTrueClass, "|");
  SAND_COPY(cTrueClass, "^");
  rb_undef_alloc_func(kit->cTrueClass);
  SAND_UNDEF(cTrueClass, "new");
  SAND_COPY_CONST(cObject, "TRUE");

  kit->cFalseClass = sandbox_defclass(kit, "FalseClass", kit->cObject);
  SAND_COPY(cFalseClass, "to_s");
  SAND_COPY(cFalseClass, "&");
  SAND_COPY(cFalseClass, "|");
  SAND_COPY(cFalseClass, "^");
  rb_undef_alloc_func(kit->cFalseClass);
  SAND_UNDEF(cFalseClass, "new");
  SAND_COPY_CONST(cObject, "FALSE");

  kit->mEnumerable = sandbox_defmodule(kit, "Enumerable");
  SAND_COPY(mEnumerable,"to_a");
  SAND_COPY(mEnumerable,"entries");

  SAND_COPY(mEnumerable,"sort");
  SAND_COPY(mEnumerable,"sort_by");
  SAND_COPY(mEnumerable,"grep");
  SAND_COPY(mEnumerable,"find");
  SAND_COPY(mEnumerable,"detect");
  SAND_COPY(mEnumerable,"find_all");
  SAND_COPY(mEnumerable,"select");
  SAND_COPY(mEnumerable,"reject");
  SAND_COPY(mEnumerable,"collect");
  SAND_COPY(mEnumerable,"map");
  SAND_COPY(mEnumerable,"inject");
  SAND_COPY(mEnumerable,"partition");
  SAND_COPY(mEnumerable,"all?");
  SAND_COPY(mEnumerable,"any?");
  SAND_COPY(mEnumerable,"min");
  SAND_COPY(mEnumerable,"max");
  SAND_COPY(mEnumerable,"member?");
  SAND_COPY(mEnumerable,"include?");
  SAND_COPY(mEnumerable,"each_with_index");
  SAND_COPY(mEnumerable, "zip");

  kit->mComparable = sandbox_defmodule(kit, "Comparable");
  SAND_COPY(mComparable, "==");
  SAND_COPY(mComparable, ">");
  SAND_COPY(mComparable, ">=");
  SAND_COPY(mComparable, "<");
  SAND_COPY(mComparable, "<=");
  SAND_COPY(mComparable, "between?");

  kit->mPrecision = sandbox_defmodule(kit, "Precision");
  SAND_COPY_S(mPrecision, "included");
  SAND_COPY(mPrecision, "prec");
  SAND_COPY(mPrecision, "prec_i");
  SAND_COPY(mPrecision, "prec_f");

  /*
  rb_global_variable((VALUE*)&top_scope);
  rb_global_variable((VALUE*)&ruby_eval_tree_begin);

  rb_global_variable((VALUE*)&ruby_eval_tree);
  rb_global_variable((VALUE*)&ruby_dyna_vars);

  rb_define_virtual_variable("$@", errat_getter, errat_setter);
  rb_define_hooked_variable("$!", &ruby_errinfo, 0, errinfo_setter);
  */

  SAND_COPY_KERNEL("eval");
  SAND_COPY_KERNEL("iterator?");
  SAND_COPY_KERNEL("block_given?");
  SAND_COPY_KERNEL("method_missing");
  SAND_COPY_KERNEL("loop");

  SAND_COPY(mKernel, "respond_to?");
  
  SAND_COPY_KERNEL("raise");
  SAND_COPY_KERNEL("fail");

  /* FIXME: reimplement and test_exploits.rb(test_caller) */
  /* SAND_COPY_KERNEL("caller"); */

  SAND_COPY_KERNEL("exit");
  SAND_COPY_KERNEL("abort");

  /* FIXME: reimplement */
  /* SAND_COPY_KERNEL("at_exit"); */

  SAND_COPY_KERNEL("catch");
  SAND_COPY_KERNEL("throw");
  SAND_COPY_KERNEL("global_variables");
  SAND_COPY_KERNEL("local_variables");

  SAND_COPY(mKernel, "send");
  SAND_COPY(mKernel, "__send__");
  SAND_COPY(mKernel, "instance_eval");

  SAND_COPY(cModule, "append_features");
  SAND_COPY(cModule, "extend_object");
  SAND_COPY(cModule, "include");
  SAND_COPY(cModule, "public");
  SAND_COPY(cModule, "protected");
  SAND_COPY(cModule, "private");
  SAND_COPY(cModule, "module_function");
  SAND_COPY(cModule, "method_defined?");
  SAND_COPY(cModule, "public_method_defined?");
  SAND_COPY(cModule, "private_method_defined?");
  SAND_COPY(cModule, "protected_method_defined?");
  SAND_COPY(cModule, "public_class_method");
  SAND_COPY(cModule, "private_class_method");
  SAND_COPY(cModule, "module_eval");
  SAND_COPY(cModule, "class_eval");

  rb_undef_method(kit->cClass, "module_function");

  SAND_COPY(cModule, "remove_method");
  SAND_COPY(cModule, "undef_method");
  SAND_COPY(cModule, "alias_method");
  SAND_COPY(cModule, "define_method");

  SAND_COPY_S(cModule, "nesting");
  SAND_COPY_S(cModule, "constants");

  SAND_COPY_MAIN("include");
  SAND_COPY_MAIN("public");
  SAND_COPY_MAIN("private");

  SAND_COPY(mKernel, "extend");

  /*
  rb_define_global_function("trace_var", rb_f_trace_var, -1);
  rb_define_global_function("untrace_var", rb_f_untrace_var, -1);

  rb_define_global_function("set_trace_func", set_trace_func, 1);
  rb_global_variable(&trace_func);

  rb_define_virtual_variable("$SAFE", safe_getter, safe_setter);
  */

  kit->cString  = sandbox_defclass(kit, "String", kit->cObject);
  rb_include_module(kit->cString, kit->mComparable);
  rb_include_module(kit->cString, kit->mEnumerable);
  SAND_COPY_ALLOC(cString);
  SAND_COPY(cString, "initialize");
  SAND_COPY(cString, "initialize_copy");
  SAND_COPY(cString, "<=>");
  SAND_COPY(cString, "==");
  SAND_COPY(cString, "eql?");
  SAND_COPY(cString, "hash");
  SAND_COPY(cString, "casecmp");
  SAND_COPY(cString, "+");
  SAND_COPY(cString, "*");
  SAND_COPY(cString, "%");
  SAND_COPY(cString, "[]");
  SAND_COPY(cString, "[]=");
  SAND_COPY(cString, "insert");
  SAND_COPY(cString, "length");
  SAND_COPY(cString, "size");
  SAND_COPY(cString, "empty?");
  SAND_COPY(cString, "=~");
  SAND_COPY(cString, "match");
  SAND_COPY(cString, "succ");
  SAND_COPY(cString, "succ!");
  SAND_COPY(cString, "next");
  SAND_COPY(cString, "next!");
  SAND_COPY(cString, "upto");
  SAND_COPY(cString, "index");
  SAND_COPY(cString, "rindex");
  SAND_COPY(cString, "replace");

  SAND_COPY(cString, "to_i");
  SAND_COPY(cString, "to_f");
  SAND_COPY(cString, "to_s");
  SAND_COPY(cString, "to_str");
  SAND_COPY(cString, "inspect");
  SAND_COPY(cString, "dump");

  SAND_COPY(cString, "upcase");
  SAND_COPY(cString, "downcase");
  SAND_COPY(cString, "capitalize");
  SAND_COPY(cString, "swapcase");

  SAND_COPY(cString, "upcase!");
  SAND_COPY(cString, "downcase!");
  SAND_COPY(cString, "capitalize!");
  SAND_COPY(cString, "swapcase!");

  SAND_COPY(cString, "hex");
  SAND_COPY(cString, "oct");
  SAND_COPY(cString, "split");
  SAND_COPY(cString, "reverse");
  SAND_COPY(cString, "reverse!");
  SAND_COPY(cString, "concat");
  SAND_COPY(cString, "<<");
  SAND_COPY(cString, "crypt");
  SAND_COPY(cString, "intern");
  SAND_COPY(cString, "to_sym");

  SAND_COPY(cString, "include?");

  SAND_COPY(cString, "scan");

  SAND_COPY(cString, "ljust");
  SAND_COPY(cString, "rjust");
  SAND_COPY(cString, "center");

  SAND_COPY(cString, "sub");
  SAND_COPY(cString, "gsub");
  SAND_COPY(cString, "chop");
  SAND_COPY(cString, "chomp");
  SAND_COPY(cString, "strip");
  SAND_COPY(cString, "lstrip");
  SAND_COPY(cString, "rstrip");

  SAND_COPY(cString, "sub!");
  SAND_COPY(cString, "gsub!");
  SAND_COPY(cString, "chop!");
  SAND_COPY(cString, "chomp!");
  SAND_COPY(cString, "strip!");
  SAND_COPY(cString, "lstrip!");
  SAND_COPY(cString, "rstrip!");

  SAND_COPY(cString, "tr");
  SAND_COPY(cString, "tr_s");
  SAND_COPY(cString, "delete");
  SAND_COPY(cString, "squeeze");
  SAND_COPY(cString, "count");

  SAND_COPY(cString, "tr!");
  SAND_COPY(cString, "tr_s!");
  SAND_COPY(cString, "delete!");
  SAND_COPY(cString, "squeeze!");

  SAND_COPY(cString, "each_line");
  SAND_COPY(cString, "each");
  SAND_COPY(cString, "each_byte");

  SAND_COPY(cString, "sum");

  SAND_COPY_KERNEL("sub");
  SAND_COPY_KERNEL("gsub");
  SAND_COPY_KERNEL("sub!");
  SAND_COPY_KERNEL("gsub!");
  SAND_COPY_KERNEL("chop");
  SAND_COPY_KERNEL("chop!");
  SAND_COPY_KERNEL("chomp");
  SAND_COPY_KERNEL("chomp!");
  SAND_COPY_KERNEL("split");
  SAND_COPY_KERNEL("scan");

  SAND_COPY(cString, "slice");
  SAND_COPY(cString, "slice!");

  /*
  rb_fs = Qnil;
  rb_define_variable("$;", &rb_fs);
  rb_define_variable("$-F", &rb_fs);
  */

  kit->eException = sandbox_defclass(kit, "Exception", kit->cObject);
  SAND_COPY_S(eException, "exception");
  SAND_COPY(eException, "exception");
  SAND_COPY(eException, "initialize");
  SAND_COPY(eException, "to_s");
  SAND_COPY(eException, "to_str");
  SAND_COPY(eException, "message");
  SAND_COPY(eException, "inspect");
  SAND_COPY(eException, "backtrace");
  SAND_COPY(eException, "set_backtrace");

  kit->eSystemExit = sandbox_defclass(kit, "SystemExit", kit->eException);
  SAND_COPY(eSystemExit, "initialize");
  SAND_COPY(eSystemExit, "status");
  SAND_COPY(eSystemExit, "success?");

  kit->eFatal = sandbox_defclass(kit, "fatal", kit->eException);
  kit->eSignal = sandbox_defclass(kit, "SignalException", kit->eException);
  kit->eInterrupt = sandbox_defclass(kit, "Interrupt", kit->eSignal);

  kit->eStandardError = sandbox_defclass(kit, "StandardError", kit->eException);
  kit->eTypeError = sandbox_defclass(kit, "TypeError", kit->eStandardError);
  kit->eArgError = sandbox_defclass(kit, "ArgumentError", kit->eStandardError);
  kit->eIndexError = sandbox_defclass(kit, "IndexError", kit->eStandardError);
  kit->eRangeError = sandbox_defclass(kit, "RangeError", kit->eStandardError);
  kit->eNameError = sandbox_defclass(kit, "NameError", kit->eStandardError);
  SAND_COPY(eNameError, "initialize");
  SAND_COPY(eNameError, "name");
  SAND_COPY(eNameError, "to_s");
#ifndef FFSAFE
  VALUE rb_cNameErrorMesg = rb_const_get_at(rb_eNameError, rb_intern("message"));
#endif
  kit->cNameErrorMesg = rb_define_class_under(kit->eNameError, "message", kit->cData);
  SAND_COPY(cNameErrorMesg, "to_str");
  SAND_COPY(cNameErrorMesg, "_dump");
  if (rb_respond_to(rb_cNameErrorMesg, rb_intern("!"))) {
    SAND_COPY_S(cNameErrorMesg, "!");
    SAND_COPY_S(cNameErrorMesg, "_load");
  } else {
    SAND_COPY(cNameErrorMesg, "!");
    SAND_COPY(cNameErrorMesg, "_load");
  }
  kit->eNoMethodError = sandbox_defclass(kit, "NoMethodError", kit->eNameError);
  SAND_COPY(eNoMethodError, "initialize");
  SAND_COPY(eNoMethodError, "args");

  kit->eScriptError = sandbox_defclass(kit, "ScriptError", kit->eException);
  kit->eSyntaxError = sandbox_defclass(kit, "SyntaxError", kit->eScriptError);
  kit->eLoadError = sandbox_defclass(kit, "LoadError", kit->eScriptError);
  kit->eNotImpError = sandbox_defclass(kit, "NotImplementedError", kit->eScriptError);

  kit->eRuntimeError = sandbox_defclass(kit, "RuntimeError", kit->eStandardError);
  kit->eSecurityError = sandbox_defclass(kit, "SecurityError", kit->eStandardError);
  kit->eNoMemError = sandbox_defclass(kit, "NoMemoryError", kit->eException);

  /*
  syserr_tbl = st_init_numtable();
  */
#ifndef FFSAFE
  VALUE rb_eSystemCallError = rb_const_get(rb_cObject, rb_intern("SystemCallError"));
#endif
  kit->eSystemCallError = sandbox_defclass(kit, "SystemCallError", kit->eStandardError);
  SAND_COPY(eSystemCallError, "initialize");
  SAND_COPY(eSystemCallError, "errno");
  SAND_COPY_S(eSystemCallError, "===");

  kit->mErrno = sandbox_defmodule(kit, "Errno");
  /*
  rb_define_global_function("warn", rb_warn_m, 1);
  */

#ifndef FFSAFE
  VALUE rb_eLocalJumpError = rb_const_get(rb_cObject, rb_intern("LocalJumpError"));
#endif
  kit->eLocalJumpError = sandbox_defclass(kit, "LocalJumpError", kit->eStandardError);
  SAND_COPY(eLocalJumpError, "exit_value");
  SAND_COPY(eLocalJumpError, "reason");

/*
  rb_global_variable(&exception_error);
  exception_error = rb_exc_new2(rb_eFatal, "exception reentered");
*/

#ifndef FFSAFE
  VALUE rb_eSysStackError = rb_const_get(rb_cObject, rb_intern("SystemStackError"));
#endif
  kit->eSysStackError = sandbox_defclass(kit, "SystemStackError", kit->eStandardError);
/*
  FIXME: 1.8.5 needs the sysstack_error and exception_error exposed. 
         Or does it?  Can the user really catch this since it's not descended from Object?
         Will the global and a general rescue expose a hole?
  FIXME: ruby_errinfo? trace_func? cont_protect?
  rb_global_variable(&sysstack_error);
  sysstack_error = rb_exc_new2(rb_eSysStackError, "stack level too deep");
  OBJ_TAINT(sysstack_error);
*/

  kit->cProc = sandbox_defclass(kit, "Proc", kit->cObject);
  rb_undef_alloc_func(kit->cProc);
  SAND_COPY_S(cProc, "new");

  SAND_COPY(cProc, "clone");
  SAND_COPY(cProc, "dup");
  SAND_COPY(cProc, "call");
  SAND_COPY(cProc, "arity");
  SAND_COPY(cProc, "[]");
  SAND_COPY(cProc, "==");
  SAND_COPY(cProc, "to_s");
  SAND_COPY(cProc, "to_proc");
  SAND_COPY(cProc, "binding");

  SAND_COPY_KERNEL("proc");
  SAND_COPY_KERNEL("lambda");

#ifndef FFSAFE
  VALUE rb_cMethod= rb_const_get(rb_cObject, rb_intern("Method"));
#endif
  kit->cMethod = sandbox_defclass(kit, "Method", kit->cObject);
  rb_undef_alloc_func(kit->cMethod);
  SAND_UNDEF(cMethod, "new");
  SAND_COPY(cMethod, "==");
  SAND_COPY(cMethod, "clone");
  SAND_COPY(cMethod, "call");
  SAND_COPY(cMethod, "[]");
  SAND_COPY(cMethod, "arity");
  SAND_COPY(cMethod, "inspect");
  SAND_COPY(cMethod, "to_s");
  SAND_COPY(cMethod, "to_proc");
  SAND_COPY(cMethod, "unbind");
  SAND_COPY(mKernel, "method");

#ifndef FFSAFE
  VALUE rb_cUnboundMethod = rb_const_get(rb_cObject, rb_intern("UnboundMethod"));
#endif
  kit->cUnboundMethod = sandbox_defclass(kit, "UnboundMethod", kit->cObject);
  rb_undef_alloc_func(kit->cUnboundMethod);
  SAND_UNDEF(cUnboundMethod, "new");
  SAND_COPY(cUnboundMethod, "==");
  SAND_COPY(cUnboundMethod, "clone");
  SAND_COPY(cUnboundMethod, "arity");
  SAND_COPY(cUnboundMethod, "inspect");
  SAND_COPY(cUnboundMethod, "to_s");
  SAND_COPY(cUnboundMethod, "bind");
  SAND_COPY(cModule, "instance_method");

  kit->eZeroDivError = sandbox_defclass(kit, "ZeroDivisionError", kit->eStandardError);
  kit->eFloatDomainError = sandbox_defclass(kit, "FloatDomainError", kit->eRangeError);
  kit->cNumeric = sandbox_defclass(kit, "Numeric", kit->cObject);

  SAND_COPY(cNumeric, "singleton_method_added");
  rb_include_module(kit->cNumeric, kit->mComparable);
  SAND_COPY(cNumeric, "initialize_copy");
  SAND_COPY(cNumeric, "coerce");

  SAND_COPY(cNumeric, "+@");
  SAND_COPY(cNumeric, "-@");
  SAND_COPY(cNumeric, "<=>");
  SAND_COPY(cNumeric, "eql?");
  SAND_COPY(cNumeric, "quo");
  SAND_COPY(cNumeric, "div");
  SAND_COPY(cNumeric, "divmod");
  SAND_COPY(cNumeric, "modulo");
  SAND_COPY(cNumeric, "remainder");
  SAND_COPY(cNumeric, "abs");
  SAND_COPY(cNumeric, "to_int");

  SAND_COPY(cNumeric, "integer?");
  SAND_COPY(cNumeric, "zero?");
  SAND_COPY(cNumeric, "nonzero?");

  SAND_COPY(cNumeric, "floor");
  SAND_COPY(cNumeric, "ceil");
  SAND_COPY(cNumeric, "round");
  SAND_COPY(cNumeric, "truncate");
  SAND_COPY(cNumeric, "step");

  kit->cInteger = sandbox_defclass(kit, "Integer", kit->cNumeric);
  rb_undef_alloc_func(kit->cInteger);
  SAND_UNDEF(cInteger, "new");

  SAND_COPY(cInteger, "integer?");
  SAND_COPY(cInteger, "upto");
  SAND_COPY(cInteger, "downto");
  SAND_COPY(cInteger, "times");
  rb_include_module(kit->cInteger, kit->mPrecision);
  SAND_COPY(cInteger, "succ");
  SAND_COPY(cInteger, "next");
  SAND_COPY(cInteger, "chr");
  SAND_COPY(cInteger, "to_i");
  SAND_COPY(cInteger, "to_int");
  SAND_COPY(cInteger, "floor");
  SAND_COPY(cInteger, "ceil");
  SAND_COPY(cInteger, "round");
  SAND_COPY(cInteger, "truncate");

  kit->cFixnum = sandbox_defclass(kit, "Fixnum", kit->cInteger);
  rb_include_module(kit->cFixnum, kit->mPrecision);
  SAND_COPY_S(cFixnum, "induced_from");
  SAND_COPY_S(cInteger, "induced_from");

  SAND_COPY(cFixnum, "to_s");

  SAND_COPY(cFixnum, "id2name");
  SAND_COPY(cFixnum, "to_sym");

  SAND_COPY(cFixnum, "-@");
  SAND_COPY(cFixnum, "+");
  SAND_COPY(cFixnum, "-");
  SAND_COPY(cFixnum, "*");
  SAND_COPY(cFixnum, "/");
  SAND_COPY(cFixnum, "div");
  SAND_COPY(cFixnum, "%");
  SAND_COPY(cFixnum, "modulo");
  SAND_COPY(cFixnum, "divmod");
  SAND_COPY(cFixnum, "quo");
  SAND_COPY(cFixnum, "**");

  SAND_COPY(cFixnum, "abs");

  SAND_COPY(cFixnum, "==");
  SAND_COPY(cFixnum, "<=>");
  SAND_COPY(cFixnum, ">");
  SAND_COPY(cFixnum, ">=");
  SAND_COPY(cFixnum, "<");
  SAND_COPY(cFixnum, "<=");

  SAND_COPY(cFixnum, "~");
  SAND_COPY(cFixnum, "&");
  SAND_COPY(cFixnum, "|");
  SAND_COPY(cFixnum, "^");
  SAND_COPY(cFixnum, "[]");

  SAND_COPY(cFixnum, "<<");
  SAND_COPY(cFixnum, ">>");

  SAND_COPY(cFixnum, "to_f");
  SAND_COPY(cFixnum, "size");
  SAND_COPY(cFixnum, "zero?");

  kit->cFloat = sandbox_defclass(kit, "Float", kit->cNumeric);

  rb_undef_alloc_func(kit->cFloat);
  SAND_UNDEF(cFloat, "new");

  SAND_COPY_S(cFloat, "induced_from");
  rb_include_module(kit->cFloat, kit->mPrecision);

  SAND_COPY_CONST(cFloat, "ROUNDS");
  SAND_COPY_CONST(cFloat, "RADIX");
  SAND_COPY_CONST(cFloat, "MANT_DIG");
  SAND_COPY_CONST(cFloat, "DIG");
  SAND_COPY_CONST(cFloat, "MIN_EXP");
  SAND_COPY_CONST(cFloat, "MAX_EXP");
  SAND_COPY_CONST(cFloat, "MIN_10_EXP");
  SAND_COPY_CONST(cFloat, "MAX_10_EXP");
  SAND_COPY_CONST(cFloat, "MIN");
  SAND_COPY_CONST(cFloat, "MAX");
  SAND_COPY_CONST(cFloat, "EPSILON");

  SAND_COPY(cFloat, "to_s");
  SAND_COPY(cFloat, "coerce");
  SAND_COPY(cFloat, "-@");
  SAND_COPY(cFloat, "+");
  SAND_COPY(cFloat, "-");
  SAND_COPY(cFloat, "*");
  SAND_COPY(cFloat, "/");
  SAND_COPY(cFloat, "%");
  SAND_COPY(cFloat, "modulo");
  SAND_COPY(cFloat, "divmod");
  SAND_COPY(cFloat, "**");
  SAND_COPY(cFloat, "==");
  SAND_COPY(cFloat, "<=>");
  SAND_COPY(cFloat, ">");
  SAND_COPY(cFloat, ">=");
  SAND_COPY(cFloat, "<");
  SAND_COPY(cFloat, "<=");
  SAND_COPY(cFloat, "eql?");
  SAND_COPY(cFloat, "hash");
  SAND_COPY(cFloat, "to_f");
  SAND_COPY(cFloat, "abs");
  SAND_COPY(cFloat, "zero?");

  SAND_COPY(cFloat, "to_i");
  SAND_COPY(cFloat, "to_int");
  SAND_COPY(cFloat, "floor");
  SAND_COPY(cFloat, "ceil");
  SAND_COPY(cFloat, "round");
  SAND_COPY(cFloat, "truncate");

  SAND_COPY(cFloat, "nan?");
  SAND_COPY(cFloat, "infinite?");
  SAND_COPY(cFloat, "finite?");

  kit->cBignum = sandbox_defclass(kit, "Bignum", kit->cInteger);

  SAND_COPY(cBignum, "to_s");
  SAND_COPY(cBignum, "coerce");
  SAND_COPY(cBignum, "-@");
  SAND_COPY(cBignum, "+");
  SAND_COPY(cBignum, "-");
  SAND_COPY(cBignum, "*");
  SAND_COPY(cBignum, "/");
  SAND_COPY(cBignum, "%");
  SAND_COPY(cBignum, "div");
  SAND_COPY(cBignum, "divmod");
  SAND_COPY(cBignum, "modulo");
  SAND_COPY(cBignum, "remainder");
  SAND_COPY(cBignum, "quo");
  SAND_COPY(cBignum, "**");
  SAND_COPY(cBignum, "&");
  SAND_COPY(cBignum, "|");
  SAND_COPY(cBignum, "^");
  SAND_COPY(cBignum, "~");
  SAND_COPY(cBignum, "<<");
  SAND_COPY(cBignum, ">>");
  SAND_COPY(cBignum, "[]");

  SAND_COPY(cBignum, "<=>");
  SAND_COPY(cBignum, "==");
  SAND_COPY(cBignum, "eql?");
  SAND_COPY(cBignum, "hash");
  SAND_COPY(cBignum, "to_f");
  SAND_COPY(cBignum, "abs");
  SAND_COPY(cBignum, "size");

  /* FIXME: rb_output_fs */
  kit->cArray  = sandbox_defclass(kit, "Array", kit->cObject);
  rb_include_module(kit->cArray, kit->mEnumerable);

  SAND_COPY_ALLOC(cArray);
  SAND_COPY_S(cArray, "[]");
  SAND_COPY(cArray, "initialize");
  SAND_COPY(cArray, "initialize_copy");

  SAND_COPY(cArray, "to_s");
  SAND_COPY(cArray, "inspect");
  SAND_COPY(cArray, "to_a");
  SAND_COPY(cArray, "to_ary");
  SAND_COPY(cArray, "frozen?");

  SAND_COPY(cArray, "==");
  SAND_COPY(cArray, "eql?");
  SAND_COPY(cArray, "hash");

  SAND_COPY(cArray, "[]");
  SAND_COPY(cArray, "[]=");
  SAND_COPY(cArray, "at");
  SAND_COPY(cArray, "fetch");
  SAND_COPY(cArray, "first");
  SAND_COPY(cArray, "last");
  SAND_COPY(cArray, "concat");
  SAND_COPY(cArray, "<<");
  SAND_COPY(cArray, "push");
  SAND_COPY(cArray, "pop");
  SAND_COPY(cArray, "shift");
  SAND_COPY(cArray, "unshift");
  SAND_COPY(cArray, "insert");
  SAND_COPY(cArray, "each");
  SAND_COPY(cArray, "each_index");
  SAND_COPY(cArray, "reverse_each");
  SAND_COPY(cArray, "length");
  rb_define_alias(kit->cArray,  "size", "length");
  SAND_COPY(cArray, "empty?");
  SAND_COPY(cArray, "index");
  SAND_COPY(cArray, "rindex");
  SAND_COPY(cArray, "indexes");
  SAND_COPY(cArray, "indices");
  SAND_COPY(cArray, "join");
  SAND_COPY(cArray, "reverse");
  SAND_COPY(cArray, "reverse!");
  SAND_COPY(cArray, "sort");
  SAND_COPY(cArray, "sort!");
  SAND_COPY(cArray, "collect");
  SAND_COPY(cArray, "collect!");
  SAND_COPY(cArray, "map");
  SAND_COPY(cArray, "map!");
  SAND_COPY(cArray, "select");
  SAND_COPY(cArray, "values_at");
  SAND_COPY(cArray, "delete");
  SAND_COPY(cArray, "delete_at");
  SAND_COPY(cArray, "delete_if");
  SAND_COPY(cArray, "reject");
  SAND_COPY(cArray, "reject!");
  SAND_COPY(cArray, "zip");
  SAND_COPY(cArray, "transpose");
  SAND_COPY(cArray, "replace");
  SAND_COPY(cArray, "clear");
  SAND_COPY(cArray, "fill");
  SAND_COPY(cArray, "include?");
  SAND_COPY(cArray, "<=>");

  SAND_COPY(cArray, "slice");
  SAND_COPY(cArray, "slice!");

  SAND_COPY(cArray, "assoc");
  SAND_COPY(cArray, "rassoc");

  SAND_COPY(cArray, "+");
  SAND_COPY(cArray, "*");

  SAND_COPY(cArray, "-");
  SAND_COPY(cArray, "&");
  SAND_COPY(cArray, "|");

  SAND_COPY(cArray, "uniq");
  SAND_COPY(cArray, "uniq!");
  SAND_COPY(cArray, "compact");
  SAND_COPY(cArray, "compact!");
  SAND_COPY(cArray, "flatten");
  SAND_COPY(cArray, "flatten!");
  SAND_COPY(cArray, "nitems");

  kit->cHash = sandbox_defclass(kit, "Hash", kit->cObject);
  rb_include_module(kit->cHash, kit->mEnumerable);

  SAND_COPY_ALLOC(cHash);
  SAND_COPY_S(cHash, "[]");
  SAND_COPY(cHash,"initialize");
  SAND_COPY(cHash,"initialize_copy");
  SAND_COPY(cHash,"rehash");

  SAND_COPY(cHash,"to_hash");
  SAND_COPY(cHash,"to_a");
  SAND_COPY(cHash,"to_s");
  SAND_COPY(cHash,"inspect");

  SAND_COPY(cHash,"==");
  SAND_COPY(cHash,"[]");
  SAND_COPY(cHash,"fetch");
  SAND_COPY(cHash,"[]=");
  SAND_COPY(cHash,"store");
  SAND_COPY(cHash,"default");
  SAND_COPY(cHash,"default=");
  SAND_COPY(cHash,"default_proc");
  SAND_COPY(cHash,"index");
  SAND_COPY(cHash,"indexes");
  SAND_COPY(cHash,"indices");
  SAND_COPY(cHash,"size");
  SAND_COPY(cHash,"length");
  SAND_COPY(cHash,"empty?");

  SAND_COPY(cHash,"each");
  SAND_COPY(cHash,"each_value");
  SAND_COPY(cHash,"each_key");
  SAND_COPY(cHash,"each_pair");
  SAND_COPY(cHash,"sort");

  SAND_COPY(cHash,"keys");
  SAND_COPY(cHash,"values");
  SAND_COPY(cHash,"values_at");

  SAND_COPY(cHash,"shift");
  SAND_COPY(cHash,"delete");
  SAND_COPY(cHash,"delete_if");
  SAND_COPY(cHash,"select");
  SAND_COPY(cHash,"reject");
  SAND_COPY(cHash,"reject!");
  SAND_COPY(cHash,"clear");
  SAND_COPY(cHash,"invert");
  SAND_COPY(cHash,"update");
  SAND_COPY(cHash,"replace");
  SAND_COPY(cHash,"merge!");
  SAND_COPY(cHash,"merge");

  SAND_COPY(cHash,"include?");
  SAND_COPY(cHash,"member?");
  SAND_COPY(cHash,"has_key?");
  SAND_COPY(cHash,"has_value?");
  SAND_COPY(cHash,"key?");
  SAND_COPY(cHash,"value?");

  kit->cStruct = sandbox_defclass(kit, "Struct", kit->cObject);
  rb_include_module(kit->cStruct, kit->mEnumerable);

  rb_undef_alloc_func(kit->cStruct);
  SAND_COPY_S(cStruct, "new");

  SAND_COPY(cStruct, "initialize");
  SAND_COPY(cStruct, "initialize_copy");

  SAND_COPY(cStruct, "==");
  SAND_COPY(cStruct, "eql?");
  SAND_COPY(cStruct, "hash");

  SAND_COPY(cStruct, "to_s");
  SAND_COPY(cStruct, "inspect");
  SAND_COPY(cStruct, "to_a");
  SAND_COPY(cStruct, "values");
  SAND_COPY(cStruct, "size");
  SAND_COPY(cStruct, "length");

  SAND_COPY(cStruct, "each");
  SAND_COPY(cStruct, "each_pair");
  SAND_COPY(cStruct, "[]");
  SAND_COPY(cStruct, "[]=");
  SAND_COPY(cStruct, "select");
  SAND_COPY(cStruct, "values_at");

  SAND_COPY(cStruct, "members");

#ifndef FFSAFE
  VALUE rb_eRegexpError = rb_const_get(rb_cObject, rb_intern("RegexpError"));
#endif
  kit->eRegexpError = sandbox_defclass(kit, "RegexpError", kit->eStandardError);

  /*
  rb_define_virtual_variable("$~", match_getter, match_setter);
  rb_define_virtual_variable("$&", last_match_getter, 0);
  rb_define_virtual_variable("$`", prematch_getter, 0);
  rb_define_virtual_variable("$'", postmatch_getter, 0);
  rb_define_virtual_variable("$+", last_paren_match_getter, 0);

  rb_define_virtual_variable("$=", ignorecase_getter, ignorecase_setter);
  rb_define_virtual_variable("$KCODE", kcode_getter, kcode_setter);
  rb_define_virtual_variable("$-K", kcode_getter, kcode_setter);
  */

  kit->cRegexp = sandbox_defclass(kit, "Regexp", kit->cObject);
  SAND_COPY_ALLOC(cRegexp);
  SAND_COPY_S(cRegexp, "compile");
  SAND_COPY_S(cRegexp, "quote");
  SAND_COPY_S(cRegexp, "escape");
  SAND_COPY_S(cRegexp, "union");
  SAND_COPY_S(cRegexp, "last_match");

  SAND_COPY(cRegexp, "initialize");
  SAND_COPY(cRegexp, "initialize_copy");
  SAND_COPY(cRegexp, "hash");
  SAND_COPY(cRegexp, "eql?");
  SAND_COPY(cRegexp, "==");
  SAND_COPY(cRegexp, "=~");
  SAND_COPY(cRegexp, "===");
  SAND_COPY(cRegexp, "~");
  SAND_COPY(cRegexp, "match");
  SAND_COPY(cRegexp, "to_s");
  SAND_COPY(cRegexp, "inspect");
  SAND_COPY(cRegexp, "source");
  SAND_COPY(cRegexp, "casefold?");
  SAND_COPY(cRegexp, "options");
  SAND_COPY(cRegexp, "kcode");

  SAND_COPY_CONST(cRegexp, "IGNORECASE");
  SAND_COPY_CONST(cRegexp, "EXTENDED");
  SAND_COPY_CONST(cRegexp, "MULTILINE");

  /* FIXME: what to do about the reg_cache? oh definitely swap. */
  /* rb_global_variable(&reg_cache); */

#ifndef FFSAFE
  VALUE rb_cMatch = rb_const_get(rb_cObject, rb_intern("MatchData"));
#endif
  kit->cMatch  = sandbox_defclass(kit, "MatchData", kit->cObject);
  rb_const_set(kit->cObject, rb_intern("MatchingData"), kit->cMatch);
  SAND_COPY_ALLOC(cMatch);
  SAND_UNDEF(cMatch, "new");

  SAND_COPY(cMatch, "initialize_copy");
  SAND_COPY(cMatch, "size");
  SAND_COPY(cMatch, "length");
  SAND_COPY(cMatch, "offset");
  SAND_COPY(cMatch, "begin");
  SAND_COPY(cMatch, "end");
  SAND_COPY(cMatch, "to_a");
  SAND_COPY(cMatch, "[]");
  SAND_COPY(cMatch, "captures");
  SAND_COPY(cMatch, "select");
  SAND_COPY(cMatch, "values_at");
  SAND_COPY(cMatch, "pre_match");
  SAND_COPY(cMatch, "post_match");
  SAND_COPY(cMatch, "to_s");
  SAND_COPY(cMatch, "inspect");
  SAND_COPY(cMatch, "string");

  SAND_COPY(cArray, "pack");
  SAND_COPY(cString, "unpack");

  kit->cRange = sandbox_defclass(kit, "Range", kit->cObject);
  rb_include_module(kit->cRange, kit->mEnumerable);
  SAND_COPY(cRange, "initialize");
  SAND_COPY(cRange, "==");
  SAND_COPY(cRange, "===");
  SAND_COPY(cRange, "eql?");
  SAND_COPY(cRange, "hash");
  SAND_COPY(cRange, "each");
  SAND_COPY(cRange, "step");
  SAND_COPY(cRange, "first");
  SAND_COPY(cRange, "last");
  SAND_COPY(cRange, "begin");
  SAND_COPY(cRange, "end");
  SAND_COPY(cRange, "to_s");
  SAND_COPY(cRange, "inspect");

  SAND_COPY(cRange, "exclude_end?");

  SAND_COPY(cRange, "member?");
  SAND_COPY(cRange, "include?");

  SAND_COPY(mKernel, "hash");
  SAND_COPY(mKernel, "__id__");
  SAND_COPY(mKernel, "object_id");

  /*
   * dir.c:static VALUE chdir_thread = Qnil;
   * eval.c:VALUE rb_load_path;
   * eval.c:VALUE ruby_dln_librefs;
   * eval.c:static VALUE rb_features;
   * eval.c:extern VALUE rb_last_status;
   * eval.c:static VALUE th_raise_exception;
   * eval.c:static VALUE th_cmd;
   * eval.c:extern VALUE *rb_gc_stack_start;
   * eval.c:static VALUE thgroup_default;
   * file.c:static VALUE separator;
   * file.c:VALUE rb_mFConst;
   * gc.c:static VALUE nomem_error;
   * gc.c:VALUE rb_mGC;
   * gc.c:VALUE *rb_gc_stack_start = 0;
   * gc.c:static VALUE mark_stack[MARK_STACK_MAX];
   * gc.c:static VALUE *mark_stack_ptr;
   * gc.c:static VALUE finalizers;
   * hash.c:static VALUE envtbl;
   * io.c:VALUE rb_rs;
   * io.c:VALUE rb_output_rs;
   * io.c:VALUE rb_default_rs;
   */
  kit->load_path = rb_ary_new();
  kit->loaded_features = rb_ary_new();
#ifdef FFSAFE
  kit->_progname = sandbox_str(kit, "(sandbox)");
  sandbox_define_hooked_variable(kit, "$0", &kit->_progname, 0, 0);
#endif
}

static void
Init_kit_load(kit)
  sandkit *kit;
{
  rb_define_readonly_variable("$:", &kit->load_path);
  rb_define_readonly_variable("$-I", &kit->load_path);
  rb_define_readonly_variable("$LOAD_PATH", &kit->load_path);

  rb_define_readonly_variable("$\"", &kit->loaded_features);
  rb_define_readonly_variable("$LOADED_FEATURES", &kit->loaded_features);

  SAND_COPY_KERNEL("load");
  SAND_COPY_KERNEL("require");
  /*
  rb_define_method(rb_cModule, "autoload",  rb_mod_autoload,   2);
  rb_define_method(rb_cModule, "autoload?", rb_mod_autoload_p, 1);
  rb_define_global_function("autoload",  rb_f_autoload,   2);
  rb_define_global_function("autoload?", rb_f_autoload_p, 1);
  */

  /*
  rb_global_variable(&ruby_wrapper);

  rb_global_variable(&ruby_dln_librefs);
  ruby_dln_librefs = rb_ary_new();
  */
}

void Init_sand_table()
{
  VALUE cSandbox = rb_define_class("Sandbox", rb_cObject);
  rb_define_const( cSandbox, "VERSION", rb_str_new2( SAND_VERSION ) );
  rb_define_const( cSandbox, "REV_ID", rb_str_new2( SAND_REV_ID ) );
  rb_define_alloc_func( cSandbox, sandbox_alloc );
  rb_define_method( cSandbox, "initialize", sandbox_initialize, -1 );
  rb_define_method( cSandbox, "eval", sandbox_eval, 1 );
  rb_define_method( cSandbox, "load", sandbox_load, 1 );
  rb_define_method( cSandbox, "import", sandbox_import, 1 );
#ifdef FFSAFE
  rb_define_singleton_method( cSandbox, "safe", sandbox_safe, 0 );
  cSandboxSafe = rb_define_class_under(cSandbox, "Safe", cSandbox);
  rb_define_method( cSandboxSafe, "eval", sandbox_safe_eval, 1 );
  rb_define_method( cSandboxSafe, "load", sandbox_safe_load, 1 );
#endif
  Qinit = ID2SYM(rb_intern("init"));
  Qimport = ID2SYM(rb_intern("import"));
  Qload = ID2SYM(rb_intern("load"));
}
