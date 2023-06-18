/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref Servers class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef SERVERS_H
#define SERVERS_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QSqlQuery>

#include <cstdint>

#include "region.h"
#include "server.h"
#include "sql_helpers.h"

class QTimer;
class DatabaseManager;

/**
 * Class used to read and write information about a server.  This cless expects a table named "server" with the
 * following definition:
 *
 *     CREATE TABLE servers (
 *         server_id            SMALLINT NOT NULL AUTO_INCREMENT,
 *         region_id            SMALLINT NOT NULL,
 *         identifier           VARCHAR(48) NOT NULL,
 *         status               ENUM('UNKNOWN','ACTIVE','INACTIVE','DEFUNCT') NOT NULL DEFAULT 'UNKNOWN',
 *         monitor_service_rate FLOAT DEFAULT NULL,
 *         cpu_loading          FLOAT DEFAULT NULL,
 *         memory_loading       FLOAT DEFAULT NULL
 *         PRIMARY KEY (server_id, address),
 *         FOREIGN KEY region_id REFERENCES regions (region_id) ON DELETE CASCADE ON UPDATE NO ACTION
 *     ) ENGINE=InnoDB DEFAULT CHARSET=latin1
 */
class Servers:public QObject, private SqlHelpers {
    Q_OBJECT

    public:
        /**
         * Type used to represent a region ID.
         */
        typedef Region::RegionId RegionId;

        /**
         * Type used to represent a server ID.
         */
        typedef Server::ServerId ServerId;

        /**
         * Type used to represent server status.
         */
        typedef Server::Status Status;

        /**
         * Type used to represent a collection of servers.
         */
        typedef QList<Server> ServerList;

        /**
         * Type used to access servers by server ID.
         */
        typedef QHash<ServerId, Server> ServersById;

        /**
         * Constructor
         *
         * \param[in] databaseManager The database manager used to fetch information about a region.
         *
         * \param[in] parent          Pointer to the parent object.
         */
        Servers(DatabaseManager* databaseManager, QObject* parent = nullptr);

        ~Servers() override;

        /**
         * Method you can use to get a server by server ID.
         *
         * \param[in] serverId The server ID of the desired server.
         *
         * \param[in] threadId An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns the \ref Server instance for the server.  An invalid server instance is returned if the
         *         server ID is not valid.
         */
        Server getServer(ServerId serverId, unsigned threadId = 0) const;

        /**
         * Method you can use to get a server by identifier.
         *
         * \param[in] identifier The server identifier.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns the \ref Server instance for the server.  An invalid server instance is returned if the
         *         server ID is not valid.
         */
        Server getServer(const QString& identifier, unsigned threadId = 0) const;

        /**
         * Method you can use to add a new server to the database.
         *
         * \param[in] regionId   The ID used to identify the region where this server resides.
         *
         * \param[in] identifier The IPv4 or IPv6 address of this server.
         *
         * \param[in] status     The reported server status.
         *
         * \param[in] server     A descriptive text for the new region.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns the newly created \ref Server instance.  An invalid server instance is returned if the
         *         server could not be created.
         */
        Server createServer(RegionId regionId, const QString& identifier, Status status, unsigned threadId = 0);

        /**
         * Method you can use to update a server in the database.
         *
         * \param[in] server   The server instance to be updated.
         *
         * \param[in] threadId An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool modifyServer(const Server& server, unsigned threadId = 0);

        /**
         * Method you can use to delete a server from the database.
         *
         * \param[in] server   The server to be deleted.
         *
         * \param[in] threadId An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool deleteServer(const Server& server, unsigned threadId = 0);

        /**
         * Method you can use to obtain all the known servers in a given region.
         *
         * \param[in] regionId The region ID of the region of interest.  A value of 0 will request all regions.
         *
         * \param[in] status   Optional status to apply to the request.
         *
         * \param[in] threadId An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns a hash of servers by server ID.
         */
        ServerList getServers(
            RegionId regionId = Region::invalidRegionId,
            Status   status = Status::ALL_UNKNOWN,
            unsigned threadId = 0
        );

        /**
         * Method you can use to obtain all the known servers by server ID.
         *
         * \param[in] threadId An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns a hash of servers by server ID.
         */
        ServersById getServersById(unsigned threadId = 0);

    private:
        /**
         * Method that converts a query to a server instance.
         *
         * \param[in]  sqlQuery THe SQL query to convert.
         *
         * \param[out] success  A pointer to a boolean value that can optionally be pupulated with True on success or
         *                      false on error.
         */
        static Server convertQueryToServer(const QSqlQuery& sqlQuery, bool* success = nullptr);

        /**
         * The underlying database manager instance.
         */
        DatabaseManager* currentDatabaseManager;
};

#endif
