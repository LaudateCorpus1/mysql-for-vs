/*
   Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.














   The lines above are intentionally left blank
*/

// Provides a generic wrapper for shcore::Object_bridge objects so that they
// can be used from JavaScript

// new_instance() can be used to create a JS object instance per class

#ifndef _JSCRIPT_OBJECT_WRAPPER_H_
#define _JSCRIPT_OBJECT_WRAPPER_H_

#include "shellcore/types.h"
#include "shellcore/include_v8.h"

namespace shcore
{
  class JScript_context;

  class JScript_object_wrapper
  {
  public:
    JScript_object_wrapper(JScript_context *context, bool indexed = false);
    ~JScript_object_wrapper();

    v8::Handle<v8::Object> wrap(boost::shared_ptr<Object_bridge> object);

    static bool unwrap(v8::Handle<v8::Object> value, boost::shared_ptr<Object_bridge> &ret_object);

  private:
    struct Collectable;
    static void handler_getter(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info);
    static void handler_setter(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info);
    static void handler_enumerator(const v8::PropertyCallbackInfo<v8::Array>& info);

    static void handler_igetter(uint32_t i, const v8::PropertyCallbackInfo<v8::Value>& info);
    static void handler_isetter(uint32_t i, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info);
    static void handler_ienumerator(const v8::PropertyCallbackInfo<v8::Array>& info);

    static void wrapper_deleted(const v8::WeakCallbackData<v8::Object, Collectable>& data);

  private:
    JScript_context *_context;
    v8::Persistent<v8::ObjectTemplate> _object_template;
  };
};

#endif
