/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref EventProcessor class.
***********************************************************************************************************************/

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <QMutex>
#include <QMutexLocker>

#include <cstdint>

#include "monitor.h"
#include "monitors.h"
#include "host_scheme.h"
#include "host_schemes.h"
#include "event.h"
#include "events.h"
#include "outbound_rest_api.h"
#include "event_processor.h"

EventProcessor::EventProcessor(
        Monitors*          monitorsDatabaseApi,
        HostSchemes*       hostSchemesDatabaseApi,
        Events*            eventDatabaseApi,
        OutboundRestApi*   websiteRestApi,
        QObject*           parent
    ):QObject(
        parent
    ),currentMonitors(
        monitorsDatabaseApi
    ),currentHostSchemes(
        hostSchemesDatabaseApi
    ),currentEvents(
        eventDatabaseApi
    ),currentWebsiteRestApi(
        websiteRestApi
    ) {
    certificateCheckTimer.setSingleShot(false);
    certificateCheckTimer.start(sslCheckIntervalMilliseconds);

    connect(&certificateCheckTimer, &QTimer::timeout, this, &EventProcessor::checkSslExpiration);
}


EventProcessor::~EventProcessor() {}


bool EventProcessor::reportEvent(
        EventProcessor::CustomerId    customerId,
        EventProcessor::MonitorId     monitorId,
        unsigned long long            unixTimestamp,
        EventProcessor::EventType     eventType,
        EventProcessor::MonitorStatus monitorStatus,
        const QString&                message,
        const QByteArray&             hash,
        unsigned                      threadId
    ) {
    QMutexLocker locker(&reportEventMutex);

    Events::EventDisposition eventDisposition = currentEvents->eventDisposition(
        eventType,
        monitorStatus,
        monitorId,
        hash,
        threadId
    );

    if (eventDisposition == Events::EventDisposition::RECORD_AND_REPORT ||
        eventDisposition == Events::EventDisposition::RECORD_ONLY          ) {
        currentEvents->recordEvent(
            customerId,
            monitorId,
            unixTimestamp,
            eventType,
            message,
            hash,
            threadId
        );
    }

    if (eventDisposition == Events::EventDisposition::RECORD_AND_REPORT) {
        QJsonObject jsonMessage;

        Monitor    monitor    = currentMonitors->getMonitor(monitorId, threadId);
        HostScheme hostScheme = currentHostSchemes->getHostScheme(monitor.hostSchemeId(), threadId);

        jsonMessage.insert("customer_id", static_cast<double>(customerId));
        jsonMessage.insert("monitor_id", static_cast<double>(monitorId));
        jsonMessage.insert("event_type", Event::toString(eventType).toLower());
        jsonMessage.insert("path", monitor.path());
        jsonMessage.insert("authority", hostScheme.url().toString());
        jsonMessage.insert("message", message);
        jsonMessage.insert("timestamp", static_cast<double>(unixTimestamp));

        QString logMessage = QString("Reported event %1 (%2), customer %3 - %4/%5")
                             .arg(Event::toString(eventType), message)
                             .arg(customerId)
                             .arg(hostScheme.url().toString(), monitor.path());

        currentWebsiteRestApi->postMessage(QString("/event/report"), jsonMessage, logMessage);
    }

    return true;
}


void EventProcessor::checkSslExpiration() {
    HostSchemes::HostSchemeHash hostSchemesById = currentHostSchemes->getHostSchemes(
        HostScheme::invalidCustomerId,
        timerDatabaseThreadId
    );

    unsigned long long currentDateTime = QDateTime::currentSecsSinceEpoch();
    unsigned long long threshold       = currentDateTime + sslCertificateExpirationMarginSeconds;

    for (  HostSchemes::HostSchemeHash::const_iterator it  = hostSchemesById.constBegin(),
                                                       end = hostSchemesById.constEnd()
         ; it != end
         ; ++it
         ) {
        HostScheme::HostSchemeId hostSchemeId = it.key();
        const HostScheme&        hostScheme   = it.value();
        unsigned long long       expiration   = hostScheme.sslExpirationTimestamp();

        if (expiration != HostScheme::invalidSslExpirationTimestamp) {
            if (expiration < threshold) {
                if (!currentExpiringHostSchemes.value(hostSchemeId)) {
                    currentExpiringHostSchemes.insert(hostSchemeId, true);

                    Monitors::MonitorList monitors = currentMonitors->getMonitorsUnderHostScheme(
                        hostSchemeId,
                        timerDatabaseThreadId
                    );
                    const Monitor& monitor = monitors.first();

                    reportEvent(
                        hostScheme.customerId(),
                        monitor.monitorId(),
                        currentDateTime,
                        EventType::SSL_CERTIFICATE_EXPIRING,
                        MonitorStatus::WORKING,
                        tr("Expiration %1 UTC").arg(
                            QDateTime::fromMSecsSinceEpoch(expiration).toString(Qt::DateFormat::RFC2822Date)
                        ),
                        QByteArray(),
                        timerDatabaseThreadId
                    );
                }
            } else {
                if (currentExpiringHostSchemes.value(hostSchemeId,true)) {
                    currentExpiringHostSchemes.insert(hostSchemeId, false);

                    Monitors::MonitorList monitors = currentMonitors->getMonitorsUnderHostScheme(
                        hostSchemeId,
                        timerDatabaseThreadId
                    );
                    const Monitor& monitor = monitors.first();

                    reportEvent(
                        hostScheme.customerId(),
                        monitor.monitorId(),
                        currentDateTime,
                        EventType::SSL_CERTIFICATE_RENEWED,
                        MonitorStatus::WORKING,
                        tr("Expiration %1 UTC").arg(
                            QDateTime::fromMSecsSinceEpoch(expiration).toString(Qt::DateFormat::RFC2822Date)
                        ),
                        QByteArray(),
                        timerDatabaseThreadId
                    );
                }
            }
        }
    }
}
