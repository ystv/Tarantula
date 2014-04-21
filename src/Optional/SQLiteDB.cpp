/******************************************************************************
*   Copyright (C) 2011 - 2013  York Student Television
*
*   Tarantula is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   Tarantula is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with Tarantula.  If not, see <http://www.gnu.org/licenses/>.
*
*   Contact     : tarantula@ystv.co.uk
*
*   File Name   : SQLiteDB.cpp
*   Version     : 1.0
*   Description : Base class for an in-memory SQLite database. A wrapper around
*                 the sqlite functions to make them a bit more C++y Also
*                 contains the implementations for DBParam and DBQuery.
*
*****************************************************************************/


#include "SQLiteDB.h"
#include "Log.h"
#include "ErrorMacro.h"
#include <iostream>

extern Log g_logger;

/*
 * DBParam constructor implementations
 */

DBParam::DBParam (int val)
{
    m_type = DBPARAM_INT;
    m_intval = val;
}
DBParam::DBParam (time_t val)
{
	m_type = DBPARAM_LONG;
	m_longval = val;
}
DBParam::DBParam (long long val)
{
    m_type = DBPARAM_LONG;
    m_longval = val;
}
DBParam::DBParam (std::string val)
{
    m_type = DBPARAM_STRING;
    m_stringval = val;
}
DBParam::DBParam (double val)
{
    m_type = DBPARAM_DOUBLE;
    m_doubleval = val;
}
DBParam::DBParam ()
{
    m_type = DBPARAM_NULL;
}

/**
 * Opens a file-based database.
 * Will create it if it doesn't exist.
 *
 * @param filename Path to database file
 */
SQLiteDB::SQLiteDB (const char* filename)
{
    int ret = sqlite3_open(filename, &m_pdb);

    if (SQLITE_OK != ret)
    {
        g_logger.error("SQLiteDB Open()" + ERROR_LOC, sqlite3_errmsg(m_pdb));
        throw std::exception();
    }
}

/**
 * Opens a file-based database.
 * Will create it if it doesn't exist.
 *
 * @param filename Path to database file
 */
SQLiteDB::SQLiteDB (std::string filename)
{
    int ret = sqlite3_open(filename.c_str(), &m_pdb);

    if (SQLITE_OK != ret)
    {
        g_logger.error("SQLiteDB Open()" + ERROR_LOC, sqlite3_errmsg(m_pdb));
        throw std::exception();
    }

    // Enable write-ahead and disable synchronous mode for performance
    oneTimeExec("PRAGMA journal_mode=WAL");
    oneTimeExec("PRAGMA synchronous=0");
}


SQLiteDB::~SQLiteDB ()
{
    sqlite3_close_v2(m_pdb);
}

/**
 * Dump the entire database out to the specified file. Good for debugging
 *
 * @param filename
 */
void SQLiteDB::dump (const char* filename)
{
    sqlite3_backup *BackupObj;
    sqlite3 *file;

    // Open file
    sqlite3_open(filename, &file);

    // Setup backup object, copy database, then finish backup object
    BackupObj = sqlite3_backup_init(file, "main", m_pdb, "main");
    sqlite3_backup_step(BackupObj, -1);
    sqlite3_backup_finish(BackupObj);

    sqlite3_close(file);
}

/**
 * Add the SQL statement to a query, with parameters replaced with ? and returns
 * a pointer to the result. Does not create duplicates.
 *
 * @param sql The SQL query string to be executed
 * @return    A pointer to the entry in the SQLiteDB instance query table.
 */
std::shared_ptr<DBQuery> SQLiteDB::prepare (std::string sql)
{
    return std::make_shared<DBQuery>(sql, m_pdb);
}

/**
 * Execute a complete SQL statement, without bothering with parameters.
 *
 * @param sql
 */
void SQLiteDB::oneTimeExec (std::string sql)
{
    // Prepare query
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(m_pdb, (const char*) sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        std::cout << "Query failed: " << (char*) sql.c_str() << std::endl;
    }

    // Run query
    int ret = sqlite3_step(stmt);
    if (SQLITE_DONE != ret && SQLITE_ROW != ret)
    {
        g_logger.error("SQLiteDB oneTimeExec()" + ERROR_LOC,
        		"Code: " + std::to_string(ret) + " - " + sqlite3_errmsg(m_pdb));
    }

    // Remove from prepared statement store to free memory
    sqlite3_finalize(stmt);
}

/**
 * Return the row ID of the last inserted row.
 * @return
 */
int SQLiteDB::getLastRowID ()
{
    return sqlite3_last_insert_rowid(m_pdb);
}

/*
 * DBQuery Implementation
 */

DBQuery::DBQuery ()
{
    m_pstmt = NULL;
    m_pdb = NULL;
}

DBQuery::DBQuery (std::string querystring, sqlite3 *pdb)
{
    sql(querystring, pdb);
}

/**
 * Prepare the query statement.
 *
 * @param sql SQL statement to execute
 * @param db  Pointer to an sqlite3 database.
 */
void DBQuery::sql (std::string querystring, sqlite3 *pdb)
{
    m_pdb = pdb;
    m_querytext = querystring;

    sqlite3_prepare_v2(pdb, (const char*) querystring.c_str(), -1, &m_pstmt, NULL);
}

DBQuery::~DBQuery ()
{
    if (m_pstmt)
        sqlite3_finalize(m_pstmt);
}

/**
 * Add a parameter to the query.
 * @param pos   Index of the parameter within the SQL statement
 * @param param Parameter data.
 */
void DBQuery::addParam (const int pos, DBParam param)
{
    m_params.insert(std::pair<int, DBParam>(pos, param));
}

/**
 * Clear all parameters from a query
 */
void DBQuery::rmParams ()
{
    if (m_pstmt == NULL)
    {
        sql(m_querytext, m_pdb);
    }

    //clear statement
    sqlite3_clear_bindings(m_pstmt);
    //clear vector
    m_params.clear();
}

/**
 * Perform internal processing to add internally stored parameters to underlying
 * SQLite construct with appropriate protection from injection attacks.
 */
void DBQuery::bindParams ()
{
    sqlite3_reset(m_pstmt);

    for (std::map<int, DBParam>::iterator it = m_params.begin();
            it != m_params.end(); it++)
    {
        switch (it->second.m_type)
        {
            case DBPARAM_INT:
                sqlite3_bind_int(m_pstmt, it->first, it->second.m_intval);
                break;
            case DBPARAM_LONG:
                sqlite3_bind_int64(m_pstmt, it->first, it->second.m_longval);
                break;
            case DBPARAM_STRING:
                sqlite3_bind_text(m_pstmt, it->first,
                        (const char*) (it->second.m_stringval.c_str()), -1,
                        SQLITE_TRANSIENT);
                break;
            case DBPARAM_NULL:
                sqlite3_bind_null(m_pstmt, it->first);
                break;
            case DBPARAM_DOUBLE:
                sqlite3_bind_double(m_pstmt, it->first, it->second.m_doubleval);
                break;
        }
    }
}

/**
 * Return a completed statement ready to execute.
 *
 * @return Statement to be passed to sqlite3_step() or similar.
 */
sqlite3_stmt* DBQuery::getStmt ()
{
    sqlite3_reset(m_pstmt);
    return m_pstmt;
}
