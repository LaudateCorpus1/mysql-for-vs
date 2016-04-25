/*
   Copyright (c) 2015 Oracle and/or its affiliates. All rights reserved.














   The lines above are intentionally left blank
*/

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef _MOD_XRESULT_H_
#define _MOD_XRESULT_H_

#include "base_resultset.h"
#include <boost/enable_shared_from_this.hpp>

namespace mysqlx
{
  class Result;
}

namespace mysh
{
  namespace mysqlx
  {
    /**
    * Base class for the different types of results returned by the server.
    */
    class SHCORE_PUBLIC BaseResult : public mysh::ShellBaseResult
    {
    public:
      BaseResult(boost::shared_ptr< ::mysqlx::Result> result);
      virtual ~BaseResult() {}

      virtual std::vector<std::string> get_members() const;
      virtual shcore::Value get_member(const std::string &prop) const;
      virtual void append_json(shcore::JSON_dumper& dumper) const;

      // The execution time is not available at the moment of creating the resultset
      void set_execution_time(unsigned long execution_time){ _execution_time = execution_time; }

      // C++ Interface
      std::string get_execution_time() const;
      uint64_t get_warning_count() const;
      virtual void buffer();
      virtual bool rewind();
      virtual bool tell(size_t &dataset, size_t &record);
      virtual bool seek(size_t dataset, size_t record);

#ifdef DOXYGEN
      Integer warningCount; //!< Same as getwarningCount()
      List warnings; //!< Same as getWarnings()
      String executionTime; //!< Same as getExecutionTime()

      Integer getWarningCount();
      List getWarnings();
      String getExecutionTime();
#endif

    protected:
      boost::shared_ptr< ::mysqlx::Result> _result;
      unsigned long _execution_time;
    };

    /**
    * Allows retrieving information about non query operations performed on the database.
    *
    * An instance of this class will be returned on the CRUD operations that change the content of the database:
    *
    * - On Table: insert, update and delete
    * - On Collection: add, modify and remove
    *
    * Other functions on the BaseSession class also return an instance of this class:
    *
    * - Transaction handling functions
    * - Drop functions
    */
    class SHCORE_PUBLIC Result : public BaseResult, public boost::enable_shared_from_this < Result >
    {
    public:
      Result(boost::shared_ptr< ::mysqlx::Result> result);

      virtual ~Result(){};

      virtual std::string class_name() const { return "Result"; }
      virtual std::vector<std::string> get_members() const;
      virtual shcore::Value get_member(const std::string &prop) const;
      virtual void append_json(shcore::JSON_dumper& dumper) const;

      // The document ID is generated on client side, we need this function to
      // Properly set the value
      void set_last_document_id(const std::string& id){ _last_document_id = id; }

      // C++ Interface
      int64_t get_affected_item_count() const;
      int64_t get_last_insert_id() const;
      std::string get_last_document_id() const;

#ifdef DOXYGEN
      Integer affectedItemCount; //!< Same as getAffectedItemCount()
      Integer lastInsertId; //!< Same as getLastInsertId()
      Integer lastDocumentId; //!< Same as getLastDocumentId()

      Integer getAffectedItemCount();
      Integer getLastInsertId();
      String getLastDocumentId();
#endif
    private:
      std::string _last_document_id;
    };

    /**
    * Allows traversing the DbDoc objects returned by a Collection.find operation.
    */
    class SHCORE_PUBLIC DocResult : public BaseResult, public boost::enable_shared_from_this < DocResult >
    {
    public:
      DocResult(boost::shared_ptr< ::mysqlx::Result> result);

      virtual ~DocResult(){};

      shcore::Value fetch_one(const shcore::Argument_list &args) const;
      shcore::Value fetch_all(const shcore::Argument_list &args) const;

      virtual std::string class_name() const { return "DocResult"; }
      virtual void append_json(shcore::JSON_dumper& dumper) const;

#ifdef DOXYGEN
      Document fetchOne();
      List fetchAll();
#endif
    };

    /**
    * Allows traversing the Row objects returned by a Table.select operation.
    */
    class SHCORE_PUBLIC RowResult : public BaseResult, public boost::enable_shared_from_this < RowResult >
    {
    public:
      RowResult(boost::shared_ptr< ::mysqlx::Result> result);

      virtual ~RowResult(){};

      shcore::Value fetch_one(const shcore::Argument_list &args) const;
      shcore::Value fetch_all(const shcore::Argument_list &args) const;

      virtual std::vector<std::string> get_members() const;
      virtual shcore::Value get_member(const std::string &prop) const;

      virtual std::string class_name() const { return "RowResult"; }
      virtual void append_json(shcore::JSON_dumper& dumper) const;

      // C++ Interface
      int64_t get_column_count() const;
      std::vector<std::string> get_column_names() const;
      shcore::Value::Array_type_ref get_columns() const;

#ifdef DOXYGEN
      Row fetchOne();
      List fetchAll();

      Integer columnCount; //!< Same as getColumnCount()
      List columnNames; //!< Same as getColumnNames()
      List columns; //!< Same as getColumns()

      Integer getColumnCount();
      List getColumnNames();
      List getColumns();
#endif

    private:
      mutable shcore::Value::Array_type_ref _columns;
    };

    /**
    * Allows browsing through the result information after performing an operation on the database
    * done through NodeSession.sql
    */
    class SHCORE_PUBLIC SqlResult : public RowResult, public boost::enable_shared_from_this < SqlResult >
    {
    public:
      SqlResult(boost::shared_ptr< ::mysqlx::Result> result);

      virtual ~SqlResult(){};

      virtual std::string class_name() const { return "SqlResult"; }
      virtual std::vector<std::string> get_members() const;
      virtual shcore::Value get_member(const std::string &prop) const;

      shcore::Value has_data(const shcore::Argument_list &args) const;
      virtual shcore::Value next_data_set(const shcore::Argument_list &args);
      virtual void append_json(shcore::JSON_dumper& dumper) const;

      // C++ Interface
      int64_t get_affected_row_count() const;
      int64_t get_last_insert_id() const;
      bool hasData();

      // TODO: Enable it once the way to have a reference to the unmanaged object is found
      //bool nextDataSet() const;

#ifdef DOXYGEN
      Integer lastInsertId; //!< Same as getLastInsertId()
      Integer affectedRowCount; //!< Same as getAffectedRowCount()

      Integer getLastInsertId();
      Integer getAffectedRowCount();
      Bool hasData();
      Bool nextDataSet();
#endif
    };
  }
};

#endif
