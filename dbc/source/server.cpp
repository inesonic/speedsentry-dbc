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
* This header implements the \ref Server class.
***********************************************************************************************************************/

#include <QString>

#include "server.h"

const Server::ServerId Server::invalidServerId = 0;

QString Server::toString(Server::Status status) {
    QString result;
    switch (status) {
        case Status::ALL_UNKNOWN: { result = QString("UNKNOWN");   break; }
        case Status::ACTIVE:      { result = QString("ACTIVE");    break; }
        case Status::INACTIVE:    { result = QString("INACTIVE");  break; }
        case Status::DEFUNCT:     { result = QString("DEFUNCT");   break; }
        default:                  { Q_ASSERT(false);               break; }
    }

    return result;
}


Server::Status Server::toStatus(const QString& str, bool* success) {
    bool   ok = true;
    Status result;

    QString s  = str.trimmed().toLower().replace("-", "_");
    if (s == "unknown") {
        result = Status::ALL_UNKNOWN;
    } else if (s == "active") {
        result = Status::ACTIVE;
    } else if (s == "inactive") {
        result = Status::INACTIVE;
    } else if (s == "defunct") {
        result = Status::DEFUNCT;
    } else {
        ok = false;
        result = Status::ALL_UNKNOWN;
    }

    if (success != nullptr) {
        *success = ok;
    }

    return result;
}
