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
* This header implements the \ref CustomerSecrets class.
***********************************************************************************************************************/

#include <QObject>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QHash>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <QRandomGenerator>

#include <cstdint>
#include <cstring>
#include <cmath>

#include <inextea.h>
#include <crypto_aes_cbc_encryptor.h>
#include <crypto_aes_cbc_decryptor.h>
#include <crypto_helpers.h>

#include "log.h"
#include "database_manager.h"
#include "cache.h"
#include "customer_secret.h"
#include "customer_secrets.h"

CustomerSecrets::CustomerSecrets(
        DatabaseManager*  databaseManager,
        const QByteArray& encryptionKey,
        const QByteArray& customerIdentifierKey,
        unsigned          maximumCacheDepth,
        QObject*          parent
    ):QObject(
        parent
    ),Cache<CustomerSecret, CustomerSecret::CustomerId>(
        maximumCacheDepth
    ),currentDatabaseManager(
        databaseManager
    ) {
    setEncryptionKeys(encryptionKey);
    setCustomerIdentifierKey(customerIdentifierKey);
}


CustomerSecrets::~CustomerSecrets() {
    std::memset(encryptionKey, 0, sizeof(encryptionKey));
    std::memset(customerIdentifierKey, 0, sizeof(customerIdentifierKey));
}


void CustomerSecrets::setEncryptionKeys(const QByteArray& newKeys) {
    std::memcpy(encryptionKey, newKeys.data(), sizeof(encryptionKey));
}


void CustomerSecrets::setCustomerIdentifierKey(const QByteArray& newKeys) {
    std::memcpy(customerIdentifierKey, newKeys.data(), sizeof(customerIdentifierKey));
}


CustomerSecret CustomerSecrets::getCustomerSecret(CustomerId customerId, bool noCacheUpdate, unsigned threadId) {
    CustomerSecret result;

    cacheMutex.lock();
    const CustomerSecret* cacheResult = getCacheEntry(customerId);
    if (cacheResult == nullptr) {
        cacheMutex.unlock();
        QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
        bool success = database.isOpen();
        if (success) {
            QSqlQuery query(database);
            query.setForwardOnly(true);

            QString queryString = QString("SELECT * FROM customer_secrets WHERE customer_id = %1")
                                  .arg(customerId);

            success = query.exec(queryString);
            if (success) {
                if (query.first()) {
                    int fieldNumber = query.record().indexOf("secret");
                    if (fieldNumber >= 0) {
                        QByteArray encryptedSecret = query.value(fieldNumber).toByteArray();
                        Crypto::AesCbcDecryptor decryptor(
                            *reinterpret_cast<const Crypto::AesCbcDecryptor::Keys*>(encryptionKey),
                            *reinterpret_cast<const Crypto::AesCbcDecryptor::IV*>(encryptedSecret.data())
                        );

                        QByteArray decryptedSecret = decryptor.decrypt(
                            encryptedSecret.mid(Crypto::AesCbcDecryptor::ivLength)
                        );

                        result = CustomerSecret(customerId, decryptedSecret);
                        if (!noCacheUpdate) {
                            cacheMutex.lock();
                            addToCache(result);
                            cacheMutex.unlock();
                        }
                    } else {
                        logWrite(
                            QString("Failed to get field index - CustomerSecrets::getCustomerSecret: %1")
                            .arg(query.lastError().text()),
                            true
                        );
                    }
                }
            } else {
                logWrite(
                    QString("Failed SELECT - CustomerSecrets::getCustomerSecret: %1")
                    .arg(query.lastError().text()),
                    true
                );
            }
        } else {
            logWrite(
                QString("Failed to open database - CustomerSecrets::getCustomerSecret: %1")
                .arg(database.lastError().text()),
                true
            );
        }

        currentDatabaseManager->closeAndRelease(database);
    } else {
        result = *cacheResult;
        cacheMutex.unlock();
    }

    return result;
}


bool CustomerSecrets::deleteCustomerSecret(CustomerId customerId, unsigned threadId) {
    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        QSqlQuery query(database);

        QString queryString = QString("DELETE FROM customer_secrets WHERE customer_id = %1")
                .arg(customerId);
        success = query.exec(queryString);

        if (!success) {
            logWrite(
                QString("Failed DELETE - CustomerSecrets::deleteCustomerSecret: %1")
                .arg(query.lastError().text()),
                true
            );
        }
    } else {
        logWrite(
            QString("Failed to open database - CustomerSecrets::deleteCustomerSecret: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);

    cacheMutex.lock();
    evictCacheEntry(customerId);
    cacheMutex.unlock();

    return success;
}


CustomerSecret CustomerSecrets::updateCustomerSecret(CustomerId customerId, unsigned threadId) {
    CustomerSecret result = getCustomerSecret(customerId, true);

    QSqlDatabase database = currentDatabaseManager->getDatabase(QString::number(threadId));
    bool success = database.isOpen();
    if (success) {
        bool doUpdate = result.isValid();
        if (!doUpdate) {
            result = CustomerSecret(customerId, QByteArray());
        }

        result.generateNewSecret();

        QByteArray plainSecret = result.padddedCustomerSecret();
        QByteArray iv          = Crypto::generateRandomArray(Crypto::AesCbcEncryptor::ivLength);

        Crypto::AesCbcEncryptor encryptor(
            *reinterpret_cast<const Crypto::AesCbcEncryptor::Keys*>(encryptionKey),
            *reinterpret_cast<const Crypto::AesCbcEncryptor::IV*>(iv.data())
        );
        QByteArray encryptedSecret = iv + encryptor.encrypt(plainSecret);

        QSqlQuery query(database);
        if (doUpdate) {
            QString queryString = QString(
                "UPDATE customer_secrets SET secret = :encrypted_secret WHERE customer_id = :customer_id"
            );

            success = query.prepare(queryString);
            if (success) {
                query.bindValue(":customer_id", customerId);
                query.bindValue(":encrypted_secret", encryptedSecret);

                success = query.exec();
                if (!success) {
                    logWrite(
                        QString("Failed UPDATE - CustomerSecrets::updateCustomerSecret: %1")
                        .arg(query.lastError().text()),
                        true
                    );
                }
            } else{
                logWrite(
                    QString("Failed prepare UPDATE - CustomerSecrets::updateCustomerSecret: %1")
                    .arg(query.lastError().text()),
                    true
                );
            }
        } else {
            QString queryString = QString("INSERT INTO customer_secrets VALUES (:customer_id, :encrypted_secret)");
            success = query.prepare(queryString);
            if (success) {
                query.bindValue(":customer_id", customerId);
                query.bindValue(":encrypted_secret", encryptedSecret);

                success = query.exec();
                if (!success) {
                    logWrite(
                        QString("Failed INSERT - CustomerSecrets::updateCustomerSecret: %1")
                        .arg(query.lastError().text()),
                        true
                    );
                }
            } else {
                logWrite(
                    QString("Failed to prepare INSERT - CustomerSecrets::updateCustomerSecret: %1")
                    .arg(query.lastError().text()),
                    true
                );
            }
        }
    } else {
        logWrite(
            QString("Failed to open database - CustomerSecrets::updateCustomerSecret: %1")
            .arg(database.lastError().text()),
            true
        );
    }

    currentDatabaseManager->closeAndRelease(database);

    if (success) {
        cacheMutex.lock();
        addToCache(result);
        cacheMutex.unlock();
    } else {
        result = CustomerSecret();
    }

    return result;
}



std::uint64_t CustomerSecrets::toCustomerIdentifier(CustomerSecrets::CustomerId customerId) const {
    // Note: Assumes a little-endian architecture.
    union {
        std::uint64_t  v;
        IneXtea::Block block;
    } u;

    IneXtea::toCustomerIdentifier(u.block, customerId, customerIdentifierKey);
    return u.v;
}


CustomerSecrets::CustomerId CustomerSecrets::toCustomerId(std::uint64_t customerIdentifier) const {
    // Note: Assumes a little-endian architecture.
    union {
        std::uint64_t  v;
        IneXtea::Block block;
    } u;

    u.v = customerIdentifier;
    return static_cast<CustomerId>(IneXtea::toCustomerId(u.block, customerIdentifierKey));
}


CustomerSecrets::CustomerId CustomerSecrets::idFromValue(const CustomerSecret& value) const {
    return value.customerId();
}
