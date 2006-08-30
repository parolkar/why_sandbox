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

#define SAND_VERSION "0.0"

#define FREAKYFREAKY "freakyfreakysandbox"

#include <ruby.h>
#include <rubysig.h>
#include <st.h>
#include <env.h>
#include <node.h>
#include <version.h>

extern st_table *rb_class_tbl;
extern VALUE ruby_top_self;
extern struct FRAME *ruby_frame;
extern struct SCOPE *ruby_scope;

#if RUBY_VERSION_CODE >= 185
#define FFSAFE
extern st_table *rb_global_tbl;
#else
#warning "** Sandbox will NOT be safe unless used with 1.8.5 -- Proceeding anyway! **"
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct SANDKIT {
  st_table *tbl;

#ifdef FFSAFE
  st_table *globals;
  VALUE _progname;
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
  VALUE cRange;
  VALUE cRegexp;
  VALUE cStat;
  VALUE cString;
  VALUE cStruct;
  VALUE cSymbol;
  VALUE cTime;
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

  VALUE load_path;
  VALUE loaded_features;
  VALUE wrapper;
  VALUE klass;
  VALUE dln_librefs;
  st_table *loading_tbl;

  NODE *top_cref;
  struct SCOPE *scope;
  struct SANDKIT *banished;
  int active;
} sandkit;

#define SAND_COPY(K, M) sandbox_copy_method(kit->K, rb_intern(M), rb_##K);
#define SAND_COPY_ALLOC(K) Check_Type(kit->K, T_CLASS); sandbox_copy_method(RBASIC(kit->K)->klass, ID_ALLOCATOR, RBASIC(rb_##K)->klass);
#define SAND_COPY_S(K, M) sandbox_copy_method(sandbox_singleton_class(kit, kit->K), rb_intern(M), rb_singleton_class(rb_##K));
#define SAND_COPY_MAIN(M) sandbox_copy_method(sandbox_singleton_class(kit, kit->oMain), rb_intern(M), rb_singleton_class(ruby_top_self));
#define SAND_COPY_CONST(K, M) rb_const_set(kit->K, rb_intern(M), rb_const_get(rb_##K, rb_intern(M)));
#define SAND_DUP_CONST(K, M) rb_const_set(kit->K, rb_intern(M), sandbox_dup_into(kit, rb_const_get(rb_##K, rb_intern(M))));
#define SAND_COPY_IF_CONST(K, M) if (rb_const_defined(rb_##K, rb_intern(M))) { SAND_COPY_CONST(K, M) }
#define SAND_COPY_KERNEL(M) SAND_COPY(mKernel, M); SAND_COPY_S(mKernel, M)
#define SAND_UNDEF(M, K) rb_undef_method(sandbox_singleton_class(kit, kit->M), K);
#define SAND_SYSERR(K, M) \
  if (rb_const_defined(rb_##K, rb_intern(M))) { \
    VALUE error = rb_define_class_under(kit->mErrno, M, kit->eSystemCallError); \
    rb_define_const(error, "Errno", rb_const_get(rb_const_get(rb_##K, rb_intern(M)), rb_intern("Errno"))); \
  }

VALUE sandbox_module_new(sandkit *);
VALUE sandbox_dummy();
VALUE sandbox_define_module_id(sandkit *, ID);
VALUE sandbox_boot(sandkit *, VALUE);
VALUE sandbox_metaclass(sandkit *, VALUE, VALUE);
VALUE sandbox_singleton_class(sandkit *, VALUE);
VALUE sandbox_defclass(sandkit *, const char *, VALUE);
VALUE sandbox_defmodule(sandkit *, const char *);
void sandbox_copy_method(VALUE, VALUE, ID);
void sandbox_foreach_method(VALUE, VALUE, int (*)(ID, long, VALUE *));
VALUE sandbox_import_class_path(sandkit *, const char *);
#ifdef FFSAFE
void sandbox_define_hooked_variable(sandkit *kit, const char *, VALUE *, VALUE (*)(), void (*)());
void sandbox_define_variable(sandkit *kit, const char *, VALUE *);
void sandbox_define_readonly_variable(sandkit *, const char  *, VALUE *);
void sandbox_define_virtual_variable(sandkit *, const char *, VALUE (*)(), void (*)());
void sandbox_mark_globals(st_table *);
#endif

#if defined(__cplusplus)
}  /* extern "C" { */
#endif

#endif /* ifndef SAND_TABLE_H */
