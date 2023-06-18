/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref SqlHelpers class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef SQL_HELPERS_H
#define SQL_HELPERS_H

#include <QString>

/**
 * Class that provides a collection of useful helpers for SQL statements.
 */
class SqlHelpers {
    public:
        SqlHelpers();

        ~SqlHelpers();

        /**
         * Method you can use to escape a string within a SQL statement.
         *
         * \param[in] rawString The string to be escaped.
         *
         * \return Returns the escaped string.
         */
        static QString escape(const QString& rawString);

        /**
         * Method you can use to convert a binary stream to a MySQL compatible textual representation.
         *
         * \param[in] binaryData The binary data to be converted.
         *
         * \return Returns the binary data converted to text.
         */
        static QString binaryToText(const QByteArray& binaryData);
};

#endif
