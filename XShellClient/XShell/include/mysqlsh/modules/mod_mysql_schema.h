/*
   Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.














   The lines above are intentionally left blank
*/

// Interactive DB access module
// (the one exposed as the db variable in the shell)

#ifndef _MOD_DB_H_
#define _MOD_DB_H_

#include "mod_common.h"
#include "base_database_object.h"
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/weak_ptr.hpp>

namespace shcore
{
  class Proxy_object;
};

namespace mysh
{
  namespace mysql
  {
    class ClassicSession;
    class ClassicTable;

    /**
    * Represents a Schema retrieved with a session created using the MySQL Protocol.
    */
    class SHCORE_PUBLIC ClassicSchema : public DatabaseObject, public boost::enable_shared_from_this<ClassicSchema>
    {
    public:
      ClassicSchema(boost::shared_ptr<ClassicSession> owner, const std::string &name);
      ClassicSchema(boost::shared_ptr<const ClassicSession> owner, const std::string &name);
      ~ClassicSchema();

      virtual std::string class_name() const{ return "ClassicSchema"; };

      virtual std::vector<std::string> get_members() const;
      virtual shcore::Value get_member(const std::string &prop) const;

      void cache_table_objects();
      void _remove_object(const std::string& name, const std::string& type);

      friend class ClassicTable;
      friend class ClassicView;

#ifdef DOXYGEN

      /**
      * Returns a list of tables for this schema.
      * This method is run against a local cache of objects, if you want to see lastest changes by other sessions you may need to create a new copy the schema object with session.getSchema().
      * \sa ClassicTable
      * \return The list of tables as a Map(String, ClassicTable).
      */
      Map getTables()
      {}

      /**
      * Returns a list of views for this schema.
      * This method is run against a local cache of objects, if you want to see lastest changes by other sessions you may need to create a new copy the schema object with session.getSchema().
      * \sa ClassicView
      * \return The list of views as a Map(String, ClassicView).
      */
      Map getViews()
      {}

      ClassicTable getTable(String name);
      ClassicView getView(String name);
#endif

    private:
      boost::shared_ptr<shcore::Value::Map_type> _tables;
      boost::shared_ptr<shcore::Value::Map_type> _views;

      shcore::Value _load_object(const std::string& name, const std::string& type = "") const;
      shcore::Value find_in_collection(const std::string& name, boost::shared_ptr<shcore::Value::Map_type>source) const;
      shcore::Value getTable(const shcore::Argument_list &args);
      shcore::Value getView(const shcore::Argument_list &args);

      void init();
    };
  };
};

#endif
