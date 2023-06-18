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
* This header defines the \ref Events class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef EVENTS_H
#define EVENTS_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QList>
#include <QMutex>

#include <cstdint>

#include "event.h"
#include "sql_helpers.h"

class QSqlQuery;
class DatabaseManager;

/**
 * Class used to read and write information about events.  THis class expects two tables named "monitor_status" and
 * "events".  The "monitor_status" table holds the current status of a given monitor.  The "events" table holds a
 * history of events.
 *
 *     CREATE TABLE monitor_status (
 *         monitor_id INTEGER NOT NULL,
 *         status ENUM('UNKNOWN', 'WORKING','FAILED') NOT NULL,
 *         KEY monitor_status_constraint_1 (monitor_id),
 *         CONSTRAINT monitor_status_constraint_1 FOREIGN KEY (monitor_id)
 *             REFERENCES monitor (monitor_id)
 *             ON DELETE CASCADE
 *             ON UPDATE NO ACTION
 *     ) ENGINE=InnoDB DEFAULT CHARSET=latin1
 *
 *     CREATE TABLE event (
 *         event_id    INTEGER NOT NULL AUTO_INCREMENT,
 *         monitor_id  INTEGER NOT NULL,
 *         customer_id BIGINT NOT NULL,
 *         timestamp   INTEGER UNSIGNED NOT NULL,
 *         event_type  ENUM(
 *                         'WORKING',
 *                         'NO_RESPONSE',
 *                         'CONTENT_CHANGED',
 *                         'KEYWORDS',
 *                         'SSL_CERTIFICATE_EXPIRING',
 *                         'SSL_CERTIFICATE_RENEWED',
 *                         'CUSTOMER_1',
 *                         'CUSTOMER_2',
 *                         'CUSTOMER_3',
 *                         'CUSTOMER_4',
 *                         'CUSTOMER_5',
 *                         'CUSTOMER_6',
 *                         'CUSTOMER_7',
 *                         'CUSTOMER_8',
 *                         'CUSTOMER_9',
 *                         'CUSTOMER_10',
 *                         'TRANSACTION',
 *                         'INQUIRY',
 *                         'SUPPORT_REQUEST',
 *                         'STORAGE_LIMIT_REACHED'
 *                     ) NOT NULL,
 *         message     VARCHAR(128),
 *         KEY event_constraint_1 (customer_id),
 *         CONSTRAINT event_constraint_1 FOREIGN KEY (customer_id)
 *             REFERENCES customer_capabilities (customer_id)
 *             ON DELETE CASCADE
 *             ON UPDATE NO ACTION,
 *         KEY event_constraint_2 (monitor_id),
 *         CONSTRAINT event_constraint_2 FOREIGN KEY (monitor_id)
 *             REFERENCES monitor (monitor_id)
 *             ON DELETE CASCADE
 *             ON UPDATE NO ACTION,
 *         PRIMARY KEY (event_id)
 *     )
 */
class Events:public QObject, private SqlHelpers {
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
         * Enumeration of monitor status values.
         */
        typedef Event::MonitorStatus MonitorStatus;

        /**
         * Type used to represent a list of events.
         */
        typedef QList<Event> EventList;

        /**
         * Type used to represent the status of all monitors.
         */
        typedef QHash<Monitor::MonitorId, MonitorStatus> MonitorStatusByMonitorId;

        /**
         * Enumeration indicating if we should ignore an event, record and event or record and report an event.
         */
        enum EventDisposition {
            /**
             * Indicates a failed disposition check.
             */
            FAILED,

            /**
             * Indicates the event should be ignored.
             */
            IGNORE,

            /**
             * Indicates the event should be recorded but not reported.
             */
            RECORD_ONLY,

            /**
             * Indicates the event should be both recorded and reported.
             */
            RECORD_AND_REPORT
        };

        /**
         * Value used to indicate an invalid event ID.
         */
        static constexpr EventId invalidEventId = Event::invalidEventId;

        /**
         * Value used to indicate an invalid customerID.
         */
        static constexpr CustomerId invalidCustomerId = Event::invalidCustomerId;

        /**
         * Constructor
         *
         * \param[in] databaseManager The database manager used to fetch information about a region.
         *
         * \param[in] parent          Pointer to the parent object.
         */
        Events(DatabaseManager* databaseManager, QObject* parent = nullptr);

        ~Events() override;

        /**
         * Method you can use to add an event.  Note that events that represent persistent state will update both
         * the event table and the monitor_status table.
         *
         * \param[in] customerId    The ID of the customer tied to the monitor.
         *
         * \param[in] monitorId     The ID of the monitor that triggered the event.
         *
         * \param[in] unixTimestamp The Unix timestamp for the event.
         *
         * \param[in] eventType     The type of event that occurred.
         *
         * \param[in] message       The event message.
         *
         * \param[in] hash          The message hash tied to this message.
         *
         * \param[in] threadId      An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns an \ref Event index representing this event.  An invalid event is returned on error.
         */
        Event recordEvent(
            CustomerId         customerId,
            MonitorId          monitorId,
            unsigned long long unixTimestamp,
            EventType          eventType,
            const QString&     message,
            const QByteArray&  hash,
            unsigned           threadId = 0
        );

        /**
         * Method you can use to obtain the current status of a monitor
         *
         * \param[in] monitorId The ID of the monitor to get the current status for.
         *
         * \param[in] threadId  An optional thread ID used to maintain independent per-thread database instances.
         */
        MonitorStatus monitorStatus(MonitorId monitorId, unsigned threadId = 0);

        /**
         * Method you can use to obtain the current status of all monitors tied to a given customer.
         *
         * \param[in] customerId The customer ID of the customer to get all monitor status for.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         */
        MonitorStatusByMonitorId monitorStatusByCustomerId(CustomerId monitorId, unsigned threadId = 0);

        /**
         * Method you can use to obtain a single event by event ID.
         *
         * \param[in] eventId  The ID of the event to retrieve.
         *
         * \param[in] threadId An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns the requested event.
         */
        Event getEvent(EventId eventId, unsigned threadId = 0);

        /**
         * Method you can use to obtain a list of events by customer ID.
         *
         * \param[in] customerId     The customer ID.  An invalid customer ID will return events for all customers.
         *
         * \param[in] startTimestamp The starting Unix timestamp for the list.  Values will be limited to the range
         *                           supported by the Zoran epoch.
         *
         * \param[in] endingTimstamp The ending Unix timestamp for the list.  Values will be limited to the range
         *                           supported by the Zoran epoch.
         *
         * \param[in] threadId       An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns a list of events.
         */
        EventList getEventsByCustomer(
            CustomerId         customerId,
            unsigned long long startTimestamp  = 0,
            unsigned long long endingTimestamp = static_cast<unsigned long long>(-1),
            unsigned           threadId = 0
        );

        /**
         * Method you can use to obtain a list of events by monitor ID.
         *
         * \param[in] monitorId      The monitor ID.
         *
         * \param[in] startTimestamp The starting Unix timestamp for the list.  Values will be limited to the range
         *                           supported by the Zoran epoch.
         *
         * \param[in] endingTimstamp The ending Unix timestamp for the list.  Values will be limited to the range
         *                           supported by the Zoran epoch.
         *
         * \param[in] threadId       An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns a list of events.
         */
        EventList getEventsByMonitor(
            MonitorId          monitorId,
            unsigned long long startTimestamp  = 0,
            unsigned long long endingTimestamp = static_cast<unsigned long long>(-1),
            unsigned           threadId = 0
        );

        /**
         * Method you can use to determine how to handle an event based on the customer history.
         *
         * \param[in] eventType     The event type.
         *
         * \param[in] monitorStatus The monitor status returned by the polling server.
         *
         * \param[in] monitorId     The ID of the host/scheme of interest.
         *
         * \param[in] hash          The cryptographic hash of the page or found keywords, if relevant.
         *
         * \param[in] threadId      An optional thread ID used to maintain independent per-thread database instance.
         *
         * \return Returns the recommended event disposition.
         */
        EventDisposition eventDisposition(
            EventType         eventType,
            MonitorStatus     monitorStatus,
            MonitorId         monitorId,
            const QByteArray& hash,
            unsigned          threadId
        );

        /**
         * Method you can use to purge events before a specified timestamp.
         *
         * \param[in] customerId The customer ID.  An invalid customer ID will purge events for all customers.
         *
         * \param[in] timestamp  The Unix timestamp to purge for.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         */
        void purgeEvents(CustomerId customerId, unsigned long long timestamp, unsigned threadId = 0);

    private:
        /**
         * Base class for all duplicate event checking.
         */
        class Checker {
            public:
                Checker();

                virtual ~Checker();

                /**
                 * Method that builds up a SQL query to use to check the event to see if it should be recorded.
                 *
                 * The default implementation creates a query which identifies the last record associated with the
                 * specified monitor.
                 *
                 * \param[in] eventType     The type of event that is triggering this query.
                 *
                 * \param[in] monitorStatus The monitor status reported by the polling server.
                 *
                 * \param[in] monitorId     The ID of the monitor that triggered the event.
                 *
                 * \param[in] hash          The cryptographic hash of the page or found keywords, if relevant.
                 *
                 * \return Returns a string containing the required SQL query.  An empty string is returned if no
                 *         check of the database is needed and the event should always be reported.
                 */
                virtual QString queryString(
                    EventType         eventType,
                    MonitorStatus     monitorStatus,
                    MonitorId         monitorId,
                    const QByteArray& hash
                ) const;

                /**
                 * Method that checks the result from a SQL query to determine if an event should be recorded.
                 *
                 * The default implementation checks to see if the first returned record from the query has the same
                 * event type as specified in the call.
                 *
                 * \param[in,out] query         The SQL query to be checked.
                 *
                 * \param[in]     eventType     The type of event that is triggering this query.
                 *
                 * \param[in]     monitorStatus The monitor status reported by the polling server.
                 *
                 * \param[in]     monitorId     The ID of the monitor that triggered the event.
                 *
                 * \param[in]     hash          The cryptographic hash of the page or found keywords, if relevant.
                 *
                 * \return Returns the required processing for the event.  The default implementation returns
                 *         EventDisposition::IGNORE if the last event for the monitor based on the query is the same
                 *         type indicated by the event type parameter.  Returns EventDisposition::RECORD_AND_REPORT
                 *         otherwise.
                 */
                virtual EventDisposition checkQuery(
                    QSqlQuery&        query,
                    EventType         eventType,
                    MonitorStatus     monitorStatus,
                    MonitorId         monitorId,
                    const QByteArray& hash
                ) const;

            protected:
                /**
                 * Method that generates a query test condition for recording.
                 *
                 * \param[in] monitorId The ID of the monitor that triggered the event.
                 *
                 * \param[in] hash      The cryptographic hash of the page or found keywords, if relevant.
                 *
                 * \returns a SQL clause indicating the condition used to limit the query.
                 */
                virtual QString queryCondition(
                    MonitorId         monitorId,
                    const QByteArray& hash
                ) const = 0;

                /**
                 * Method that generates a list of columns to be returned in the query.
                 *
                 * \return Returns a list of columns to be returned in the query.  The default implementation returns
                 *         event_type AS event_type.
                 */
                virtual QString queryColumns() const;

                /**
                 * Method that provides the default disposition if an empty query is returned.
                 *
                 * \param[in] eventType     The type of event that is triggering this query.
                 *
                 * \param[in] monitorStatus The monitor status reported by the polling server.
                 *
                 * \return Returns the default disposition for an empty query.  The default implementation returns
                 *         \ref EventDisposition::IGNORE.
                 */
                virtual EventDisposition defaultDisposition(EventType eventType, MonitorStatus monitorStatus) const;
        };

        /**
         * Class used to test several types of per host-scheme events.
         */
        class PerHostSchemeChecker:public Checker {
            public:
                PerHostSchemeChecker();

                ~PerHostSchemeChecker() override;

            protected:
                /**
                 * Method that generates a query test condition.
                 *
                 * \param[in] monitorId The ID of the monitor that triggered the event.
                 *
                 * \param[in] hash      The cryptographic hash of the page or found keywords, if relevant.
                 */
                QString queryCondition(
                    MonitorId         monitorId,
                    const QByteArray& hash
                ) const override;

                /**
                 * Method you can overload to return the list of event types to look for.
                 *
                 * \return Returns a list of event types as a string.
                 */
                virtual QString eventTypes() const = 0;
        };

        /**
         * Class used to test several types of per monitor events.
         */
        class PerMonitorChecker:public Checker {
            public:
                PerMonitorChecker();

                ~PerMonitorChecker() override;

            protected:
                /**
                 * Method that generates a query test condition for recording.
                 *
                 * \param[in] monitorId The ID of the monitor that triggered the event.
                 *
                 * \param[in] hash      The cryptographic hash of the page or found keywords, if relevant.
                 */
                QString queryCondition(
                    MonitorId         monitorId,
                    const QByteArray& hash
                ) const override;

                /**
                 * Method you can overload to return the list of event types to look for.
                 *
                 * \return Returns a list of event types as a string.
                 */
                virtual QString eventTypes() const = 0;
        };

        /**
         * Class used to test content changed events.
         */
        class HashedEventChecker:public PerMonitorChecker {
            public:
                HashedEventChecker();

                ~HashedEventChecker() override;

                /**
                 * Method that checks the result from a SQL query to determine if an event should be reported.
                 *
                 * \param[in,out] query         The SQL query to be checked.
                 *
                 * \param[in]     eventType     The type of event that is triggering this query.
                 *
                 * \param[in]     monitorStatus The monitor status reported by the polling server.
                 *
                 * \param[in]     monitorId     The ID of the monitor that triggered the event.
                 *
                 * \param[in]     hash          The cryptographic hash of the page or found keywords, if relevant.
                 *
                 * \return Returns EventDisposition::IGNORE if the event is a repeat event.  Returns
                 *         EventDisposition::RECORD_AND_REPORT if the event is a new event.
                 */
                EventDisposition checkQuery(
                    QSqlQuery&        query,
                    EventType         eventType,
                    MonitorStatus     monitorStatus,
                    MonitorId         monitorId,
                    const QByteArray& hash
                ) const override;

            protected:
                /**
                 * Method that generates a list of columns to be returned in the query.
                 *
                 * \return Returns a list of columns to be returned in the query.  The default implementation returns
                 *         event_type AS event_type.
                 */
                QString queryColumns() const override;

                /**
                 * Method that provides the default disposition if an empty query is returned.
                 *
                 * \param[in] eventType     The type of event that is triggering this query.
                 *
                 * \param[in] monitorStatus The monitor status reported by the polling server.
                 *
                 * \return Returns the default disposition for an empty query.  This version always returns
                 *         \ref EventDisposition::RECORD_AND_REPORT.
                 */
                EventDisposition defaultDisposition(EventType eventType, MonitorStatus monitorStatus) const override;
        };

        /**
         * Class used to test SSL certificate issues.
         */
        class SslChecker:public PerHostSchemeChecker {
            public:
                SslChecker();

                ~SslChecker() override;

            protected:
                /**
                 * Method you can overload to return the list of event types to look for.
                 *
                 * \return Returns a list of event types as a string.  This implementation returns:
                 *         'SSL_CERTIFICATE_EXPIRING', 'SSL_CERTIFICATE_RENEWED'
                 */
                QString eventTypes() const override;
        };

        /**
         * Class used to test a working events.
         */
        class WorkingChecker:public PerMonitorChecker {
            public:
                WorkingChecker();

                ~WorkingChecker() override;

            protected:
                /**
                 * Method you can overload to return the list of event types to look for.
                 *
                 * \return Returns a list of event types as a string.  This implementation returns
                 *         'WORKING', 'NO_RESPONSE'.
                 */
                QString eventTypes() const override;

                /**
                 * Method that provides the default disposition if an empty query is returned.
                 *
                 * \param[in] eventType     The type of event that is triggering this query.
                 *
                 * \param[in] monitorStatus The monitor status reported by the polling server.
                 *
                 * \return Returns the default disposition for an empty query.  This version returns
                 *         \ref EventDisposition::RECORD_ONLY if the monitor status is \ref MonitorStatus::UNKNOWN or
                 *         \ref EventDisposition::IGNORE if the monitor status is anything else.
                 */
                EventDisposition defaultDisposition(EventType eventType, MonitorStatus monitorStatus) const override;
        };

        /**
         * Class used to test no response events.
         */
        class NoResponseChecker:public PerMonitorChecker {
            public:
                NoResponseChecker();

                ~NoResponseChecker() override;

            protected:
                /**
                 * Method you can overload to return the list of event types to look for.
                 *
                 * \return Returns a list of event types as a string.  This implementation returns
                 *         'WORKING', 'NO_RESPONSE'.
                 */
                QString eventTypes() const override;

                /**
                 * Method that provides the default disposition if an empty query is returned.
                 *
                 * \param[in] eventType     The type of event that is triggering this query.
                 *
                 * \param[in] monitorStatus The monitor status reported by the polling server.
                 *
                 * \return Returns the default disposition for an empty query.  This version always returns
                 *         \ref EventDisposition::RECORD_AND_REPORT.
                 */
                EventDisposition defaultDisposition(EventType eventType, MonitorStatus monitorStatus) const override;
        };

        /**
         * Class used to test content changed events.
         */
        class ContentChangedChecker:public HashedEventChecker {
            public:
                ContentChangedChecker();

                ~ContentChangedChecker() override;

            protected:
                /**
                 * Method you can overload to return the list of event types to look for.
                 *
                 * \return Returns a list of event types as a string.  This implementation returns
                 *         'CONTENT_CHANGED'.
                 */
                QString eventTypes() const override;
        };

        /**
         * Class used to test keyword events.
         */
        class KeywordsChecker:public HashedEventChecker {
            public:
                KeywordsChecker();

                ~KeywordsChecker() override;

            protected:
                /**
                 * Method you can overload to return the list of event types to look for.
                 *
                 * \return Returns a list of event types as a string.  This implementation returns
                 *         'KEYWORDS'.
                 */
                QString eventTypes() const override;
        };

        /**
         * Class used to test SSL certificate expiring events.
         */
        class SslCertificateExpiringChecker:public SslChecker {
            public:
                SslCertificateExpiringChecker();

                ~SslCertificateExpiringChecker() override;
        };

        /**
         * Class used to test SSL certificate renewed events.
         */
        class SslCertificateRenewedChecker:public SslChecker {
            public:
                SslCertificateRenewedChecker();

                ~SslCertificateRenewedChecker() override;
        };

        /**
         * Class used to test customer events.
         */
        class CustomerEventChecker:public Checker {
            public:
                CustomerEventChecker();

                ~CustomerEventChecker() override;

                /**
                 * Method that builds up a SQL query to use to check the event to see if it should be recorded.
                 *
                 * The default implementation creates a query which identifies the last record associated with the
                 * specified monitor.
                 *
                 * \param[in] eventType     The type of event that is triggering this query.
                 *
                 * \param[in] monitorStatus The monitor status reported by the polling server.
                 *
                 * \param[in] monitorId     The ID of the monitor that triggered the event.
                 *
                 * \param[in] hash          The cryptographic hash of the page or found keywords, if relevant.
                 *
                 * \return Returns a string containing the required SQL query.  An empty string is returned if no
                 *         check of the database is needed and the event should always be reported.  This version always
                 *         returns an empty string.
                 */
                QString queryString(
                    EventType         eventType,
                    MonitorStatus     monitorStatus,
                    MonitorId         monitorId,
                    const QByteArray& hash
                ) const override;

                /**
                 * Method that generates a query test condition for recording.
                 *
                 * \param[in] monitorId The ID of the monitor that triggered the event.
                 *
                 * \param[in] hash      The cryptographic hash of the page or found keywords, if relevant.
                 *
                 * \returns a SQL clause indicating the condition used to limit the query.  This version asserts.
                 */
                QString queryCondition(
                    MonitorId         monitorId,
                    const QByteArray& hash
                ) const override;
        };

        /**
         * Method used internally to parse a long query.
         *
         * \param[in] sqlQuery The query to be parsed.
         *
         * \return Returns an EventList holding the result.
         */
        static EventList parseLongQuery(QSqlQuery& sqlQuery);

        /**
         * Method that converts a Unix timestamp to a Zoran timestamp with capping.
         *
         * \param[in] unixTimestamp The Unix timestamp to be converted.
         *
         * \return Returns the Zoran timestamp.
         */
        ZoranTimeStamp toZoranTimestamp(unsigned long long unixTimestamp);

        /**
         * Hash table of event checkers.
         */
        QHash<EventType, Checker*> eventCheckers;

        /**
         * The underlying database manager instance.
         */
        DatabaseManager* currentDatabaseManager;
};

#endif
