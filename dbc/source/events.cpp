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
* This header implements the \ref Events class.
***********************************************************************************************************************/

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

#include <cstdint>

#include "log.h"
#include "event.h"
#include "latency_entry.h"
#include "database_manager.h"
#include "events.h"

/***********************************************************************************************************************
* Events::Checker
*/

Events::Checker::Checker() {}


Events::Checker::~Checker() {}


QString Events::Checker::queryString(
        Events::EventType     /* eventType */,
        Events::MonitorStatus /* monitorStatus */,
        Events::MonitorId     monitorId,
        const QByteArray&     hash
    ) const {
    return QString(
        "SELECT %1 FROM event "
            "WHERE "
                "timestamp = (SELECT MAX(timestamp) FROM event WHERE %2) "
            "AND "
                "%2"
    ).arg(queryColumns(), queryCondition(monitorId, hash));
}


Events::EventDisposition Events::Checker::checkQuery(
        QSqlQuery&            query,
        Events::EventType     eventType,
        Events::MonitorStatus monitorStatus,
        Events::MonitorId     /* monitorId */,
        const QByteArray&     /* hash */
    ) const {
    EventDisposition result = EventDisposition::IGNORE;

    if (query.first()) {
        int eventTypeField = query.record().indexOf("event_type");
        if (eventTypeField >= 0) {
            QString eventTypeString = query.value(eventTypeField).toString().toUpper();
            if (eventTypeString != Event::toString(eventType)) {
                result = EventDisposition::RECORD_AND_REPORT;
            }
        }
    } else {
        result = defaultDisposition(eventType, monitorStatus);
    }

    return result;
}


QString Events::Checker::queryCondition(MonitorId /* monitorId */, const QByteArray& /* hash */) const {
    Q_ASSERT(false);
    return QString();
}


QString Events::Checker::queryColumns() const {
    return QString("event_type AS event_type");
}


Events::EventDisposition Events::Checker::defaultDisposition(
        Events::EventType     /* eventType */,
        Events::MonitorStatus /* monitorStatus */
    ) const {
    return EventDisposition::IGNORE;
}

/***********************************************************************************************************************
* Events::PerHostSchemeChecker
*/

Events::PerHostSchemeChecker::PerHostSchemeChecker() {}


Events::PerHostSchemeChecker::~PerHostSchemeChecker() {}


QString Events::PerHostSchemeChecker::queryCondition(MonitorId monitorId, const QByteArray& /* hash */) const {
    QString queryCondition = QString(
            "monitor_id IN ("
                "SELECT monitor_id FROM monitor WHERE host_scheme_id = ("
                    "SELECT host_scheme_id FROM monitor WHERE monitor_id = %1"
                ")"
            ")"
        "AND "
            "event_type IN (%2)"
    ).arg(monitorId).arg(eventTypes());

    return queryCondition;
}

/***********************************************************************************************************************
* Events::PerMonitorChecker
*/

Events::PerMonitorChecker::PerMonitorChecker() {}


Events::PerMonitorChecker::~PerMonitorChecker() {}


QString Events::PerMonitorChecker::queryCondition(MonitorId monitorId, const QByteArray& /* hash */) const {
    return QString("monitor_id = %1 AND event_type IN (%2)").arg(monitorId).arg(eventTypes());
}

/***********************************************************************************************************************
* Events::HashedEventChecker
*/

Events::HashedEventChecker::HashedEventChecker() {}


Events::HashedEventChecker::~HashedEventChecker() {}


Events::EventDisposition Events::HashedEventChecker::checkQuery(
        QSqlQuery&            query,
        Events::EventType     eventType,
        Events::MonitorStatus monitorStatus,
        Events::MonitorId     /* monitorId */,
        const QByteArray&     hash
    ) const {
    EventDisposition result = EventDisposition::IGNORE;

    if (query.first()) {
        int eventTypeField = query.record().indexOf("event_type");
        int hashField      = query.record().indexOf("hash");
        if (eventTypeField >= 0 && hashField >= 0) {
            QString eventTypeString = query.value(eventTypeField).toString().toUpper();
            if (eventTypeString == Event::toString(eventType)) {
                QByteArray previousHash = query.value(hashField).toByteArray();
                if (previousHash != hash) {
                    result = EventDisposition::RECORD_AND_REPORT;
                }
            }
        }
    } else {
        result = defaultDisposition(eventType, monitorStatus);
    }

    return result;
}


QString Events::HashedEventChecker::queryColumns() const {
    return QString("event_type AS event_type, hash AS hash");
}


Events::EventDisposition Events::HashedEventChecker::defaultDisposition(
        EventType     /* eventType */,
        MonitorStatus /* monitorStatus */
    ) const {
    return EventDisposition::RECORD_AND_REPORT;
}

/***********************************************************************************************************************
* Events::SslChecker
*/

Events::SslChecker::SslChecker() {}


Events::SslChecker::~SslChecker() {}


QString Events::SslChecker::eventTypes() const {
    return QString("'SSL_CERTIFICATE_EXPIRING', 'SSL_CERTIFICATE_RENEWED'");
}

/***********************************************************************************************************************
* Events::WorkingChecker
*/

Events::WorkingChecker::WorkingChecker() {}


Events::WorkingChecker::~WorkingChecker() {}


QString Events::WorkingChecker::eventTypes() const {
    return QString("'WORKING', 'NO_RESPONSE'");
}


Events::EventDisposition Events::WorkingChecker::defaultDisposition(
        EventType     /* eventType */,
        MonitorStatus monitorStatus
    ) const {
    return monitorStatus == MonitorStatus::UNKNOWN ? EventDisposition::RECORD_ONLY : EventDisposition::IGNORE;
}

/***********************************************************************************************************************
* Events::NoResponseChecker
*/

Events::NoResponseChecker::NoResponseChecker() {}


Events::NoResponseChecker::~NoResponseChecker() {}


QString Events::NoResponseChecker::eventTypes() const {
    return QString("'WORKING', 'NO_RESPONSE'");
}


Events::EventDisposition Events::NoResponseChecker::defaultDisposition(
        EventType     /* eventType */,
        MonitorStatus /* monitorStatus */
    ) const {
    return EventDisposition::RECORD_AND_REPORT;
}

/***********************************************************************************************************************
* Events::ContentChangedChecker
*/

Events::ContentChangedChecker::ContentChangedChecker() {}


Events::ContentChangedChecker::~ContentChangedChecker() {}


QString Events::ContentChangedChecker::eventTypes() const {
    return QString("'CONTENT_CHANGED'");
}

/***********************************************************************************************************************
* Events::KeywordsChecker
*/

Events::KeywordsChecker::KeywordsChecker() {}


Events::KeywordsChecker::~KeywordsChecker() {}


QString Events::KeywordsChecker::eventTypes() const {
    return QString("'KEYWORDS'");
}

/***********************************************************************************************************************
* Events::SslCertificateExpiringChecker
*/

Events::SslCertificateExpiringChecker::SslCertificateExpiringChecker() {}


Events::SslCertificateExpiringChecker::~SslCertificateExpiringChecker() {}

/***********************************************************************************************************************
* Events::SslCertificateRenewedChecker
*/

Events::SslCertificateRenewedChecker::SslCertificateRenewedChecker() {}


Events::SslCertificateRenewedChecker::~SslCertificateRenewedChecker() {}

/***********************************************************************************************************************
* Events::CustomerEventChecker
*/

Events::CustomerEventChecker::CustomerEventChecker() {}


Events::CustomerEventChecker::~CustomerEventChecker() {}


QString Events::CustomerEventChecker::queryString(
        Events::EventType     /* eventType */,
        Events::MonitorStatus /* monitorStatus */,
        Events::MonitorId     /* monitorId */,
        const QByteArray&     /* hash */
    ) const {
    return QString();
}


QString Events::CustomerEventChecker::queryCondition(
        MonitorId         /* monitorId */,
        const QByteArray& /* hash */
    ) const {
    Q_ASSERT(false);
    return QString();
}

/***********************************************************************************************************************
* Events
*/

Events::Events(DatabaseManager* databaseManager, QObject* parent):QObject(parent) {
    currentDatabaseManager = databaseManager;

    eventCheckers.insert(EventType::WORKING,                  new WorkingChecker);
    eventCheckers.insert(EventType::NO_RESPONSE,              new NoResponseChecker);
    eventCheckers.insert(EventType::CONTENT_CHANGED,          new ContentChangedChecker);
    eventCheckers.insert(EventType::KEYWORDS,                 new KeywordsChecker);
    eventCheckers.insert(EventType::SSL_CERTIFICATE_EXPIRING, new SslCertificateExpiringChecker);
    eventCheckers.insert(EventType::SSL_CERTIFICATE_RENEWED,  new SslCertificateRenewedChecker);
    eventCheckers.insert(EventType::CUSTOMER_1,               new CustomerEventChecker);
    eventCheckers.insert(EventType::CUSTOMER_2,               new CustomerEventChecker);
    eventCheckers.insert(EventType::CUSTOMER_3,               new CustomerEventChecker);
    eventCheckers.insert(EventType::CUSTOMER_4,               new CustomerEventChecker);
    eventCheckers.insert(EventType::CUSTOMER_5,               new CustomerEventChecker);
    eventCheckers.insert(EventType::CUSTOMER_6,               new CustomerEventChecker);
    eventCheckers.insert(EventType::CUSTOMER_7,               new CustomerEventChecker);
    eventCheckers.insert(EventType::CUSTOMER_8,               new CustomerEventChecker);
    eventCheckers.insert(EventType::CUSTOMER_9,               new CustomerEventChecker);
    eventCheckers.insert(EventType::CUSTOMER_10,              new CustomerEventChecker);
    eventCheckers.insert(EventType::TRANSACTION,              new CustomerEventChecker);
    eventCheckers.insert(EventType::INQUIRY,                  new CustomerEventChecker);
    eventCheckers.insert(EventType::SUPPORT_REQUEST,          new CustomerEventChecker);
    eventCheckers.insert(EventType::STORAGE_LIMIT_REACHED,    new CustomerEventChecker);
}


Events::~Events() {
    for (  QHash<EventType, Checker*>::const_iterator it  = eventCheckers.constBegin(),
                                                      end = eventCheckers.constEnd()
         ; it != end
         ; ++it
        ) {
        delete it.value();
    }
}


Event Events::recordEvent(
        Events::CustomerId customerId,
        Events::MonitorId  monitorId,
        unsigned long long unixTimestamp,
        Events::EventType  eventType,
        const QString&     message,
        const QByteArray&  hash,
        unsigned           threadId
    ) {
    Event result;

    MonitorStatus currentStatus = monitorStatus(monitorId, threadId);

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        if (database.driver()->hasFeature(QSqlDriver::DriverFeature::Transactions)) {
            database.transaction();
        }

        ZoranTimeStamp zoranTimestamp = toZoranTimestamp(unixTimestamp);
        QString        queryString    = QString(
            "INSERT INTO event (monitor_id, customer_id, timestamp, event_type, message, hash) "
            "VALUES (:monitor_id, :customer_id, :timestamp, :event_type, :message, :hash)"
        );

        QSqlQuery query(database);
        query.setForwardOnly(true);

        success = query.prepare(queryString);
        if (success) {
            query.bindValue(":monitor_id", monitorId);
            query.bindValue(":customer_id", customerId);
            query.bindValue(":timestamp", zoranTimestamp);
            query.bindValue(":event_type", Event::toString(eventType));
            query.bindValue(":message", message);
            query.bindValue(":hash", hash);

            success = query.exec();
            if (success) {
                QVariant eventIdVariant = query.lastInsertId();
                if (eventIdVariant.isValid()) {
                    EventId eventId = eventIdVariant.toUInt(&success);
                    if (success && eventId > 0) {
                        MonitorStatus newStatus;
                        QString       newStatusString;
                        if (eventType == EventType::CONTENT_CHANGED          ||
                            eventType == EventType::WORKING                  ||
                            eventType == EventType::KEYWORDS                 ||
                            eventType == EventType::SSL_CERTIFICATE_EXPIRING ||
                            eventType == EventType::SSL_CERTIFICATE_RENEWED  ||
                            Event::isCustomerEvent(eventType)                   ) {
                            newStatus       = MonitorStatus::WORKING;
                            newStatusString = QString("WORKING");
                        } else {
                            newStatus       = MonitorStatus::FAILED;
                            newStatusString = QString("FAILED");
                        }

                        if (newStatus != currentStatus) {
                            if (currentStatus == MonitorStatus::UNKNOWN) {
                                queryString = QString("INSERT INTO monitor_status (monitor_id, status) VALUES (%1, '%2')")
                                              .arg(monitorId)
                                              .arg(newStatusString);
                            } else {
                                queryString = QString("UPDATE monitor_status SET status = '%1' WHERE monitor_id = %2")
                                              .arg(newStatusString)
                                              .arg(monitorId);
                            }

                            success = query.exec(queryString);
                            if (success) {
                                result = Event(
                                    eventId,
                                    monitorId,
                                    customerId,
                                    zoranTimestamp,
                                    eventType,
                                    message,
                                    hash
                                );
                            } else {
                                if (currentStatus == MonitorStatus::UNKNOWN) {
                                    logWrite(
                                        QString("Failed INSERT into monitor_status - Events::recordEvent: %1")
                                        .arg(query.lastError().text()),
                                        true
                                    );
                                } else {
                                    logWrite(
                                        QString("Failed UPDATE into monitor_status - Events::recordEvent: %1")
                                        .arg(query.lastError().text()),
                                        true
                                    );
                                }
                            }
                        } else {
                            result = Event(
                                eventId,
                                monitorId,
                                customerId,
                                zoranTimestamp,
                                eventType,
                                message,
                                hash
                            );
                        }
                    } else {
                        logWrite(QString("Invalid event ID - Events::recordEvent"), true);
                    }
                } else {
                    logWrite(
                        QString("Failed to get field index - Events::recordEvent: %1")
                        .arg(query.lastError().text()),
                        true
                    );
                }
            } else {
                logWrite(
                    QString("Failed INSERT into event (exec) - Events::recordEvent: %1")
                    .arg(query.lastError().text()),
                    true
                );
            }
        } else {
            logWrite(
                QString("Failed INSERT into event (prepare) - Events::recordEvent: %1")
                .arg(query.lastError().text()),
                true
            );
        }

        if (success) {
            success = database.commit();
            if (!success) {
                logWrite(
                    QString("Failed commit - Events::recordEvent: %1")
                    .arg(database.lastError().text()),
                    true
                );
            }
        } else {
            bool rollbackSuccess = database.rollback();
            if (!rollbackSuccess) {
                logWrite(
                    QString("Failed rollback - Events::recordEvent: %1")
                    .arg(database.lastError().text()),
                    true
                );
            }
        }
    } else {
        logWrite(
            QString("Failed to open database - Events::recordEvent: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


Events::MonitorStatus Events::monitorStatus(Events::MonitorId monitorId, unsigned threadId) {
    MonitorStatus result = MonitorStatus::UNKNOWN;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        success = query.exec(QString("SELECT * FROM monitor_status WHERE monitor_id = %1").arg(monitorId));
        if (success) {
            if (query.first()) {
                int statusField = query.record().indexOf("status");
                if (statusField >= 0) {
                    QString statusString = query.value(statusField).toString().toLower();
                    if (statusString == QString("working")) {
                        result = MonitorStatus::WORKING;
                    } else if (statusString == QString("failed")) {
                        result = MonitorStatus::FAILED;
                    } else if (statusString == QString("unknown")) {
                        result = MonitorStatus::UNKNOWN;
                    } else {
                        logWrite(
                            QString("Failed invalid status value - Events::monitorStatus: \"%1\"")
                            .arg(statusString),
                            true
                        );
                    }
                } else {
                    logWrite(QString("Failed invalid status field index - Events::monitorStatus"), true);
                }
            } else {
                // We're OK but there's no record.  Return UNKNOWN status.
            }
        } else {
            logWrite(
                QString("Failed SELECT - Events::monitorStatus: %1").arg(query.lastError().text()),
                true
            );
        }
    } else {
        logWrite(
            QString("Failed to open database - Events::recordEvent: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


Events::MonitorStatusByMonitorId Events::monitorStatusByCustomerId(Events::CustomerId customerId, unsigned threadId) {
    MonitorStatusByMonitorId result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        QString queryString;
        if (customerId == Events::invalidCustomerId) {
            queryString = QString("SELECT * FROM monitor_status");
        } else{
            queryString = QString(
                "SELECT * FROM monitor_status "
                "WHERE monitor_id IN (SELECT monitor_id FROM monitor WHERE customer_id = %1)"
            ).arg(customerId);
        }

        success = query.exec(queryString);
        if (success) {
            int monitorIdField = query.record().indexOf("monitor_id");
            int statusField    = query.record().indexOf("status");
            if (monitorIdField >= 0 && statusField >= 0) {
                while (success && query.next()) {
                    Events::MonitorId monitorId = query.value(monitorIdField).toUInt(&success);
                    if (success && monitorId != 0) {
                        QString statusString = query.value(statusField).toString().toLower();
                        if (statusString == QString("working")) {
                            result.insert(monitorId, MonitorStatus::WORKING);
                        } else if (statusString == QString("failed")) {
                            result.insert(monitorId, MonitorStatus::FAILED);
                        } else if (statusString == QString("unknown")) {
                            result.insert(monitorId, MonitorStatus::UNKNOWN);
                        } else {
                            logWrite(
                                QString(
                                    "Failed invalid status value - Events::monitorStatusByCustomerId: "
                                    "status = \"%1\""
                                ).arg(statusString),
                                true
                            );
                        }
                    } else {
                        logWrite(
                            QString("Failed invalid monitor ID value - Events::monitorStatusByCustomerId"),
                            true
                        );
                        success = false;
                    }
                }
            } else {
                logWrite(
                    QString("Failed invalid field index - Events::monitorStatusByCustomerId"),
                    true
                );
                success = false;
            }
        } else {
            logWrite(QString("Failed SELECT - Events::monitorStatus: %1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(
            QString("Failed to open database - Events::recordEvent: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


Event Events::getEvent(Events::EventId eventId, unsigned threadId) {
    Event result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        QString queryString = QString("SELECT * FROM event WHERE event_id = %1").arg(eventId);
        success = query.exec(queryString);
        if (success) {
            if (query.first()) {
                int eventIdField    = query.record().indexOf("event_id");
                int monitorIdField  = query.record().indexOf("monitor_id");
                int customerIdField = query.record().indexOf("customer_id");
                int timestampField  = query.record().indexOf("timestamp");
                int eventTypeField  = query.record().indexOf("event_type");
                int messageField    = query.record().indexOf("message");
                int hashField       = query.record().indexOf("hash");

                if (eventIdField >= 0    &&
                    monitorIdField >= 0  &&
                    customerIdField >= 0 &&
                    timestampField >= 0  &&
                    eventTypeField >= 0  &&
                    messageField >= 0    &&
                    hashField >= 0          ) {
                    bool success = true;
                    QString message = query.value(messageField).toString();

                    EventId eventId = query.value(eventIdField).toUInt(&success);
                    if (success && eventId > 0) {
                        MonitorId monitorId = query.value(monitorIdField).toUInt(&success);
                        if (success && monitorId > 0) {
                            CustomerId customerId = query.value(customerIdField).toUInt(&success);
                            if (success && customerId > 0) {
                                ZoranTimeStamp zoranTimestamp = query.value(timestampField).toUInt(&success);
                                if (success) {
                                    EventType eventType = Event::toEventType(
                                        query.value(eventTypeField).toString(),
                                        &success
                                    );
                                    if (success) {
                                        QByteArray hash = query.value(hashField).toByteArray();

                                        result = Event(
                                            eventId,
                                            monitorId,
                                            customerId,
                                            zoranTimestamp,
                                            eventType,
                                            message,
                                            hash
                                        );
                                    } else {
                                        logWrite(QString("Failed invalid event type - Events::getEvents"), true);
                                    }
                                } else {
                                    logWrite(QString("Failed invalid timestamp - Events::getEvents"), true);
                                }
                            } else {
                                logWrite(QString("Failed invalid customer_id - Events::getEvents"), true);
                                success = false;
                            }
                        } else {
                            logWrite(QString("Failed invalid monitor_id - Events::getEvents"), true);
                            success = false;
                        }
                    } else {
                        logWrite(QString("Failed invalid event_id - Events::getEvents"), true);
                        success = false;
                    }
                }
            }
        }
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


Events::EventList Events::getEventsByCustomer(
        Events::CustomerId customerId,
        unsigned long long startTimestamp,
        unsigned long long endingTimestamp,
        unsigned           threadId
    ) {
    EventList result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        ZoranTimeStamp startZoranTimestamp = toZoranTimestamp(startTimestamp);
        ZoranTimeStamp endZoranTimestamp   = toZoranTimestamp(endingTimestamp);
        QString        queryString         = QString("SELECT * FROM event");
        bool           hasCondition        = false;

        if (customerId != invalidCustomerId) {
            queryString += QString(" WHERE customer_id = %1").arg(customerId);
            hasCondition = true;
        }

        if (startZoranTimestamp != 0) {
            if (hasCondition) {
                queryString += QString(" AND timestamp >= %1").arg(startZoranTimestamp);
            } else {
                queryString += QString(" WHERE timestamp >= %1").arg(startZoranTimestamp);
                hasCondition = true;
            }
        }

        if (endZoranTimestamp != std::numeric_limits<ZoranTimeStamp>::max()) {
            if (hasCondition) {
                queryString += QString(" AND timestamp <= %1").arg(endZoranTimestamp);
            } else {
                queryString += QString(" WHERE timestamp <= %1").arg(endZoranTimestamp);
                // hasCondition = true;
            }
        }

        queryString += QString(" ORDER BY timestamp ASC");
        success = query.exec(queryString);
        if (success) {
            result = parseLongQuery(query);
        } else {
            logWrite(QString("Failed SELECT - Events::getEvents: %1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(
            QString("Failed to open database - Events::recordEvent: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


Events::EventList Events::getEventsByMonitor(
        Events::MonitorId  monitorId,
        unsigned long long startTimestamp,
        unsigned long long endingTimestamp,
        unsigned           threadId
    ) {
    EventList result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        ZoranTimeStamp startZoranTimestamp = toZoranTimestamp(startTimestamp);
        ZoranTimeStamp endZoranTimestamp   = toZoranTimestamp(endingTimestamp);
        QString        queryString         = QString("SELECT * FROM event WHERE monitor_id = %1").arg(monitorId);

        if (startZoranTimestamp != 0) {
            queryString += QString(" AND timestamp >= %1").arg(startZoranTimestamp);
        }

        if (endZoranTimestamp != std::numeric_limits<ZoranTimeStamp>::max()) {
            queryString += QString(" AND timestamp <= %1").arg(endZoranTimestamp);
        }

        queryString += QString(" ORDER BY timestamp ASC");
        success = query.exec(queryString);
        if (success) {
            result = parseLongQuery(query);
        } else {
            logWrite(QString("Failed SELECT - Events::getEventsByMonitor: %1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(
            QString("Failed to open database - Events::recordEvent: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


Events::EventDisposition Events::eventDisposition(
        Events::EventType     eventType,
        Events::MonitorStatus monitorStatus,
        Events::MonitorId     monitorId,
        const QByteArray&     hash,
        unsigned              threadId
    ) {
    EventDisposition result = EventDisposition::FAILED;

    Checker* checker = eventCheckers.value(eventType, nullptr);
    if (checker != nullptr) {
        QString queryString = checker->queryString(eventType, monitorStatus, monitorId, hash);
        if (!queryString.isEmpty()) {
            QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
            bool success = database.isOpen();
            if (success) {
                QSqlQuery query(database);
                query.setForwardOnly(true);

                success = query.exec(queryString);
                if (success) {
                    result = checker->checkQuery(query, eventType, monitorStatus, monitorId, hash);
                } else {
                    logWrite(QString("Failed exec when checking for a repeat event: %1").arg(queryString), true);
                }
            } else {
                logWrite(
                    QString("Failed to open database - Events::isRepeatEvent: %1")
                    .arg(database.lastError().text()),
                    true
                );
            }

            currentDatabaseManager->closeAndRelease(database);
        } else {
            result = EventDisposition::RECORD_AND_REPORT;
        }
    } else {
        logWrite(QString("No event checker for event %1").arg(static_cast<unsigned>(eventType)), true);
    }

    return result;
}


void Events::purgeEvents(CustomerId customerId, unsigned long long timestamp, unsigned threadId) {
    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        ZoranTimeStamp zoranTimestamp = toZoranTimestamp(timestamp);
        QString queryString = QString("DELETE FROM regions WHERE timestamp <= %1").arg(zoranTimestamp);
        if (customerId != invalidCustomerId) {
            queryString += QString(" AND customer_id = %1").arg(customerId);
        }

        success = query.exec(queryString);
        if (!success) {
            logWrite(QString("Failed DEKETE - Events::purgeEvents: %1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(
            QString("Failed to open database - Events::recordEvent: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
}


Events::EventList Events::parseLongQuery(QSqlQuery& sqlQuery) {
    EventList result;

    if (sqlQuery.size() > 0) {
        int eventIdField    = sqlQuery.record().indexOf("event_id");
        int monitorIdField  = sqlQuery.record().indexOf("monitor_id");
        int customerIdField = sqlQuery.record().indexOf("customer_id");
        int timestampField  = sqlQuery.record().indexOf("timestamp");
        int eventTypeField  = sqlQuery.record().indexOf("event_type");
        int messageField    = sqlQuery.record().indexOf("message");
        int hashField       = sqlQuery.record().indexOf("hash");

        if (eventIdField >= 0    &&
            monitorIdField >= 0  &&
            customerIdField >= 0 &&
            timestampField >= 0  &&
            eventTypeField >= 0  &&
            messageField >= 0    &&
            hashField >= 0          ) {
            bool success = true;
            while (success && sqlQuery.next()) {
                QString message = sqlQuery.value(messageField).toString();

                EventId eventId = sqlQuery.value(eventIdField).toUInt(&success);
                if (success && eventId > 0) {
                    MonitorId monitorId = sqlQuery.value(monitorIdField).toUInt(&success);
                    if (success && monitorId > 0) {
                        CustomerId customerId = sqlQuery.value(customerIdField).toUInt(&success);
                        if (success && customerId > 0) {
                            ZoranTimeStamp zoranTimestamp = sqlQuery.value(timestampField).toUInt(&success);
                            if (success) {
                                EventType eventType = Event::toEventType(
                                    sqlQuery.value(eventTypeField).toString(),
                                    &success
                                );
                                if (success) {
                                    QByteArray hash = sqlQuery.value(hashField).toByteArray();

                                    Event event = Event(
                                        eventId,
                                        monitorId,
                                        customerId,
                                        zoranTimestamp,
                                        eventType,
                                        message,
                                        hash
                                    );
                                    result.append(event);
                                } else {
                                    logWrite(QString("Failed invalid event type - Events::getEvents"), true);
                                }
                            } else {
                                logWrite(QString("ailed invalid timestamp - Events::getEvents"), true);                            }
                        } else {
                            logWrite(QString("Failed invalid customer_id - Events::getEvents"), true);
                            success = false;
                        }
                    } else {
                        logWrite(QString("Failed invalid monitor_id - Events::getEvents"), true);
                        success = false;
                    }
                } else {
                    logWrite(QString("Failed invalid event_id - Events::getEvents"), true);
                    success = false;
                }
            }

            if (!success) {
                result = EventList();
            }
        } else {
            logWrite(QString("Failed invalid status field index - Events::getEvents"), true);
        }
    } else {
        // No issue, just no data to report.
    }

    return result;
}


Events::ZoranTimeStamp Events::toZoranTimestamp(unsigned long long unixTimestamp) {
    unsigned long long result =   unixTimestamp < LatencyEntry::startOfZoranEpoch
                                ? 0
                                : unixTimestamp - LatencyEntry::startOfZoranEpoch;

    if (result > std::numeric_limits<ZoranTimeStamp>::max()) {
        result = std::numeric_limits<ZoranTimeStamp>::max();
    }

    return static_cast<ZoranTimeStamp>(result);
}
