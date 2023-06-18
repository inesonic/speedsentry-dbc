/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref Resources class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef RESOURCES_H
#define RESOURCES_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QMutex>

#include <cstdint>

#include "resource.h"
#include "active_resources.h"
#include "cache.h"
#include "sql_helpers.h"

class QSqlQuery;
class QTimer;
class DatabaseManager;

/**
 * Class used to read and write resources data.  This class is managed by a single table.
 *
 *     CREATE TABLE resources (
 *         customer_id BIGINT NOT NULL,
 *         value_type  SMALLINT NOT NULL,
 *         value REAL NOT NULL,
 *         timestamp1 INTEGER NOT NULL,
 *         timestamp2 SMALLINT NOT NULL,
 *         PRIMARY KEY(customer_id, value_type, timestamp1),
 *         CONSTRAINT resources_customer_id_fk_constraint
 *             FOREIGN KEY (customer_id) REFERENCES customer_capabilities(customer_id)
 *             ON DELETE_CASCADE
 *     ) ENGINE=InnoDB DEFAULT CHARSET=latin1
 */
class Resources:public QThread, private SqlHelpers, public Cache<ActiveResources, Resource::CustomerId> {
    Q_OBJECT

    public:
        /**
         * Type used to represent a customer ID.
         */
        typedef Resource::CustomerId CustomerId;

        /**
         * Type used to indicate the type of value being stored.
         */
        typedef Resource::ValueType ValueType;

        /**
         * Type used to store the value.
         */
        typedef Resource::Value Value;

        /**
         * Type used to contain a collection of resource entries.
         */
        typedef QList<Resource> ResourceList;

        /**
         * Constructor
         *
         * \param[in] databaseManager The database manager used to fetch information about a region.
         *
         * \param[in]
         *
         * \param[in] parent          Pointer to the parent object.
         */
        Resources(DatabaseManager* databaseManager, QObject* parent = nullptr);

        ~Resources() override;

        /**
         * Method you can use to determine if a customer has resource data.
         *
         * \param[in] customerId The ID of the customer in question.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true if the customer has resource data.  Returns false if the customer does not have
         *         resource data.
         */
        ActiveResources hasResourceData(CustomerId customerId, unsigned threadId = 0);

        /**
         * Method you can use to add resource data.
         *
         * \param[in] customerId    The ID of the customer.
         *
         * \param[in] valueType     The type for the value to be stored.
         *
         * \param[in] value         The value to be stored.
         *
         * \param[in] unixTimestamp The Unix timestamp for the event.
         *
         * \param[in] threadId      An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns an \ref Event index representing this event.  An invalid event is returned on error.
         */
        Resource recordResource(
            CustomerId         customerId,
            ValueType          valueType,
            Value              value,
            unsigned long long unixTimestamp,
            unsigned           threadId = 0
        );

        /**
         * Method you can use to obtain resources for a given customer and type.
         *
         * \param[in] customerId     The ID of the customer.
         *
         * \param[in] valueType      The type for the value to be stored.
         *
         * \param[in] startTimestamp The optional starting timestamp for the request.  A value of 0 indicates no
         *                           starting timestamp.
         *
         * \param[in] endTimestamp   The optional ending timestamp for the request.  A value of 0 indicates no ending
         *                           timestamp.
         */
        ResourceList getResources(
            CustomerId         customerId,
            ValueType          valueType,
            unsigned long long startTimestamp = 0,
            unsigned long long endTimestamp = 0,
            unsigned           threadId = 0
        );

        /**
         * Method you can use to purge resources before a specified timestamp.
         *
         * \param[in] customerId The customer ID.  An invalid customer ID will purge events for all customers.
         *
         * \param[in] timestamp  The Unix timestamp to purge for.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         */
        void purgeResources(CustomerId customerId, unsigned long long timestamp, unsigned threadId = 0);

        /**
         * Method you can use to set the maximum age for resource data.
         *
         * \param[in] newMaximumAge The new maximum age value.
         */
        void setMaximumAge(unsigned long newMaximumAge);

    protected:
        /**
         * Method that obtains the ID used to access a specific value.
         *
         * \param[in] value The value to calculate the ID for.
         *
         * \return Returns the ID to associate with this value.
         */
        CustomerId idFromValue(const ActiveResources& value) const final;

        /**
         * Method that performs the expunge operation in the background.
         */
        void run() final;

    private slots:
        /**
         * Slot that starts the expunge operation.
         */
        void startExpunge();

    private:
        /**
         * Value indicating the maximum resources cache size.
         */
        static const unsigned maximumCacheDepth = 10000;

        /**
         * Value indicating the expunge timer check interval in milliseconds.
         */
        static const unsigned expungeTimerPeriod = 1000 * 60 * 60 * 24;

        /**
         * The thread ID to use when expunging old entries.
         */
        static const unsigned expungeThreadId = static_cast<unsigned>(-4);

        /**
         * The underlying database manager instance.
         */
        DatabaseManager* currentDatabaseManager;

        /**
         * Mutex used to protect the cache.
         */
        QMutex cacheMutex;

        /**
         * The maximum age for resource entries.
         */
        unsigned long currentMaximumResourceDataAge;

        /**
         * Timer used to trigger the expunge thread at periodic intervals.
         */
        QTimer* expungeTimer;
};

#endif
