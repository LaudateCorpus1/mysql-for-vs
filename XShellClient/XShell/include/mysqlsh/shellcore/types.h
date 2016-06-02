/*
 * Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _TYPES_H_
#define _TYPES_H_

#include "types_common.h"

#include <vector>
#include <map>
#include <string>
#include <stdexcept>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include "shellcore/common.h"
#include "utils/utils_json.h"

namespace shcore {
  /** Basic types that can be passed around code in different languages.

   With the exception of Native and Function, all types can be serialized to JSON.
   */
  enum Value_type
  {
    Undefined, //! Undefined
    Null, //! Null/None value
    Bool, //! true or false
    String, //! String values, UTF-8 encoding
    Integer, //! 64bit integer numbers
    Float, //! double numbers

    Object, //! Native/bridged C++ object references, may or may not be serializable

    Array, //! Array/List container
    Map, //! Dictionary/Map/Object container
    MapRef, //! A weak reference to a Map

    Function, //! A function reference, not serializable.

    UInteger //! unsigned 64bit integer numbers
  };

  class Object_bridge;
  typedef boost::shared_ptr<Object_bridge> Object_bridge_ref;

  /** A generic value that can be used from any language we support.

   Anything that can be represented using this can be passed as a parameter to
   scripting functions or stored in the internal registry or anywhere. If serializable
   types are used, then they may also be stored as a JSON document.

   Values are exposed to scripting languages according to the following rules:

   - Simple types (Null, Bool, String, Integer, Float, UInteger) are converted directly to the target type, both ways

   - Arrays and Maps are converted directly to the target type, both ways

   - Functions are wrapped into callable objects from C++ to scripting language
   - Scripting language functions are wrapped into an instance of a language specific subclass of Function_base

   - C++ Objects are generically wrapped into a scripting language object, except when there's a specific native counterpart

   - Scripting language objects are either generically wrapped to a language specific generic object wrapper or converted to a specific C++ Object subclass

   Example: JS Date object is converted to a C++ Date object and vice-versa, but Mysql_connection is wrapped generically
   */
  struct SHCORE_PUBLIC Value
  {
    typedef std::vector<Value> Array_type;
    typedef boost::shared_ptr<Array_type> Array_type_ref;

    class SHCORE_PUBLIC Map_type
    {
    public:
      typedef std::map<std::string, Value> container_type;
      typedef container_type::const_iterator const_iterator;
      typedef container_type::iterator iterator;

      inline bool has_key(const std::string &k) const { return find(k) != end(); }

      Value_type get_type(const std::string &k) const;

      std::string get_string(const std::string &k, const std::string &def = "") const;
      bool get_bool(const std::string &k, bool def = false) const;
      int64_t get_int(const std::string &k, int64_t def = 0) const;
      uint64_t get_uint(const std::string &k, uint64_t def = 0) const;
      double get_double(const std::string &k, double def = 0.0) const;
      boost::shared_ptr<Value::Map_type> get_map(const std::string &k,
              boost::shared_ptr<Map_type> def = boost::shared_ptr<Map_type>()) const;
      boost::shared_ptr<Value::Array_type> get_array(const std::string &k,
              boost::shared_ptr<Array_type> def = boost::shared_ptr<Array_type>()) const;
      void merge_contents(boost::shared_ptr<Map_type> source, bool overwrite);

      template<class C>
      boost::shared_ptr<C> get_object(const std::string &k,
                                      boost::shared_ptr<C> def = boost::shared_ptr<C>()) const
      {
        const_iterator iter = find(k);
        if (iter == end())
          return def;
        iter->second.check_type(Object);
        return iter->second.as_object<C>();
      }

      const_iterator find(const std::string &k) const { return _map.find(k); }
      iterator find(const std::string &k) { return _map.find(k); }

      void erase(const std::string &k) { _map.erase(k); }

      const_iterator begin() const { return _map.begin(); }
      iterator begin() { return _map.begin(); }

      const_iterator end() const { return _map.end(); }
      iterator end() { return _map.end(); }

      container_type::mapped_type& operator [](const std::string &k) { return _map[k]; }
      bool operator == (const Map_type &other) const { return _map == other._map; }

      size_t size() const { return _map.size(); }
      size_t count(const std::string &k) const { return _map.count(k); }
    private:
      container_type _map;
    };
    typedef boost::shared_ptr<Map_type> Map_type_ref;

    Value_type type;
    union
    {
      bool b;
      std::string *s;
      int64_t i;
      uint64_t ui;
      double d;
      boost::shared_ptr<class Object_bridge> *o;
      boost::shared_ptr<Array_type> *array;
      boost::shared_ptr<Map_type> *map;
      boost::weak_ptr<Map_type> *mapref;
      boost::shared_ptr<class Function_base> *func;
    } value;

    Value() : type(Undefined) {}
    Value(const Value &copy);

    explicit Value(const std::string &s);
    explicit Value(const char *);
    explicit Value(const char *, size_t n);
    explicit Value(int i);
    explicit Value(unsigned int ui);
    explicit Value(int64_t i);
    explicit Value(uint64_t ui);
    explicit Value(double d);
    explicit Value(bool b);
    explicit Value(boost::shared_ptr<Function_base> f);
    explicit Value(boost::shared_ptr<Object_bridge> o);
    explicit Value(boost::weak_ptr<Map_type> n);
    explicit Value(Map_type_ref n);
    explicit Value(Array_type_ref n);

    static Value wrap(Object_bridge *o) { return Value(boost::shared_ptr<Object_bridge>(o)); }

    template<class T>
    static Value wrap(T *o) { return Value(boost::static_pointer_cast<Object_bridge>(boost::shared_ptr<T>(o))); }

    static Value new_array() { return Value(boost::shared_ptr<Array_type>(new Array_type())); }
    static Value new_map() { return Value(boost::shared_ptr<Map_type>(new Map_type())); }

    static Value Null() { Value v; v.type = shcore::Null; return v; }
    static Value True() { Value v; v.type = shcore::Bool; v.value.b = true; return v; }
    static Value False() { Value v; v.type = shcore::Bool; v.value.b = false; return v; }

    //! parse a string returned by repr() back into a Value
    static Value parse(const std::string &s);

    ~Value();

    Value &operator= (const Value &other);

    bool operator == (const Value &other) const;

    bool operator != (const Value &other) const { return !(*this == other); }

    operator bool() const
    {
      return type != Undefined && type != shcore::Null;
    }

    //! returns a human-readable description text for the value.
    // if pprint is true, it will try to pretty-print it (like adding newlines)
    std::string descr(bool pprint = false) const;

    //! returns a string representation of the serialized object, suitable to be passed to parse()
    std::string repr() const;

    //! returns a JSON representation of the object
    std::string json(bool pprint = false) const;

    std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const;
    std::string &append_repr(std::string &s_out) const;

    void check_type(Value_type t) const;

    bool as_bool() const { check_type(Bool); return value.b; }
    int64_t as_int() const;
    uint64_t as_uint() const;
    double as_double() const;
    const std::string &as_string() const { check_type(String); return *value.s; }
    template<class C>
    boost::shared_ptr<C> as_object() const { check_type(Object); return boost::static_pointer_cast<C>(*value.o); }
    boost::shared_ptr<Object_bridge> as_object() const { check_type(Object); return boost::static_pointer_cast<Object_bridge>(*value.o); }
    boost::shared_ptr<Map_type> as_map() const { check_type(Map); return *value.map; }
    boost::shared_ptr<Array_type> as_array() const { check_type(Array); return *value.array; }
    boost::shared_ptr<Function_base> as_function() const { check_type(Function); return boost::static_pointer_cast<Function_base>(*value.func); }

  private:
    static Value parse(char **pc);
    static Value parse_map(char **pc);
    static Value parse_array(char **pc);
    static Value parse_string(char **pc, char quote);
    static Value parse_single_quoted_string(char **pc);
    static Value parse_double_quoted_string(char **pc);
    static Value parse_number(char **pc);
  };

  class SHCORE_PUBLIC Argument_list
  {
  public:
    const std::string &string_at(unsigned int i) const;
    bool bool_at(unsigned int i) const;
    int64_t int_at(unsigned int i) const;
    uint64_t uint_at(unsigned int i) const;
    double double_at(unsigned int i) const;
    boost::shared_ptr<Object_bridge> object_at(unsigned int i) const;
    boost::shared_ptr<Value::Map_type> map_at(unsigned int i) const;
    boost::shared_ptr<Value::Array_type> array_at(unsigned int i) const;

    void ensure_count(unsigned int c, const char *context) const;
    void ensure_count(unsigned int minc, unsigned  int maxc, const char *context) const;
    void ensure_at_least(unsigned int minc, const char *context) const;

    void push_back(const Value &value) { _args.push_back(value); }
    size_t size() const { return _args.size(); }
    const Value &at(size_t i) const { return _args.at(i); }
    Value &operator [](size_t i) { return _args[i]; }
    const Value &operator [](size_t i) const { return _args[i]; }
    void clear() { _args.clear(); }
  private:
    std::vector<Value> _args;
  };

  template<class T> struct value_type_for_native { };

  template<> struct value_type_for_native<bool> { static const Value_type type = Bool; };
  template<> struct value_type_for_native<int64_t> { static const Value_type type = Integer; };
  template<> struct value_type_for_native<uint64_t> { static const Value_type type = UInteger; };
  template<> struct value_type_for_native<double> { static const Value_type type = Float; };
  template<> struct value_type_for_native<std::string> { static const Value_type type = String; };
  template<> struct value_type_for_native<Object_bridge*> { static const Value_type type = Object; };
  template<> struct value_type_for_native<Value::Map_type> { static const Value_type type = Map; };
  template<> struct value_type_for_native<Value::Array_type> { static const Value_type type = Array; };

  std::string SHCORE_PUBLIC type_name(Value_type type);

  /** An instance of an object, that's implemented in some language.
   *
   * Objects of this type can be interacted with through it's accessor methods
   * can be used as generic object wrappers/references.
   *
   * They can be instantiated through their Metaclass factory
   */
  class SHCORE_PUBLIC Object_bridge
  {
  public:
    virtual ~Object_bridge() {}

    virtual std::string class_name() const = 0;

    //! Appends JSON representation of the object if supported
    virtual void append_json(JSON_dumper& dumper) const;

    //! Appends descriptive text to the string, suitable for showing to the user
    virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const = 0;

    //! Returns a representation of the object suitable to be passed to a Factory
    // constructor of this object, which would create an equivalent instance of the object
    // Format for repr() of Native_objects must follow this pattern:
    // <Typename:data> where Typename is the same name used to register the type
    virtual std::string &append_repr(std::string &s_out) const = 0;

    //! Returns the list of members that this object has
    virtual std::vector<std::string> get_members() const = 0;

    //! Implements equality operator
    virtual bool operator == (const Object_bridge &other) const = 0;

    virtual bool operator != (const Object_bridge &other) const { return !(*this == other); }

    //! Returns the value of a member
    virtual Value get_member(const std::string &prop) const = 0;

    //! Verifies if the object has a member
    virtual bool has_member(const std::string &prop) const = 0;

    //! Sets the value of a member
    virtual void set_member(const std::string &prop, Value value) = 0;

    //! Returns the value of a member
    virtual bool is_indexed() const = 0;

    //! Returns the value of a member
    virtual Value get_member(size_t index) const = 0;

    //! Sets the value of a member
    virtual void set_member(size_t index, Value value) = 0;

    //! Calls the named method with the given args
    virtual Value call(const std::string &name, const Argument_list &args) = 0;
  };

  class SHCORE_PUBLIC Function_base
  {
  public:
    //! The name of the function
    virtual std::string name() = 0;

    //! Returns the signature of the function, as a list of
    // argname - description, Type pairs
    virtual std::vector<std::pair<std::string, Value_type> > signature() = 0;

    //! Return type of the function, as a description, Type pair
    virtual std::pair<std::string, Value_type> return_type() = 0;

    //! Implements equality operator
    virtual bool operator == (const Function_base &other) const = 0;

    //! Invokes the function and passes back the return value.
    // arglist must match the signature
    virtual Value invoke(const Argument_list &args) = 0;
  };

  /** Pointer to a function that may be implemented in any language.
   */
  typedef boost::shared_ptr<Function_base> Function_base_ref;

  class SHCORE_PUBLIC Exception : public std::exception
  {
    boost::shared_ptr<Value::Map_type> _error;

  public:
    Exception(const boost::shared_ptr<Value::Map_type> e);

    virtual ~Exception() BOOST_NOEXCEPT_OR_NOTHROW{}

    static Exception runtime_error(const std::string &message);
    static Exception argument_error(const std::string &message);
    static Exception attrib_error(const std::string &message);
    static Exception value_error(const std::string &message);
    static Exception type_error(const std::string &message);
    static Exception logic_error(const std::string &message);

    static Exception mysql_error_with_code(const std::string &message, int code)
    {
      return error_with_code("MySQL Error", message, code);
    }

    static Exception mysql_error_with_code_and_state(const std::string &message, int code, const char *sqlstate)
    {
      return error_with_code_and_state("MySQL Error", message, code, sqlstate);
    }

    static Exception error_with_code(const std::string &type, const std::string &message, int code);
    static Exception error_with_code_and_state(const std::string &type, const std::string &message, int code, const char *sqlstate);
    static Exception parser_error(const std::string &message);
    static Exception scripting_error(const std::string &message);

    bool is_argument() const;
    bool is_attribute() const;
    bool is_value() const;
    bool is_type() const;
    bool is_server() const;
    bool is_mysql() const;

    virtual const char *what() const BOOST_NOEXCEPT_OR_NOTHROW;

    const char *type() const BOOST_NOEXCEPT_OR_NOTHROW;

    int64_t code() const BOOST_NOEXCEPT_OR_NOTHROW;

    boost::shared_ptr<Value::Map_type> error() const { return _error; }

    std::string format();
  };

  bool my_strnicmp(const char *c1, const char *c2, size_t n);
}

#endif // _TYPES_H_
