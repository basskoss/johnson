#include "ruby_land_proxy.h"
#include "conversions.h"
#include "error.h"

static VALUE proxy_class = Qnil;

static VALUE /* [] */
get(VALUE self, VALUE name)
{
  RubyLandProxy* proxy;
  Data_Get_Struct(self, RubyLandProxy, proxy);

  JS_AddNamedRoot(proxy->context->js, &(proxy->value), "RubyLandProxy#get");

  jsval js_value;  

  switch(TYPE(name)) {
    case T_FIXNUM:
      if(!(JS_GetElement(proxy->context->js,
          JSVAL_TO_OBJECT(proxy->value), NUM2INT(name), &js_value))) {
        JS_RemoveRoot(proxy->context->js, &(proxy->value));
        raise_js_error_in_ruby(proxy->context);
      }
      break;
    default:
      Check_Type(name, T_STRING);
      if(!(JS_GetProperty(proxy->context->js,
          JSVAL_TO_OBJECT(proxy->value), StringValueCStr(name), &js_value))) {
        JS_RemoveRoot(proxy->context->js, &(proxy->value));
        raise_js_error_in_ruby(proxy->context);
      }
      break;
  }

  JS_RemoveRoot(proxy->context->js, &(proxy->value));
  return convert_to_ruby(proxy->context, js_value);
}

static VALUE /* []= */
set(VALUE self, VALUE name, VALUE value)
{
  RubyLandProxy* proxy;
  Data_Get_Struct(self, RubyLandProxy, proxy);
  
  JS_AddNamedRoot(proxy->context->js, &(proxy->value), "RubyLandProxy#set");

  jsval js_value;
  if(!convert_to_js(proxy->context, value, &js_value)) {
    JS_RemoveRoot(proxy->context->js, &(proxy->value));
    raise_js_error_in_ruby(proxy->context);
  }
  
  if(!JS_AddNamedRoot(proxy->context->js, &js_value, "RubyLandProxy#set")) {
    JS_RemoveRoot(proxy->context->js, &(proxy->value));
    raise_js_error_in_ruby(proxy->context);
  }

  switch(TYPE(name)) {
    case T_FIXNUM:
      if(!(JS_SetElement(proxy->context->js,
              JSVAL_TO_OBJECT(proxy->value), NUM2INT(name), &js_value))) {
        JS_RemoveRoot(proxy->context->js, &js_value);
        JS_RemoveRoot(proxy->context->js, &(proxy->value));
        raise_js_error_in_ruby(proxy->context);
      }
      break;
    default:
      Check_Type(name, T_STRING);
      if(!(JS_SetProperty(proxy->context->js,
            JSVAL_TO_OBJECT(proxy->value), StringValueCStr(name), &js_value))) {
        JS_RemoveRoot(proxy->context->js, &js_value);
        JS_RemoveRoot(proxy->context->js, &(proxy->value));
        raise_js_error_in_ruby(proxy->context);
      }
      break;
  }

  JS_RemoveRoot(proxy->context->js, &js_value);
  JS_RemoveRoot(proxy->context->js, &(proxy->value));
  
  return value;
}

static VALUE /* function? */
function_p(VALUE self)
{
  RubyLandProxy* proxy;
  Data_Get_Struct(self, RubyLandProxy, proxy);
  return JS_TypeOfValue(proxy->context->js, proxy->value) == JSTYPE_FUNCTION;
}

static VALUE /* respond_to?(sym) */
respond_to_p(VALUE self, VALUE sym)
{
  RubyLandProxy* proxy;
  Data_Get_Struct(self, RubyLandProxy, proxy);
  
  char* name = rb_id2name(SYM2ID(sym));
  
  // assignment is always okay
  if (name[strlen(name) - 1] == '=')
    return Qtrue;
  
  JS_AddNamedRoot(proxy->context->js, &(proxy->value), "RubyLandProxy#respond_to?");

  JSObject *obj;
  JSBool found;
  
  if(!JS_ValueToObject(proxy->context->js, proxy->value, &obj)) {
    JS_RemoveRoot(proxy->context->js, &(proxy->value));
    raise_js_error_in_ruby(proxy->context);
  }
  if(!JS_AddNamedRoot(proxy->context->js, &obj, "RubyLandProxy#respond_to?")) {
    JS_RemoveRoot(proxy->context->js, &(proxy->value));
    raise_js_error_in_ruby(proxy->context);
  }
  if(!JS_HasProperty(proxy->context->js, obj, name, &found)) {
    JS_RemoveRoot(proxy->context->js, &obj);
    JS_RemoveRoot(proxy->context->js, &(proxy->value));
    raise_js_error_in_ruby(proxy->context);
  }

  JS_RemoveRoot(proxy->context->js, &obj);
  JS_RemoveRoot(proxy->context->js, &(proxy->value));

  return found ? Qtrue : rb_call_super(1, &sym);
}

/* private */ static VALUE /* native_call(global, *args) */
native_call(int argc, VALUE* argv, VALUE self)
{
  if (!function_p(self))
    Johnson_Error_raise("This Johnson::SpiderMonkey::RubyLandProxy isn't a function.");

  RubyLandProxy* proxy;
  Data_Get_Struct(self, RubyLandProxy, proxy);
  
  JS_AddNamedRoot(proxy->context->js, &(proxy->value), "RubyLandProxy#native_call");

  jsval global;
  if(!convert_to_js(proxy->context, argv[0], &global)) {
    JS_RemoveRoot(proxy->context->js, &(proxy->value));
    raise_js_error_in_ruby(proxy->context);
  }
  JS_AddNamedRoot(proxy->context->js, &global, "RubyLandProxy#native_call:global");

  jsval args[argc - 1];
  jsval js;
  
  int i;

  JSBool okay = JS_TRUE;

  for (i = 1; i < argc; ++i) {
    if (convert_to_js(proxy->context, argv[i], &(args[i - 1]))) {
      JS_AddNamedRoot(proxy->context->js, &(args[i - 1]), "RubyLandProxy#native_call:argN");
    } else {
      okay = JS_FALSE;
      break;
    }
  }

  if (okay)
    okay = JS_CallFunctionValue(proxy->context->js,
      JSVAL_TO_OBJECT(global), proxy->value, (unsigned) argc - 1, args, &js);
  if (okay)
    okay = JS_AddNamedRoot(proxy->context->js, &js, "RubyLandProxy#native_call:result");

  for(--i; i > 0; --i) {
    JS_RemoveRoot(proxy->context->js, &args[i - 1]);
  }
  JS_RemoveRoot(proxy->context->js, &global);
  JS_RemoveRoot(proxy->context->js, &(proxy->value));

  if (!okay)
    return JS_FALSE;

  JS_RemoveRoot(proxy->context->js, &js);

  return convert_to_ruby(proxy->context, js);
}

static VALUE /* each(&block) */ 
each(VALUE self)
{
  RubyLandProxy* proxy;
  Data_Get_Struct(self, RubyLandProxy, proxy);
  
  JS_AddNamedRoot(proxy->context->js, &(proxy->value), "RubyLandProxy#each");

  JSObject* value = JSVAL_TO_OBJECT(proxy->value);
  JS_AddNamedRoot(proxy->context->js, &value, "RubyLandProxy#each:value");
  
  // arrays behave like you'd expect, indexes in order
  if (JS_IsArrayObject(proxy->context->js, value))
  {
    jsuint length;
    if (!JS_GetArrayLength(proxy->context->js, value, &length))
    {
      JS_RemoveRoot(proxy->context->js, &value);
      JS_RemoveRoot(proxy->context->js, &(proxy->value));
      raise_js_error_in_ruby(proxy->context);
    }
    
    jsuint i = 0;
    for (i = 0; i < length; ++i)
    {
      jsval element;
      if(!JS_GetElement(proxy->context->js, value, (signed) i, &element))
      {
        JS_RemoveRoot(proxy->context->js, &value);
        JS_RemoveRoot(proxy->context->js, &(proxy->value));
        raise_js_error_in_ruby(proxy->context);
      }
      rb_yield(convert_to_ruby(proxy->context, element));
    }
  }
  else
  {
    // not an array? behave like each on Hash; yield [key, value]
    JSIdArray *ids = JS_Enumerate(proxy->context->js, value);
    if (!ids)
    {
      JS_RemoveRoot(proxy->context->js, &value);
      JS_RemoveRoot(proxy->context->js, &(proxy->value));
      raise_js_error_in_ruby(proxy->context);
    }
  
    int i;
    for (i = 0; i < ids->length; ++i)
    {
      jsval js_key, js_value;

      if (!JS_IdToValue(proxy->context->js, ids->vector[i], &js_key))
      {
        JS_RemoveRoot(proxy->context->js, &value);
        JS_RemoveRoot(proxy->context->js, &(proxy->value));
        raise_js_error_in_ruby(proxy->context);
      }
      JS_AddNamedRoot(proxy->context->js, &js_key, "RubyLandProxy#each:js_key");

      JSBool okay;
      if (JSVAL_IS_STRING(js_key))
      {
        // regular properties have string keys
        okay = JS_GetProperty(proxy->context->js, value,
          JS_GetStringBytes(JSVAL_TO_STRING(js_key)), &js_value)
          && JS_AddNamedRoot(proxy->context->js, &js_value, "RubyLandProxy#each:js_value");
      }
      else
      {
        // it's a numeric property, use array access
        okay = JS_GetElement(proxy->context->js, value,
          JSVAL_TO_INT(js_key), &js_value)
          && JS_AddNamedRoot(proxy->context->js, &js_value, "RubyLandProxy#each:js_value");
      }

      if (!okay)
      {
        JS_RemoveRoot(proxy->context->js, &value);
        JS_RemoveRoot(proxy->context->js, &(proxy->value));
        raise_js_error_in_ruby(proxy->context);
      }
    
      VALUE key = convert_to_ruby(proxy->context, js_key);
      VALUE value = convert_to_ruby(proxy->context, js_value);

      rb_yield(rb_ary_new3(2, key, value));

      JS_RemoveRoot(proxy->context->js, &js_value);
      JS_RemoveRoot(proxy->context->js, &js_key);
    }
  
    JS_DestroyIdArray(proxy->context->js, ids);
  }

  JS_RemoveRoot(proxy->context->js, &value);
  JS_RemoveRoot(proxy->context->js, &(proxy->value));
  
  return self; 
}

static VALUE
length(VALUE self)
{
  RubyLandProxy* proxy;
  Data_Get_Struct(self, RubyLandProxy, proxy);
  
  JS_AddNamedRoot(proxy->context->js, &(proxy->value), "RubyLandProxy#length");

  JSObject* value = JSVAL_TO_OBJECT(proxy->value);
  JS_AddNamedRoot(proxy->context->js, &value, "RubyLandProxy#length");
  
  if (JS_IsArrayObject(proxy->context->js, value))
  {
    jsuint length;
    JSBool okay = JS_GetArrayLength(proxy->context->js, value, &length);
    JS_RemoveRoot(proxy->context->js, &value);
    JS_RemoveRoot(proxy->context->js, &(proxy->value));

    if (!okay)
      raise_js_error_in_ruby(proxy->context);

    return INT2FIX(length);
  }
  else
  {
    JSIdArray *ids = JS_Enumerate(proxy->context->js, value);
    VALUE length = Qnil;
    if (ids)
      length = INT2FIX(ids->length);
    
    JS_DestroyIdArray(proxy->context->js, ids);
    JS_RemoveRoot(proxy->context->js, &value);
    JS_RemoveRoot(proxy->context->js, &(proxy->value));

    if (!ids)
      raise_js_error_in_ruby(proxy->context);

    return length;
  }
}

/* private */ static VALUE /* context */
context(VALUE self)
{
  RubyLandProxy* proxy;
  Data_Get_Struct(self, RubyLandProxy, proxy);
  return (VALUE)JS_GetContextPrivate(proxy->context->js);
}

/* private */ static VALUE /* function_property?(name) */
function_property_p(VALUE self, VALUE name)
{
  Check_Type(name, T_STRING);
  
  RubyLandProxy* proxy;
  Data_Get_Struct(self, RubyLandProxy, proxy);

  JS_AddNamedRoot(proxy->context->js, &(proxy->value), "RubyLandProxy#property?");

  jsval js_value;  

  if(!JS_GetProperty(proxy->context->js,
      JSVAL_TO_OBJECT(proxy->value), StringValueCStr(name), &js_value)) {
    JS_RemoveRoot(proxy->context->js, &(proxy->value));
    raise_js_error_in_ruby(proxy->context);
  }

  JS_AddNamedRoot(proxy->context->js, &js_value, "RubyLandProxy#function_property?");

  JSType type = JS_TypeOfValue(proxy->context->js, js_value);

  JS_RemoveRoot(proxy->context->js, &js_value);
  JS_RemoveRoot(proxy->context->js, &(proxy->value));

  return type == JSTYPE_FUNCTION;
}

/* private */ static VALUE
call_function_property(int argc, VALUE* argv, VALUE self)
{
  RubyLandProxy* proxy;
  Data_Get_Struct(self, RubyLandProxy, proxy);
  
  JS_AddNamedRoot(proxy->context->js, &(proxy->value), "RubyLandProxy#call_function_property");

  jsval function;
  
  if (!JS_GetProperty(proxy->context->js,
    JSVAL_TO_OBJECT(proxy->value), StringValueCStr(argv[0]), &function))
  {
    JS_RemoveRoot(proxy->context->js, &(proxy->value));
    raise_js_error_in_ruby(proxy->context);
  }
  
  // should never be anything but a function
  if (!JS_TypeOfValue(proxy->context->js, function) == JSTYPE_FUNCTION)
  {
    JS_RemoveRoot(proxy->context->js, &(proxy->value));
    raise_js_error_in_ruby(proxy->context);
  }
  
  // first thing in argv is the property name; skip it
  jsval args[argc - 1];
  int i;
  
  for(i = 1; i < argc; ++i)
    if (!convert_to_js(proxy->context, argv[i], &(args[i - 1])))
    {
      JS_RemoveRoot(proxy->context->js, &(proxy->value));
      raise_js_error_in_ruby(proxy->context);
    }
  
  jsval js;
  
  if (!JS_CallFunctionValue(proxy->context->js,
    JSVAL_TO_OBJECT(proxy->value), function, (unsigned) argc - 1, args, &js))
  {
    JS_RemoveRoot(proxy->context->js, &(proxy->value));
    raise_js_error_in_ruby(proxy->context);
  }
  
  JS_RemoveRoot(proxy->context->js, &(proxy->value));
  return convert_to_ruby(proxy->context, js);
}

///////////////////////////////////////////////////////////////////////////
//// INFRASTRUCTURE BELOW HERE ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static void finalize(RubyLandProxy* proxy)
{
  // could get finalized after the context has been freed
  if (proxy->context && proxy->context->jsids)
  {
    // remove this proxy from the OID map
    JS_HashTableRemove(proxy->context->jsids, (void *)proxy->value);
  
    // remove our GC handle on the JS value
    char key[10];
  	sprintf(key, "%x", (int)proxy->value);
    JS_DeleteProperty(proxy->context->js, proxy->context->gcthings, key);
    
    proxy->context = 0;
  }
  
  proxy->value = 0;
  free(proxy);
}

bool ruby_value_is_proxy(VALUE maybe_proxy)
{
  return proxy_class == CLASS_OF(maybe_proxy); 
}

JSBool unwrap_ruby_land_proxy(OurContext* UNUSED(context), VALUE wrapped, jsval* retval)
{
  assert(ruby_value_is_proxy(wrapped));
  
  RubyLandProxy* proxy;
  Data_Get_Struct(wrapped, RubyLandProxy, proxy);
  
  *retval = proxy->value; 
  return JS_TRUE;
}

VALUE make_ruby_land_proxy(OurContext* context, jsval value)
{
  VALUE id = (VALUE)JS_HashTableLookup(context->jsids, (void *)value);
  
  if (id)
  {
    // if we already have a proxy, return it
    return rb_funcall(rb_const_get(rb_cObject,
      rb_intern("ObjectSpace")), rb_intern("_id2ref"), 1, id);
  }
  else
  {
    // otherwise make one and cache it
    RubyLandProxy* our_proxy; 
    VALUE proxy = Data_Make_Struct(proxy_class, RubyLandProxy, 0, finalize, our_proxy);

    JS_AddNamedRoot(context->js, &value, "RubyLandProxy->value");

    our_proxy->value = value;
    our_proxy->context = context;

  	// put the proxy OID in the id map
    assert(JS_HashTableAdd(context->jsids, (void *)value, (void *)rb_obj_id(proxy)));
    
    // root the value for JS GC
    char key[10];
  	sprintf(key, "%x", (int)value);
  	JS_SetProperty(context->js, context->gcthings, key, &value);

    JS_RemoveRoot(context->js, &value);
    
    return proxy;
  }
}

static VALUE to_s(VALUE self)
{
  RubyLandProxy* proxy;
  Data_Get_Struct(self, RubyLandProxy, proxy);

  JS_AddNamedRoot(proxy->context->js, &(proxy->value), "RubyLandProxy#to_s");
  JSString* str = JS_ValueToString(proxy->context->js, proxy->value);
  JS_RemoveRoot(proxy->context->js, &(proxy->value));

  return convert_jsstring_to_ruby(proxy->context, str);
}

void init_Johnson_SpiderMonkey_Proxy(VALUE spidermonkey)
{
  proxy_class = rb_define_class_under(spidermonkey, "RubyLandProxy", rb_cObject);

  rb_define_method(proxy_class, "[]", get, 1);
  rb_define_method(proxy_class, "[]=", set, 2);
  rb_define_method(proxy_class, "function?", function_p, 0);
  rb_define_method(proxy_class, "respond_to?", respond_to_p, 1);
  rb_define_method(proxy_class, "each", each, 0);
  rb_define_method(proxy_class, "length", length, 0);
  rb_define_method(proxy_class, "to_s", to_s, 0);

  rb_define_private_method(proxy_class, "native_call", native_call, -1);
  rb_define_private_method(proxy_class, "context", context, 0);
  rb_define_private_method(proxy_class, "function_property?", function_property_p, 1);
  rb_define_private_method(proxy_class, "call_function_property", call_function_property, -1);
}
