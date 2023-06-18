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
* This header defines the \ref CustomersCapabilities class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef CUSTOMERS_CAPABILITIES_H
#define CUSTOMERS_CAPABILITIES_H

#include <QObject>
#include <QString>
#include <QSqlQuery>
#include <QHash>
#include <QSet>
#include <QMutex>

#include <cstdint>

#include "sql_helpers.h"
#include "customer_capabilities.h"
#include "cache.h"

class QTimer;
class DatabaseManager;

/**
 * Class used to manage the allowed capabilities by each customer.  This cless expects a table named
 * "customer_capabilities" with the following definition:
 *
 *     CREATE TABLE customer_capabilities (
 *         customer_id      BIGINT NOT NULL,   // TODO: change to INTEGER UNIGNED
 *         number_monitors  SMALLINT NOT NULL, // TODO: change to SMALLINT UNSIGNED
 *         polling_interval SMALLINT NOT NULL, // TODO: change to SMALLINT UNSIGNED
 *         expiration_days  INTEGER NOT NULL,  // TODO: change to INTEGER UNSIGNED
 *         flags            SMALLINT NOT NULL  // TODO: change to SMALLINT UNSIGNED
 *         PRIMARY KEY (customer_id)
 *     ) ENGINE=InnoDB DEFAULT CHARSET=latin1
 *
 * Note that, for performance reasons, this class maintains an cache of customer capabilities with random eviction.
 */
class CustomersCapabilities:public QObject, public Cache<CustomerCapabilities, CustomerCapabilities::CustomerId> {
    Q_OBJECT

    public:
        /**
         * Type used to represent a customer ID.
         */
        typedef CustomerCapabilities::CustomerId CustomerId;

        /**
         * Type used to hold a hash of customers by customer ID.
         */
        typedef QHash<CustomerId, CustomerCapabilities> CapabilitiesByCustomerId;

        /**
         * Type used to hold a collection of unique customer IDs.
         */
        typedef QSet<CustomerId> CustomerIdSet;

        /**
         * Value representing the default cache depth.
         */
        static constexpr unsigned defaultCacheDepth = 10000;

        /**
         * Constructor
         *
         * \param[in] databaseManager   The database manager used to fetch information about a region.
         *
         * \param[in] maximumCacheDepth The maximum allowed cache depth.
         *
         * \param[in] parent            Pointer to the parent object.
         */
        CustomersCapabilities(
            DatabaseManager*  databaseManager,
            unsigned          maximumCacheDepth = defaultCacheDepth,
            QObject*          parent = nullptr
        );

        ~CustomersCapabilities() override;

        /**
         * Method you can use to get a customer capabilities for a customer.
         *
         * \param[in] customerId    The ID of the customer to get the capabilities for.
         *
         * \param[in] noCacheUpdate If true, then reading this value will not change the cache state.
         *
         * \param[in] threadId      An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns the customer capabilities.  An invalid capabilities is returned if the customer has no capabilities.
         */
        CustomerCapabilities getCustomerCapabilities(
            CustomerId customerId,
            bool       noCacheUpdate = false,
            unsigned   threadId = 0
        );

        /**
         * Method you can use to delete a customer capabilities.
         *
         * \param[in] customerId The ID of the customer delete.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success. Returns false on error.
         */
        bool deleteCustomerCapabilities(CustomerId customerId, unsigned threadId = 0);

        /**
         * Method you can use to purge one or more customers by customer ID.
         *
         * \param[in] customerIds The IDs of the customer delete.
         *
         * \param[in] threadId    An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success. Returns false on error.
         */
        bool purgeCustomerCapabilities(const CustomerIdSet& customerId, unsigned threadId = 0);

        /**
         * Method you can use to update a customer capabilities.
         *
         * \param[in] customerCapabilities The customer capabilities to be updated.
         *
         * \param[in] threadId             An optional thread ID used to maintain independent per-thread database
         *                                 instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool updateCustomerCapabilities(
            const CustomerCapabilities& customerCapabilities,
            unsigned                    threadId = 0
        );

        /**
         * Method you can use to get all the capabilities for every customer.
         *
         * \param[in] threadId      An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns the customer capabilities.  An invalid capabilities is returned if the customer has no capabilities.
         */
        CapabilitiesByCustomerId getAllCustomerCapabilities(unsigned threadId = 0);

    protected:
        /**
         * Method that obtains the ID used to access a specific value.
         *
         * \param[in] value The value to calculate the ID for.
         *
         * \return Returns the ID to associate with this value.
         */
        CustomerId idFromValue(const CustomerCapabilities& value) const final;

    private:
        /**
         * The underlying database manager instance.
         */
        DatabaseManager* currentDatabaseManager;

        /**
         * Mutex used to protect the cache.
         */
        QMutex cacheMutex;
};

#endif
