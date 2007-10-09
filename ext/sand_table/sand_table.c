/*
 * sand_table.c
 *
 * $Author$
 * $Date$
 *
 * Copyright (C) 2006 why the lucky stiff
 */
#include "sand_table.h"
#include <math.h>

#define SAND_REV_ID "$Rev$"

#ifndef rb_curr_thread
#define curr_thread rb_curr_thread
#endif

VALUE ruby_sandbox = Qnil;
static sandkit real;
static sandkit base;

static VALUE Qimport, Qcopy, Qinit, Qload, Qenv, Qio, Qreal, Qref, Qall;
VALUE rb_cSandbox, rb_cSandboxFull, rb_cSandboxSafe, rb_eSandboxException, rb_cSandboxRef, rb_cSandboxWick, rb_cSandboxTransfer;
static ID s_options, s_to_s, s_copy_ivar;

static VALUE old_toplevel;

static VALUE sandbox_run_begin(VALUE wick);
static VALUE sandbox_run_wick(VALUE v);
static VALUE sandbox_run_rescue(VALUE v, VALUE exc);
static VALUE sandbox_run_ensure(VALUE v);
static void Init_kit _((sandkit *, int));
static void Init_kit_load _((sandkit *, int));
static void Init_kit_io _((sandkit *, int));
static void Init_kit_env _((sandkit *, int));
static void Init_kit_real _((sandkit *, int));
static void Init_kit_prelude _((sandkit *));
void sandbox_swap(sandkit *kit, int mode);

typedef struct {
  VALUE (*f)();
  VALUE data;
} mini_closure;

#define SANDBOX_STORE 1
#define SANDBOX_REPLACE 2

static void
mark_sandbox(kit)
  sandkit *kit;
{
  rb_mark_tbl(kit->tbl);
#ifdef KIT2
  rb_gc_mark_maybe(kit->cBasicObject);
#endif
  rb_gc_mark_maybe(kit->cObject);
  rb_gc_mark_maybe(kit->cModule);
  rb_gc_mark_maybe(kit->cClass);
  rb_gc_mark_maybe(kit->mKernel);
  rb_gc_mark_maybe(kit->oMain);
  rb_gc_mark_maybe(kit->cArray);
  rb_gc_mark_maybe(kit->cBignum);
  rb_gc_mark_maybe(kit->cBinding);
  rb_gc_mark_maybe(kit->mComparable);
  rb_gc_mark_maybe(kit->cCont);
  rb_gc_mark_maybe(kit->cData);
  rb_gc_mark_maybe(kit->cDir);
  rb_gc_mark_maybe(kit->mEnumerable);
  rb_gc_mark_maybe(kit->eException);
  rb_gc_mark_maybe(kit->cFalseClass);
  rb_gc_mark_maybe(kit->mFConst);
  rb_gc_mark_maybe(kit->cFile);
  rb_gc_mark_maybe(kit->mFileTest);
  rb_gc_mark_maybe(kit->cFixnum);
  rb_gc_mark_maybe(kit->cFloat);
  rb_gc_mark_maybe(kit->mGC);
  rb_gc_mark_maybe(kit->cHash);
  rb_gc_mark_maybe(kit->cInteger);
  rb_gc_mark_maybe(kit->cIO);
  rb_gc_mark_maybe(kit->mMarshal);
  rb_gc_mark_maybe(kit->mMath);
  rb_gc_mark_maybe(kit->cMatch);
  rb_gc_mark_maybe(kit->cMethod);
  rb_gc_mark_maybe(kit->cNilClass);
  rb_gc_mark_maybe(kit->cNumeric);
  rb_gc_mark_maybe(kit->mObSpace);
  rb_gc_mark_maybe(kit->mPrecision);
  rb_gc_mark_maybe(kit->cProc);
  rb_gc_mark_maybe(kit->mProcess);
  rb_gc_mark_maybe(kit->cRange);
  rb_gc_mark_maybe(kit->cRegexp);
  rb_gc_mark_maybe(kit->cStat);
  rb_gc_mark_maybe(kit->cString);
  rb_gc_mark_maybe(kit->cStruct);
  rb_gc_mark_maybe(kit->cSymbol);
  rb_gc_mark_maybe(kit->cThread);
  rb_gc_mark_maybe(kit->cThGroup);
  rb_gc_mark_maybe(kit->cTime);
  rb_gc_mark_maybe(kit->sTms);
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
  rb_gc_mark_maybe(kit->eThreadError);
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
  rb_gc_mark_maybe(kit->cBoxedClass);
  rb_gc_mark_maybe(kit->load_path);
  rb_gc_mark_maybe(kit->loaded_features);
  rb_gc_mark((VALUE)kit->scope);
  rb_gc_mark((VALUE)kit->top_cref);
  rb_gc_mark((VALUE)kit->ruby_cref);
  rb_gc_mark_maybe(kit->rclass);
  sandbox_mark_globals(kit->globals);
  rb_mark_tbl(kit->syserrs);
}

static VALUE
sandbox_save(thread)
  rb_thread_t thread;
{
  if (NIL_P(thread->sandbox))
  {
    sandbox_swap(&real, SANDBOX_STORE);
  }
  else
  {
    sandkit *kit;
    Data_Get_Struct( thread->sandbox, sandkit, kit );
    sandbox_swap(kit, SANDBOX_STORE);
  }
  return Qnil;
}

static VALUE
sandbox_restore(thread)
  rb_thread_t thread;
{
  if (NIL_P(thread->sandbox))
  {
    thread->sandbox = real.self;
  }
  if (ruby_sandbox != thread->sandbox)
  {
    sandkit *kit;
    ruby_sandbox = thread->sandbox;
    Data_Get_Struct( thread->sandbox, sandkit, kit );
    sandbox_swap(kit, SANDBOX_REPLACE);
  }
  return Qnil;
}

void
free_sandbox(kit)
  sandkit *kit;
{   
  st_free_table(kit->tbl);
  st_free_table(kit->globals);
  st_free_table(kit->syserrs);
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

static struct SCOPE *
alloc_scope()
{
  NEWOBJ(_scope, struct SCOPE);
  OBJSETUP(_scope, 0, T_SCOPE);
  _scope->local_tbl = 0;
  _scope->local_vars = 0;
  _scope->flags = 0;
  return _scope;
}

VALUE sandbox_alloc _((VALUE));
VALUE 
sandbox_alloc(class)
  VALUE class;
{
  sandkit *kit = ALLOC(sandkit);
  MEMZERO(kit, sandkit, 1);
  Init_kit(kit, 1);

  kit->scope = alloc_scope();

  kit->top_cref = rb_node_newnode(NODE_CREF,kit->cObject,0,0);
  kit->ruby_cref = kit->top_cref;
  kit->ruby_cref->nd_clss = kit->cObject;
  kit->rclass = kit->cObject;

  kit->self = Data_Wrap_Struct( class, mark_sandbox, free_sandbox, kit );
  return kit->self;
}

#define SAND_INIT(N) if ( init_##N == 0 ) { Init_kit_##N(kit, 1); init_##N = 1; }

/*
 *  call-seq:
 *     Sandbox::Full.new(opts={})   => obj
 *
 *  Returns a newly created sandbox; a hash containing default options for
 *  Sandbox#eval in this sandbox may be given in +opts+.  (See Sandbox#eval.)
 *
 */
static VALUE
sandbox_initialize(argc, argv, self)
  int argc;
  VALUE *argv;
  VALUE self;
{
  sandkit *kit;
  VALUE opts, import, init, ref;

  if (rb_scan_args(argc, argv, "01", &opts) == 0)
  {
    opts = rb_hash_new();
  }
  else
  {
    Check_Type(opts, T_HASH);
  }

  Data_Get_Struct( self, sandkit, kit );

  init = rb_hash_aref(opts, Qinit);
  if (!NIL_P(init))
  {
    int i;
    int init_load = 0, init_env = 0, init_io = 0, init_real = 0;
    init = rb_Array(init);
    for ( i = 0; i < rb_ary_len(init); i++ )
    {
      VALUE mod = rb_ary_entry(init, i);
      if ( mod == Qload ) {
        SAND_INIT(load);
      } else if ( mod == Qenv ) {
        SAND_INIT(env);
      } else if ( mod == Qio ) {
        SAND_INIT(io);
      } else if ( mod == Qreal ) {
        SAND_INIT(real);
      } else if ( mod == Qall ) {
        SAND_INIT(load);
        SAND_INIT(io);
        SAND_INIT(env);
        SAND_INIT(real);
      } else {
        rb_raise(rb_eArgError, "no '%s' module available for the sandbox", RSTRING(rb_inspect(mod))->ptr);
      }
    }
  }

  Init_kit_prelude(kit);

  import = rb_hash_aref(opts, Qimport);
  if (!NIL_P(import))
  {
    int i;
    Check_Type(import, T_ARRAY);
    for ( i = 0; i < rb_ary_len(import); i++ )
    {
      rb_funcall(self, rb_intern("import"), 1, rb_ary_entry(import, i));
    }
  }

  ref = rb_hash_aref(opts, Qref);
  if (!NIL_P(ref))
  {
    int i;
    Check_Type(import, T_ARRAY);
    for ( i = 0; i < rb_ary_len(ref); i++ )
    {
      rb_funcall(self, rb_intern("ref"), 1, rb_ary_entry(ref, i));
    }
  }

  rb_ivar_set(self, s_options, opts);
  return self;
}

#define TRANS_NONE    40
#define TRANS_MARSHAL 41
#define TRANS_LINK    42

/* Simple struct for dealing with the arg transfer. */
typedef struct {
  VALUE self;
  VALUE val;
  char trans;
} sandtransfer;

void
mark_sandtransfer(transfer)
  sandtransfer *transfer;
{
  rb_gc_mark_maybe(transfer->val);
}

void
free_sandtransfer(transfer)
  sandtransfer *transfer;
{
  rb_gc_unregister_address(&transfer->self);
  free(transfer);
}

sandtransfer *
alloc_sandtransfer()
{
  sandtransfer *transfer = ALLOC(sandtransfer);
  transfer->val = Qnil;
  transfer->trans = 0;
  transfer->self = Data_Wrap_Struct(rb_cSandboxTransfer, mark_sandtransfer, free_sandtransfer, transfer);
  rb_gc_register_address(&transfer->self);

  return transfer;
}

void
mark_sandwick(wick)
  sandwick *wick;
{
  int i;
  for ( i = 0 ; i < wick->argc ; i++ ) {
    rb_gc_mark_maybe(wick->argv[i]);
  }
  rb_gc_mark_maybe(wick->link);
  rb_gc_mark(wick->exception);
  if (wick->kit) {
    rb_gc_mark(wick->kit->self);
  }
  rb_gc_mark(wick->banished);
  rb_gc_mark((VALUE)wick->scope);
  rb_gc_mark((VALUE)wick->dyna_vars);
}

void
free_sandwick(wick)
  sandwick *wick;
{
  rb_gc_unregister_address(&wick->self);
  if (wick->calltype == SANDBOX_COPY)
    free(wick->argv);
  free(wick);
}

sandwick *
alloc_sandwick()
{
  sandwick *wick;
  wick = ALLOC(sandwick);
  wick->self = Qnil;
  wick->argc = 0;
  wick->argv = NULL;
  wick->link = Qnil;
  wick->action = NULL;
  wick->exception = Qnil;
  wick->kit = NULL;
  wick->banished = Qnil;
  wick->scope = NULL;
  wick->dyna_vars = NULL;
  wick->self = Data_Wrap_Struct(rb_cSandboxWick, mark_sandwick, free_sandwick, wick);
  rb_gc_register_address(&wick->self);

  return wick;
}

/*
 * A "wick" for starting a sandbox that calls
 * a method of a linked object inside that same
 * sandbox.
 */
sandwick *
sandbox_copy_wick(link, obj_name, obj)
  VALUE link;
  VALUE obj_name;
  VALUE obj;
{
  sandwick *wick = alloc_sandwick();
  wick->calltype = SANDBOX_COPY;
  wick->link = link;
  wick->argc = 2;
  wick->argv = malloc(sizeof(VALUE) * 2);
  wick->argv[0] = obj_name;
  wick->argv[1] = obj;
  return wick;
}

/*
 * A "wick" for starting a sandbox that calls
 * a method of a linked object inside that same
 * sandbox.
 */
sandwick *
sandbox_method_wick(link, argc, argv)
  VALUE link;
  int argc;
  VALUE *argv;
{
  sandwick *wick = alloc_sandwick();
  wick->calltype = SANDBOX_METHOD_CALL;
  wick->link = link;
  wick->argc = argc;
  wick->argv = argv;
  return wick;
}

/*
 * Runs the method call action.
 */
static VALUE
sandbox_run_method_call(wick)
  sandwick *wick;
{
  return rb_funcall3(wick->link, SYM2ID(wick->argv[0]), wick->argc - 1, &wick->argv[1]);
}

static VALUE
sandbox_run_copy(wick)
  sandwick *wick;
{
  char *eval = NULL;
  VALUE str = wick->argv[0];
  rb_ivar_set(ruby_top_self, s_copy_ivar, wick->argv[1]);

  str = rb_funcall(str, s_to_s, 0);
  StringValue(str);
  eval = ALLOC_N(char, RSTRING_LEN(str) + 20);
  sprintf(eval, "%s = @____SANDBOX_COPY", RSTRING_PTR(str));

  str = rb_eval_string(eval);

  rb_ivar_set(ruby_top_self, s_copy_ivar, Qnil);
  free(eval);
  return str;
}

/*
 * A "wick" for evaling a string inside the
 * sandbox.
 */
sandwick *
sandbox_eval_wick(str)
  VALUE str;
{
  sandwick *wick = alloc_sandwick();
  wick->calltype = SANDBOX_EVAL;
  wick->link = str;
  return wick;
}

/* 
 * Runs the eval action.
 */
static VALUE
sandbox_run_eval(wick)
  sandwick *wick;
{
  VALUE str = wick->link;
  StringValue(str);
  return rb_eval_string(rb_str_ptr(str));
}

/*
 * A "wick" for running a C callback
 * inside the sandbox.
 */
sandwick *
sandbox_action_wick(action, link)
  VALUE (*action)();
  VALUE link;
{
  sandwick *wick = alloc_sandwick();
  wick->calltype = SANDBOX_ACTION;
  wick->action = action;
  wick->link = link;
  wick->argc = 0;
  wick->argv = NULL;
  return wick;
}

/* 
 * Runs the callback action.
 */
static VALUE
sandbox_run_action(wick)
  sandwick *wick;
{
  return wick->action(wick->link);
}

#define SWAP(N) \
  if (mode == SANDBOX_STORE) kit->N = rb_##N; \
  else if (kit->N != 0)      rb_##N = kit->N;
#define SWAP_VAR(N, V) \
  if (mode == SANDBOX_STORE) kit->V = N; \
  else                       N = kit->V;
#define SWAP_GVAR(N, V) \
  if (mode == SANDBOX_STORE) kit->V = rb_gv_get(N); \
  else                       sandbox_swap_global(&real, N, kit->V);

void
sandbox_swap(kit, mode)
  sandkit *kit;
  int mode;
{
  SWAP_VAR(ruby_sandbox, self);
  SWAP_VAR(rb_class_tbl, tbl);
#ifdef KIT2
  SWAP(cBasicObject);
#endif
  SWAP(cObject);
  SWAP(cModule);
  SWAP(cClass);
  SWAP(mKernel);
  SWAP(cArray);
  SWAP(cBignum);
  SWAP(cBinding);
  SWAP(mComparable);
  SWAP(cCont);
  SWAP(cData);
  SWAP(cDir);
  SWAP(mEnumerable);
  SWAP(cFalseClass);
  SWAP(cFile);
  SWAP(mFileTest);
  SWAP(cFixnum);
  SWAP(cFloat);
  SWAP(mGC);
  SWAP(cHash);
  SWAP(cInteger);
  SWAP(cIO);
  SWAP(mMath);
  SWAP(cNilClass);
  SWAP(cNumeric);
  SWAP(mPrecision);
  SWAP(cProc);
  SWAP(cRange);
  SWAP(cRegexp);
  SWAP(cStat);
  SWAP(cString);
  SWAP(cStruct);
  SWAP(cSymbol);
  SWAP(cThread);
  SWAP(cTime);
  SWAP(cTrueClass);
  SWAP(eException);
  SWAP(eStandardError);
  SWAP(eSystemExit);
  SWAP(eInterrupt);
  SWAP(eSignal);
  SWAP(eFatal);
  SWAP(eArgError);
  SWAP(eEOFError);
  SWAP(eIndexError);
  SWAP(eRangeError);
  SWAP(eIOError);
  SWAP(eRuntimeError);
  SWAP(eSecurityError);
  SWAP(eSystemCallError);
  SWAP(eThreadError);
  SWAP(eTypeError);
  SWAP(eZeroDivError);
  SWAP(eNotImpError);
  SWAP(eNoMemError);
  SWAP(eNoMethodError);
  SWAP(eFloatDomainError);
  SWAP(eScriptError);
  SWAP(eNameError);
  SWAP(eSyntaxError);
  SWAP(eLoadError);
  SWAP(mErrno);
  SWAP_GVAR("$LOAD_PATH", load_path);
  SWAP_GVAR("$LOADED_FEATURES", loaded_features);
  SWAP_VAR(ruby_top_self, oMain);
  SWAP_VAR(ruby_top_cref, top_cref);
  SWAP_VAR(rb_global_tbl, globals);
  SWAP_VAR(rb_syserr_tbl, syserrs);
  SWAP_VAR(ruby_cref, ruby_cref);
  // SWAP_VAR(ruby_class, rclass);
  SWAP(cMatch);
  SWAP(cMethod);
  SWAP(cUnboundMethod);
  SWAP(eRegexpError);
  SWAP(cNameErrorMesg);
  SWAP(eSysStackError);
  SWAP(eLocalJumpError);
}

/*
 * Swaps in the sandbox, turns it "on".
 */
void
sandbox_on( kit, wick )
  sandkit *kit;
  sandwick *wick;
{
  /* save everything */
  wick->banished = ruby_sandbox;
  wick->scope = ruby_scope;
  wick->dyna_vars = ruby_dyna_vars;
  curr_thread->sandbox = kit->self;

  sandbox_swap(kit, SANDBOX_REPLACE);
  ruby_scope = kit->scope;
  ruby_dyna_vars = 0;

  wick->exception = Qnil;
  wick->kit = kit;

  wick->old_toplevel = ruby_top_self;
}

/*
 * Swaps out the sandbox, turns it "off".
 */
void
sandbox_off( wick )
  sandwick *wick;
{
  sandkit *banished;

  Data_Get_Struct( wick->banished, sandkit, banished );
  sandbox_swap(banished, SANDBOX_REPLACE);
  ruby_scope = wick->scope;
  ruby_dyna_vars = wick->dyna_vars;
  curr_thread->sandbox = ruby_sandbox;
  ruby_top_self = wick->old_toplevel;
}

/*
 * Imports an object into a sandbox.  The matching class is found in the
 * sandbox.  If that class is a BoxedClass, we generate a reference.  Otherwise,
 * the object is marshalled. NOTE: This gets called before sandbox_on, then
 * sandbox_arg_load gets called on each.
 *
 * The +kit+ argument refers to the kit that the arg is being prepared for.
 */
static VALUE
sandbox_arg_prep(kit, obj)
  sandkit *kit;
  VALUE obj;
{
  VALUE link;
  VALUE klass;
  sandtransfer *t = alloc_sandtransfer();
  t->trans = TRANS_NONE;
  if (SPECIAL_CONST_P(obj))
  {
    t->val = obj;
    return (VALUE)t;
  }

  link = sandbox_get_linked_class(obj);
  if (!NIL_P(link))
  {
    VALUE box = sandbox_get_linked_box(obj);
    if (box == kit->self)
    {
      t->val = link;
      return (VALUE)t;
    }
  }

  klass = Qnil;
  link = Qnil;
  if (rb_type(obj) == T_OBJECT) {
    klass = sandbox_const_find(kit, rb_obj_classname(obj));
  }
  if (!NIL_P(klass))
  {
    link = sandbox_get_linked_class(klass);
  }
  if (NIL_P(link))
  {
    t->trans = TRANS_MARSHAL;
    t->val = rb_marshal_dump(obj, Qnil);
  }
  else
  {
    t->val = rb_obj_alloc(klass);
    sandbox_link_class(obj, t->val);
  }
  return (VALUE)t;
}

static VALUE
sandbox_arg_load(obj)
  VALUE obj;
{
  sandtransfer *t = (sandtransfer *)obj;
  if (t->trans == TRANS_MARSHAL)
  {
    obj = rb_marshal_load(t->val);
  }
  else
  {
    obj = t->val;
  }
  return obj;
}

/*
 * Wraps the sandbox action in a rescue block.
 */
static VALUE
sandbox_run_begin(wick)
  VALUE wick;
{
  return rb_rescue2(sandbox_run_wick, wick, sandbox_run_rescue, wick, rb_cObject, 0);
}

/*
 * The body of the sandbox begin..end.  Determines how the wick is wired and
 * works on carrying it out right.
 */
static VALUE
sandbox_run_wick(v)
  VALUE v;
{
  VALUE obj;
  sandkit *banished;
  sandwick *wick = (sandwick *)v;
  if (!rb_const_defined(rb_cObject, rb_intern("TOPLEVEL_BINDING"))) {
    rb_const_set(rb_cObject, rb_intern("TOPLEVEL_BINDING"), rb_eval_string("binding"));
  }
  ruby_top_self = rb_const_get(rb_cObject, rb_intern("TOPLEVEL_BINDING"));
  switch (wick->calltype)
  {
    case SANDBOX_EVAL:
      obj = sandbox_run_eval(wick);
      break;
    case SANDBOX_METHOD_CALL:
      obj = sandbox_run_method_call(wick);
      break;
    case SANDBOX_ACTION:
      obj = sandbox_run_action(wick);
      break;
    case SANDBOX_COPY:
      obj = sandbox_run_copy(wick);
      break;
  }

  Data_Get_Struct( wick->banished, sandkit, banished );
  return sandbox_arg_prep(banished, obj);
}

/*
 * Rescues any exception thrown inside the sandbox call.
 */
static VALUE
sandbox_run_rescue(v, exc)
  VALUE v, exc;
{
  sandwick *wick = (sandwick *)v;
  wick->exception = exc;
  return Qnil;
}

/*
 * Ensures the sandbox is closed properly and re-raises any exception in the
 * right environment.
 *
 * Used to be sandbox_whoa_whoa_whoa.
 */
static VALUE
sandbox_run_ensure(v)
  VALUE v;
{
  sandwick *wick = (sandwick *)v;
  VALUE exc = wick->exception;
  VALUE msg;
  const char *path;

  if (!NIL_P(exc))
  {
    msg = rb_funcall(exc, rb_intern("message"), 0);
    path = rb_obj_classname(exc);
  }
  sandbox_off( wick );
  if (!NIL_P(exc))
  {
    rb_raise(rb_eSandboxException, "%s: %s", path, rb_str_ptr(msg));
  }
}

/*
 * This is the central function now for all sandbox calling.  You pass in the
 * sandkit, which is essentially the parts of the Ruby interpreter needed to
 * run the sandbox.  The wick is a bit of information about how the sandbox
 * is to be called, what objects are to be imported, and so on.
 *
 * When combusted within this method, the sandbox gets rolling and either returns
 * the resulting object or tosses an exception from within the calling sandbox.
 */
VALUE
sandbox_run(kit, wick)
  sandkit *kit;
  sandwick *wick;
{
  VALUE obj;
  int i;

  for (i = 1; i < wick->argc; i++) {
    wick->argv[i] = sandbox_arg_prep(kit, wick->argv[i]);
  }
  sandbox_on(kit, wick);
  for (i = 1; i < wick->argc; i++) {
    wick->argv[i] = sandbox_arg_load(wick->argv[i]);
  }

  obj = rb_ensure(sandbox_run_begin, (VALUE)wick, sandbox_run_ensure, (VALUE)wick);
  return sandbox_arg_load(obj);
}

/* :nodoc: */
VALUE sandbox_copy( self, obj_name, obj )
  VALUE self;
  VALUE obj_name;
  VALUE obj;
{
  VALUE link;
  sandkit *kit;
  Data_Get_Struct( self, sandkit, kit );
  link = Qnil;//sandbox_get_linked_class(self);
  return sandbox_run(kit, sandbox_copy_wick(link, obj_name, obj));
}


/* :nodoc: */
VALUE
sandbox_eval( self, str )
  VALUE self, str;
{
  sandkit *kit;
  Data_Get_Struct( self, sandkit, kit );
  return sandbox_run(kit, sandbox_eval_wick(str));
}

/* :nodoc: */
void
check_import_type( klass )
  VALUE klass;
{
  if (!(TYPE(klass) == T_CLASS || TYPE(klass) == T_MODULE))
  {
    rb_raise(rb_eTypeError, "wrong argument type (expected Class or Module)");
  }
}

/*
 *  call-seq:
 *     sandbox.import(class)   => nil
 *
 *  Imports +class+ into +sandbox+.
 *
 */
VALUE
sandbox_import( self, klass )
  VALUE self, klass;
{
  VALUE sandklass;
  sandkit *kit;
  check_import_type( klass );
  Data_Get_Struct( self, sandkit, kit );
  sandklass = sandbox_import_class_path( kit, rb_class2name( klass ), 0 );
  return Qnil;
}

/*
 *  call-seq:
 *     sandbox.ref(class)   => nil
 *
 *  References top-level +class+ in the +sandbox+.
 *
 */
VALUE
sandbox_ref( self, klass )
  VALUE self, klass;
{
  sandkit *kit;
  check_import_type( klass );
  Data_Get_Struct( self, sandkit, kit );
  sandbox_import_class_path( kit, rb_class2name( klass ), 1 );
  return Qnil;
}

/*
 *  call-seq:
 *     sandbox.main   => obj
 *
 *  Returns the top-level 'main' object in +sandbox+.
 *
 */
VALUE
sandbox_get_main( self )
  VALUE self;
{
  sandkit *kit;
  Data_Get_Struct( self, sandkit, kit );
  return kit->oMain;
}

/*
 *  call-seq:
 *     Sandbox.new(opts={})   => obj
 *
 *  Returns a new Sandbox::Full.  (An alias for Sandbox::Full.new.)
 *
 */
VALUE sandbox_new( argc, argv, self )
  int argc;
  VALUE *argv;
  VALUE self;
{
  VALUE opts;
  if (rb_scan_args(argc, argv, "01", &opts) == 0)
  {
    return rb_funcall( rb_cSandboxFull, rb_intern("new"), 0 );
  }
  else
  {
    return rb_funcall( rb_cSandboxFull, rb_intern("new"), 1, opts );
  }
}

/*
 *  call-seq:
 *     Sandbox.safe(opts={})   => obj
 *
 *  Returns a newly created restricted sandbox.
 *  (See Sandbox.new and Sandbox::Safe.)
 *
 */
VALUE sandbox_safe( argc, argv, self )
  int argc;
  VALUE *argv;
  VALUE self;
{
  VALUE opts;
  if (rb_scan_args(argc, argv, "01", &opts) == 0)
  {
    return rb_funcall( rb_cSandboxSafe, rb_intern("new"), 0 );
  }
  else
  {
    return rb_funcall( rb_cSandboxSafe, rb_intern("new"), 1, opts );
  }
}

/*
 *  call-seq:
 *     Sandbox.current   => sandbox
 *
 *  Returns the current sandbox.
 *
 */
VALUE sandbox_current( self )
  VALUE self;
{
  return ruby_sandbox;
}

/*
 *  call-seq:
 *     BoxedClass.method_missing => obj
 *
 *  Executes the method under the class (or object)'s original environment,
 *  passing in and returning references as needed.
 */
static VALUE
sandbox_boxedclass_method_missing(argc, argv, self)
  int argc;
  VALUE *argv;
  VALUE self;
{
  VALUE link = sandbox_get_linked_class(self);
  if (NIL_P(link)) {
    /* FIXME: oh, wait, this shouldn't happen! */
    rb_raise(rb_eNoMethodError, "no link for %s", RSTRING(rb_inspect(self))->ptr);
  } else {
    int i;
    sandkit *kit;
    VALUE box = sandbox_get_linked_box(self);
    Data_Get_Struct(box, sandkit, kit);
    return sandbox_run(kit, sandbox_method_wick(link, argc, argv));
  }
}

static void
Init_kit(kit, use_base)
  sandkit *kit;
  int use_base;
{
  VALUE metaclass;
  VALUE norm_cFloat;
  VALUE rb_mMarshal;

  kit->tbl = st_init_numtable();
  kit->globals = st_init_numtable();
  kit->syserrs = st_init_numtable();
  kit->cObject = 0;

#ifdef KIT2
  kit->cBasicObject = sandbox_bootclass(kit, "BasicObject", 0);
  kit->cObject = sandbox_bootclass(kit, "Object", kit->cBasicObject);
#else
  kit->cObject = sandbox_bootclass(kit, "Object", 0);
#endif
  kit->cModule = sandbox_bootclass(kit, "Module", kit->cObject);
  kit->cClass =  sandbox_bootclass(kit, "Class",  kit->cModule);

#ifdef KIT2
  metaclass = sandbox_metaclass(kit, kit->cBasicObject, kit->cClass);
  metaclass = sandbox_metaclass(kit, kit->cObject, metaclass);
  SAND_COPY(cBasicObject, "==");
  SAND_COPY(cBasicObject, "equal?");
#else
  metaclass = sandbox_metaclass(kit, kit->cObject, kit->cClass);
#endif
  metaclass = sandbox_metaclass(kit, kit->cModule, metaclass);
  metaclass = sandbox_metaclass(kit, kit->cClass, metaclass);

  kit->mKernel = sandbox_defmodule(kit, "Kernel");
  rb_include_module(kit->cObject, kit->mKernel);
  rb_define_alloc_func(kitBasicObject, sandbox_alloc_obj);

  rb_define_private_method(kit->cModule, "method_added", sandbox_dummy, 1);
  rb_define_private_method(kitBasicObject, "initialize", sandbox_dummy, 0);
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
  rb_define_method(kit->cModule, "name", sandbox_mod_name, 0);
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
  sandbox_define_hooked_variable(kit, "$!", &ruby_errinfo, 0, sandbox_errinfo_setter);

  SAND_COPY_KERNEL("eval");
  SAND_COPY_KERNEL("iterator?");
  SAND_COPY_KERNEL("block_given?");
  SAND_COPY_KERNEL("method_missing");
  SAND_COPY_KERNEL("loop");

  SAND_COPY(mKernel, "respond_to?");
  
  SAND_COPY_KERNEL("raise");
  SAND_COPY_KERNEL("fail");

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

  kit->cString = sandbox_defclass(kit, "String", kit->cObject);
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

  kit->cNameErrorMesg = sandbox_defclass_under(kit, kit->eNameError, "message", kit->cData);
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
  kit->eSystemCallError = sandbox_defclass(kit, "SystemCallError", kit->eStandardError);
  SAND_COPY(eSystemCallError, "initialize");
  SAND_COPY(eSystemCallError, "errno");
  SAND_COPY_S(eSystemCallError, "===");

  kit->mErrno = sandbox_defmodule(kit, "Errno");
  SAND_SYSERR(mErrno, "EPERM");
  SAND_SYSERR(mErrno, "ENOENT");
  SAND_SYSERR(mErrno, "ESRCH");
  SAND_SYSERR(mErrno, "EINTR");
  SAND_SYSERR(mErrno, "EIO");
  SAND_SYSERR(mErrno, "ENXIO");
  SAND_SYSERR(mErrno, "E2BIG");
  SAND_SYSERR(mErrno, "ENOEXEC");
  SAND_SYSERR(mErrno, "EBADF");
  SAND_SYSERR(mErrno, "ECHILD");
  SAND_SYSERR(mErrno, "EAGAIN");
  SAND_SYSERR(mErrno, "ENOMEM");
  SAND_SYSERR(mErrno, "EACCES");
  SAND_SYSERR(mErrno, "EFAULT");
  SAND_SYSERR(mErrno, "ENOTBLK");
  SAND_SYSERR(mErrno, "EBUSY");
  SAND_SYSERR(mErrno, "EEXIST");
  SAND_SYSERR(mErrno, "EXDEV");
  SAND_SYSERR(mErrno, "ENODEV");
  SAND_SYSERR(mErrno, "ENOTDIR");
  SAND_SYSERR(mErrno, "EISDIR");
  SAND_SYSERR(mErrno, "EINVAL");
  SAND_SYSERR(mErrno, "ENFILE");
  SAND_SYSERR(mErrno, "EMFILE");
  SAND_SYSERR(mErrno, "ENOTTY");
  SAND_SYSERR(mErrno, "ETXTBSY");
  SAND_SYSERR(mErrno, "EFBIG");
  SAND_SYSERR(mErrno, "ENOSPC");
  SAND_SYSERR(mErrno, "ESPIPE");
  SAND_SYSERR(mErrno, "EROFS");
  SAND_SYSERR(mErrno, "EMLINK");
  SAND_SYSERR(mErrno, "EPIPE");
  SAND_SYSERR(mErrno, "EDOM");
  SAND_SYSERR(mErrno, "ERANGE");
  SAND_SYSERR(mErrno, "EDEADLK");
  SAND_SYSERR(mErrno, "ENAMETOOLONG");
  SAND_SYSERR(mErrno, "ENOLCK");
  SAND_SYSERR(mErrno, "ENOSYS");
  SAND_SYSERR(mErrno, "ENOTEMPTY");
  SAND_SYSERR(mErrno, "ELOOP");
  SAND_SYSERR(mErrno, "EWOULDBLOCK");
  SAND_SYSERR(mErrno, "ENOMSG");
  SAND_SYSERR(mErrno, "EIDRM");
  SAND_SYSERR(mErrno, "ECHRNG");
  SAND_SYSERR(mErrno, "EL2NSYNC");
  SAND_SYSERR(mErrno, "EL3HLT");
  SAND_SYSERR(mErrno, "EL3RST");
  SAND_SYSERR(mErrno, "ELNRNG");
  SAND_SYSERR(mErrno, "EUNATCH");
  SAND_SYSERR(mErrno, "ENOCSI");
  SAND_SYSERR(mErrno, "EL2HLT");
  SAND_SYSERR(mErrno, "EBADE");
  SAND_SYSERR(mErrno, "EBADR");
  SAND_SYSERR(mErrno, "EXFULL");
  SAND_SYSERR(mErrno, "ENOANO");
  SAND_SYSERR(mErrno, "EBADRQC");
  SAND_SYSERR(mErrno, "EBADSLT");
  SAND_SYSERR(mErrno, "EDEADLOCK");
  SAND_SYSERR(mErrno, "EBFONT");
  SAND_SYSERR(mErrno, "ENOSTR");
  SAND_SYSERR(mErrno, "ENODATA");
  SAND_SYSERR(mErrno, "ETIME");
  SAND_SYSERR(mErrno, "ENOSR");
  SAND_SYSERR(mErrno, "ENONET");
  SAND_SYSERR(mErrno, "ENOPKG");
  SAND_SYSERR(mErrno, "EREMOTE");
  SAND_SYSERR(mErrno, "ENOLINK");
  SAND_SYSERR(mErrno, "EADV");
  SAND_SYSERR(mErrno, "ESRMNT");
  SAND_SYSERR(mErrno, "ECOMM");
  SAND_SYSERR(mErrno, "EPROTO");
  SAND_SYSERR(mErrno, "EMULTIHOP");
  SAND_SYSERR(mErrno, "EDOTDOT");
  SAND_SYSERR(mErrno, "EBADMSG");
  SAND_SYSERR(mErrno, "EOVERFLOW");
  SAND_SYSERR(mErrno, "ENOTUNIQ");
  SAND_SYSERR(mErrno, "EBADFD");
  SAND_SYSERR(mErrno, "EREMCHG");
  SAND_SYSERR(mErrno, "ELIBACC");
  SAND_SYSERR(mErrno, "ELIBBAD");
  SAND_SYSERR(mErrno, "ELIBSCN");
  SAND_SYSERR(mErrno, "ELIBMAX");
  SAND_SYSERR(mErrno, "ELIBEXEC");
  SAND_SYSERR(mErrno, "EILSEQ");
  SAND_SYSERR(mErrno, "ERESTART");
  SAND_SYSERR(mErrno, "ESTRPIPE");
  SAND_SYSERR(mErrno, "EUSERS");
  SAND_SYSERR(mErrno, "ENOTSOCK");
  SAND_SYSERR(mErrno, "EDESTADDRREQ");
  SAND_SYSERR(mErrno, "EMSGSIZE");
  SAND_SYSERR(mErrno, "EPROTOTYPE");
  SAND_SYSERR(mErrno, "ENOPROTOOPT");
  SAND_SYSERR(mErrno, "EPROTONOSUPPORT");
  SAND_SYSERR(mErrno, "ESOCKTNOSUPPORT");
  SAND_SYSERR(mErrno, "EOPNOTSUPP");
  SAND_SYSERR(mErrno, "EPFNOSUPPORT");
  SAND_SYSERR(mErrno, "EAFNOSUPPORT");
  SAND_SYSERR(mErrno, "EADDRINUSE");
  SAND_SYSERR(mErrno, "EADDRNOTAVAIL");
  SAND_SYSERR(mErrno, "ENETDOWN");
  SAND_SYSERR(mErrno, "ENETUNREACH");
  SAND_SYSERR(mErrno, "ENETRESET");
  SAND_SYSERR(mErrno, "ECONNABORTED");
  SAND_SYSERR(mErrno, "ECONNRESET");
  SAND_SYSERR(mErrno, "ENOBUFS");
  SAND_SYSERR(mErrno, "EISCONN");
  SAND_SYSERR(mErrno, "ENOTCONN");
  SAND_SYSERR(mErrno, "ESHUTDOWN");
  SAND_SYSERR(mErrno, "ETOOMANYREFS");
  SAND_SYSERR(mErrno, "ETIMEDOUT");
  SAND_SYSERR(mErrno, "ECONNREFUSED");
  SAND_SYSERR(mErrno, "EHOSTDOWN");
  SAND_SYSERR(mErrno, "EHOSTUNREACH");
  SAND_SYSERR(mErrno, "EALREADY");
  SAND_SYSERR(mErrno, "EINPROGRESS");
  SAND_SYSERR(mErrno, "ESTALE");
  SAND_SYSERR(mErrno, "EUCLEAN");
  SAND_SYSERR(mErrno, "ENOTNAM");
  SAND_SYSERR(mErrno, "ENAVAIL");
  SAND_SYSERR(mErrno, "EISNAM");
  SAND_SYSERR(mErrno, "EREMOTEIO");
  SAND_SYSERR(mErrno, "EDQUOT");

  kit->eLocalJumpError = sandbox_defclass(kit, "LocalJumpError", kit->eStandardError);
  SAND_COPY(eLocalJumpError, "exit_value");
  SAND_COPY(eLocalJumpError, "reason");

/*
  rb_global_variable(&exception_error);
  exception_error = rb_exc_new2(rb_eFatal, "exception reentered");
*/

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

  kit->cBinding = sandbox_defclass(kit, "Binding", kit->cObject);
  rb_undef_alloc_func(kit->cBinding);
  SAND_UNDEF(cBinding, "new");
  SAND_COPY(cBinding, "clone");
  SAND_COPY(cBinding, "dup");
  SAND_COPY_KERNEL("binding");

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

  kit->eRegexpError = sandbox_defclass(kit, "RegexpError", kit->eStandardError);

  sandbox_define_virtual_variable(kit, "$~", sandbox_match_getter, sandbox_match_setter);
  sandbox_define_virtual_variable(kit, "$&", sandbox_last_match_getter, 0);
  sandbox_define_virtual_variable(kit, "$`", sandbox_prematch_getter, 0);
  sandbox_define_virtual_variable(kit, "$'", sandbox_postmatch_getter, 0);
  sandbox_define_virtual_variable(kit, "$+", sandbox_last_paren_match_getter, 0);

  /*
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

  kit->cTime = sandbox_defclass(kit, "Time", kit->cObject);
  rb_include_module(kit->cTime, kit->mComparable);

  SAND_COPY_ALLOC(cTime);
  SAND_COPY_S(cTime, "now");
  SAND_COPY_S(cTime, "at");
  SAND_COPY_S(cTime, "utc");
  SAND_COPY_S(cTime, "gm");
  SAND_COPY_S(cTime, "local");
  SAND_COPY_S(cTime, "mktime");

  SAND_COPY_S(cTime, "times");

  SAND_COPY(cTime, "to_i");
  SAND_COPY(cTime, "to_f");
  SAND_COPY(cTime, "<=>");
  SAND_COPY(cTime, "eql?");
  SAND_COPY(cTime, "hash");
  SAND_COPY(cTime, "initialize");
  SAND_COPY(cTime, "initialize_copy");

  SAND_COPY(cTime, "localtime");
  SAND_COPY(cTime, "gmtime");
  SAND_COPY(cTime, "utc");
  SAND_COPY(cTime, "getlocal");
  SAND_COPY(cTime, "getgm");
  SAND_COPY(cTime, "getutc");

  SAND_COPY(cTime, "ctime");
  SAND_COPY(cTime, "asctime");
  SAND_COPY(cTime, "to_s");
  SAND_COPY(cTime, "inspect");
  SAND_COPY(cTime, "to_a");

  SAND_COPY(cTime, "+");
  SAND_COPY(cTime, "-");

  SAND_COPY(cTime, "succ");
  SAND_COPY(cTime, "sec");
  SAND_COPY(cTime, "min");
  SAND_COPY(cTime, "hour");
  SAND_COPY(cTime, "mday");
  SAND_COPY(cTime, "day");
  SAND_COPY(cTime, "mon");
  SAND_COPY(cTime, "month");
  SAND_COPY(cTime, "year");
  SAND_COPY(cTime, "wday");
  SAND_COPY(cTime, "yday");
  SAND_COPY(cTime, "isdst");
  SAND_COPY(cTime, "dst?");
  SAND_COPY(cTime, "zone");
  SAND_COPY(cTime, "gmtoff");
  SAND_COPY(cTime, "gmt_offset");
  SAND_COPY(cTime, "utc_offset");

  SAND_COPY(cTime, "utc?");
  SAND_COPY(cTime, "gmt?");

  SAND_COPY(cTime, "tv_sec");
  SAND_COPY(cTime, "tv_usec");
  SAND_COPY(cTime, "usec");

  SAND_COPY(cTime, "strftime");

  /* methods for marshaling */
  SAND_COPY(cTime, "_dump");
  SAND_COPY_S(cTime, "_load");

  SAND_COPY_KERNEL("srand");
  SAND_COPY_KERNEL("rand");

  kit->mMath = sandbox_defmodule(kit, "Math");

  norm_cFloat = rb_cFloat;
  rb_cFloat = kit->cFloat;
  rb_define_const(kit->mMath, "PI", rb_float_new(atan(1.0)*4.0));
  rb_define_const(kit->mMath, "E", rb_float_new(exp(1.0)));
  rb_cFloat = norm_cFloat;

  SAND_COPY_S(mMath, "atan2");
  SAND_COPY_S(mMath, "cos");
  SAND_COPY_S(mMath, "sin");
  SAND_COPY_S(mMath, "tan");

  SAND_COPY_S(mMath, "acos");
  SAND_COPY_S(mMath, "asin");
  SAND_COPY_S(mMath, "atan");

  SAND_COPY_S(mMath, "cosh");
  SAND_COPY_S(mMath, "sinh");
  SAND_COPY_S(mMath, "tanh");

  SAND_COPY_S(mMath, "acosh");
  SAND_COPY_S(mMath, "asinh");
  SAND_COPY_S(mMath, "atanh");

  SAND_COPY_S(mMath, "exp");
  SAND_COPY_S(mMath, "log");
  SAND_COPY_S(mMath, "log10");
  SAND_COPY_S(mMath, "sqrt");

  SAND_COPY_S(mMath, "frexp");
  SAND_COPY_S(mMath, "ldexp");

  SAND_COPY_S(mMath, "hypot");

  SAND_COPY_S(mMath, "erf");
  SAND_COPY_S(mMath, "erfc");

  rb_mMarshal = rb_const_get(rb_cObject, rb_intern("Marshal"));
  kit->mMarshal = sandbox_defmodule(kit, "Marshal");

  SAND_COPY_S(mMarshal, "dump");
  SAND_COPY_S(mMarshal, "load");
  SAND_COPY_S(mMarshal, "restore");

  SAND_COPY_CONST(mMarshal, "MAJOR_VERSION");
  SAND_COPY_CONST(mMarshal, "MINOR_VERSION");

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
  kit->load_path = rb_funcall(kit->cArray, rb_intern("new"), 0);
  kit->loaded_features = rb_funcall(kit->cArray, rb_intern("new"), 0);
  kit->_progname = sandbox_str(kit, "(sandbox)");
  sandbox_define_hooked_variable(kit, "$0", &kit->_progname, 0, 0);

  /* BoxedClass hooks */
  kit->cBoxedClass = sandbox_defclass(kit, "BoxedClass", kit->cObject);
  rb_define_singleton_method( kit->cBoxedClass, "method_missing", sandbox_boxedclass_method_missing, -1 );
  rb_define_method( kit->cBoxedClass, "method_missing", sandbox_boxedclass_method_missing, -1 );
  SAND_UNDEF(cBoxedClass, "new");
}

static void
Init_kit_load(kit, use_base)
  sandkit *kit;
  int use_base;
{
  sandbox_define_readonly_variable(kit, "$:", &kit->load_path);
  sandbox_define_readonly_variable(kit, "$-I", &kit->load_path);
  sandbox_define_readonly_variable(kit, "$LOAD_PATH", &kit->load_path);

  sandbox_define_readonly_variable(kit, "$\"", &kit->loaded_features);
  sandbox_define_readonly_variable(kit, "$LOADED_FEATURES", &kit->loaded_features);

  SAND_COPY_KERNEL("load");
  SAND_COPY_KERNEL("require");
  SAND_COPY(cModule, "autoload");
  SAND_COPY(cModule, "autoload?");
  SAND_COPY_KERNEL("autoload");
  SAND_COPY_KERNEL("autoload?");

  /*
  rb_global_variable(&ruby_wrapper);

  rb_global_variable(&ruby_dln_librefs);
  ruby_dln_librefs = rb_ary_new();
  */
}

static void
Init_kit_io(kit, use_base)
  sandkit *kit;
  int use_base;
{
  VALUE rb_mFConst;
  
  kit->eIOError = sandbox_defclass(kit, "IOError", kit->eStandardError);
  kit->eEOFError = sandbox_defclass(kit, "EOFError", kit->eIOError);

  kit->cIO = sandbox_defclass(kit, "IO", kit->cObject);
  rb_include_module(kit->cIO, kit->mEnumerable);

  SAND_COPY_ALLOC(cIO);
  SAND_COPY_S(cIO, "new");
  SAND_COPY_S(cIO, "open");
  SAND_COPY_S(cIO, "sysopen");
  SAND_COPY_S(cIO, "for_fd");
  SAND_COPY_S(cIO, "popen");
  SAND_COPY_S(cIO, "foreach");
  SAND_COPY_S(cIO, "readlines");
  SAND_COPY_S(cIO, "read");
  SAND_COPY_S(cIO, "select");
  SAND_COPY_S(cIO, "pipe");

  SAND_COPY(cIO, "initialize");

  /*
  rb_output_fs = Qnil;
  rb_define_hooked_variable("$,", &rb_output_fs, 0, rb_str_setter);

  rb_define_hooked_variable("$/", &rb_rs, 0, rb_str_setter);
  rb_define_hooked_variable("$-0", &rb_rs, 0, rb_str_setter);
  rb_define_hooked_variable("$\\", &rb_output_rs, 0, rb_str_setter);

  rb_define_hooked_variable("$.", &lineno, 0, lineno_setter);
  rb_define_virtual_variable("$_", rb_lastline_get, rb_lastline_set);
  */

  SAND_COPY(cIO, "initialize_copy");
  SAND_COPY(cIO, "reopen");

  SAND_COPY(cIO, "print");
  SAND_COPY(cIO, "putc");
  SAND_COPY(cIO, "puts");
  SAND_COPY(cIO, "printf");

  SAND_COPY(cIO, "each");
  SAND_COPY(cIO, "each_line");
  SAND_COPY(cIO, "each_byte");

  SAND_COPY(cIO, "syswrite");
  SAND_COPY(cIO, "sysread");

  SAND_COPY(cIO, "fileno");
  rb_define_alias(kit->cIO, "to_i", "fileno");
  SAND_COPY(cIO, "to_io");

  SAND_COPY(cIO, "fsync");
  SAND_COPY(cIO, "sync");
  SAND_COPY(cIO, "sync=");

  SAND_COPY(cIO, "lineno");
  SAND_COPY(cIO, "lineno=");

  SAND_COPY(cIO, "readlines");

  SAND_COPY(cIO, "read_nonblock");
  SAND_COPY(cIO, "write_nonblock");
  SAND_COPY(cIO, "readpartial");
  SAND_COPY(cIO, "read");
  SAND_COPY(cIO, "write");
  SAND_COPY(cIO, "gets");
  SAND_COPY(cIO, "readline");
  SAND_COPY(cIO, "getc");
  SAND_COPY(cIO, "readchar");
  SAND_COPY(cIO, "ungetc");
  SAND_COPY(cIO, "<<");
  SAND_COPY(cIO, "flush");
  SAND_COPY(cIO, "tell");
  SAND_COPY(cIO, "seek");
  SAND_COPY_CONST(cIO, "SEEK_SET");
  SAND_COPY_CONST(cIO, "SEEK_CUR");
  SAND_COPY_CONST(cIO, "SEEK_END");
  SAND_COPY(cIO, "rewind");
  SAND_COPY(cIO, "pos");
  SAND_COPY(cIO, "pos=");
  SAND_COPY(cIO, "eof");
  SAND_COPY(cIO, "eof?");

  SAND_COPY(cIO, "close");
  SAND_COPY(cIO, "closed?");
  SAND_COPY(cIO, "close_read");
  SAND_COPY(cIO, "close_write");

  SAND_COPY(cIO, "isatty");
  SAND_COPY(cIO, "tty?");
  SAND_COPY(cIO, "binmode");
  SAND_COPY(cIO, "sysseek");

  SAND_COPY(cIO, "ioctl");
  SAND_COPY(cIO, "fcntl");
  SAND_COPY(cIO, "pid");
  SAND_COPY(cIO, "inspect");

  kit->mFileTest = sandbox_defmodule(kit, "FileTest");
  kit->cFile = sandbox_defclass(kit, "File", kit->cIO);

#define SAND_COPY_FILETEST(M) SAND_COPY_S(mFileTest, M); SAND_COPY_S(cFile, M);

  SAND_COPY_FILETEST("directory?");
  SAND_COPY_FILETEST("exist?");
  SAND_COPY_FILETEST("exists?"); /* temporary */
  SAND_COPY_FILETEST("readable?");
  SAND_COPY_FILETEST("readable_real?");
  SAND_COPY_FILETEST("writable?");
  SAND_COPY_FILETEST("writable_real?");
  SAND_COPY_FILETEST("executable?");
  SAND_COPY_FILETEST("executable_real?");
  SAND_COPY_FILETEST("file?");
  SAND_COPY_FILETEST("zero?");
  SAND_COPY_FILETEST("size?");
  SAND_COPY_FILETEST("size");
  SAND_COPY_FILETEST("owned?");
  SAND_COPY_FILETEST("grpowned?");

  SAND_COPY_FILETEST("pipe?");
  SAND_COPY_FILETEST("symlink?");
  SAND_COPY_FILETEST("socket?");

  SAND_COPY_FILETEST("blockdev?");
  SAND_COPY_FILETEST("chardev?");

  SAND_COPY_FILETEST("setuid?");
  SAND_COPY_FILETEST("setgid?");
  SAND_COPY_FILETEST("sticky?");

  SAND_COPY_FILETEST("identical?");

  SAND_COPY_S(cFile, "stat");
  SAND_COPY_S(cFile, "lstat");
  SAND_COPY_S(cFile, "ftype");

  SAND_COPY_S(cFile, "atime");
  SAND_COPY_S(cFile, "mtime");
  SAND_COPY_S(cFile, "ctime");

  SAND_COPY_S(cFile, "utime");
  SAND_COPY_S(cFile, "chmod");
  SAND_COPY_S(cFile, "chown");
  SAND_COPY_S(cFile, "lchmod");
  SAND_COPY_S(cFile, "lchown");

  SAND_COPY_S(cFile, "link");
  SAND_COPY_S(cFile, "symlink");
  SAND_COPY_S(cFile, "readlink");

  SAND_COPY_S(cFile, "unlink");
  SAND_COPY_S(cFile, "delete");
  SAND_COPY_S(cFile, "rename");
  SAND_COPY_S(cFile, "umask");
  SAND_COPY_S(cFile, "truncate");
  SAND_COPY_S(cFile, "expand_path");
  SAND_COPY_S(cFile, "basename");
  SAND_COPY_S(cFile, "dirname");
  SAND_COPY_S(cFile, "extname");

  SAND_COPY_S(cFile, "split");
  SAND_COPY_S(cFile, "join");

  // SAND_DUP_CONST(cFile, "Separator");
  // SAND_DUP_CONST(cFile, "SEPARATOR");
  // SAND_DUP_CONST(cFile, "ALT_SEPARATOR");
  // SAND_DUP_CONST(cFile, "PATH_SEPARATOR");

  SAND_COPY(cIO, "stat"); /* this is IO's method */
  SAND_COPY(cFile, "lstat");

  SAND_COPY(cFile, "atime");
  SAND_COPY(cFile, "mtime");
  SAND_COPY(cFile, "ctime");

  SAND_COPY(cFile, "chmod");
  SAND_COPY(cFile, "chown");
  SAND_COPY(cFile, "truncate");

  SAND_COPY(cFile, "flock");

  rb_mFConst = rb_const_get_at(rb_cFile, rb_intern("Constants"));
  kit->mFConst = sandbox_defmodule_under(kit, kit->cFile, "Constants");
  rb_include_module(kit->cIO, kit->mFConst);
  SAND_COPY_CONST(mFConst, "LOCK_SH");
  SAND_COPY_CONST(mFConst, "LOCK_EX");
  SAND_COPY_CONST(mFConst, "LOCK_UN");
  SAND_COPY_CONST(mFConst, "LOCK_NB");

  SAND_COPY(cFile, "path");
  SAND_COPY_KERNEL("test");

  kit->cStat = sandbox_defclass_under(kit, kit->cFile, "Stat", kit->cObject);
  SAND_COPY_ALLOC(cStat);
  SAND_COPY(cStat, "initialize");
  SAND_COPY(cStat, "initialize_copy");

  rb_include_module(kit->cStat, kit->mComparable);

  SAND_COPY(cStat, "<=>");

  SAND_COPY(cStat, "dev");
  SAND_COPY(cStat, "dev_major");
  SAND_COPY(cStat, "dev_minor");
  SAND_COPY(cStat, "ino");
  SAND_COPY(cStat, "mode");
  SAND_COPY(cStat, "nlink");
  SAND_COPY(cStat, "uid");
  SAND_COPY(cStat, "gid");
  SAND_COPY(cStat, "rdev");
  SAND_COPY(cStat, "rdev_major");
  SAND_COPY(cStat, "rdev_minor");
  SAND_COPY(cStat, "size");
  SAND_COPY(cStat, "blksize");
  SAND_COPY(cStat, "blocks");
  SAND_COPY(cStat, "atime");
  SAND_COPY(cStat, "mtime");
  SAND_COPY(cStat, "ctime");

  SAND_COPY(cStat, "inspect");

  SAND_COPY(cStat, "ftype");

  SAND_COPY(cStat, "directory?");
  SAND_COPY(cStat, "readable?");
  SAND_COPY(cStat, "readable_real?");
  SAND_COPY(cStat, "writable?");
  SAND_COPY(cStat, "writable_real?");
  SAND_COPY(cStat, "executable?");
  SAND_COPY(cStat, "executable_real?");
  SAND_COPY(cStat, "file?");
  SAND_COPY(cStat, "zero?");
  SAND_COPY(cStat, "size?");
  SAND_COPY(cStat, "owned?");
  SAND_COPY(cStat, "grpowned?");

  SAND_COPY(cStat, "pipe?");
  SAND_COPY(cStat, "symlink?");
  SAND_COPY(cStat, "socket?");

  SAND_COPY(cStat, "blockdev?");
  SAND_COPY(cStat, "chardev?");

  SAND_COPY(cStat, "setuid?");
  SAND_COPY(cStat, "setgid?");
  SAND_COPY(cStat, "sticky?");

  SAND_COPY(cFile, "initialize");

  SAND_COPY_CONST(mFConst, "RDONLY");
  SAND_COPY_CONST(mFConst, "WRONLY");
  SAND_COPY_CONST(mFConst, "RDWR");
  SAND_COPY_CONST(mFConst, "APPEND");
  SAND_COPY_CONST(mFConst, "CREAT");
  SAND_COPY_CONST(mFConst, "EXCL");
  SAND_COPY_IF_CONST(mFConst, "NONBLOCK");
  SAND_COPY_CONST(mFConst, "TRUNC");
  SAND_COPY_IF_CONST(mFConst, "NOCTTY");
  SAND_COPY_IF_CONST(mFConst, "BINARY");
  SAND_COPY_IF_CONST(mFConst, "SYNC");

  kit->cDir = sandbox_defclass(kit, "Dir", kit->cObject);

  rb_include_module(kit->cDir, kit->mEnumerable);

  SAND_COPY_ALLOC(cDir);
  SAND_COPY_S(cDir, "open");
  SAND_COPY_S(cDir, "foreach");
  SAND_COPY_S(cDir, "entries");

  SAND_COPY(cDir, "initialize");
  SAND_COPY(cDir, "path");
  SAND_COPY(cDir, "read");
  SAND_COPY(cDir, "each");
  SAND_COPY(cDir, "rewind");
  SAND_COPY(cDir, "tell");
  SAND_COPY(cDir, "seek");
  SAND_COPY(cDir, "pos");
  SAND_COPY(cDir, "pos=");
  SAND_COPY(cDir, "close");

  SAND_COPY_S(cDir, "chdir");
  SAND_COPY_S(cDir, "getwd");
  SAND_COPY_S(cDir, "pwd");
  SAND_COPY_S(cDir, "chroot");
  SAND_COPY_S(cDir, "mkdir");
  SAND_COPY_S(cDir, "rmdir");
  SAND_COPY_S(cDir, "delete");
  SAND_COPY_S(cDir, "unlink");

  SAND_COPY_S(cDir, "glob");
  SAND_COPY_S(cDir, "[]");

  SAND_COPY_S(cFile, "fnmatch");
  SAND_COPY_S(cFile, "fnmatch?");

  SAND_COPY_CONST(mFConst, "FNM_NOESCAPE");
  SAND_COPY_CONST(mFConst, "FNM_PATHNAME");
  SAND_COPY_CONST(mFConst, "FNM_DOTMATCH");
  SAND_COPY_CONST(mFConst, "FNM_CASEFOLD");
  SAND_COPY_CONST(mFConst, "FNM_SYSCASE");
}

static void
Init_kit_env(kit, use_base)
  sandkit *kit;
  int use_base;
{
  // rb_define_const(kit->cObject, "ENV", sandbox_dup_into(kit, 
  //       rb_funcall(rb_const_get(rb_cObject, rb_intern("ENV")), rb_intern("to_hash"), 0)));
  SAND_COPY_KERNEL("syscall");
  SAND_COPY_KERNEL("open");
  SAND_COPY_KERNEL("printf");
  SAND_COPY_KERNEL("print");
  SAND_COPY_KERNEL("putc");
  SAND_COPY_KERNEL("puts");
  SAND_COPY_KERNEL("gets");
  SAND_COPY_KERNEL("readline");
  SAND_COPY_KERNEL("getc");
  SAND_COPY_KERNEL("select");
  SAND_COPY_KERNEL("readlines");
  SAND_COPY_KERNEL("`");
  SAND_COPY_KERNEL("p");
  SAND_COPY(mKernel, "display");

  // kit->load_path = sandbox_dup_into(kit, rb_gv_get("$LOAD_PATH"));
  /*
  rb_define_variable("$stdin", &rb_stdin);
  rb_stdin = prep_stdio(stdin, FMODE_READABLE, rb_cIO);
  rb_define_hooked_variable("$stdout", &rb_stdout, 0, stdout_setter);
  rb_stdout = prep_stdio(stdout, FMODE_WRITABLE, rb_cIO);
  rb_define_hooked_variable("$stderr", &rb_stderr, 0, stdout_setter);
  rb_stderr = prep_stdio(stderr, FMODE_WRITABLE, rb_cIO);
  rb_define_hooked_variable("$>", &rb_stdout, 0, stdout_setter);
  orig_stdout = rb_stdout;
  rb_deferr = orig_stderr = rb_stderr;
  */

  /* variables to be removed in 1.8.1 */
  /*
  rb_define_hooked_variable("$defout", &rb_stdout, 0, defout_setter);
  rb_define_hooked_variable("$deferr", &rb_stderr, 0, deferr_setter);
  */

  /* constants to hold original stdin/stdout/stderr */
  SAND_COPY_CONST(cObject, "STDIN");
  SAND_COPY_CONST(cObject, "STDOUT");
  SAND_COPY_CONST(cObject, "STDERR");

  /*
  rb_define_readonly_variable("$<", &argf);
  VALUE argf = rb_obj_alloc(kit->cObject);
  rb_extend_object(argf, kit->mEnumerable);
  rb_define_global_const("ARGF", argf);
  rb_define_const(kit->cObject, "ARGV", argf);

  rb_define_singleton_method(argf, "to_s", argf_to_s, 0);
  rb_define_singleton_method(argf, "fileno", argf_fileno, 0);
  rb_define_singleton_method(argf, "to_i", argf_fileno, 0);
  rb_define_singleton_method(argf, "to_io", argf_to_io, 0);
  rb_define_singleton_method(argf, "each",  argf_each_line, -1);
  rb_define_singleton_method(argf, "each_line",  argf_each_line, -1);
  rb_define_singleton_method(argf, "each_byte",  argf_each_byte, 0);

  rb_define_singleton_method(argf, "read",  argf_read, -1);
  rb_define_singleton_method(argf, "readlines", rb_f_readlines, -1);
  rb_define_singleton_method(argf, "to_a", rb_f_readlines, -1);
  rb_define_singleton_method(argf, "gets", rb_f_gets, -1);
  rb_define_singleton_method(argf, "readline", rb_f_readline, -1);
  rb_define_singleton_method(argf, "getc", argf_getc, 0);
  rb_define_singleton_method(argf, "readchar", argf_readchar, 0);
  rb_define_singleton_method(argf, "tell", argf_tell, 0);
  rb_define_singleton_method(argf, "seek", argf_seek_m, -1);
  rb_define_singleton_method(argf, "rewind", argf_rewind, 0);
  rb_define_singleton_method(argf, "pos", argf_tell, 0);
  rb_define_singleton_method(argf, "pos=", argf_set_pos, 1);
  rb_define_singleton_method(argf, "eof", argf_eof, 0);
  rb_define_singleton_method(argf, "eof?", argf_eof, 0);
  rb_define_singleton_method(argf, "binmode", argf_binmode, 0);

  rb_define_singleton_method(argf, "filename", argf_filename, 0);
  rb_define_singleton_method(argf, "path", argf_filename, 0);
  rb_define_singleton_method(argf, "file", argf_file, 0);
  rb_define_singleton_method(argf, "skip", argf_skip, 0);
  rb_define_singleton_method(argf, "close", argf_close_m, 0);
  rb_define_singleton_method(argf, "closed?", argf_closed, 0);

  rb_define_singleton_method(argf, "lineno",   argf_lineno, 0);
  rb_define_singleton_method(argf, "lineno=",  argf_set_lineno, 1);

  rb_global_variable(&current_file);
  rb_define_readonly_variable("$FILENAME", &filename);
  filename = rb_str_new2("-");

  rb_define_virtual_variable("$-i", opt_i_get, opt_i_set);
  */
}

static void
Init_kit_real(kit, use_base)
  sandkit *kit;
  int use_base;
{
  VALUE cThGroup;
  VALUE rb_cThGroup;
  VALUE rb_cProcStatus;
  VALUE rb_mProcess;
  VALUE rb_mProcUID;
  VALUE rb_mProcGID;
  VALUE rb_mProcID_Syscall;
  VALUE rb_mObSpace;


  /* FIXME: reimplement */
  SAND_COPY_KERNEL("abort");
  SAND_COPY_KERNEL("at_exit");
  SAND_COPY_KERNEL("caller");
  SAND_COPY_KERNEL("exit");
  SAND_COPY_KERNEL("trace_var");
  SAND_COPY_KERNEL("untrace_var");
  SAND_COPY_KERNEL("set_trace_func");
  SAND_COPY_KERNEL("warn");

  kit->eThreadError = sandbox_defclass(kit, "ThreadError", kit->eStandardError);
  kit->cThread = sandbox_defclass(kit, "Thread", kit->cObject);
  rb_undef_alloc_func(kit->cThread);

  SAND_COPY_S(cThread, "new");
  SAND_COPY(cThread, "initialize");
  SAND_COPY_S(cThread, "start");
  SAND_COPY_S(cThread, "fork");

  SAND_COPY_S(cThread, "stop");
  SAND_COPY_S(cThread, "kill");
  SAND_COPY_S(cThread, "exit");
  SAND_COPY_S(cThread, "pass");
  SAND_COPY_S(cThread, "current");
  SAND_COPY_S(cThread, "main");
  SAND_COPY_S(cThread, "list");

  SAND_COPY_S(cThread, "critical");
  SAND_COPY_S(cThread, "critical=");

  SAND_COPY_S(cThread, "abort_on_exception");
  SAND_COPY_S(cThread, "abort_on_exception=");

  SAND_COPY(cThread, "run");
  SAND_COPY(cThread, "wakeup");
  SAND_COPY(cThread, "kill");
  SAND_COPY(cThread, "terminate");
  SAND_COPY(cThread, "exit");
  SAND_COPY(cThread, "value");
  SAND_COPY(cThread, "status");
  SAND_COPY(cThread, "join");
  SAND_COPY(cThread, "alive?");
  SAND_COPY(cThread, "stop?");
  SAND_COPY(cThread, "raise");

  SAND_COPY(cThread, "abort_on_exception");
  SAND_COPY(cThread, "abort_on_exception=");

  SAND_COPY(cThread, "priority");
  SAND_COPY(cThread, "priority=");
  SAND_COPY(cThread, "safe_level");
  SAND_COPY(cThread, "group");

  SAND_COPY(cThread, "[]");
  SAND_COPY(cThread, "[]=");
  SAND_COPY(cThread, "key?");
  SAND_COPY(cThread, "keys");

  SAND_COPY(cThread, "inspect");

  kit->cCont = sandbox_defclass(kit, "Continuation", kit->cObject);
  rb_undef_alloc_func(kit->cCont);
  SAND_UNDEF(cCont, "new");
  SAND_COPY(cCont, "call");
  SAND_COPY(cCont, "[]");
  SAND_COPY_KERNEL("callcc");

  rb_cThGroup = rb_const_get_at(rb_cObject, rb_intern("ThreadGroup"));
  kit->cThGroup = sandbox_defclass(kit, "ThreadGroup", kit->cObject);
  SAND_COPY_ALLOC(cThGroup);
  SAND_COPY(cThGroup, "list");
  SAND_COPY(cThGroup, "enclose");
  SAND_COPY(cThGroup, "enclosed?");
  SAND_COPY(cThGroup, "add");
  /* rb_define_const(cThGroup, "Default", thgroup_default); */

  SAND_COPY_KERNEL("trap");
  /*
#ifndef MACOS_UNUSE_SIGNAL
    VALUE mSignal = rb_define_module("Signal");

    rb_define_global_function("trap", sig_trap, -1);
    rb_define_module_function(mSignal, "trap", sig_trap, -1);
    rb_define_module_function(mSignal, "list", sig_list, 0);

    install_sighandler(SIGINT, sighandler);
#ifdef SIGHUP
    install_sighandler(SIGHUP, sighandler);
#endif
#ifdef SIGQUIT
    install_sighandler(SIGQUIT, sighandler);
#endif
#ifdef SIGALRM
    install_sighandler(SIGALRM, sighandler);
#endif
#ifdef SIGUSR1
    install_sighandler(SIGUSR1, sighandler);
#endif
#ifdef SIGUSR2
    install_sighandler(SIGUSR2, sighandler);
#endif

#ifdef SIGBUS
    install_sighandler(SIGBUS, sigbus);
#endif
#ifdef SIGSEGV
    install_sighandler(SIGSEGV, sigsegv);
#endif
#ifdef SIGPIPE
    install_sighandler(SIGPIPE, sigpipe);
#endif

#ifdef SIGCLD
    init_sigchld(SIGCLD);
#endif
#ifdef SIGCHLD
    init_sigchld(SIGCHLD);
#endif

#endif

    rb_define_virtual_variable("$$", get_pid, 0);
    rb_define_readonly_variable("$?", &rb_last_status);
    */
  SAND_COPY_KERNEL("exec");
  SAND_COPY_KERNEL("fork");
  SAND_COPY_KERNEL("exit!");
  SAND_COPY_KERNEL("system");
  SAND_COPY_KERNEL("sleep");

  rb_mProcess = rb_const_get_at(rb_cObject, rb_intern("Process"));
  kit->mProcess = sandbox_defmodule(kit, "Process");
  SAND_COPY_CONST(mProcess, "WNOHANG");
  SAND_COPY_CONST(mProcess, "WUNTRACED");

  SAND_COPY_S(mProcess, "fork");
  SAND_COPY_S(mProcess, "exit!");
  SAND_COPY_S(mProcess, "exit");
  SAND_COPY_S(mProcess, "abort");

  SAND_COPY_S(mProcess, "kill");
  SAND_COPY_S(mProcess, "wait");
  SAND_COPY_S(mProcess, "wait2");
  SAND_COPY_S(mProcess, "waitpid");
  SAND_COPY_S(mProcess, "waitpid2");
  SAND_COPY_S(mProcess, "waitall");
  SAND_COPY_S(mProcess, "detach");

  rb_cProcStatus = rb_const_get_at(rb_mProcess, rb_intern("Status"));
  kit->cProcStatus = sandbox_defclass_under(kit, kit->mProcess, "Status", kit->cObject);
  SAND_UNDEF(cProcStatus, "new");

  SAND_COPY(cProcStatus, "==");
  SAND_COPY(cProcStatus, "&");
  SAND_COPY(cProcStatus, ">>");
  SAND_COPY(cProcStatus, "to_i");
  SAND_COPY(cProcStatus, "to_int");
  SAND_COPY(cProcStatus, "to_s");
  SAND_COPY(cProcStatus, "inspect");

  SAND_COPY(cProcStatus, "pid");

  SAND_COPY(cProcStatus, "stopped?");
  SAND_COPY(cProcStatus, "stopsig");
  SAND_COPY(cProcStatus, "signaled?");
  SAND_COPY(cProcStatus, "termsig");
  SAND_COPY(cProcStatus, "exited?");
  SAND_COPY(cProcStatus, "exitstatus");
  SAND_COPY(cProcStatus, "success?");
  SAND_COPY(cProcStatus, "coredump?");

  SAND_COPY_S(mProcess, "pid");
  SAND_COPY_S(mProcess, "ppid");

  SAND_COPY_S(mProcess, "getpgrp");
  SAND_COPY_S(mProcess, "setpgrp");
  SAND_COPY_S(mProcess, "getpgid");
  SAND_COPY_S(mProcess, "setpgid");

  SAND_COPY_S(mProcess, "setsid");

  SAND_COPY_S(mProcess, "getpriority");
  SAND_COPY_S(mProcess, "setpriority");

  SAND_COPY_IF_CONST(mProcess, "PRIO_PROCESS");
  SAND_COPY_IF_CONST(mProcess, "PRIO_PGRP");
  SAND_COPY_IF_CONST(mProcess, "PRIO_USER");

  SAND_COPY_S(mProcess, "getrlimit");
  SAND_COPY_S(mProcess, "setrlimit");

  SAND_COPY_IF_CONST(mProcess, "RLIM_INFINITY");
  SAND_COPY_IF_CONST(mProcess, "RLIM_SAVED_MAX");
  SAND_COPY_IF_CONST(mProcess, "RLIM_SAVED_CUR");
  SAND_COPY_IF_CONST(mProcess, "RLIMIT_CORE");
  SAND_COPY_IF_CONST(mProcess, "RLIMIT_CPU");
  SAND_COPY_IF_CONST(mProcess, "RLIMIT_DATA");
  SAND_COPY_IF_CONST(mProcess, "RLIMIT_FSIZE");
  SAND_COPY_IF_CONST(mProcess, "RLIMIT_NOFILE");
  SAND_COPY_IF_CONST(mProcess, "RLIMIT_STACK");
  SAND_COPY_IF_CONST(mProcess, "RLIMIT_AS");
  SAND_COPY_IF_CONST(mProcess, "RLIMIT_MEMLOCK");
  SAND_COPY_IF_CONST(mProcess, "RLIMIT_NPROC");
  SAND_COPY_IF_CONST(mProcess, "RLIMIT_RSS");
  SAND_COPY_IF_CONST(mProcess, "RLIMIT_SBSIZE");

  SAND_COPY_S(mProcess, "uid");
  SAND_COPY_S(mProcess, "uid=");
  SAND_COPY_S(mProcess, "gid");
  SAND_COPY_S(mProcess, "gid=");
  SAND_COPY_S(mProcess, "euid");
  SAND_COPY_S(mProcess, "euid=");
  SAND_COPY_S(mProcess, "egid");
  SAND_COPY_S(mProcess, "egid=");
  SAND_COPY_S(mProcess, "initgroups");
  SAND_COPY_S(mProcess, "groups");
  SAND_COPY_S(mProcess, "groups=");
  SAND_COPY_S(mProcess, "maxgroups");
  SAND_COPY_S(mProcess, "maxgroups=");

  SAND_COPY_S(mProcess, "times");

  /*
  SAVED_USER_ID = geteuid();
  SAVED_GROUP_ID = getegid();
  */

  rb_mProcUID = rb_const_get_at(rb_mProcess, rb_intern("UID"));
  rb_mProcGID = rb_const_get_at(rb_mProcess, rb_intern("GID"));
  kit->mProcUID = sandbox_defmodule_under(kit, kit->mProcess, "UID");
  kit->mProcGID = sandbox_defmodule_under(kit, kit->mProcess, "GID");

  SAND_COPY_S(mProcUID, "rid");
  SAND_COPY_S(mProcGID, "rid");
  SAND_COPY_S(mProcUID, "eid");
  SAND_COPY_S(mProcGID, "eid");
  SAND_COPY_S(mProcUID, "change_privilege");
  SAND_COPY_S(mProcGID, "change_privilege");
  /*
  SAND_COPY_S(mProcUID, "grant_privilege");
  SAND_COPY_S(mProcGID, "grant_privilege");
  rb_define_alias(kit->mProcUID, "eid=", "grant_privilege");
  rb_define_alias(kit->mProcGID, "eid=", "grant_privilege");
  */
  SAND_COPY_S(mProcUID, "re_exchange");
  SAND_COPY_S(mProcGID, "re_exchange");
  SAND_COPY_S(mProcUID, "re_exchangeable?");
  SAND_COPY_S(mProcGID, "re_exchangeable?");
  SAND_COPY_S(mProcUID, "sid_available?");
  SAND_COPY_S(mProcGID, "sid_available?");
  SAND_COPY_S(mProcUID, "switch");
  SAND_COPY_S(mProcGID, "switch");

  rb_mProcID_Syscall = rb_const_get_at(rb_mProcess, rb_intern("Sys"));
  kit->mProcID_Syscall = sandbox_defmodule_under(kit, kit->mProcess, "Sys");

  SAND_COPY_S(mProcID_Syscall, "getuid");
  SAND_COPY_S(mProcID_Syscall, "geteuid");
  SAND_COPY_S(mProcID_Syscall, "getgid");
  SAND_COPY_S(mProcID_Syscall, "getegid");

  SAND_COPY_S(mProcID_Syscall, "setuid");
  SAND_COPY_S(mProcID_Syscall, "setgid");

  SAND_COPY_S(mProcID_Syscall, "setruid");
  SAND_COPY_S(mProcID_Syscall, "setrgid");

  SAND_COPY_S(mProcID_Syscall, "seteuid");
  SAND_COPY_S(mProcID_Syscall, "setegid");

  SAND_COPY_S(mProcID_Syscall, "setreuid");
  SAND_COPY_S(mProcID_Syscall, "setregid");

  SAND_COPY_S(mProcID_Syscall, "setresuid");
  SAND_COPY_S(mProcID_Syscall, "setresgid");
  SAND_COPY_S(mProcID_Syscall, "issetugid");

  kit->mGC = sandbox_defmodule(kit, "GC");
  SAND_COPY_S(mGC, "start");
  SAND_COPY_S(mGC, "enable");
  SAND_COPY_S(mGC, "disable");
  SAND_COPY(mGC, "garbage_collect");

  rb_mObSpace = rb_const_get_at(rb_cObject, rb_intern("ObjectSpace"));
  kit->mObSpace = sandbox_defmodule(kit, "ObjectSpace");
  SAND_COPY_S(mObSpace, "each_object");
  SAND_COPY_S(mObSpace, "garbage_collect");
  SAND_COPY_S(mObSpace, "add_finalizer");
  SAND_COPY_S(mObSpace, "remove_finalizer");
  SAND_COPY_S(mObSpace, "finalizers");
  SAND_COPY_S(mObSpace, "call_finalizer");

  SAND_COPY_S(mObSpace, "define_finalizer");
  SAND_COPY_S(mObSpace, "undefine_finalizer");

  SAND_COPY_S(mObSpace, "_id2ref");

  /*
  rb_gc_register_address(&rb_mObSpace);
  rb_global_variable(&finalizers);
  rb_gc_unregister_address(&rb_mObSpace);
  finalizers = rb_ary_new();

  source_filenames = st_init_strtable();

  rb_global_variable(&nomem_error);
  nomem_error = rb_exc_new2(rb_eNoMemError, "failed to allocate memory");
  */

  SAND_COPY(mKernel, "hash");
  SAND_COPY(mKernel, "__id__");
  SAND_COPY(mKernel, "object_id");

  /*
  VALUE v = rb_obj_freeze(rb_str_new2(ruby_version));
  VALUE d = rb_obj_freeze(rb_str_new2(ruby_release_date));
  VALUE p = rb_obj_freeze(rb_str_new2(ruby_platform));
  */

  SAND_COPY_CONST(cObject, "RUBY_VERSION");
  SAND_COPY_CONST(cObject, "RUBY_RELEASE_DATE");
  SAND_COPY_CONST(cObject, "RUBY_PLATFORM");

  SAND_COPY_CONST(cObject, "VERSION");
  SAND_COPY_CONST(cObject, "RELEASE_DATE");
  SAND_COPY_CONST(cObject, "PLATFORM");
}

typedef struct {
  sandkit *kit;
  VALUE prelude;
} Init_kit_prelude_args;

static VALUE
Init_kit_prelude_inner(value)
  VALUE value;
{
  Init_kit_prelude_args *args=(Init_kit_prelude_args *)value;
#if defined(HAVE_TIMES) || defined(_WIN32)
  if (args->kit->mProcess != 0) {
    args->kit->sTms = rb_struct_define("Tms", "utime", "stime", "cutime", "cstime", NULL);
  }
#endif
  rb_load(args->prelude, 0);
  return Qnil;
}

static void
Init_kit_prelude(kit)
  sandkit *kit;
{
  Init_kit_prelude_args args;
  args.kit = kit;
  args.prelude = rb_const_get(rb_cSandbox, rb_intern("PRELUDE"));
  StringValue(args.prelude);
  sandbox_run(kit, sandbox_action_wick(Init_kit_prelude_inner, (VALUE)&args));
}

void Init_sand_table()
{
  ruby_sandbox_save = sandbox_save;
  ruby_sandbox_restore = sandbox_restore;

  Init_kit(&base, 0);
  Init_kit_load(&base, 0);
  Init_kit_io(&base, 0);
  Init_kit_env(&base, 0);
  Init_kit_real(&base, 0);

  rb_cSandbox = rb_define_module("Sandbox");
  rb_cSandboxFull = rb_define_class_under(rb_cSandbox, "Full", rb_cObject);
  rb_define_const( rb_cSandbox, "VERSION", rb_str_new2( SAND_VERSION ) );
  rb_define_const( rb_cSandbox, "REV_ID", rb_str_new2( SAND_REV_ID ) );
  rb_define_alloc_func( rb_cSandboxFull, sandbox_alloc );
  /* Default options for Sandbox#eval in this sandbox. (See Sandbox.new.) */
  rb_define_attr( rb_cSandboxFull, "options", 1, 0 );
  rb_define_method( rb_cSandboxFull, "initialize", sandbox_initialize, -1 );
  rb_define_method( rb_cSandboxFull, "_eval", sandbox_eval, 1 );
  rb_define_method( rb_cSandboxFull, "import", sandbox_import, 1 );
  rb_define_method( rb_cSandboxFull, "ref", sandbox_ref, 1 );
  rb_define_method( rb_cSandboxFull, "copy", sandbox_copy, 2 );
  rb_define_method( rb_cSandboxFull, "main", sandbox_get_main, 0 );
  rb_define_singleton_method( rb_cSandbox, "new", sandbox_new, -1 );
  rb_define_singleton_method( rb_cSandbox, "safe", sandbox_safe, -1 );
  rb_define_singleton_method( rb_cSandbox, "current", sandbox_current, 0 );
  rb_cSandboxSafe = rb_define_class_under(rb_cSandbox, "Safe", rb_cSandboxFull);
  rb_eSandboxException = rb_define_class_under(rb_cSandbox, "Exception", rb_eException);
  rb_cSandboxRef = rb_define_class_under(rb_cSandbox, "Ref", rb_cObject);
  rb_cSandboxWick = rb_define_class_under(rb_cSandbox, "Wick", rb_cObject);
  rb_cSandboxTransfer = rb_define_class_under(rb_cSandbox, "Transfer", rb_cObject);

  s_copy_ivar = rb_intern("@____SANDBOX_COPY");
  s_options = rb_intern("@options");
  s_to_s = rb_intern("to_s");
  Qinit = ID2SYM(rb_intern("init"));
  Qimport = ID2SYM(rb_intern("import"));
  Qcopy = ID2SYM(rb_intern("copy"));
  Qload = ID2SYM(rb_intern("load"));
  Qenv = ID2SYM(rb_intern("env"));
  Qreal = ID2SYM(rb_intern("real"));
  Qref = ID2SYM(rb_intern("ref"));
  Qio = ID2SYM(rb_intern("io"));
  Qall = ID2SYM(rb_intern("all"));

  real.self = Qnil;
  rb_global_variable(&real.self);
  sandbox_swap(&real, SANDBOX_STORE);
  real.scope = alloc_scope();
  real.self = Data_Wrap_Struct( rb_cSandboxFull, mark_sandbox, NULL, &real );
  ruby_sandbox = real.self;
  rb_ivar_set(real.self, s_options, rb_hash_new());
}
