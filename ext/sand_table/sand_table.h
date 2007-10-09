/*
 * sand_table.h
 *
 * $Author$
 * $Date$
 *
 * Copyright (C) 2003 why the lucky stiff
 */

#ifndef SAND_TABLE_H
#define SAND_TABLE_H

#define SAND_VERSION "0.3"

#define FREAKYFREAKY "freakyfreakysandbox"

#include <ruby.h>
#include <rubysig.h>
#include <st.h>
#include <env.h>
#include <node.h>
#include <version.h>

extern VALUE ruby_sandbox;
extern st_table *rb_class_tbl;
extern VALUE ruby_top_self;
extern struct FRAME *ruby_frame;
extern struct SCOPE *ruby_scope;
extern st_table *rb_global_tbl;
extern st_table *rb_syserr_tbl;

#ifdef RARRAY_LEN
#undef T_SYMBOL
#define T_SYMBOL T_STRING
static inline long  rb_str_len(VALUE s) {return RSTRING_LEN(s);}                                                               
static inline char *rb_str_ptr(VALUE s) {return RSTRING_PTR(s);}                                                               
static inline long  rb_ary_len(VALUE s) {return  RARRAY_LEN(s);}                                                               
static inline VALUE *rb_ary_ptr(VALUE s) {return  RARRAY_PTR(s);}
#else                                                                                                                         
static inline long  rb_str_len(VALUE s) {return RSTRING(s)->len;}                                                              
static inline char *rb_str_ptr(VALUE s) {return RSTRING(s)->ptr;}                                                              
static inline long  rb_ary_len(VALUE s) {return  RARRAY(s)->len;}                                                              
static inline VALUE *rb_ary_ptr(VALUE s) {return  RARRAY(s)->ptr;}                                                              
#endif 

#if RUBY_VERSION_CODE <= 185
#warning "** Sandbox will NOT compile without a patched 1.8.5 -- Proceeding anyway! **"
#endif

#if RUBY_VERSION_CODE > 190
#define KIT2
#define kitBasicObject kit->cBasicObject
#else
#define kitBasicObject kit->cObject
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct SANDKIT {
  st_table *tbl;
  st_table *globals;
  st_table *syserrs;
  VALUE self;
  VALUE _progname;

#if KIT2
  VALUE cBasicObject;
#endif
  VALUE cObject;
  VALUE cModule;
  VALUE cClass;
  VALUE mKernel;
  VALUE oMain;
  VALUE cArray;
  VALUE cBignum;
  VALUE cBinding;
  VALUE mComparable;
  VALUE cCont;
  VALUE cData;
  VALUE cDir;
  VALUE mEnumerable;
  VALUE eException;
  VALUE cFalseClass;
  VALUE mFConst;
  VALUE cFile;
  VALUE mFileTest;
  VALUE cFixnum;
  VALUE cFloat;
  VALUE mGC;
  VALUE cHash;
  VALUE cInteger;
  VALUE cIO;
  VALUE mMarshal;
  VALUE mMath;
  VALUE cMatch;
  VALUE cMethod;
  VALUE cNilClass;
  VALUE cNumeric;
  VALUE mObSpace;
  VALUE mPrecision;
  VALUE cProc;
  VALUE mProcess;
  VALUE cProcStatus;
  VALUE mProcUID;
  VALUE mProcGID;
  VALUE mProcID_Syscall;
  VALUE cRange;
  VALUE cRegexp;
  VALUE cStat;
  VALUE cString;
  VALUE cStruct;
  VALUE cSymbol;
  VALUE cThread;
  VALUE cThGroup;
  VALUE cTime;
  VALUE sTms;
  VALUE cTrueClass;
  VALUE cUnboundMethod;
  VALUE eStandardError;
  VALUE eSystemExit;
  VALUE eInterrupt;
  VALUE eSignal;
  VALUE eFatal;
  VALUE eArgError;
  VALUE eEOFError;
  VALUE eIndexError;
  VALUE eRangeError;
  VALUE eRegexpError;
  VALUE eIOError;
  VALUE eRuntimeError;
  VALUE eSecurityError;
  VALUE eSystemCallError;
  VALUE eSysStackError;
  VALUE eThreadError;
  VALUE eTypeError;
  VALUE eZeroDivError;
  VALUE eNotImpError;
  VALUE eNoMemError;
  VALUE eNoMethodError;
  VALUE eFloatDomainError;
  VALUE eScriptError;
  VALUE eNameError;
  VALUE cNameErrorMesg;
  VALUE eSyntaxError;
  VALUE eLoadError;
  VALUE eLocalJumpError;
  VALUE mErrno;
  VALUE cBoxedClass;

  VALUE load_path;
  VALUE loaded_features;
  VALUE wrapper;
  VALUE klass;
  VALUE dln_librefs;
  st_table *loading_tbl;

  NODE *top_cref;
  NODE *ruby_cref;
  VALUE rclass;
  struct SCOPE *scope;
} sandkit;

#define SANDBOX_EVAL        10
#define SANDBOX_METHOD_CALL 11
#define SANDBOX_ACTION      12
#define SANDBOX_SET         13

/* The sandwick struct is just the old go_cart struct,
 * but I want its initalization and execution methods
 * to be responsible for marshalling and linking stuff
 * in and out of the sandbox.  This way, checking all
 * the arguments and returns from method calls or evals
 * can get some sanity.
 */
typedef struct {
  VALUE self;

  /* how is the sandbox to be called? */
  char calltype;
  int argc;
  VALUE *argv;
  VALUE link;
  VALUE (*action)();

  /* used to negotiate the swap */
  VALUE exception;
  sandkit *kit;
  VALUE banished;
  VALUE old_toplevel;
  struct SCOPE *scope;
  struct RVarmap *dyna_vars;
} sandwick;

#define SAND_BASE(K) (use_base == 0 ? rb_##K : base.K)
#define SAND_COPY(K, M) sandbox_copy_method(kit->K, rb_intern(M), SAND_BASE(K));
#define SAND_COPY_ALLOC(K) Check_Type(kit->K, T_CLASS); sandbox_copy_method(RBASIC(kit->K)->klass, ID_ALLOCATOR, RBASIC(SAND_BASE(K))->klass);
#define SAND_COPY_S(K, M) sandbox_copy_method(sandbox_singleton_class(kit, kit->K), rb_intern(M), (use_base == 0 ? rb_singleton_class(rb_##K) : sandbox_singleton_class(&base, base.K)));
#define SAND_COPY_MAIN(M) sandbox_copy_method(sandbox_singleton_class(kit, kit->oMain), rb_intern(M), (use_base == 0 ? rb_singleton_class(ruby_top_self) : sandbox_singleton_class(&base, base.oMain)));
#define SAND_COPY_CONST(K, M) rb_const_set(kit->K, rb_intern(M), rb_const_get(SAND_BASE(K), rb_intern(M)));
#define SAND_COPY_IF_CONST(K, M) if (rb_const_defined(SAND_BASE(K), rb_intern(M))) { SAND_COPY_CONST(K, M) }
#define SAND_COPY_KERNEL(M) SAND_COPY(mKernel, M); SAND_COPY_S(mKernel, M)
#define SAND_UNDEF(M, K) rb_undef_method(sandbox_singleton_class(kit, kit->M), K);
#define SAND_SYSERR(K, M) \
  if (rb_const_defined(SAND_BASE(K), rb_intern(M))) { \
    VALUE error = rb_define_class_under(kit->mErrno, M, kit->eSystemCallError); \
    VALUE no = rb_const_get(rb_const_get(SAND_BASE(K), rb_intern(M)), rb_intern("Errno")); \
    rb_define_const(error, "Errno", no); \
    st_add_direct(kit->syserrs, NUM2INT(no), error); \
  }

sandwick *sandbox_method_wick(VALUE, int, VALUE *);
sandwick *sandbox_eval_wick(VALUE);
sandwick *sandbox_action_wick(VALUE (*)(), VALUE);
VALUE sandbox_run(sandkit *kit, sandwick *wick);

VALUE sandbox_module_new(sandkit *);
VALUE sandbox_mod_name(VALUE mod);
VALUE sandbox_dummy();
VALUE sandbox_define_module_id(sandkit *, ID);
VALUE sandbox_boot(sandkit *, VALUE);
VALUE sandbox_metaclass(sandkit *, VALUE, VALUE);
VALUE sandbox_singleton_class(sandkit *, VALUE);
VALUE sandbox_defclass(sandkit *, const char *, VALUE);
VALUE sandbox_defmodule(sandkit *, const char *);
void sandbox_copy_method(VALUE, VALUE, ID);
VALUE sandbox_get_linked_class(VALUE);
VALUE sandbox_get_linked_box(VALUE);
void sandbox_foreach_method(VALUE, VALUE, int (*)(ID, long, VALUE *));
VALUE sandbox_import_class_path(sandkit *, const char *, unsigned char);
void sandbox_define_hooked_variable(sandkit *kit, const char *, VALUE *, VALUE (*)(), void (*)());
void sandbox_define_variable(sandkit *kit, const char *, VALUE *);
void sandbox_define_readonly_variable(sandkit *, const char  *, VALUE *);
void sandbox_define_virtual_variable(sandkit *, const char *, VALUE (*)(), void (*)());
void sandbox_mark_globals(st_table *);
void sandbox_errinfo_setter(VALUE val, ID id, VALUE *var);
VALUE sandbox_last_match_getter();
VALUE sandbox_prematch_getter();
VALUE sandbox_postmatch_getter();
VALUE sandbox_last_paren_match_getter();
VALUE sandbox_match_getter();
void sandbox_match_setter(VALUE);

#if defined(__cplusplus)
}  /* extern "C" { */
#endif

#endif /* ifndef SAND_TABLE_H */
