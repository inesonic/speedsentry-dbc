/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref HostSchemes class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef HOST_SCHEMES_H
#define HOST_SCHEMES_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QSqlQuery>

#include <cstdint>

#include "host_scheme.h"
#include "sql_helpers.h"

class QTimer;
class DatabaseManager;

/**
 * Class used to read and write information about server hostnames and schemes used to access them.  This cless expects
 * a table named "host_scheme" with the following definition:
 *
 *     CREATE TABLE host_scheme (
 *         host_scheme_id BIGINT NOT NULL AUTO_INCREMENT, // TODO: Change to INTEGER UNSIGNED
 *         customer_id BIGINT NOT NULL,                   // TODO: Change to INTEGER UNSIGNED
 *         host VARCHAR(128) NOT NULL,
 *         ssl_expiration_timestamp BIGINT NOT NULL,
 *         PRIMARY KEY (host_scheme_id),
 *         KEY host_scheme_constraint_1 (customer_id),
 *         CONSTRAINT host_scheme_constraint_1 FOREIGN KEY (customer_id)
 *             REFERENCES customer_capabilities (customer_id)
 *             ON DELETE CASCADE
 *             ON UPDATE NO ACTION
 *     ) ENGINE=InnoDB AUTO_INCREMENT=102 DEFAULT CHARSET=latin1
 */
class HostSchemes:public QObject, private SqlHelpers {
    Q_OBJECT

    public:
        /**
         * The host/scheme ID used to access this entry.
         */
        typedef HostScheme::HostSchemeId HostSchemeId;

        /**
         * Type used to represent a customer ID.
         */
        typedef HostScheme::CustomerId CustomerId;

        /**
         * Type used to represent a collection of host/schemes by host/scheme ID.
         */
        typedef QHash<HostSchemeId, HostScheme> HostSchemeHash;

        /**
         * Constructor
         *
         * \param[in] databaseManager The database manager used to fetch information about host/scheme instances.
         *
         * \param[in] parent          Pointer to the parent object.
         */
        HostSchemes(DatabaseManager* databaseManager, QObject* parent = nullptr);

        ~HostSchemes() override;

        /**
         * Method you can use to get a single host/scheme instance by ID.
         *
         * \param[in] hostSchemeId The host/scheme ID of the desired host/scheme.
         *
         * \param[in] threadId     An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns the \ref HostScheme instance.  An invalid host/scheme instance is returned if the
         *         host/scheme ID is invalid.
         */
        HostScheme getHostScheme(HostSchemeId hostSchemeId, unsigned threadId = 0) const;

        /**
         * Method you can use to add a new host/scheme.
         *
         * \param[in] customerId The customer ID to tie to this host scheme.
         *
         * \param[in] url        The host/scheme URL.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns the newly created host/scheme instance.
         */
        HostScheme createHostScheme(CustomerId customerId, const QUrl& url, unsigned threadId = 0) const;

        /**
         * Method you can use to update a host scheme in the database.
         *
         * \param[in] hostScheme The host/scheme to be updated.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool modifyHostScheme(const HostScheme& hostScheme, unsigned threadId = 0);

        /**
         * Method you can use to delete a host scheme from the database.
         *
         * \param[in] hostScheme The host/scheme to be deleted.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool deleteHostScheme(const HostScheme& hostScheme, unsigned threadId = 0);

        /**
         * Method you can use to delete all host schemes associated with a customer.
         *
         * \param[in] customerId The customer ID of the customer who's host/scheme values should be purged.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool deleteCustomerHostSchemes(CustomerId customerId, unsigned threadId = 0);

        /**
         * Method you can use to obtain host schemes for a given customer or all customers.
         *
         * \param[in] customerId The customer ID of the customer to get host schemes for.  Setting to
         *                       \ref HostScheme::invalidCustomerId will return all known host/scheme instances.
         *
         * \param[in] threadId An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns a hash of host/scheme instances by host/scheme ID.
         */
        HostSchemeHash getHostSchemes(CustomerId customerId = HostScheme::invalidCustomerId, unsigned threadId = 0);

    private:
        /**
         * Method that converts a query to a host/scheme instance.
         *
         * \param[in]  sqlQuery THe SQL query to convert.
         *
         * \param[out] success  A pointer to a boolean value that can optionally be pupulated with True on success or
         *                      false on error.
         */
        static HostScheme convertQueryToHostScheme(const QSqlQuery& sqlQuery, bool* success = nullptr);

        /**
         * The underlying database manager instance.
         */
        DatabaseManager* currentDatabaseManager;
};

#endif
