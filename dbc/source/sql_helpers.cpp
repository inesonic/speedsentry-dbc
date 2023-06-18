/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
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
