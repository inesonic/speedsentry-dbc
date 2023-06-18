/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
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
