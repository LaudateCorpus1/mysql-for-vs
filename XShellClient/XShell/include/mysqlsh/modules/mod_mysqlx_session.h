/*
   Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.














   The lines above are intentionally left blank
*/

// Interactive session access module for MySQL X sessions
// Exposed as "session" in the shell

#ifndef _MOD_XSESSION_H_
#define _MOD_XSESSION_H_

#include "mod_common.h"
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include "shellcore/ishell_core.h"
#include "base_session.h"
#include "mysqlxtest/mysqlx.h"

#include <boost/enable_shared_from_this.hpp>

namespace shcore
{
  class Proxy_object;
};

namespace mysh
{
  class DatabaseObject;
  namespace mysqlx
  {
    class Schema;
    /**
    * Base functionality for Session classes through the X Protocol.
    *
    * This class encloses the core functionaliti to be made available on both the XSession and NodeSession classes, such functionality includes
    *
    * - Accessing available schemas.
    * - Schema management operations.
    * - Enabling/disabling warning generation.
    * - Retrieval of connection information.
    *
    * #### JavaScript Examples
    *
    * \include "js_dev_api_examples/concepts/Working_with_a_Session_Object.js"
    *
    * #### Python Examples
    *
    * \include "py_dev_api_examples/concepts/Working_with_a_Session_Object.py"
    *
    * #### Dynamic Properties
    *
    * In addition to the properties documented above, when a session object is created the schemas available on the target
    * MySQL Server are cached.
    *
    * A dynamic property is added to the session object in order to access each available Schema as a session member.
    *
    * These dynamic properties are named as the Schema's name, so the schemas are accessible as follows:
    *
    * \sa mysqlx.getSession(String connectionData, String password)
    * \sa mysqlx.getSession(Map connectionData, String password)
    * \sa mysqlx.getNodeSession(String connectionData, String password)
    * \sa mysqlx.getNodeSession(Map connectionData, String password)
    */
    class SHCORE_PUBLIC BaseSession : public ShellBaseSession
    {
    public:
      BaseSession();
      BaseSession(const BaseSession& s);
      virtual ~BaseSession() { reset_session(); }

      virtual std::vector<std::string> get_members() const;
      virtual shcore::Value get_member(const std::string &prop) const;
      virtual bool has_member(const std::string &prop) const;

      virtual shcore::Value connect(const shcore::Argument_list &args);
      virtual shcore::Value close(const shcore::Argument_list &args);
      virtual shcore::Value sql(const shcore::Argument_list &args);
      virtual shcore::Value create_schema(const shcore::Argument_list &args);
      virtual shcore::Value startTransaction(const shcore::Argument_list &args);
      virtual shcore::Value commit(const shcore::Argument_list &args);
      virtual shcore::Value rollback(const shcore::Argument_list &args);
      virtual shcore::Value drop_schema(const shcore::Argument_list &args);
      virtual shcore::Value drop_schema_object(const shcore::Argument_list &args, const std::string& type);
      virtual shcore::Value set_current_schema(const shcore::Argument_list &args);

      shcore::Value executeAdminCommand(const std::string& command, bool expect_data, const shcore::Argument_list &args);
      shcore::Value execute_sql(const std::string& query, const shcore::Argument_list &args);
      virtual bool is_connected() const { return _session ? true : false; }
      virtual shcore::Value get_status(const shcore::Argument_list &args);
      virtual shcore::Value get_capability(const std::string& name);

      virtual shcore::Value get_schema(const shcore::Argument_list &args) const;

      virtual std::string db_object_exists(std::string &type, const std::string &name, const std::string& owner);

      shcore::Value set_fetch_warnings(const shcore::Argument_list &args);

      boost::shared_ptr< ::mysqlx::Session> session_obj() const { return _session; }

      static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);

      bool table_name_compare(const std::string &n1, const std::string &n2);

      virtual void set_option(const char *option, int value);

      virtual uint64_t get_connection_id() const;

#ifdef DOXYGEN
      String uri; //!< Same as getUri()
      Map schemas; //!< Same as getSchemas()
      Schema defaultSchema; //!< Same as getDefaultSchema()

      Schema createSchema(String name);
      Schema getSchema(String name);
      Schema getDefaultSchema();
      Map getSchemas();
      String getUri();
      Undefined close();
      Undefined setFetchWarnings(Bool value);
      Result startTransaction();
      Result commit();
      Result rollback();
      Result dropSchema(String name);
      Result dropTable(String schema, String name);
      Result dropCollection(String schema, String name);
      Result dropView(String schema, String name);

#endif
    protected:
      ::mysqlx::ArgumentValue get_argument_value(shcore::Value source);
      shcore::Value executeStmt(const std::string &domain, const std::string& command, bool expect_data, const shcore::Argument_list &args);
      virtual boost::shared_ptr<BaseSession> _get_shared_this() const = 0;
      boost::shared_ptr< ::mysqlx::Result> _last_result;
      std::string _retrieve_current_schema();
      void _load_schemas();
      void _remove_schema(const std::string& name);
      void _retrieve_session_info(std::string &current_schema, int &case_sensitive_table_names);

      virtual int get_default_port() { return 33060; };

      boost::shared_ptr< ::mysqlx::Session> _session;

      std::string _default_schema;
      boost::shared_ptr<shcore::Value::Map_type> _schemas;

      bool _case_sensitive_table_names;
      void init();
      uint64_t _connection_id;
      void set_connection_id();
    private:
      void reset_session();
    };

    /**
    * Enables interaction with an X Protocol enabled MySQL Product.
    *
    * Note that this class inherits the behavior described on the BaseSession class.
    *
    * In the future this class will be improved to support interacting not only with MySQL Server but with other products.
    *
    * \sa BaseSession
    */
    class SHCORE_PUBLIC XSession : public BaseSession, public boost::enable_shared_from_this<XSession>
    {
    public:
      XSession(){};
      XSession(const XSession& s) : BaseSession(s) {}
      virtual ~XSession(){};
      virtual std::string class_name() const { return "XSession"; };
      static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);

      virtual boost::shared_ptr<BaseSession> _get_shared_this() const;
    };

    /**
    * Enables interaction with an X Protocol enabled MySQL Server, this includes SQL Execution.
    *
    * Note that this class inherits the behavior described on the BaseSession class.
    *
    * \sa BaseSession
    */
    class SHCORE_PUBLIC NodeSession : public BaseSession, public boost::enable_shared_from_this<NodeSession>
    {
    public:
      NodeSession();
      NodeSession(const NodeSession& s);
      virtual ~NodeSession(){};
      virtual std::string class_name() const { return "NodeSession"; };
      virtual std::vector<std::string> get_members() const;

      static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);
      virtual boost::shared_ptr<BaseSession> _get_shared_this() const;
      virtual shcore::Value get_member(const std::string &prop) const;
      shcore::Value sql(const shcore::Argument_list &args);
      shcore::Value quote_name(const shcore::Argument_list &args);
#ifdef DOXYGEN
      Schema currentSchema; //!< Same as getCurrentSchema()

      Schema getCurrentSchema();
      Schema setCurrentSchema(String name);
      SqlExecute sql(String sql);
      String quoteName(String id);
#endif
    protected:
      void init();
    };
  }
}

#endif
