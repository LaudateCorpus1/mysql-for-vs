/*
   Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.














   The lines above are intentionally left blank
*/

#ifndef _ISHELL_CORE_
#define _ISHELL_CORE_

#include "shellcore/object_registry.h"
#include "shellcore/types_common.h"
#include "shellcore/types.h"
#include "shellcore/lang_base.h"

#include <iostream>

namespace shcore
{
  enum Interactive_input_state
  {
    Input_ok,
    Input_continued_single,
    Input_continued_block,
  };

  class SHCORE_PUBLIC IShell_core
  {
  public:
    enum Mode
    {
      Mode_None,
      Mode_SQL,
      Mode_JScript,
      Mode_Python
    };

    IShell_core(void);
    virtual ~IShell_core();

    virtual Mode interactive_mode() const = 0;
    virtual bool switch_mode(Mode mode, bool &lang_initialized) = 0;

    virtual void set_global(const std::string &name, const Value &value) = 0;
    virtual Value get_global(const std::string &name) = 0;
    virtual Value active_session() const = 0;

    virtual Object_registry *registry() = 0;
    virtual void handle_input(std::string &code, Interactive_input_state &state, boost::function<void(shcore::Value)> result_processor) = 0;
    virtual bool handle_shell_command(const std::string &code) = 0;
    virtual std::string get_handled_input() = 0;
    virtual int process_stream(std::istream& stream, const std::string& source, boost::function<void(shcore::Value)> result_processor) = 0;

    virtual std::string prompt() = 0;

    virtual Interpreter_delegate *lang_delegate() = 0;

    virtual void print(const std::string &s) = 0;
    virtual void print_error(const std::string &s) = 0;
    virtual bool password(const std::string &s, std::string &ret_pass) = 0;
    virtual const std::string& get_input_source() = 0;
    virtual bool print_help(const std::string& topic) = 0;
    virtual void abort() = 0;
  };
};

#endif
