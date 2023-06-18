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
* This header implements the \ref SqlHelpers class.
***********************************************************************************************************************/

#include <QString>

#include "sql_helpers.h"

SqlHelpers::SqlHelpers() {}


SqlHelpers::~SqlHelpers() {}


QString SqlHelpers::escape(const QString& rawString) {
    QString result = rawString;
    result.replace("\\", "\\\\");
    result.replace("'", "\\'");
    result.replace("\"", "\\\"");
    result.replace("\x08", "\\b");
    result.replace("\n", "\\n");
    result.replace("\r", "\\r");
    result.replace("\t", "\\t");
    result.replace("\x1A", "\\Z");
    result.replace("%", "\\%");
    result.replace("_", "\\_");

    return result;
}


QString SqlHelpers::binaryToText(const QByteArray& binaryData) {
    QString  result("0x");
    unsigned dataLength = static_cast<unsigned>(binaryData.size());
    if (dataLength > 0) {
        for (unsigned i=0; i<dataLength ; ++i) {
            QString v = QString::number(static_cast<unsigned>(binaryData[i]) & 0xFF, 16);
            Q_ASSERT(v.length() != 0 && v.length() <= 2);

            if (v.length() == 1) {
                result.append('0');
            }

            result.append(v);
        }
    } else {
        result += QString("00");
    }

    return result;
}
