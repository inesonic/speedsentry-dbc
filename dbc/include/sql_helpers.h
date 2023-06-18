/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 - 2023 Inesonic, LLC.
*
* GNU Public License, Version 3:
*   This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
*   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
*   version.
*   
*   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
*   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
*   details.
*   
*   You should have received a copy of the GNU General Public License along with this program.  If not, see
*   <https://www.gnu.org/licenses/>.
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
