/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref DatabaseManager class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <QObject>
#include <QAtomicInteger>
#include <QString>
#include <QByteArray>
#include <QMutex>
#include <QSqlDatabase>

/**
 * A database manager API you can use to create database connection instances.
 */
class DatabaseManager:public QObject {
    public:
        /**
         * The default database driver.
         */
        static const QString defaultDatabaseDriver;

        /**
         * The default database port.
         */
        static const unsigned short defaultDatabasePort;

        DatabaseManager();

        ~DatabaseManager() override;

        /**
         * Method you can use to obtain a new database instance.  The database instance will be opened and prepared for
         * use.
         *
         * \param[in] instanceName The name to assign to this instance.
         *
         * \return Returns the opened database instance will be closed if an error occurs.
         */
        QSqlDatabase getDatabase(const QString& instanceName);

        /**
         * Method you can use to obtain a new database instance.  The database instance will be opened and prepared for
         * use.  This version will create a new database with a unique instance name.
         *
         * \return Returns the opened database instance will be closed if an error occurs.
         */
        QSqlDatabase getDatabase();

        /**
         * Method that closes a database, if open, and releases the connection.
         *
         * \param[in] database The database to be closed and released.
         */
        void closeAndRelease(QSqlDatabase& database);

    public slots:
        /**
         * Slot you can use to set the database connection parameters.  Values will be used for all new database
         * connections.
         *
         * \param[in] databaseUsername The database username.
         *
         * \param[in] databasePassword The database password.
         *
         * \param[in] databaseName     The name of the database we are to write into.
         *
         * \param[in] databaseServer   The database server name.
         *
         * \param[in] databasePort     The database port number.
         *
         * \param[in] databaseDriver   The default database driver.
         */
        void setDatabaseConnectionSettings(
            const QString& databaseUsername,
            const QString& databasePassword,
            const QString& databaseName,
            const QString& databaseServer,
            unsigned short databasePort = defaultDatabasePort,
            const QString& databaseDriver = defaultDatabaseDriver
        );

    private:
        /**
         * Counter used to create new, unique database instances.
         */
        static QAtomicInteger<unsigned long long> instanceCounter;

        /**
         * Mutex used to make certain settings updates occur atomically.
         */
        QMutex databaseMutex;

        /**
         * The current database username.
         */
        QString currentDatabaseUsername;

        /**
         * The current database password.
         */
        QString currentDatabasePassword;

        /**
         * The current database name.
         */
        QString currentDatabaseName;

        /**
         * The current database server name.
         */
        QString currentDatabaseServer;

        /**
         * The database port number.
         */
        unsigned short currentDatabasePort;

        /**
         * The current database driver.
         */
        QString currentDatabaseDriver;
};

#endif
