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
* This header defines the \ref CustomerSecrets class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef CUSTOMER_SECRETS_H
#define CUSTOMER_SECRETS_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QSqlQuery>
#include <QLinkedList>
#include <QMutex>

#include <cstdint>
#include <inextea.h>

#include "sql_helpers.h"
#include "customer_secret.h"
#include "cache.h"

class QTimer;
class DatabaseManager;

/**
 * Class used to manage REST API secrets used by customers.  This cless expects a table named
 * "customer_secrets" with the following definition:
 *
 *     CREATE TABLE customer_secrets (
 *         customer_id BIGINT NOT NULL,
 *         secret      BINARY(80) NOT NULL,
 *         KEY customer_secrets_constraint_1 (customer_id),
 *         CONSTRAINT customer_secrets_constraint_1 FOREIGN KEY (customer_id)
 *             REFERENCES customer_capabilities (customer_id)
 *             ON DELETE CASCADE
 *             ON UPDATE NO ACTION
 *     ) ENGINE=InnoDB DEFAULT CHARSET=latin1
 *
 * The encrypted secret contains the 16 byte IV followed by the encrypted secret itself.
 *
 * Note that, for performance reasons, this class maintains an cache of customer secrets with random eviction.
 */
class CustomerSecrets:public QObject,
                      public Cache<CustomerSecret, CustomerSecret::CustomerId>,
                      private SqlHelpers {
    Q_OBJECT

    public:
        /**
         * Type used to represent a customer ID.
         */
        typedef CustomerSecret::CustomerId CustomerId;

        /**
         * Value representing the default cache depth.
         */
        static constexpr unsigned defaultCacheDepth = 10000;

        /**
         * Constructor
         *
         * \param[in] databaseManager       The database manager used to fetch information about a region.
         *
         * \param[in] encryptionKey         The encryption key used to encrypt customer secrets at rest.
         *
         * \param[in] customerIdentifierKey Key used to generate customer identifiers.
         *
         * \param[in] maximumCacheDepth     The maximum allowed cache depth.
         *
         * \param[in] parent                Pointer to the parent object.
         */
        CustomerSecrets(
            DatabaseManager*  databaseManager,
            const QByteArray& encryptionKey,
            const QByteArray& customerIdentifierKey,
            unsigned          maximumCacheDepth = defaultCacheDepth,
            QObject*          parent = nullptr
        );

        ~CustomerSecrets() override;

        /**
         * Method you can use to change the AES encryption keys on the fly.
         *
         * \param[in] newKeys  The new AES encryption keys.
         */
        void setEncryptionKeys(const QByteArray& newKeys);

        /**
         * Method you can use to change the XTEA customer identifier key on the fly.
         *
         * \param[in] newKeys  The new customer identifier key.
         */
        void setCustomerIdentifierKey(const QByteArray& newKeys);

        /**
         * Method you can use to get a customer secret for a customer.
         *
         * \param[in] customerId    The ID of the customer to get the secret for.
         *
         * \param[in] noCacheUpdate If true, then reading this value will not change the cache state.
         *
         * \param[in] threadId      An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns the customer secret.  An invalid secret is returned if the customer has no secret.
         */
        CustomerSecret getCustomerSecret(CustomerId customerId, bool noCacheUpdate = false, unsigned threadId = 0);

        /**
         * Method you can use to delete a customer secret.
         *
         * \param[in] customerId The ID of the customer delete.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success. Returns false on error.
         */
        bool deleteCustomerSecret(CustomerId customerId, unsigned threadId = 0);

        /**
         * Method you can use to update a customer secret.
         *
         * \param[in] customerSecret The customer secret to be updated.
         *
         * \param[in] threadId       An optional thread ID used to maintain independent per-thread database instances.
         */
        CustomerSecret updateCustomerSecret(CustomerId customerId, unsigned threadId = 0);

        /**
         * Method that generates a customer identifier from a customer ID.
         *
         * \param[in] customerId The customer ID to convert.
         *
         * \return Returns the customer identifier as a 64-bit value.
         */
        std::uint64_t toCustomerIdentifier(CustomerId customerId) const;

        /**
         * Method that generates a customer ID from a customer identifier.
         *
         * \param[in] customerIdentifier The customer identifier to convert.
         *
         * \return Returns the customer ID.  A value of 0 is returned if the customer identifier is invalid.
         */
        CustomerId toCustomerId(std::uint64_t customerIdentifier) const;

    protected:
        /**
         * Method that obtains the ID used to access a specific value.
         *
         * \param[in] value The value to calculate the ID for.
         *
         * \return Returns the ID to associate with this value.
         */
        CustomerId idFromValue(const CustomerSecret& value) const final;

    private:
        /**
         * The underlying database manager instance.
         */
        DatabaseManager* currentDatabaseManager;

        /**
         * The AES-CBC encryption key.
         */
        std::uint8_t encryptionKey[32];

        /**
         * The XTEA encryption key used to generate customer identifiers.
         */
        std::uint8_t customerIdentifierKey[16];

        /**
         * Mutex used to protect the cache.
         */
        QMutex cacheMutex;
};

#endif
