/*
 * sand_table.c
 *
 * $Author: why $
 * $Date: 2006-05-08 22:03:50 -0600 (Mon, 08 May 2006) $
 *
 * Copyright (C) 2006 why the lucky stiff
 */
#include <ruby.h>
#include <st.h>
#include <env.h>
#include <node.h>

extern st_table *rb_class_tbl;
/* extern st_table *rb_global_tbl; */
extern VALUE ruby_top_self;

typedef struct {
  st_table *tbl;
  st_table *globals;

  VALUE cObject;
  VALUE cModule;
  VALUE cClass;
  VALUE mKernel;
  VALUE oMain;

  VALUE cArray;
  VALUE cBignum;
  VALUE mComparable;
  VALUE cData;
  VALUE mEnumerable;
  VALUE cFalseClass;
  VALUE cFixnum;
  VALUE cFloat;
  VALUE cHash;
  VALUE cInteger;
  VALUE cNilClass;
  VALUE cNumeric;
  VALUE mPrecision;
  VALUE cProc;
  VALUE cRange;
  VALUE cRegexp;
  VALUE cString;
  VALUE cSymbol;
  VALUE cTrueClass;
} sandkit;

void Init_kit _((sandkit *));

static void
mark_sandbox(kit)
  sandkit *kit;
{
  rb_mark_tbl(kit->tbl);
  rb_mark_tbl(kit->globals);
  rb_gc_mark(kit->cObject);
  rb_gc_mark(kit->cModule);
  rb_gc_mark(kit->cClass);
  rb_gc_mark(kit->mKernel);
}

void
free_sandbox(kit)
  sandkit *kit;
{   
  st_free_table(kit->tbl);
  st_free_table(kit->globals);
  free(kit);
}
 
VALUE
sandbox_module_new(kit)
  sandkit *kit;
{
  NEWOBJ(mdl, struct RClass);
  OBJSETUP(mdl, kit->cModule, T_MODULE);
  
  mdl->super = 0;
  mdl->iv_tbl = 0;
  mdl->m_tbl = 0;
  mdl->m_tbl = st_init_numtable();
  
  return (VALUE)mdl;
}

VALUE
sandbox_dummy()
{
    return Qnil;
}

VALUE
sandbox_define_module_id(kit, id)
  sandkit *kit;
  ID id;
{   
  VALUE mdl;

  mdl = sandbox_module_new(kit);
  rb_name_class(mdl, id);

  return mdl;
}

VALUE
sandbox_boot(kit, super)
  sandkit *kit;
  VALUE super;
{
  NEWOBJ(klass, struct RClass);
  OBJSETUP(klass, kit->cClass, T_CLASS);
      
  klass->super = super;
  klass->iv_tbl = 0;
  klass->m_tbl = 0;       /* safe GC */
  klass->m_tbl = st_init_numtable();
  
  OBJ_INFECT(klass, super);
  return (VALUE)klass;
}

VALUE
sandbox_metaclass(kit, obj, super)
  sandkit *kit;
  VALUE obj, super;
{
  VALUE klass = sandbox_boot(kit, super);
  FL_SET(klass, FL_SINGLETON);
  RBASIC(obj)->klass = klass;
  rb_singleton_class_attached(klass, obj);
  if (BUILTIN_TYPE(obj) == T_CLASS && FL_TEST(obj, FL_SINGLETON)) {
    RBASIC(klass)->klass = klass;
    RCLASS(klass)->super = RBASIC(rb_class_real(RCLASS(obj)->super))->klass;
  }
  else {
    VALUE metasuper = RBASIC(rb_class_real(super))->klass;

    /* metaclass of a superclass may be NULL at boot time */
    if (metasuper) {
        RBASIC(klass)->klass = metasuper;
    }
  }

  return klass;
}

static VALUE
sandbox_defclass(kit, name, super)
  sandkit *kit;
  char *name;
  VALUE super;
{   
  VALUE obj = sandbox_boot(kit, super);
  ID id = rb_intern(name);

  rb_name_class(obj, id);
  st_add_direct(kit->tbl, id, obj);
  rb_const_set((kit->cObject ? kit->cObject : obj), id, obj);
  return obj;
}

VALUE
sandbox_defmodule(kit, name)
  sandkit *kit;
  const char *name;
{
  VALUE module;
  ID id;

  id = rb_intern(name);
  if (rb_const_defined(kit->cObject, id)) {
    module = rb_const_get(kit->cObject, id);
    if (TYPE(module) == T_MODULE)
        return module;
    rb_raise(rb_eTypeError, "%s is not a module", rb_obj_classname(module));
  }
  module = sandbox_define_module_id(kit, id);
  st_add_direct(kit->tbl, id, module);
  rb_const_set(kit->cObject, id, module);

  return module;
}

#define SAND_COPY(K, M) sandbox_copy_method(kit->K, M, rb_##K);

void
sandbox_copy_method(klass, name, oklass)
  VALUE klass, oklass;
  const char *name;
{
  ID def = rb_intern(name);
  NODE *body;
     
  /* FIXME: handle failure? */
  body = rb_method_node(oklass, def);
  rb_add_method(klass, def, NEW_CFUNC(body->nd_cfnc, body->nd_argc), NOEX_PUBLIC);
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
  Init_kit(kit);
  return Data_Wrap_Struct( class, mark_sandbox, free_sandbox, kit );
}

typedef struct {
  VALUE *argv;
  sandkit *kit;
  sandkit *norm;
} go_cart;

VALUE
sandbox_go_go_go(go)
  go_cart *go;
{
  return rb_obj_instance_eval(1, go->argv, go->kit->oMain);
}

VALUE
sandbox_whoa_whoa_whoa(go)
  go_cart *go;
{
  /* okay, move it all back */
  sandkit *norm = go->norm;
  rb_class_tbl = norm->tbl;
  /* rb_global_tbl = norm->globals; */
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
  rb_cSymbol = norm->cSymbol;
  rb_cTrueClass = norm->cTrueClass;
  ruby_top_self = norm->oMain;
  free(go->norm);
  free(go);
}

VALUE
sandbox_eval( self, str )
  VALUE self, str;
{
  sandkit *kit, *norm;
  go_cart *go;
  Data_Get_Struct( self, sandkit, kit );

  /* save everything */
  norm = ALLOC(sandkit);
  norm->tbl = rb_class_tbl;
  /* norm->globals = rb_global_tbl; */
  norm->cObject = rb_cObject;
  norm->cModule = rb_cModule;
  norm->cClass = rb_cClass;
  norm->mKernel = rb_mKernel;
  norm->oMain = ruby_top_self;
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
  norm->cSymbol = rb_cSymbol;
  norm->cTrueClass = rb_cTrueClass;

  /* replace everything */
  rb_class_tbl = kit->tbl;
  /* rb_global_tbl = kit->globals; */
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
  rb_cSymbol = kit->cSymbol;
  rb_cTrueClass = kit->cTrueClass;
  ruby_top_self = kit->oMain;

  go = ALLOC(go_cart);
  go->argv = ALLOC(VALUE);
  go->argv[0] = str;
  go->norm = norm;
  go->kit = kit;

  return rb_ensure(sandbox_go_go_go, (VALUE)go, sandbox_whoa_whoa_whoa, (VALUE)go);
}

/*
 * STATIC METHODS I HAD TO COPY OVER
 */
static VALUE kit_str_alloc _((VALUE));
static VALUE
kit_str_alloc(klass)
    VALUE klass;
{
    NEWOBJ(str, struct RString);
    OBJSETUP(str, klass, T_STRING);

    str->ptr = 0;
    str->len = 0;
    str->aux.capa = 0;

    return (VALUE)str;
}

void Init_kit(kit)
  sandkit *kit;
{
  VALUE metaclass;

  kit->tbl = st_init_numtable();
  kit->globals = st_init_numtable();
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
  kit->oMain = rb_obj_alloc(kit->cObject);

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

/*
  rb_define_global_function("sprintf", rb_f_sprintf, -1);
  rb_define_global_function("format", rb_f_sprintf, -1); 

  rb_define_global_function("Integer", rb_f_integer, 1);
  rb_define_global_function("Float", rb_f_float, 1);

  rb_define_global_function("String", rb_f_string, 1);
  rb_define_global_function("Array", rb_f_array, 1);
*/

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
  rb_undef_method(CLASS_OF(kit->cNilClass), "new");
  /* rb_define_global_const("NIL", Qnil); */

  kit->cSymbol = sandbox_defclass(kit, "Symbol", kit->cObject);
  /* rb_define_singleton_method(kit->cSymbol, "all_symbols", rb_sym_all_symbols, 0); */
  rb_undef_alloc_func(kit->cSymbol);
  rb_undef_method(CLASS_OF(kit->cSymbol), "new");

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

  /* rb_define_alloc_func(kit->cModule, rb_module_s_alloc); */
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
  /* rb_define_alloc_func(kit->cClass, rb_class_s_alloc); */
  rb_undef_method(kit->cClass, "extend_object");
  rb_undef_method(kit->cClass, "append_features");

  kit->cTrueClass = sandbox_defclass(kit, "TrueClass", kit->cObject);
  SAND_COPY(cTrueClass, "to_s");
  SAND_COPY(cTrueClass, "&");
  SAND_COPY(cTrueClass, "|");
  SAND_COPY(cTrueClass, "^");
  rb_undef_alloc_func(kit->cTrueClass);
  rb_undef_method(CLASS_OF(kit->cTrueClass), "new");
  /* rb_define_global_const("TRUE", Qtrue); */

  kit->cFalseClass = sandbox_defclass(kit, "FalseClass", kit->cObject);
  SAND_COPY(cFalseClass, "to_s");
  SAND_COPY(cFalseClass, "&");
  SAND_COPY(cFalseClass, "|");
  SAND_COPY(cFalseClass, "^");
  rb_undef_alloc_func(kit->cFalseClass);
  rb_undef_method(CLASS_OF(kit->cFalseClass), "new");
  /* rb_define_global_const("FALSE", Qfalse); */

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
  /* singleton; SAND_COPY(mPrecision, "included"); */
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

  rb_define_global_function("eval", rb_f_eval, -1);
  rb_define_global_function("iterator?", rb_f_block_given_p, 0);
  rb_define_global_function("block_given?", rb_f_block_given_p, 0);
  rb_define_global_function("method_missing", rb_method_missing, -1);
  rb_define_global_function("loop", rb_f_loop, 0);
  */

  SAND_COPY(mKernel, "respond_to?");
  /*
  respond_to   = rb_intern("respond_to?");
  rb_global_variable((VALUE*)&basic_respond_to);
  basic_respond_to = rb_method_node(rb_cObject, respond_to);
  
  rb_define_global_function("raise", rb_f_raise, -1);
  rb_define_global_function("fail", rb_f_raise, -1);

  rb_define_global_function("caller", rb_f_caller, -1);

  rb_define_global_function("exit", rb_f_exit, -1);
  rb_define_global_function("abort", rb_f_abort, -1);

  rb_define_global_function("at_exit", rb_f_at_exit, 0);

  rb_define_global_function("catch", rb_f_catch, 1);
  rb_define_global_function("throw", rb_f_throw, -1);
  rb_define_global_function("global_variables", rb_f_global_variables, 0);
  rb_define_global_function("local_variables", rb_f_local_variables, 0);
  */

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

  /*
  rb_define_singleton_method(rb_cModule, "nesting", rb_mod_nesting, 0);
  rb_define_singleton_method(rb_cModule, "constants", rb_mod_s_constants, 0);

  rb_define_singleton_method(ruby_top_self, "include", top_include, -1);
  rb_define_singleton_method(ruby_top_self, "public", top_public, -1);
  rb_define_singleton_method(ruby_top_self, "private", top_private, -1);
  */

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
  rb_define_alloc_func(kit->cString, kit_str_alloc);
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

  /*
  rb_define_global_function("sub", rb_f_sub, -1);
  rb_define_global_function("gsub", rb_f_gsub, -1);

  rb_define_global_function("sub!", rb_f_sub_bang, -1);
  rb_define_global_function("gsub!", rb_f_gsub_bang, -1);

  rb_define_global_function("chop", rb_f_chop, 0);
  rb_define_global_function("chop!", rb_f_chop_bang, 0);

  rb_define_global_function("chomp", rb_f_chomp, -1);
  rb_define_global_function("chomp!", rb_f_chomp_bang, -1);

  rb_define_global_function("split", rb_f_split, -1);
  rb_define_global_function("scan", rb_f_scan, 1);
  */

  SAND_COPY(cString, "slice");
  SAND_COPY(cString, "slice!");

  /*
  rb_fs = Qnil;
  rb_define_variable("$;", &rb_fs);
  rb_define_variable("$-F", &rb_fs);
  */
}

void Init_sand_table()
{
  VALUE cSandbox = rb_define_class("Sandbox", rb_cObject);
  rb_define_alloc_func( cSandbox, sandbox_alloc );
  rb_define_method( cSandbox, "eval", sandbox_eval, 1 );
}
