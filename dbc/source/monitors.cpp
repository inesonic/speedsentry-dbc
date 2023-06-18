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
* This header implements the \ref Monitors class.
***********************************************************************************************************************/

#include <QObject>
#include <QString>
#include <QHash>
#include <QList>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

#include <cstdint>

#include "log.h"
#include "database_manager.h"
#include "host_scheme.h"
#include "scheme_host_path.h"
#include "monitor.h"
#include "monitors.h"

Monitors::Monitors(
        DatabaseManager* databaseManager,
        QObject*         parent
    ):QObject(
        parent
    ),currentDatabaseManager(
        databaseManager
    ) {}


Monitors::~Monitors() {}


Monitor Monitors::getMonitor(Monitors::MonitorId monitorId, unsigned threadId) const {
    Monitor result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        QString queryString = QString("SELECT * FROM monitor WHERE monitor_id = %1").arg(monitorId);

        success = query.exec(queryString);
        if (success) {
            if (query.first()) {
                result = convertQueryToMonitor(query, &success);
            } else {
                // We're OK but there was no record for the requested region.
            }
        } else {
            logWrite(QString("Failed SELECT - Monitors::getMonitor: %1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(QString("Failed to open database - Monitors::getMonitor: %1").arg(database.lastError().text()), true);
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


Monitors::MonitorList Monitors::getMonitorsByUserOrder(Monitors::CustomerId customerId, unsigned threadId) const {
    MonitorList result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        QString queryString;
        if (customerId == Monitors::invalidCustomerId) {
            queryString = QString("SELECT * FROM monitor ORDER BY monitor_id ASC");
        } else {
            queryString = QString("SELECT * FROM monitor WHERE customer_id = %1 ORDER BY user_ordering ASC")
                          .arg(customerId);
        }

        success = query.exec(queryString);
        if (success) {
            while (success && query.next()) {
                Monitor monitor = convertQueryToMonitor(query, &success);
                if (success) {
                    result.append(monitor);
                }
            }

            if (!success) {
                result.clear();
            }
        } else {
            logWrite(
                QString("Failed SELECT - Monitors::getMonitorsByUserOrder: %1").arg(query.lastError().text()),
                true
            );
        }
    } else {
        logWrite(
            QString("Failed to open database - Monitors::getMonitorsByUserOrder: %1").arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


Monitors::MonitorsById Monitors::getMonitorsByCustomerId(Monitors::CustomerId customerId, unsigned threadId) const {
    MonitorsById result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        QString queryString;
        if (customerId == Monitors::invalidCustomerId) {
            queryString = QString("SELECT * FROM monitor ORDER BY monitor_id ASC");
        } else {
            queryString = QString("SELECT * FROM monitor WHERE customer_id = %1 ORDER BY user_ordering ASC")
                          .arg(customerId);
        }

        success = query.exec(queryString);
        if (success) {
            while (success && query.next()) {
                Monitor monitor = convertQueryToMonitor(query, &success);
                if (success) {
                    result.insert(monitor.monitorId(), monitor);
                }
            }

            if (!success) {
                result.clear();
            }
        } else {
            logWrite(
                QString("Failed SELECT - Monitors::getMonitorsByCustomerId: %1").arg(query.lastError().text()),
                true
            );
        }
    } else {
        logWrite(
            QString("Failed to open database - Monitors::getMonitorsByCustomerId: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


Monitors::MonitorsBySchemeHostPath Monitors::getMonitorsBySchemeHostPath(
        Monitors::CustomerId customerId,
        unsigned             threadId
    ) const {
    MonitorsBySchemeHostPath result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        QString queryString = QString("SELECT * FROM monitor WHERE customer_id = %1").arg(customerId);

        success = query.exec(queryString);
        if (success) {
            while (success && query.next()) {
                Monitor monitor = convertQueryToMonitor(query, &success);
                if (success) {
                    SchemeHostPath shp(monitor.hostSchemeId(), monitor.path());
                    result.insert(shp, monitor);
                }
            }

            if (!success) {
                result.clear();
            }
        } else {
            logWrite(
                QString("Failed SELECT - Monitors::getMonitorsBySchemeHostPath: %1").arg(query.lastError().text()),
                true
            );
        }
    } else {
        logWrite(
            QString("Failed to open database - Monitors::getMonitorsBySchemeHostPath: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


Monitors::MonitorList Monitors::getMonitorsUnderHostScheme(
        Monitors::HostSchemeId hostSchemeId,
        unsigned               threadId
    ) const {
    MonitorList result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        QString queryString = QString("SELECT * FROM monitor WHERE host_scheme_id = %1 ORDER BY user_ordering ASC")
                              .arg(hostSchemeId);

        success = query.exec(queryString);
        if (success) {
            while (success && query.next()) {
                Monitor monitor = convertQueryToMonitor(query, &success);
                if (success) {
                    result.append(monitor);
                }
            }

            if (!success) {
                result.clear();
            }
        } else {
            logWrite(
                QString("Failed SELECT - Monitors::getMonitorsUnderSchemeHostPath: %1").arg(query.lastError().text()),
                true
            );
        }
    } else {
        logWrite(
            QString("Failed to open database - Monitors::getMonitorsUnderSchemeHostPath: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


Monitors::MonitorsById Monitors::getMonitorsById(unsigned threadId) const {
    MonitorsById result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);
        query.setForwardOnly(true);

        QString queryString = QString("SELECT * FROM monitor");
        success = query.exec(queryString);
        if (success) {
            while (success && query.next()) {
                Monitor monitor = convertQueryToMonitor(query, &success);
                if (success) {
                    result.insert(monitor.monitorId(), monitor);
                }
            }

            if (!success) {
                result.clear();
            }
        } else {
            logWrite(QString("Failed SELECT - Monitors::getMonitorsById: %1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(
            QString("Failed to open database - Monitors::getMonitorsById: %1").arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


Monitor Monitors::createMonitor(
        Monitors::CustomerId       customerId,
        Monitors::HostSchemeId     hostSchemeId,
        Monitors::UserOrdering     userOrdering,
        const QString&             path,
        Monitors::Method           method,
        Monitors::ContentCheckMode contentCheckMode,
        const KeywordList&         keywords,
        Monitors::ContentType      contentType,
        const QString&             userAgent,
        const QByteArray&          postContent,
        unsigned                   threadId
    ) {
    Monitor result;

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        QString queryString  = QString(
            "INSERT INTO monitor("
                "customer_id,"
                "host_scheme_id,"
                "user_ordering,"
                "path,"
                "method,"
                "content_check_mode,"
                "keywords,"
                "post_content_type,"
                "post_user_agent,"
                "post_content"
            ") VALUES ("
                ":customer_id,"
                ":host_scheme_id,"
                ":user_ordering,"
                ":path,"
                ":method,"
                ":content_check_mode,"
                ":keywords,"
                ":post_content_type,"
                ":post_user_agent,"
                ":post_content"
            ")"
        );

        query.prepare(queryString);
        query.bindValue(":customer_id", customerId);
        query.bindValue(":host_scheme_id", hostSchemeId);
        query.bindValue(":user_ordering", userOrdering);
        query.bindValue(":path", path);
        query.bindValue(":method", Monitor::toString(method));
        query.bindValue(":content_check_mode", Monitor::toString(contentCheckMode));
        query.bindValue(":keywords", qCompress(Monitor::toByteArray(keywords)));
        query.bindValue(":post_content_type", Monitor::toString(contentType));
        query.bindValue(":post_user_agent", userAgent);
        query.bindValue(":post_content", qCompress(postContent));

        success = query.exec();
        if (success) {
            QVariant monitorIdVariant = query.lastInsertId();
            if (monitorIdVariant.isValid()) {
                unsigned unsignedMonitorId = monitorIdVariant.toUInt(&success);
                if (success) {
                    MonitorId monitorId = static_cast<MonitorId>(unsignedMonitorId);

                    result = Monitor(
                        monitorId,
                        customerId,
                        hostSchemeId,
                        userOrdering,
                        path,
                        method,
                        contentCheckMode,
                        keywords,
                        contentType,
                        userAgent,
                        postContent
                    );
                } else {
                    logWrite(QString("Invalid monitor ID, not integer- Monitors::createMonitor: "), true);
                }
            } else {
                logWrite(
                    QString("Failed to get field index - Monitors::createMonitor: %1").arg(query.lastError().text()),
                    true
                );
            }
        } else {
            logWrite(QString("Failed INSERT- Monitors::createMonitor: %1").arg(query.lastError().text()), true);
        }
    } else {
        logWrite(
            QString("Failed to open database - Monitors::createMonitor: %1").arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return result;
}


bool Monitors::modifyMonitor(const Monitor& monitor, unsigned threadId) {
    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        QString queryString  = QString(
            "UPDATE monitor SET "
                "customer_id = :customer_id, "
                "host_scheme_id = :host_scheme_id, "
                "user_ordering = :user_ordering, "
                "path = :path, "
                "method = :method, "
                "content_check_mode = :content_check_mode, "
                "keywords = :keywords, "
                "post_content_type = :post_content_type, "
                "post_user_agent = :post_user_agent, "
                "post_content = :post_content "
            "WHERE monitor_id = :monitor_id"
        );

        query.prepare(queryString);
        query.bindValue(":monitor_id", monitor.monitorId());
        query.bindValue(":customer_id", monitor.customerId());
        query.bindValue(":host_scheme_id", monitor.hostSchemeId());
        query.bindValue(":user_ordering", monitor.userOrdering());
        query.bindValue(":path", monitor.path());
        query.bindValue(":method", Monitor::toString(monitor.method()));
        query.bindValue(":content_check_mode", Monitor::toString(monitor.contentCheckMode()));
        query.bindValue(":keywords", qCompress(Monitor::toByteArray(monitor.keywords())));
        query.bindValue(":post_content_type", Monitor::toString(monitor.contentType()));
        query.bindValue(":post_user_agent", monitor.userAgent());
        query.bindValue(":post_content", qCompress(monitor.postContent()));

        success = query.exec();
        if (!success) {
            logWrite(
                QString("Failed UPDATE - Monitors::modifyMonitor: %1").arg(query.lastError().text()),
                true
            );
        }
    } else {
        logWrite(
            QString("Failed to open database - Monitors::modifyMonitor: %1").arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return success;
}


bool Monitors::deleteMonitor(const Monitor& monitor, unsigned threadId) {
    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        QString queryString = QString("DELETE FROM monitor WHERE monitor_id = %1").arg(monitor.monitorId());
        success = query.exec(queryString);
        if (!success) {
            logWrite(
                QString("Failed DELETE - Monitors::deleteMonitor: %1").arg(query.lastError().text()),
                true
            );
        }
    } else {
        logWrite(
            QString("Failed to open database - Monitors::deleteMonitor: %1").arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return success;
}


bool Monitors::deleteMonitors(Monitors::CustomerId customerId, unsigned threadId) {
    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        QString queryString = QString("DELETE FROM monitor WHERE customer_id = %1").arg(customerId);
        success = query.exec(queryString);
        if (!success) {
            logWrite(
                QString("Failed DELETE - Monitors::deleteMonitors: %1").arg(query.lastError().text()),
                true
            );
        }
    } else {
        logWrite(
            QString("Failed to open database - Monitors::deleteMonitors: %1").arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);
    return success;
}


Monitor Monitors::convertQueryToMonitor(const QSqlQuery& sqlQuery, bool* success) {
    Monitor result;
    bool    ok = true;

    int monitorIdField        = sqlQuery.record().indexOf("monitor_id");
    int customerIdField       = sqlQuery.record().indexOf("customer_id");
    int hostSchemeIdField     = sqlQuery.record().indexOf("host_scheme_id");
    int userOrderingField     = sqlQuery.record().indexOf("user_ordering");
    int pathField             = sqlQuery.record().indexOf("path");
    int methodField           = sqlQuery.record().indexOf("method");
    int contentCheckModeField = sqlQuery.record().indexOf("content_check_mode");
    int keywordsField         = sqlQuery.record().indexOf("keywords");
    int postContentTypeField  = sqlQuery.record().indexOf("post_content_type");
    int postUserAgentField    = sqlQuery.record().indexOf("post_user_agent");
    int postContentField      = sqlQuery.record().indexOf("post_content");

    if (monitorIdField >= 0        &&
        customerIdField >= 0       &&
        hostSchemeIdField >= 0     &&
        userOrderingField >= 0     &&
        pathField >= 0             &&
        methodField >= 0           &&
        contentCheckModeField >= 0 &&
        keywordsField >= 0         &&
        postContentTypeField >= 0  &&
        postUserAgentField >= 0    &&
        postContentField >= 0         ) {
        QString          path             = sqlQuery.value(pathField).toString();
        QString          postUserAgent    = sqlQuery.value(postUserAgentField).toString();
        MonitorId        monitorId        = sqlQuery.value(monitorIdField).toUInt(&ok);
        QByteArray       postContent      = qUncompress(sqlQuery.value(postContentField).toByteArray());
        CustomerId       customerId       = invalidCustomerId;
        HostSchemeId     hostSchemeId     = invalidHostSchemeId;
        UserOrdering     userOrdering     = 0;
        Method           method           = Method::GET;
        ContentCheckMode contentCheckMode = ContentCheckMode::NO_CHECK;
        ContentType      postContentType  = ContentType::TEXT;
        KeywordList      keywords;

        if (ok) {
            customerId = sqlQuery.value(customerIdField).toUInt(&ok);
        }

        if (ok) {
            hostSchemeId = sqlQuery.value(hostSchemeIdField).toUInt(&ok);
        }

        if (ok) {
            unsigned userOrderingUnsigned = sqlQuery.value(userOrderingField).toUInt(&ok);
            if (ok && userOrderingField <= 0xFFFF) {
                userOrdering = static_cast<UserOrdering>(userOrderingUnsigned);
            } else {
                ok = false;
            }
        }

        if (ok) {
            method = Monitor::toMethod(sqlQuery.value(methodField).toString(), &ok);
        }

        if (ok) {
            contentCheckMode = Monitor::toContentCheckMode(sqlQuery.value(contentCheckModeField).toString(), &ok);
        }

        if (ok) {
            postContentType = Monitor::toContentType(sqlQuery.value(postContentTypeField).toString(), &ok);
        }

        if (ok) {
            keywords = Monitor::toKeywordList(qUncompress(sqlQuery.value(keywordsField).toByteArray()), &ok);
        }

        if (ok) {
            result = Monitor(
                monitorId,
                customerId,
                hostSchemeId,
                userOrdering,
                path,
                method,
                contentCheckMode,
                keywords,
                postContentType,
                postUserAgent,
                postContent
            );
        }
    }

    if (success != nullptr) {
        *success = ok;
    }

    return result;
}
