/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref CustomerMapping class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef CUSTOMER_MAPPING_H
#define CUSTOMER_MAPPING_H

#include <QObject>
#include <QString>
#include <QSet>

#include <cstdint>

#include "customer_capabilities.h"
#include "server.h"
#include "sql_helpers.h"

class QTimer;
class DatabaseManager;

/**
 * Class used to track which servers a customer has been assigned to.  This class expects a table named
 * "customer_mapping" with the following definition:
 *
 *     CREATE TABLE customer_mapping (
 *         customer_id    BIGINT NOT NULL,
 *         server_id      SMALLINT NOT NULL,
 *         primary_server BOOLEAN NOT NULL,
 *         PRIMARY KEY (customer_id, server_id),
 *         KEY customer_mapping_constraint_2 (server_id),
 *         CONSTRAINT customer_mapping_constraint_1 FOREIGN KEY (customer_id)
 *             REFERENCES customer_capabilities (customer_id) ON DELETE CASCADE ON UPDATE NO ACTION
 *         CONSTRAINT customer_mapping_constraint_2 FOREIGN KEY (server_id)
 *             REFERENCES servers (server_id) ON DELETE CASCADE ON UPDATE NO ACTION
 *     ) ENGINE=InnoDB DEFAULT CHARSET=latin1
 */
class CustomerMapping:public QObject {
    Q_OBJECT

    public:
        /**
         * Type used to represent a customer ID.
         */
        typedef CustomerCapabilities::CustomerId CustomerId;

        /**
         * Type used to represent a server ID.
         */
        typedef Server::ServerId ServerId;

        /**
         * Value used to represent an invalid server ID.
         */
        static constexpr ServerId invalidServerId = 0;

        /**
         * Type used to represent a list of servers.
         */
        typedef QSet<Server::ServerId> ServerSet;

        /**
         * Class that tracks a customer/server mapping.
         */
        class Mapping:public ServerSet {
            public:
                inline Mapping():currentPrimaryServerId(invalidServerId) {}

                /**
                 * Constructor
                 *
                 * \param[in] primaryServerId
                 */
                inline Mapping(ServerId primaryServerId):currentPrimaryServerId(primaryServerId) {}

                /**
                 * Constructor
                 *
                 * \param[in] primaryServerId The server ID of the primary server for this customer.
                 *
                 * \param[in] serverSet       Set containing the server IDs of the servers serving this customer.
                 */
                inline Mapping(
                        ServerId         primaryServerId,
                        const ServerSet& serverSet
                    ):ServerSet(
                        serverSet
                    ),currentPrimaryServerId(
                        primaryServerId
                    ) {}

                /**
                 * Copy constructor.
                 *
                 * \param[in] other The instance to assign to this instance.
                 */
                inline Mapping(
                        const Mapping& other
                    ):ServerSet(
                        other
                    ),currentPrimaryServerId(
                        other.currentPrimaryServerId
                    ) {}

                /**
                 * Move constructor.
                 *
                 * \param[in] other The instance to assign to this instance.
                 */
                inline Mapping(
                        Mapping&& other
                    ):ServerSet(
                        other
                    ),currentPrimaryServerId(
                        other.currentPrimaryServerId
                    ) {}

                /**
                 * Method you can use to determine if this mapping is valid.
                 *
                 * \return Returns true if the mapping is valid.  Returns false if the mapping is invalid.
                 */
                inline bool isValid() const {
                    return currentPrimaryServerId != invalidServerId;
                }

                /**
                 * Method you can use to determine if this mapping is invalid.
                 *
                 * \return Returns true if the mapping is invalid.  Returns false if the mapping is valid.
                 */
                inline bool isInvalid() const {
                    return !isValid();
                }

                /**
                 * Method you can use to change the primary server ID.
                 *
                 * \param[in] newPrimaryServerId The new primary server ID.
                 */
                inline void setPrimaryServer(ServerId newPrimaryServerId) {
                    currentPrimaryServerId = newPrimaryServerId;
                }

                /**
                 * Method you can use to determine the primary server ID.
                 *
                 * \return Returns the primary server ID.
                 */
                inline ServerId primaryServerId() const {
                    return currentPrimaryServerId;
                }

                /**
                 * Assignment operator.
                 *
                 * \param[in] other The instance to assign to this instance.
                 *
                 * \return Returns a reference to this instance.
                 */
                inline Mapping& operator=(const Mapping& other) {
                    ServerSet::operator=(other);
                    currentPrimaryServerId = other.currentPrimaryServerId;

                    return *this;
                }

                /**
                 * Assignment operator (move semantics).
                 *
                 * \param[in] other The instance to assign to this instance.
                 *
                 * \return Returns a reference to this instance.
                 */
                inline Mapping& operator=(Mapping&& other) {
                    ServerSet::operator=(other);
                    currentPrimaryServerId = other.currentPrimaryServerId;

                    return *this;
                }

            private:
                /**
                 * The ID of the primary server.
                 */
                ServerId currentPrimaryServerId;
        };

        /**
         * Type used to represent a mapping by customer ID.
         */
        typedef QHash<CustomerId, Mapping> MappingsByCustomerId;

        /**
         * Constructor
         *
         * \param[in] databaseManager The database manager used to fetch information about a region.
         *
         * \param[in] parent          Pointer to the parent object.
         */
        CustomerMapping(DatabaseManager* databaseManager, QObject* parent = nullptr);

        ~CustomerMapping() override;

        /**
         * Method you can use to assign a mapping to a customer.
         *
         * \param[in] customerId The customer ID of the desired customer.
         *
         * \param[in] mapping    The mapping to assign to this customer.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool updateMapping(CustomerId customerId, const Mapping& mapping, unsigned threadId = 0);

        /**
         * Method you can use to obtain a customer mapping.
         *
         * \param[in] customerId The customer ID of the desired customer.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        Mapping mapping(CustomerId customerId, unsigned threadId = 0);

        /**
         * Method you can use to obtain a collection of mappings by customer ID.
         *
         * \param[in] serverId The server ID of the server we want mappings for.  An invalid server ID will return all
         *                     mappings for all servers.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        MappingsByCustomerId mappings(ServerId serverId = invalidServerId, unsigned threadId = 0);

    private:
        /**
         * The underlying database manager instance.
         */
        DatabaseManager* currentDatabaseManager;
};

#endif
