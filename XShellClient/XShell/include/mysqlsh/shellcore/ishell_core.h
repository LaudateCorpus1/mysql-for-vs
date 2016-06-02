/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef _ISHELL_CORE_
#define _ISHELL_CORE_

#include "shellcore/object_registry.h"
#include "shellcore/types_common.h"
#include "shellcore/types.h"
#include "shellcore/lang_base.h"
#include <boost/shared_ptr.hpp>

#include <iostream>

namespace mysh
{
  // The session types that can be produced by connect_session
  enum SessionType
  {
    Application,
    Node,
    Classic
  };
  class ShellDevelopmentSession;
};

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

    // Development Session Handling
    virtual boost::shared_ptr<mysh::ShellDevelopmentSession> connect_dev_session(const Argument_list &args, mysh::SessionType session_type) = 0;
    virtual boost::shared_ptr<mysh::ShellDevelopmentSession> set_dev_session(boost::shared_ptr<mysh::ShellDevelopmentSession> session) = 0;
    virtual boost::shared_ptr<mysh::ShellDevelopmentSession> get_dev_session() = 0;

    // Global Schema
    virtual shcore::Value set_current_schema(const std::string& name) = 0;

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
