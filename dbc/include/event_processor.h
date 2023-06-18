/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref EventProcessor class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef EVENT_PROCESSOR_H
#define EVENT_PROCESSOR_H

#include <QObject>
#include <QString>
#include <QList>
#include <QTimer>
#include <QHash>
#include <QMutex>

#include <cstdint>

#include "event.h"

class Events;
class Monitors;
class HostSchemes;
class OutboundRestApi;

/**
 * Class that handles reported events.  This class records the event and then triggers outbound reporting of the event.
 * The class also periodically scans SSL expiration date/time values to identify certificates that are about to expire.
 */
class EventProcessor:public QObject {
    Q_OBJECT

    public:
        /**
         * Value used to represent an event ID.
         */
        typedef Event::EventId EventId;

        /**
         * Value used to represent a monitor ID.
         */
        typedef Event::MonitorId MonitorId;

        /**
         * Type used to represent a customer ID.
         */
        typedef Event::CustomerId CustomerId;

        /**
         * Type used to represent a timestamp.
         */
        typedef Event::ZoranTimeStamp ZoranTimeStamp;

        /**
         * Enumeration of supported event types.
         */
        typedef Event::EventType EventType;

        /**
         * Enumeration of monitor status codes.
         */
        typedef Event::MonitorStatus MonitorStatus;

        /**
         * Value indicating the margin used to determine if an SSL certificate is expiring, in seconds.
         */
        static const unsigned long sslCertificateExpirationMarginSeconds = 3 * 24 * 3600;

        /**
         * Constructor
         *
         * \param[in] monitorDatabaseApi    The API used to access monitor entries.
         *
         * \param[in] hostSchemeDatabaseApi The API used to access host/scheme data.
         *
         * \param[in] eventDatabaseApi      The API used to access the event database API.
         *
         * \param[in] websiteRestApi        The API used to talk to the website.  Used to report events.
         *
         * \param[in] parent                Pointer to the parent object.
         */
        EventProcessor(
            Monitors*        monitorDatabaseApi,
            HostSchemes*     hostSchemeDatabaseApi,
            Events*          eventDatabaseApi,
            OutboundRestApi* websiteRestApi,
            QObject*         parent = nullptr
        );

        ~EventProcessor() override;

        /**
         * Method you can use to report an event.  This method will also record the event.
         *
         * \param[in] customerId    The ID of the customer tied to the monitor.
         *
         * \param[in] monitorId     The ID of the monitor that triggered the event.
         *
         * \param[in] unixTimestamp The Unix timestamp for the event.
         *
         * \param[in] eventType     The type of event that occurred.
         *
         * \param[in] monitorStatus The monitor status code reported from the polling server.
         *
         * \param[in] message       A message associated with this event.
         *
         * \param[in] hash          The cryptographic hash of the page or found keywords, if relevant.
         *
         * \param[in] threadId      An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool reportEvent(
            CustomerId         customerId,
            MonitorId          monitorId,
            unsigned long long unixTimestamp,
            EventType          eventType,
            MonitorStatus      monitorStatus,
            const QString&     message,
            const QByteArray&  hash,
            unsigned           threadId = 0
        );

    private slots:
        /**
         * Slot that is triggered when the SSL expiration check timer fires.
         */
        void checkSslExpiration();

    private:
        /**
         * SSL expiration check interval.
         */
        static const unsigned sslCheckIntervalMilliseconds = 2000;

        /**
         * Database ID for the timer function used to check SSL expiration.
         */
        static const unsigned timerDatabaseThreadId = static_cast<unsigned>(-2);

        /**
         * The underlying monitors database API.
         */
        Monitors* currentMonitors;

        /**
         * The underlying host/scheme database API.
         */
        HostSchemes* currentHostSchemes;

        /**
         * The underlying events reporting system.
         */
        Events* currentEvents;

        /**
         * API used to talk to the website.
         */
        OutboundRestApi* currentWebsiteRestApi;

        /**
         * Timer used to schedule periodic checks of SSL certificates.
         */
        QTimer certificateCheckTimer;

        /**
         * Mutex used to make certain event reporting/recording is always serialized.
         */
        QMutex reportEventMutex;

        /**
         * Hash of currently expiring/expired certificates.
         */
        QHash<HostScheme::HostSchemeId, bool> currentExpiringHostSchemes;
};

#endif
