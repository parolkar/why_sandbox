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
#define SAND_REV_ID "$Rev$"

#define FREAKYFREAKY "freakyfreakysandbox"

#include <ruby.h>
#include <rubysig.h>
#include <st.h>
#include <env.h>
#include <node.h>
#include <version.h>

extern st_table *rb_class_tbl;
extern VALUE ruby_top_self;

#if RUBY_VERSION_CODE >= 185
#define FFSAFE
extern st_table *rb_global_tbl;
extern VALUE rb_eRegexpError;
extern VALUE rb_cMatch;
extern VALUE rb_cNameErrorMesg;
#else
#warning "** Sandbox will NOT be safe unless used with 1.8.5 -- Proceeding anyway! **"
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct SANDKIT {
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
  VALUE eException;
  VALUE cFalseClass;
  VALUE cFixnum;
  VALUE cFloat;
  VALUE cHash;
  VALUE cInteger;
  VALUE cMatch;
  VALUE cNilClass;
  VALUE cNumeric;
  VALUE mPrecision;
  VALUE cProc;
  VALUE cRange;
  VALUE cRegexp;
  VALUE cString;
  VALUE cStruct;
  VALUE cSymbol;
  VALUE cTrueClass;

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

  struct SANDKIT *banished;
} sandkit;


VALUE sandbox_module_new(sandkit *);
VALUE sandbox_dummy();
VALUE sandbox_define_module_id(sandkit *, ID);
VALUE sandbox_boot(sandkit *, VALUE);
VALUE sandbox_metaclass(sandkit *, VALUE, VALUE);
VALUE sandbox_singleton_class(sandkit *, VALUE);
VALUE sandbox_defclass(sandkit *, const char *, VALUE);
VALUE sandbox_defmodule(sandkit *, const char *);

#if defined(__cplusplus)
}  /* extern "C" { */
#endif

#endif /* ifndef SAND_TABLE_H */
