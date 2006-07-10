// Copyright (C) 2004-2006 MySQL AB
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as published by
// the Free Software Foundation
//
// There are special exceptions to the terms and conditions of the GPL 
// as it is applied to this software. View the full text of the 
// exception in file EXCEPTIONS in the directory of this software 
// distribution.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 

using System;
using System.Data;
using System.Data.Common;
using System.Collections;
using MySql.Data.Types;
using System.Data.SqlTypes;

namespace MySql.Data.MySqlClient
{
	/// <include file='docs/MySqlDataReader.xml' path='docs/ClassSummary/*'/>
	public sealed class MySqlDataReader : DbDataReader, IDataReader, IDisposable, IDataRecord
	{
		// The DataReader should always be open when returned to the user.
		private bool			isOpen = true;

		// Keep track of the results and position
		// within the resultset (starts prior to first record).
		private MySqlField[] fields;
        private IMySqlValue[] values;
		private CommandBehavior	commandBehavior;
		private MySqlCommand command;
		private bool canRead;
		private bool hasRows;
        private long affectedRows;
		private bool[] uaFieldsUsed;
        private Driver driver;
        private long lastInsertId;
        private Statement statement;
        private int seqIndex;
        private bool hasRead;
//        private ServerStatusFlags serverFlags;

		/* 
		 * Keep track of the connection in order to implement the
		 * CommandBehavior.CloseConnection flag. A null reference means
		 * normal behavior (do not automatically close).
		 */
		private MySqlConnection connection = null;

		/*
		 * Because the user should not be able to directly create a 
		 * DataReader object, the constructors are
		 * marked as internal.
		 */
		internal MySqlDataReader(MySqlCommand cmd, Statement statement, CommandBehavior behavior)
		{
			this.command = cmd;
			connection = (MySqlConnection)command.Connection;
			commandBehavior = behavior;
            driver = connection.driver;
            affectedRows = -1;
            this.statement = statement;
        }

        #region Properties

        internal long InsertedId
        {
            get { return this.lastInsertId; }
        }

        internal CommandBehavior Behavior 
		{
			get { return commandBehavior; }
		}

        /// <summary>
        /// Gets a value indicating the depth of nesting for the current row.  This method is not 
        /// supported currently and always returns 0.
        /// </summary>
        public override int Depth
        {
            get { return 0; }
        }

        /// <summary>
        /// Gets the number of columns in the current row.
        /// </summary>
        public override int FieldCount
        {
            // Return the count of the number of columns, which in
            // this case is the size of the column metadata
            // array.
            get
            {
                if (fields != null)
                    return fields.Length;
                return 0;
            }
        }

        /// <summary>
        /// Gets a value indicating whether the MySqlDataReader contains one or more rows.
        /// </summary>
        public override bool HasRows
        {
            get { return hasRows; }
        }

        /// <summary>
		/// Gets a value indicating whether the data reader is closed.
		/// </summary>
		public override bool IsClosed
		{
			get  { return ! isOpen; }
		}

        /// <summary>
        /// Gets the number of rows changed, inserted, or deleted by execution of the SQL statement.
        /// </summary>
        public override int RecordsAffected
        {
            // RecordsAffected returns the number of rows affected in batch
            // statments from insert/delete/update statments.  This property
            // is not completely accurate until .Close() has been called.
            get { return (affectedRows >= 0) ? (int)(affectedRows + 1) : -1; }
        }

        /// <summary>
        /// Overloaded. Gets the value of a column in its native format.
        /// In C#, this property is the indexer for the MySqlDataReader class.
        /// </summary>
        public override object this[int i]
        {
            get { return GetValue(i); }
        }

        /// <summary>
        /// Gets the value of a column in its native format.
        ///	[C#] In C#, this property is the indexer for the MySqlDataReader class.
        /// </summary>
        public override object this[String name]
        {
            // Look up the ordinal and return 
            // the value at that position.
            get { return this[GetOrdinal(name)]; }
        }

        #endregion

		/// <summary>
		/// Closes the MySqlDataReader object.
		/// </summary>
		public override void Close()
		{
			if (! isOpen) return;

            bool shouldCloseConnection = (commandBehavior & CommandBehavior.CloseConnection) != 0;
            commandBehavior = CommandBehavior.Default;
			connection.Reader = null;

            while (NextResult()) { }

            if (shouldCloseConnection)
				connection.Close();

			isOpen = false;
		}

		#region TypeSafe Accessors
		/// <summary>
		/// Gets the value of the specified column as a Boolean.
		/// </summary>
		/// <param name="i"></param>
		/// <returns></returns>
		public override bool GetBoolean(int i)
		{
			return Convert.ToBoolean(GetValue(i));
		}

		/// <summary>
		/// Gets the value of the specified column as a byte.
		/// </summary>
		/// <param name="i"></param>
		/// <returns></returns>
		public override byte GetByte(int i)
		{
			IMySqlValue v = GetFieldValue(i);
			if (v is MySqlUByte)
				return ((MySqlUByte)v).Value;
			else
				return (byte)((MySqlByte)v).Value;
		}

		/// <summary>
		/// Reads a stream of bytes from the specified column offset into the buffer an array starting at the given buffer offset.
		/// </summary>
		/// <param name="i">The zero-based column ordinal. </param>
		/// <param name="dataIndex">The index within the field from which to begin the read operation. </param>
		/// <param name="buffer">The buffer into which to read the stream of bytes. </param>
		/// <param name="bufferIndex">The index for buffer to begin the read operation. </param>
		/// <param name="length">The maximum length to copy into the buffer. </param>
		/// <returns>The actual number of bytes read.</returns>
		/// <include file='docs/MySqlDataReader.xml' path='MyDocs/MyMembers[@name="GetBytes"]/*'/>
		public override long GetBytes(int i, long dataIndex, byte[] buffer, int bufferIndex, int length)
		{
			if (i >= fields.Length) 
				throw new IndexOutOfRangeException();

			IMySqlValue val = GetFieldValue(i);

			if (! (val is MySqlBinary))
				throw new MySqlException("GetBytes can only be called on binary columns");

			MySqlBinary binary = (MySqlBinary)val;
			if (buffer == null) 
				return (long)binary.Value.Length;

			if (bufferIndex >= buffer.Length || bufferIndex < 0)
				throw new IndexOutOfRangeException("Buffer index must be a valid index in buffer");
			if (buffer.Length < (bufferIndex + length))
				throw new ArgumentException( "Buffer is not large enough to hold the requested data" );
			if (dataIndex < 0 || 
				((ulong)dataIndex >= (ulong)binary.Value.Length && (ulong)binary.Value.Length > 0))
				throw new IndexOutOfRangeException( "Data index must be a valid index in the field" );

			byte[] bytes = (byte[])binary.Value; 

			// adjust the length so we don't run off the end
			if ( (ulong)binary.Value.Length < (ulong)(dataIndex+length)) 
			{
				length = (int)((ulong)binary.Value.Length - (ulong)dataIndex);
			}

			Array.Copy( bytes, (int)dataIndex, buffer, (int)bufferIndex, (int)length );

			return length;
		}

		/// <summary>
		/// Gets the value of the specified column as a single character.
		/// </summary>
		/// <param name="i"></param>
		/// <returns></returns>
		public override char GetChar(int i)
		{
			string s = GetString(i);
			return s[0];
		}

		/// <summary>
		/// Reads a stream of characters from the specified column offset into the buffer as an array starting at the given buffer offset.
		/// </summary>
		/// <param name="i"></param>
		/// <param name="fieldOffset"></param>
		/// <param name="buffer"></param>
		/// <param name="bufferoffset"></param>
		/// <param name="length"></param>
		/// <returns></returns>
		public override long GetChars(int i, long fieldOffset, char[] buffer, int bufferoffset, int length)
		{
			if (i >= fields.Length) 
				throw new IndexOutOfRangeException();

			string valAsString = GetString(i);

			if (buffer == null) return valAsString.Length;

			if (bufferoffset >= buffer.Length || bufferoffset < 0)
				throw new IndexOutOfRangeException("Buffer index must be a valid index in buffer");
			if (buffer.Length < (bufferoffset + length))
				throw new ArgumentException( "Buffer is not large enough to hold the requested data" );
			if (fieldOffset < 0 || fieldOffset >= valAsString.Length )
				throw new IndexOutOfRangeException( "Field offset must be a valid index in the field" );
			
			if (valAsString.Length < length)
				length = valAsString.Length;
			valAsString.CopyTo( (int)fieldOffset, buffer, bufferoffset, length );
			return length;
		}

		/// <summary>
		/// Gets the name of the source data type.
		/// </summary>
		/// <param name="i"></param>
		/// <returns></returns>
		public override String GetDataTypeName(int i)
		{
			if (! isOpen) throw new Exception("No current query in data reader");
			if (i >= fields.Length) throw new IndexOutOfRangeException();

			// return the name of the type used on the backend
			return values[i].MySqlTypeName;
		}

		/// <include file='docs/MySqlDataReader.xml' path='docs/GetMySqlDateTime/*'/>
		public MySqlDateTime GetMySqlDateTime(int index)
		{
			return (MySqlDateTime)GetFieldValue(index);
		}

		/// <include file='docs/MySqlDataReader.xml' path='docs/GetDateTime/*'/>
		public override DateTime GetDateTime(int index)
		{
			IMySqlValue val = GetFieldValue(index);
            if (val is MySqlDateTime)
            {
				MySqlDateTime dt = (MySqlDateTime)val;
				if (connection.Settings.ConvertZeroDateTime && !dt.IsValidDateTime)
					return DateTime.MinValue;
//				else
//					return dt.GetDateTime();
			}
			else if (val is MySqlString)
			{
                MySqlDateTime dt = MySqlDateTime.Parse(val.ToString(), this.connection.driver.Version);
                val = dt;
//				MySqlDateTime d = new MySqlDateTime(MySqlDbType.Datetime);
//				d = d.ParseMySql( (val as MySqlString).Value, true );
//				return d.GetDateTime();
            }
            return Convert.ToDateTime(val);
		}

		/// <include file='docs/MySqlDataReader.xml' path='docs/GetDecimal/*'/>
		public override Decimal GetDecimal(int index)
		{
			IMySqlValue v = GetFieldValue(index);
			if (v is MySqlDecimal)
				return ((MySqlDecimal)v).Value;
			return Convert.ToDecimal(v.Value);
		}

		/// <include file='docs/MySqlDataReader.xml' path='docs/GetDouble/*'/>
		public override double GetDouble(int index)
		{
			IMySqlValue v = GetFieldValue(index);
			if (v is MySqlDouble)
				return ((MySqlDouble)v).Value;
			return Convert.ToDouble(v.Value);
		}

		/// <summary>
		/// Gets the Type that is the data type of the object.
		/// </summary>
		/// <param name="i"></param>
		/// <returns></returns>
		public override Type GetFieldType(int i)
		{
			if (! isOpen) throw new Exception("No current query in data reader");
			if (i >= fields.Length) throw new IndexOutOfRangeException();

			if (values[i] is MySqlDateTime && !connection.Settings.AllowZeroDateTime)
				return typeof(DateTime);
			return values[i].SystemType;
		}

		/// <include file='docs/MySqlDataReader.xml' path='docs/GetFloat/*'/>
		public override float GetFloat(int index)
		{
			IMySqlValue v = GetFieldValue(index);
			if (v is MySqlSingle)
				return ((MySqlSingle)v).Value;
			return Convert.ToSingle(v.Value);
		}

		/// <include file='docs/MySqlDataReader.xml' path='docs/GetGuid/*'/>
		public override Guid GetGuid(int index)
		{
			return new Guid( GetString(index) );
		}

		/// <include file='docs/MySqlDataReader.xml' path='docs/GetInt16/*'/>
		public override Int16 GetInt16(int index)
		{
			IMySqlValue v = GetFieldValue(index);
			if (v is MySqlInt16)
				return ((MySqlInt16)v).Value;
            
            connection.UsageAdvisor.Converting(command.CommandText,
                fields[index].ColumnName, v.MySqlTypeName, "Int16");
			return ((IConvertible)v.Value).ToInt16(null);
		}

		/// <include file='docs/MySqlDataReader.xml' path='docs/GetInt32/*'/>
		public override Int32 GetInt32(int index)
		{
			IMySqlValue v = GetFieldValue(index);
			if (v is MySqlInt32)
				return ((MySqlInt32)v).Value;
            
            connection.UsageAdvisor.Converting(command.CommandText,
                fields[index].ColumnName, v.MySqlTypeName, "Int32");
            return ((IConvertible)v.Value).ToInt32(null); 
		}

		/// <include file='docs/MySqlDataReader.xml' path='docs/GetInt64/*'/>
		public override Int64 GetInt64(int index)
		{
			IMySqlValue v = GetFieldValue(index);
			if (v is MySqlInt64)
				return ((MySqlInt64)v).Value;

            connection.UsageAdvisor.Converting(command.CommandText,
                fields[index].ColumnName, v.MySqlTypeName, "Int64");
            return ((IConvertible)v.Value).ToInt64(null); 
		}

		/// <summary>
		/// Gets the name of the specified column.
		/// </summary>
		/// <param name="i"></param>
		/// <returns></returns>
		public override String GetName(int i)
		{
			return fields[i].ColumnName;
		}

		/// <summary>
		/// Gets the column ordinal, given the name of the column.
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		public override int GetOrdinal(string name)
		{
			if (! isOpen)
				throw new Exception("No current query in data reader");

			name = name.ToLower(System.Globalization.CultureInfo.InvariantCulture);
			for (int i=0; i < fields.Length; i ++) 
			{
				if (fields[i].ColumnName.ToLower(System.Globalization.CultureInfo.InvariantCulture) == name)
					return i;
			}

			// Throw an exception if the ordinal cannot be found.
			throw new IndexOutOfRangeException("Could not find specified column in results");
		}

		/// <summary>
		/// Returns a DataTable that describes the column metadata of the MySqlDataReader.
		/// </summary>
		/// <returns></returns>
		public override DataTable GetSchemaTable()
		{
			// Only Results from SQL SELECT Queries 
			// get a DataTable for schema of the result
			// otherwise, DataTable is null reference
			if (fields.Length == 0) return null;

			DataTable dataTableSchema = new DataTable ("SchemaTable");
			
			dataTableSchema.Columns.Add ("ColumnName", typeof (string));
			dataTableSchema.Columns.Add ("ColumnOrdinal", typeof (int));
			dataTableSchema.Columns.Add ("ColumnSize", typeof (int));
			dataTableSchema.Columns.Add ("NumericPrecision", typeof (int));
			dataTableSchema.Columns.Add ("NumericScale", typeof (int));
			dataTableSchema.Columns.Add ("IsUnique", typeof (bool));
			dataTableSchema.Columns.Add ("IsKey", typeof (bool));
			DataColumn dc = dataTableSchema.Columns["IsKey"];
			dc.AllowDBNull = true; // IsKey can have a DBNull
			dataTableSchema.Columns.Add ("BaseCatalogName", typeof (string));
			dataTableSchema.Columns.Add ("BaseColumnName", typeof (string));
			dataTableSchema.Columns.Add ("BaseSchemaName", typeof (string));
			dataTableSchema.Columns.Add ("BaseTableName", typeof (string));
			dataTableSchema.Columns.Add ("DataType", typeof(Type));
			dataTableSchema.Columns.Add ("AllowDBNull", typeof (bool));
			dataTableSchema.Columns.Add ("ProviderType", typeof (int));
			dataTableSchema.Columns.Add ("IsAliased", typeof (bool));
			dataTableSchema.Columns.Add ("IsExpression", typeof (bool));
			dataTableSchema.Columns.Add ("IsIdentity", typeof (bool));
			dataTableSchema.Columns.Add ("IsAutoIncrement", typeof (bool));
			dataTableSchema.Columns.Add ("IsRowVersion", typeof (bool));
			dataTableSchema.Columns.Add ("IsHidden", typeof (bool));
			dataTableSchema.Columns.Add ("IsLong", typeof (bool));
			dataTableSchema.Columns.Add ("IsReadOnly", typeof (bool));

			int ord = 1;
			for (int i=0; i < fields.Length; i++)
			{
				MySqlField f = fields[i];
				DataRow r = dataTableSchema.NewRow();
				r["ColumnName"] = f.ColumnName;
				r["ColumnOrdinal"] = ord++;
				r["ColumnSize"] = f.ColumnLength;
				int prec = f.Precision;
				int pscale = f.Scale;
				if (prec != -1)
					r["NumericPrecision"] = (short)prec;
				if (pscale != -1)
					r["NumericScale"] = (short)pscale;
				r["DataType"] = GetFieldType(i);
                r["ProviderType"] = (int)f.Type;
				r["IsLong"] = f.IsBlob && f.ColumnLength > 255;
				r["AllowDBNull"] = f.AllowsNull;
				r["IsReadOnly"] = false;
				r["IsRowVersion"] = false;
				r["IsUnique"] = f.IsUnique || f.IsPrimaryKey;
				r["IsKey"] = f.IsPrimaryKey;
				r["IsAutoIncrement"] = f.IsAutoIncrement;
				r["BaseSchemaName"] = f.DatabaseName;
				r["BaseCatalogName"] = f.DatabaseName;
				r["BaseTableName"] = f.RealTableName;
				r["BaseColumnName"] = f.OriginalColumnName;

				dataTableSchema.Rows.Add( r );
			}

			return dataTableSchema;
		}

		/// <include file='docs/MySqlDataReader.xml' path='docs/GetString/*'/>
		public override String GetString(int index)
		{
			IMySqlValue val = GetFieldValue(index);
            if (val.IsNull)
                throw new SqlNullValueException();

			if (val is MySqlBinary)
			{
				byte[] v = ((MySqlBinary)val).Value;
				return fields[index].Encoding.GetString(v, 0, v.Length);
			}

			return val.Value.ToString();
		}

		/// <include file='docs/MySqlDataReader.xml' path='docs/GetTimeSpan/*'/>
		public TimeSpan GetTimeSpan(int index)
		{
			MySqlTimeSpan ts = (MySqlTimeSpan)GetFieldValue(index);
			return ts.Value;
		}

		/// <summary>
		/// Gets the value of the specified column in its native format.
		/// </summary>
		/// <param name="i"></param>
		/// <returns></returns>
		public override object GetValue(int i)
		{
			if (! isOpen) throw new Exception("No current query in data reader");
			if (i >= fields.Length) throw new IndexOutOfRangeException();

			IMySqlValue val = GetFieldValue(i);
			if (val.IsNull) return DBNull.Value;

			// if the column is a date/time, then we return a MySqlDateTime
			// so .ToString() will print '0000-00-00' correctly
			if (val is MySqlDateTime) 
			{
                MySqlDateTime dt = (MySqlDateTime)val;
				if (! dt.IsValidDateTime && connection.Settings.ConvertZeroDateTime)
					return DateTime.MinValue;
				else if (connection.Settings.AllowZeroDateTime) 
					return val;
				else
					return dt.GetDateTime();
			}

			return val.Value;
		}

		/// <summary>
		/// Gets all attribute columns in the collection for the current row.
		/// </summary>
		/// <param name="values"></param>
		/// <returns></returns>
		public override int GetValues(object[] values)
		{
			if (! hasRead) return 0;
			int numCols = Math.Min( values.Length, fields.Length );
			for (int i=0; i < numCols; i ++) 
				values[i] = GetValue(i);

			return numCols;
		}

		/// <include file='docs/MySqlDataReader.xml' path='docs/GetUInt16/*'/>
		public UInt16 GetUInt16(int index)
		{
			IMySqlValue v = GetFieldValue(index);
			if (v is MySqlUInt16)
				return ((MySqlUInt16)v).Value;

            connection.UsageAdvisor.Converting(command.CommandText,
                fields[index].ColumnName, v.MySqlTypeName, "UInt16");
            return Convert.ToUInt16(v.Value);
		}

		/// <include file='docs/MySqlDataReader.xml' path='docs/GetUInt32/*'/>
		public UInt32 GetUInt32(int index)
		{
			IMySqlValue v = GetFieldValue(index);
			if (v is MySqlUInt32)
				return ((MySqlUInt32)v).Value;

            connection.UsageAdvisor.Converting(command.CommandText,
                fields[index].ColumnName, v.MySqlTypeName, "UInt32");
            return Convert.ToUInt32(v.Value);
		}

		/// <include file='docs/MySqlDataReader.xml' path='docs/GetUInt64/*'/>
		public UInt64 GetUInt64(int index)
		{
			IMySqlValue v = GetFieldValue(index);
			if (v is MySqlUInt64)
				return ((MySqlUInt64)v).Value;

            connection.UsageAdvisor.Converting(command.CommandText,
                fields[index].ColumnName, v.MySqlTypeName, "UInt64");
            return Convert.ToUInt64(v.Value);
		}


		#endregion

		IDataReader IDataRecord.GetData(int i)
		{
			return base.GetData(i);
		}

		/// <summary>
		/// Gets a value indicating whether the column contains non-existent or missing values.
		/// </summary>
		/// <param name="i"></param>
		/// <returns></returns>
		public override bool IsDBNull(int i)
		{
			return DBNull.Value == GetValue(i);
		}

        /// <summary>
        /// GetResultSet is the core resultset processing method.  It gets called by NextResult
        /// and will loop until it finds a select resultset at which point it will return the 
        /// number of columns in that result.  It will _not_ return for non-select resultsets instead
        /// just updating the internal lastInsertId and affectedRows variables.  
        /// </summary>
        /// <returns>-1 if no more results exist, >= 0 for select results</returns>
        private long GetResultSet()
        {
            while (true)
            {
                ulong affectedRowsTemp = 0;
                long fieldCount = driver.ReadResult(ref affectedRowsTemp, ref lastInsertId);
                if (fieldCount > 0)
                    return fieldCount;
                else if (fieldCount == 0)
                {
                    command.lastInsertedId = lastInsertId;
                    affectedRows += (long)affectedRowsTemp;
                }
                else if (fieldCount == -1)
                    if (!statement.ExecuteNext())
                        break;
            }
            return -1;
        }

		/// <summary>
		/// Advances the data reader to the next result, when reading the results of batch SQL statements.
		/// </summary>
		/// <returns></returns>
		public override bool NextResult()
		{
			if (! isOpen)
				throw new MySqlException("Invalid attempt to NextResult when reader is closed.");

            bool firstResult = fields == null;
            if (fields != null)
            {
                ClearCurrentResultset();
                fields = null;
            }

            // single result means we only return a single resultset.  If we have already
            // returned one, then we return false;
            if (!firstResult && (commandBehavior & CommandBehavior.SingleResult) != 0)
                return false;

			// tell our command to continue execution of the SQL batch until it its
			// another resultset
			try 
			{
                long fieldCount = GetResultSet();
                if (fieldCount == -1)
                    return false;

				// issue any requested UA warnings
				if (connection.Settings.UseUsageAdvisor) 
				{
					if ((connection.driver.ServerStatus & ServerStatusFlags.NoIndex) != 0)
						connection.UsageAdvisor.UsingNoIndex( command.CommandText );
					if ((connection.driver.ServerStatus & ServerStatusFlags.BadIndex) != 0)
						connection.UsageAdvisor.UsingBadIndex( command.CommandText );
				}

                fields = driver.ReadColumnMetadata((int)fieldCount);
                values = new IMySqlValue[fields.Length];
                for (int i = 0; i < fields.Length; i++)
                    values[i] = fields[i].GetValueObject();
                hasRead = false;

                uaFieldsUsed = new bool[fields.Length];
                hasRows = canRead = driver.FetchDataRow(statement.Id, 0, fields.Length);
                return true;
            }
			catch (MySqlException ex) 
			{
				if (ex.IsFatal) 
                    connection.Abort();
				throw;
			}

			// if there was no more resultsets, then signal done
//			if (currentResult == null) 
//			{
//				canRead = false;
//				return false;
//			}

//            SchemaTableColumn = null;

			// When executing query statements, the result byte that is returned
			// from MySql is the column count.  That is why we reference the LastResult
			// property here to dimension our field array
			//connection.SetState( ConnectionState.Fetching );

			// load in our field defs and set our internal variables so we know
			// what we can do (canRead, hasRows)
//			try 
//			{
//				canRead = hasRows = currentResult.Load();
//				fields = currentResult.Fields;
//				return true;
//			}
//			catch (Exception ex) 
//			{
//				if (ex.IsFatal) 
//					connection.Close();
//				else
//					connection.SetState( ConnectionState.Open );
//				throw;
//			}
//			finally 
//			{
//				if (connection.State != ConnectionState.Closed && connection.State != ConnectionState.Open)
//					connection.SetState( ConnectionState.Open );
//			}
		}

/*        private void ReadDataRow()
        {
            System.Diagnostics.Debug.Assert(data == null, "Column data array should be null");
            data = new IMySqlValue[FieldCount];
            if ((commandBehavior & CommandBehavior.SequentialAccess) == 0)
                connection.driver.ReadDataRow(fields, data);
        }
        */
		/// <summary>
		/// Advances the MySqlDataReader to the next record.
		/// </summary>
		/// <returns></returns>
		public override bool Read()
		{
			if (! isOpen)
				throw new MySqlException("Invalid attempt to Read when reader is closed.");

			if (! canRead) 
                return false;

            if (Behavior == CommandBehavior.SingleRow && hasRead)
                return false;

            try 
			{
				bool isSequential = (Behavior & CommandBehavior.SequentialAccess) != 0;
                seqIndex = -1;
                if (hasRead)
                    canRead = driver.FetchDataRow(statement.Id, 0, fields.Length);
                hasRead = true;
                if (canRead && !isSequential)
                    for (int i = 0; i < fields.Length; i++)
                        values[i] = driver.ReadColumnValue(i, fields[i], values[i]);

                return canRead;
			}
			catch (MySqlException ex) 
			{
				if (ex.IsFatal) 
                    connection.Abort();
				throw;
			}
		}


		private IMySqlValue GetFieldValue(int index) 
		{
			if (index < 0 || index >= fields.Length) 
				throw new ArgumentException( "You have specified an invalid column ordinal." );

			if (! hasRead)
				throw new MySqlException("Invalid attempt to access a field before calling Read()");

			// keep count of how many columns we have left to access
			this.uaFieldsUsed[index] = true;

            if ((this.commandBehavior & CommandBehavior.SequentialAccess) != 0 && 
                index != seqIndex)
            {
                if (index < seqIndex)
                    throw new MySqlException("Invalid attempt to read a prior column using SequentialAccess");
                while (seqIndex < (index-1))
                    driver.SkipColumnValue(values[++seqIndex]);
                values[index] = driver.ReadColumnValue(index, fields[index], values[index]);
                seqIndex = index;
            }

            return values[index];
		}

		private void ClearCurrentResultset() 
		{
            if (!canRead) return;
            while (driver.SkipDataRow())
            {
            }

			if (!connection.Settings.UseUsageAdvisor) return;

			// we were asked to run the usage advisor so report if the resultset
			// was not entirely read.
			if (canRead)
				connection.UsageAdvisor.ReadPartialResultSet(command.CommandText);

			bool readAll = true;
			foreach (bool b in this.uaFieldsUsed)
				readAll &= b;
			if (! readAll)
				connection.UsageAdvisor.ReadPartialRowSet(command.CommandText, uaFieldsUsed, fields);
		}

		#region IEnumerator

		public override IEnumerator	GetEnumerator()
		{
			return new DbEnumerator(this);
		}

		#endregion
	}
}
