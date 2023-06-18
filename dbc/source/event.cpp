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
* This header implements the \ref Event class.
***********************************************************************************************************************/

#include <QString>

#include "event.h"

QString Event::toString(Event::EventType value) {
    QString result;
    switch (value) {
        case EventType::INVALID:                  { result = QString("INVALID");                   break; }
        case EventType::WORKING:                  { result = QString("WORKING");                   break; }
        case EventType::NO_RESPONSE:              { result = QString("NO_RESPONSE");               break; }
        case EventType::CONTENT_CHANGED:          { result = QString("CONTENT_CHANGED");           break; }
        case EventType::KEYWORDS:                 { result = QString("KEYWORDS");                  break; }
        case EventType::SSL_CERTIFICATE_EXPIRING: { result = QString("SSL_CERTIFICATE_EXPIRING");  break; }
        case EventType::SSL_CERTIFICATE_RENEWED:  { result = QString("SSL_CERTIFICATE_RENEWED");   break; }
        case EventType::CUSTOMER_1:               { result = QString("CUSTOMER_1");                break; }
        case EventType::CUSTOMER_2:               { result = QString("CUSTOMER_2");                break; }
        case EventType::CUSTOMER_3:               { result = QString("CUSTOMER_3");                break; }
        case EventType::CUSTOMER_4:               { result = QString("CUSTOMER_4");                break; }
        case EventType::CUSTOMER_5:               { result = QString("CUSTOMER_5");                break; }
        case EventType::CUSTOMER_6:               { result = QString("CUSTOMER_6");                break; }
        case EventType::CUSTOMER_7:               { result = QString("CUSTOMER_7");                break; }
        case EventType::CUSTOMER_8:               { result = QString("CUSTOMER_8");                break; }
        case EventType::CUSTOMER_9:               { result = QString("CUSTOMER_9");                break; }
        case EventType::CUSTOMER_10:              { result = QString("CUSTOMER_10");               break; }
        case EventType::TRANSACTION:              { result = QString("TRANSACTION");               break; }
        case EventType::INQUIRY:                  { result = QString("INQUIRY");                   break; }
        case EventType::SUPPORT_REQUEST:          { result = QString("SUPPORT_REQUEST");           break; }
        case EventType::STORAGE_LIMIT_REACHED:    { result = QString("STORAGE_LIMIT_REACHED");     break; }
        default:                                  { Q_ASSERT(false);                               break; }
    }

    return result;
}


Event::EventType Event::toEventType(const QString& str, bool* ok) {
    bool      success = true;
    EventType result;

    QString s  = str.trimmed().toLower().replace("-", "_");
    if (s == "invalid") {
        result = EventType::INVALID;
    } else if (s == "working") {
        result = EventType::WORKING;
    } else if (s == "no_response") {
        result = EventType::NO_RESPONSE;
    } else if (s == "content_changed") {
        result = EventType::CONTENT_CHANGED;
    } else if (s == "keywords") {
        result = EventType::KEYWORDS;
    } else if (s == "ssl_certificate_expiring") {
        result = EventType::SSL_CERTIFICATE_EXPIRING;
    } else if (s == "ssl_certificate_renewed") {
        result = EventType::SSL_CERTIFICATE_RENEWED;
    } else if (s == "customer_1") {
        result = EventType::CUSTOMER_1;
    } else if (s == "customer_2") {
        result = EventType::CUSTOMER_2;
    } else if (s == "customer_3") {
        result = EventType::CUSTOMER_3;
    } else if (s == "customer_4") {
        result = EventType::CUSTOMER_4;
    } else if (s == "customer_5") {
        result = EventType::CUSTOMER_5;
    } else if (s == "customer_6") {
        result = EventType::CUSTOMER_6;
    } else if (s == "customer_7") {
        result = EventType::CUSTOMER_7;
    } else if (s == "customer_8") {
        result = EventType::CUSTOMER_8;
    } else if (s == "customer_9") {
        result = EventType::CUSTOMER_9;
    } else if (s == "customer_10") {
        result = EventType::CUSTOMER_10;
    } else if (s == "transaction") {
        result = EventType::TRANSACTION;
    } else if (s == "inquiry") {
        result = EventType::INQUIRY;
    } else if (s == "support_request") {
        result = EventType::SUPPORT_REQUEST;
    } else if (s == "storage_limit_reached") {
        result = EventType::STORAGE_LIMIT_REACHED;
    } else {
        success = false;
        result = EventType::INVALID;
    }

    if (ok != nullptr) {
        *ok = success;
    }

    return result;
}


Event::EventType Event::toCustomerEventType(unsigned typeIndex) {
    static constexpr unsigned numberEvents = (
          static_cast<unsigned>(EventType::STORAGE_LIMIT_REACHED)
        - static_cast<unsigned>(EventType::CUSTOMER_1)
        + 1
    );

    return   (typeIndex > 0 && typeIndex <= numberEvents)
           ? static_cast<EventType>(static_cast<unsigned>(EventType::CUSTOMER_1) + typeIndex - 1)
           : EventType::INVALID;
}


bool Event::isCustomerEvent(EventType eventType) {
    unsigned i = static_cast<unsigned>(eventType);
    return i >= static_cast<unsigned>(EventType::CUSTOMER_1) && i <= static_cast<unsigned>(EventType::CUSTOMER_10);
}
