/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref Monitors class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef MONITORS_H
#define MONITORS_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QMultiHash>
#include <QSqlQuery>

#include <cstdint>

#include "host_scheme.h"
#include "scheme_host_path.h"
#include "monitor.h"
#include "sql_helpers.h"

class DatabaseManager;

/**
 * Class used to read and write information about user monitors.  This cless expects a number of different tables
 * defined as follows:
 *
 *     CREATE TABLE monitor (
 *         monitor_id INTEGER NOT NULL AUTO_INCREMENT,
 *         customer_id BIGINT NOT NULL,
 *         host_scheme_id BIGINT NOT NULL,
 *         user_ordering SMALLINT NOT NULL,
 *         path TEXT NOT NULL,
 *         method ENUM('GET','POST') NOT NULL,
 *         content_check_mode ENUM(
 *             'NO_CHECK',
 *             'CONTENT_MATCH',
 *             'ANY_KEYWORDS',
 *             'ALL_KEYWORDS',
 *             'SMART_CONTENT_MATCH'
 *         ) NOT NULL,
 *         keywords BLOB NOT NULL,
 *         post_content_type ENUM('JSON','XML','TEXT') NOT NULL,
 *         post_user_agent VARCHAR(128),
 *         post_content BLOB,
 *         PRIMARY KEY (monitor_id),
 *         KEY monitor_constraint_1 (host_scheme_id),
 *         KEY monitor_constraint_2 (`customer_id`),
 *         CONSTRAINT monitor_constraint_1 FOREIGN KEY (host_scheme_id)
 *             REFERENCES host_scheme (host_scheme_id)
 *             ON DELETE CASCADE
 *             ON UPDATE NO ACTION,
 *         CONSTRAINT monitor_constraint_2 FOREIGN KEY (customer_id)
 *             REFERENCES customer_capabilities (customer_id)
 *             ON DELETE CASCADE
 *             ON UPDATE NO ACTION
 *     ) ENGINE=InnoDB DEFAULT CHARSET=latin1
 */
class Monitors:public QObject, private SqlHelpers {
    Q_OBJECT

    public:
        /**
         * Value used to represent a host/scheme ID.
         */
        typedef Monitor::MonitorId MonitorId;

        /**
         * Type used to represent a customer ID.
         */
        typedef HostScheme::CustomerId CustomerId;

        /**
         * Type used to represent a host/scheme ID.
         */
        typedef HostScheme::HostSchemeId HostSchemeId;

        /**
         * Value used to indicate user ordering in the GUI.
         */
        typedef Monitor::UserOrdering UserOrdering;

        /**
         * Enumeration of supported access methods.
         */
        typedef Monitor::Method Method;

        /**
         * Enumeration of content check modes.
         */
        typedef Monitor::ContentCheckMode ContentCheckMode;

        /**
         * Enumeration of supported POST content types.
         */
        typedef Monitor::ContentType ContentType;

        /**
         * Type used to represent a list of keywords.
         */
        typedef Monitor::KeywordList KeywordList;

        /**
         * Value used to indicate an invalid monitor ID.
         */
        static constexpr MonitorId invalidMonitorId = Monitor::invalidMonitorId;

        /**
         * Value used to indicate an invalid host/scheme ID.
         */
        static constexpr HostSchemeId invalidHostSchemeId = HostScheme::invalidHostSchemeId;

        /**
         * Value used to indicate an invalid customer ID.
         */
        static constexpr CustomerId invalidCustomerId = HostScheme::invalidCustomerId;

        /**
         * Type used to represent a collection of monitors.
         */
        typedef QList<Monitor> MonitorList;

        /**
         * Type used to represent a collection of monitors.
         */
        typedef QMultiHash<SchemeHostPath, Monitor> MonitorsBySchemeHostPath;

        /**
         * Type used to represent a collection of monitors by monitor ID.
         */
        typedef QHash<MonitorId, Monitor> MonitorsById;

        /**
         * Constructor
         *
         * \param[in] databaseManager The database manager used to fetch information about a monitor.
         *
         * \param[in] parent          Pointer to the parent object.
         */
        Monitors(DatabaseManager* databaseManager, QObject* parent = nullptr);

        ~Monitors() override;

        /**
         * Method you can use to get a monitor by monitor ID.
         *
         * \param[in] monitorId The monitor ID of the desired monitor.
         *
         * \param[in] threadId  An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns the \ref Monitor instance for the monitor.  An invalid server instance is returned if the
         *         monitor ID is not valid.
         */
        Monitor getMonitor(MonitorId monitorId, unsigned threadId = 0) const;

        /**
         * Method you can use to get all the monitors for a given customer.  Monitors are sorted by user ordering
         * value.
         *
         * \param[in] customerId The ID of the customer we want to obtain all monitors for.  An invalid customer ID
         *                       will cause all monitors for all customers to be returned.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns a \ref MonitorList instance for every monitor tied to the specified customer.  An empty list
         *         is returned if the customer ID is invalid.
         */
        MonitorList getMonitorsByUserOrder(CustomerId customerId = invalidCustomerId, unsigned threadId = 0) const;

        /**
         * Method you can use to get all the monitors for a given customer.  Monitors are hashed by monitor ID.
         *
         * \param[in] customerId The ID of the customer we want to obtain all monitors for.  An invalid customer ID
         *                       will cause all monitors for all customers to be returned.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns a \ref MonitorList instance for every monitor tied to the specified customer.  An empty list
         *         is returned if the customer ID is invalid.
         */
        MonitorsById getMonitorsByCustomerId(CustomerId customerId = invalidCustomerId, unsigned threadId = 0) const;

        /**
         * Method you can use to get all the monitors for a given customer by host/scheme and path.
         *
         * \param[in] customerId The ID of the customer we want to obtain all monitors for.  An invalid customer ID
         *                       will cause all monitors for all customers to be returned.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns a \ref MonitorsBySchemeHostPath instance giving every monitor tied to this customer.  An
         *         empty instance is returned if the customer ID is invalid.
         */
        MonitorsBySchemeHostPath getMonitorsBySchemeHostPath(CustomerId customerId, unsigned threadId = 0) const;

        /**
         * Method you can use to get all the monitors tied to a specific host/scheme.
         *
         * \param[in] hostSchemeId The ID of the host scheme we want monitors for.
         *
         * \param[in] threadId     An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns the \ref Server instance for the server.  An invalid server instance is returned if the
         *         server ID is not valid.
         */
        MonitorList getMonitorsUnderHostScheme(HostSchemeId hostSchemeId, unsigned threadId = 0) const;

        /**
         * Method you can use to get all the monitors by monitor ID.
         *
         * \param[in] threadId     An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns the \ref Server instance for the server.  An invalid server instance is returned if the
         *         server ID is not valid.
         */
        MonitorsById getMonitorsById(unsigned threadId = 0) const;

        /**
         * Method you can use to add a new monitor to the database.
         *
         * \param[in] customerId       The customer ID of the customer owning this monitor.
         *
         * \param[in] hostSchemeId     The host/scheme ID indicating the server and scheme this monitor is tied to.
         *
         * \param[in] userOrdering     A zero based numerical user ordering.  The value is intended to assist user
         *                             interfaces.
         *
         * \param[in] path             The path under the scheme and host to be checked.
         *
         * \param[in] method           The method used to check the URL.
         *
         * \param[in] contentCheckMode The content check mode for this monitor.
         *
         * \param[in] keywords         The list of content check keywords for this monitor.
         *
         * \param[in] contentType      The type of content to supply as part of a POST message.
         *
         * \param[in] userAgent        The user-agent string to be reported during a POST message.
         *
         * \param[in] postContent      An array of byte data to be sent as the post message.
         *
         * \param[in] threadId         An optional thread ID used to maintain independent per-thread database
         *                             instances.
         *
         * \return Returns the newly created \ref Monitor instance.  An invalid monitor instance is returned if the
         *         monitor could not be created.
         */
        Monitor createMonitor(
            CustomerId         customerId,
            HostSchemeId       hostSchemeId,
            UserOrdering       userOrdering,
            const QString&     path,
            Method             method,
            ContentCheckMode   contentCheckMode,
            const KeywordList& keywords,
            ContentType        contentType,
            const QString&     userAgent,
            const QByteArray&  postContent,
            unsigned           threadId = 0
        );

        /**
         * Method you can use to update a monitor in the database.
         *
         * \param[in] monitor  The monitor instance to be updated.
         *
         * \param[in] threadId An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool modifyMonitor(const Monitor& monitor, unsigned threadId = 0);

        /**
         * Method you can use to delete a single monitor from the database.
         *
         * \param[in] monitor  The monitor to be deleted.
         *
         * \param[in] threadId An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool deleteMonitor(const Monitor& monitor, unsigned threadId = 0);

        /**
         * Method you can use to delete all monitors for a given customer.
         *
         * \param[in] customerId The customer ID of the customer to purge data for.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool deleteMonitors(CustomerId customerId, unsigned threadId = 0);

    private:
        /**
         * Method that converts a query to a monitor instance.
         *
         * \param[in]  sqlQuery The SQL query to convert.
         *
         * \param[out] success  A pointer to a boolean value that can optionally be pupulated with True on success or
         *                      false on error.
         */
        static Monitor convertQueryToMonitor(const QSqlQuery& sqlQuery, bool* success = nullptr);

        /**
         * The underlying database manager instance.
         */
        DatabaseManager* currentDatabaseManager;
};

#endif
