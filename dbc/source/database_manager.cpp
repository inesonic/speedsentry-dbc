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
* This file implements the \ref DatabaseManager class.
***********************************************************************************************************************/

#include <QCoreApplication>
#include <QAtomicInteger>
#include <QString>
#include <QSqlDatabase>
#include <QMutex>
#include <QMutexLocker>

#include "log.h"
#include "database_manager.h"

const QString                      DatabaseManager::defaultDatabaseDriver("QPSQL");
const unsigned short               DatabaseManager::defaultDatabasePort = 5432;
QAtomicInteger<unsigned long long> DatabaseManager::instanceCounter(0);

DatabaseManager::DatabaseManager() {}


DatabaseManager::~DatabaseManager() {}


QSqlDatabase DatabaseManager::getDatabase(const QString& instanceName) {
    QMutexLocker databaseMutexLocker(&databaseMutex);

    if (QSqlDatabase::contains(instanceName)) {
        QSqlDatabase::removeDatabase(instanceName);
    }

    QSqlDatabase database = QSqlDatabase::addDatabase(currentDatabaseDriver, instanceName);
    database.setHostName(currentDatabaseServer);
    database.setPort(currentDatabasePort);
    database.setDatabaseName(currentDatabaseName);

    bool ok = database.open(currentDatabaseUsername, currentDatabasePassword);
    if (!ok && database.isOpen()) {
        database.close();
    }

    return database;
}


QSqlDatabase DatabaseManager::getDatabase() {
    return getDatabase(QString("i") + QString::number(instanceCounter.fetchAndAddRelaxed(1)));
}


void DatabaseManager::closeAndRelease(QSqlDatabase& database) {
    if (database.isOpen()) {
        database.close();
    }
}


void DatabaseManager::setDatabaseConnectionSettings(
        const QString& databaseUsername,
        const QString& databasePassword,
        const QString& databaseName,
        const QString& databaseServer,
        unsigned short databasePort,
        const QString& databaseDriver
    ) {
    QMutexLocker databaseMutexLocker(&databaseMutex);

    currentDatabaseUsername = databaseUsername;
    currentDatabasePassword = databasePassword;
    currentDatabaseName     = databaseName;
    currentDatabaseServer   = databaseServer;
    currentDatabasePort     = databasePort;
    currentDatabaseDriver   = databaseDriver;
}
