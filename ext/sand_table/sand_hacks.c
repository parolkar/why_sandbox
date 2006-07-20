/*
 * sand_hacks.c
 *
 * Let's keep all the hacked versions of rb_* equivalents in here.
 *
 * $Author$
 * $Date$
 *
 * Copyright (C) 2006 why the lucky stiff
 */
#include "sand_table.h"
 
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

VALUE
sandbox_singleton_class(kit, obj)
  sandkit *kit;
  VALUE obj;
{
  VALUE klass;

  if (FIXNUM_P(obj) || SYMBOL_P(obj)) {
    rb_raise(rb_eTypeError, "can't define singleton");
  }
  if (rb_special_const_p(obj)) {
    rb_raise(rb_eTypeError, "no special constants in the sandbox");
  }

  DEFER_INTS;
  if (FL_TEST(RBASIC(obj)->klass, FL_SINGLETON) &&
  rb_iv_get(RBASIC(obj)->klass, "__attached__") == obj) {
    klass = RBASIC(obj)->klass;
  }
  else {
    klass = sandbox_metaclass(kit, obj, RBASIC(obj)->klass);
  }
  if (OBJ_TAINTED(obj)) {
    OBJ_TAINT(klass);
  }
  else {
    FL_UNSET(klass, FL_TAINT);
  }
  if (OBJ_FROZEN(obj)) OBJ_FREEZE(klass);
  ALLOW_INTS;
  
  return klass;
}

VALUE
sandbox_defclass(kit, name, super)
  sandkit *kit;
  const char *name;
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

struct global_entry {
  struct global_variable *var;
  ID id;
};

/*
struct global_entry*
rb_global_entry(id)
    ID id;
{
  struct global_entry *entry;

  if (!st_lookup(rb_global_tbl, id, (st_data_t *)&entry)) {
    struct global_variable *var;
    entry = ALLOC(struct global_entry);
    var = ALLOC(struct global_variable);
    entry->id = id;
    entry->var = var;
    var->counter = 1;
    var->data = 0;
    var->getter = undef_getter;
    var->setter = undef_setter;
    var->marker = undef_marker;

    var->block_trace = 0;
    var->trace = 0;
    st_add_direct(rb_global_tbl, id, (st_data_t)entry);
  }
  return entry;
}
*/

