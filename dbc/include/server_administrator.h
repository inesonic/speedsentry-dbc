/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref ServerAdministrator class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef SERVER_ADMINISTRATOR_H
#define SERVER_ADMINISTRATOR_H

#include <QObject>
#include <QString>
#include <QMutex>
#include <QHash>
#include <QSet>
#include <QMap>
#include <QJsonObject>

#include "server.h"
#include "servers.h"
#include "region.h"
#include "monitor.h"
#include "host_scheme.h"
#include "monitors.h"
#include "host_schemes.h"
#include "customer_capabilities.h"
#include "customer_mapping.h"

class Servers;
class Regions;
class Monitors;
class HostSchemes;
class CustomersCapabilities;
class OutboundRestApiFactory;

/**
 * Class that administrates our polling servers.
 */
class ServerAdministrator:public QObject {
    Q_OBJECT

    public:
        /**
         * Value used to indicate an invalid server ID.
         */
        static constexpr Server::ServerId invalidServerId = 0;

        /**
         * Type used to represent a region ID.
         */
        typedef Region::RegionId RegionId;

        /**
         * Type used to represent a server ID.
         */
        typedef Server::ServerId ServerId;

        /**
         * Type used to represent a customer ID.
         */
        typedef CustomerCapabilities::CustomerId CustomerId;

        /**
         * Type used to represent server status.
         */
        typedef Server::Status Status;

        /**
         * Type used to represent a collection of servers.
         */
        typedef Servers::ServerList ServerList;

        /**
         * Type used to access servers by server ID.
         */
        typedef Servers::ServersById ServersById;

        /**
         * Type used to represent a server exclusion list.
         */
        typedef QSet<ServerId> ServerExclusionList;

        /**
         * Type used to represent a list of customers.
         */
        typedef QList<CustomerId> CustomerList;

        /**
         * Constructor
         *
         * \param[in] serverDatabaseApi                Class used to manage servers entries in the database.
         *
         * \param[in] regionDatabaseApi                Class used to manage servers entries in the database.
         *
         * \param[in] monitorDatabaseApi               Class used to manage monitor data.
         *
         * \param[in] hostSchemeDatabaseApi            Class used to manage host/scheme data.
         *
         * \param[in] customersCapabilitiesDatabaseApi Class used to track customer capabilities.
         *
         * \param[in] customerMappingDatabaseApi       Class used to track which servers are servicing a customer.
         *
         * \param[in] outboundRestApiFactory           The outbound REST API factory used for C&C functions.
         *
         * \param[in] parent                           Pointer to the parent object.
         */
        ServerAdministrator(
            Servers*                serverDatabaseApi,
            Regions*                regionDatabaseApi,
            Monitors*               monitorDatabaseApi,
            HostSchemes*            hostSchemesDatabaseApi,
            CustomersCapabilities*  customersCapabilitiesDatabaseApi,
            CustomerMapping*        customerMappingDatabaseApi,
            OutboundRestApiFactory* outboundRestApiFactory,
            QObject*                parent = nullptr
        );

        ~ServerAdministrator() override;

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
        Server getServer(ServerId serverId, unsigned threadId = 0);

        /**
         * Method you can use to get a server by address.
         *
         * \param[in] identifier The server IPv4 or IPv6 address.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns the \ref Server instance for the server.  An invalid server instance is returned if the
         *         server ID is not valid.
         */
        Server getServer(const QString& address, unsigned threadId = 0);

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
        const ServersById& getServersById(unsigned threadId = 0);

    public slots:
        /**
         * Slot you can use to add a new server.  This method will create a server that is inactive and then
         * command that server to go inactive.
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
         * Method you can use to update a server in the database and local cache.  This method does not command the
         * server.
         *
         * \param[in] server   The server instance to be updated.
         *
         * \param[in] threadId An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool modifyServer(const Server& server, unsigned threadId = 0);

        /**
         * Method you can use to delete a server from the database.  The server can only be deleted if it's status is
         * defunct.
         *
         * \param[in] serverId The ID of the server to be deleted.
         *
         * \param[in] threadId An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool deleteServer(ServerId serverId, unsigned threadId = 0);

        /**
         * Slot you can use to trigger all servers marked in the database as active to go active.  This will also
         * update the region index for every server.
         *
         * \param[in] threadId An optional thread ID used to maintain independent per-thread database instances.
         */
        void sendGoActive(unsigned threadId = 0);

        /**
         * Slot you can use to change the server status.  Calling this method will trigger servers state information
         * to be updated.  If the number of active regions changes, then all active servers will be updated with the
         * new information.
         *
         * \param[in] serverId  The zero based ID of the server being activated.
         *
         * \param[in] newStatus The new server status.
         *
         * \param[in] threadId  An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool changeServerStatus(ServerId serverId, Status newStatus, unsigned threadId = 0);

        /**
         * Slot you can use to start a server and push assigned customers to it.
         *
         * \param[in] serverId  The zero based ID of the server being activated.
         *
         * \param[in] threadId  An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool startServer(ServerId serverId, unsigned threadId = 0);

        /**
         * Slot you can use to update polling servers to support a customer.
         *
         * \param[in] customerId The zero based ID of the customer to update.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool activateCustomer(CustomerId customerId, unsigned threadId);

        /**
         * Slot you can use to remove a customer from the polling servers.
         *
         * \param[in] customerId The zero based ID of the customer to update.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns a textual description of a failure reason or an empty string if no failure occurred.
         */
        bool deactivateCustomer(CustomerId customerId, unsigned threadId);

        /**
         * Slot you can use to pause or resume a customer.
         *
         * \param[in] customerId The ID of the customer to be paused or resumed.
         *
         * \param[in] nowPaused  If true, the customer will be paused.  If false, the customer will be resumed.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool setPaused(CustomerId customerId, bool nowPaused, unsigned threadId);

        /**
         * Slot you can use to move all work from one server to another server and then optionally mark the server as
         * inactive.
         *
         * \param[in] fromServerId The server to move work from.  The server will be marked inactive at the start of
         *                         this process.
         *
         * \param[in] customers    A list of customers to be reassigned.  An empty list indicates all customers tied
         *                         to the server should be reassigned.  An empty list will also cause the server to
         *                         go inactive.
         *
         * \param[in] toServerId   The server to move work to.  An invalid server ID will cause the work to be
         *                         redistributed across all other servers in the same region.  Note that this server
         *                         should be inactive or defunct.
         *
         * \param[in] threadId     An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool reassignWorkload(
            ServerId            fromServerId,
            const CustomerList& customers = CustomerList(),
            ServerId            toServerId = invalidServerId,
            unsigned            threadId = 0
        );

    private:
        /**
         * Type used to track servers by region.  We use a map to impose consistent ordering of regions.
         */
        typedef QMap<RegionId, ServersById> ServersByServerIdByRegionId;

        /**
         * Type used to map a host address to a server.
         */
        typedef QHash<QString, ServerId> ServerIdsByIdentifier;

        /**
         * The endpoint used to make a polling server inactive.
         */
        static const QString pollingServerStateInactiveEndpoint;

        /**
         * The endpoint used to make a polling server inactive.
         */
        static const QString pollingServerRegionChangeEndpoint;

        /**
         * The endpoint used to change a customer's paused status.
         */
        static const QString pollingServerCustomerPauseEndpoint;

        /**
         * Method you can use to assign a collection of servers to this customer.  Unless told otherwise, the method
         * will attempt to reuse existing servers as much as possible.
         *
         * This method assumes the cache has been loaded and that we currently have a lock on our internal data
         * structures.
         *
         * \param[in] customerId     The customer ID of the customer.
         *
         * \param[in] ignoreExisting If true, then any existing assignments will be ignored.
         *
         * \param[in] multiRegion    If true, then this customer should have a multi-region configuration.
         *
         * \param[in] exclusionList  A list of servers to be excluded.
         *
         * \param[in] threadId       An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns a tuple containing the updated mapping and the server ID of any servers removed from the
         *         mapping.
         */
        QPair<CustomerMapping::Mapping, CustomerMapping::ServerSet> assignServersToCustomer(
            CustomerId                 customerId,
            bool                       ignoreExisting,
            bool                       multiRegion,
            const ServerExclusionList& exclusionList = ServerExclusionList(),
            unsigned                   threadId = 0
        );

        /**
         * Method that identifies the least loaded server in a collection of servers.
         *
         * This method assumes the cache has been loaded and that we currently have a lock on our internal data
         * structures.
         *
         * \param[in] servers       Servers to be queried.  Only active servers will be considered.
         *
         * \param[in] exclusionList A list of servers to be excluded.
         *
         * \return Returns the least loaded server in the set.
         */
        static Server leastLoadedServer(const ServersById& servers, const ServerExclusionList& exclusionList);

        /**
         * Method that is used to update our local hashes from the database.
         *
         * \param[in] threadId An optional thread ID used to maintain independent per-thread database instances.
         */
        void updateLocalCache(unsigned threadId = 0);

        /**
         * Method that updates the region data for all active servers.
         */
        void globalUpdateRegionData();

        /**
         * Method used internally to modify a server's status.
         *
         * \param[in] server      The server instance to be updated.
         *
         * \param[in] threadId    The thread ID used to maintain independent per-thread database instances.
         *
         * \param[in] forceModify If true, always modify this server's status.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool modifyServerHelper(const Server& server, unsigned threadId, bool forceModify);

        /**
         * Method that adds a server to our local cache.  The server will be replaced if it already exists.
         *
         * \param[in] server The server to be added.
         */
        void addServer(const Server& server);

        /**
         * Method that removes a server from our local cache.
         *
         * \param[in] server The server to be removed.
         */
        void removeServer(const Server& server);

        /**
         * Method that gets a region table by server status.
         *
         * \param[in] status The server status.
         *
         * \return Returns a pointer to a hash table where this server should reside.
         */
        ServersByServerIdByRegionId* getRegionTable(Status status);

        /**
         * Method you can use to add a server to a region table.
         *
         * \param[in] server The server to be removed.
         */
        void addToRegionTable(const Server& server);

        /**
         * Method you can use to remove a server from a region table.
         *
         * \param[in] server The server to be removed.
         */
        void removeFromRegionTable(const Server& server);

        /**
         * Method that commands a server to go inactive.
         *
         * \param[in] server The server to receive the command.
         */
        void sendGoInactive(const Server& server);

        /**
         * Method that commands a server to go active.  This method will also update the number of regions and the
         * server's region index.
         *
         * \param[in] server        The server to receive the command.
         *
         * \param[in] regionIndex   The zero based region index.
         *
         * \param[in] numberRegions The number of regions.
         */
        void sendGoActive(const Server& server, unsigned regionIndex, unsigned numberRegions);

        /**
         * Method that builds a customer polling server update message.
         *
         * \param[in] pollingInterval              The per-server polling interval
         *
         * \param[in] supportsPingTesting          If true, the message should be marked as supporting ping testing.
         *
         * \param[in] supportsSslExpirationTesting If true, the message should be marked as supporting SSL expiration
         *                                         testing.
         *
         * \param[in] multiRegion                  If true, the message should be marked to indicate that this customer
         *                                         is checked across multiple regions.
         *
         * \param[in] supportsLatencyMeasurements  If true, the message should be marked as requiring latency
         *                                         reporting.
         *
         * \param[in] hostSchemesById              A hash table of host schemes by host/scheme ID.
         *
         * \param[in] monitorsById                 A hash table of monitors by monitor ID.
         *
         * \return Returns a QJsonObject holding the settings to report to the remote server.
         */
        static QJsonObject buildCustomerMessage(
            unsigned                           pollingInterval,
            bool                               supportsPingTesting,
            bool                               supportsSslExpirationTesting,
            bool                               multiRegion,
            bool                               supportsLatencyMeasurements,
            const HostSchemes::HostSchemeHash& hostSchemesById,
            const Monitors::MonitorsById&      monitorsById
        );

        /**
         * Method that deactivates a customer across a set of servers.
         *
         * \param[in] customerId The customer ID of the customer to be deactivated.
         *
         * \param[in] servers    The set of servers to perform the deactivation on.
         */
        void applyCustomerDeactivation(CustomerId customerId, const CustomerMapping::ServerSet& servers);

        /**
         * Method that updates server settings for a customer.
         *
         * \param[in] mapping        The customer mapping to be employed.
         *
         * \param[in] removedServers Servers to remove the customer from.
         *
         * \param[in] limitToServers A set of server IDs to limit our activation to.  An empty set will be interpreted
         *                           as allow all servers.  This value is not applied to the set of removed servers.
         *
         * \param[in] capabilities   The customer capabilities to apply.
         *
         * \param[in] threadId       The thread ID used to maintain independent per-thread database instances.
         */
        void applyCustomerActivation(
            const CustomerMapping::Mapping&   mapping,
            const CustomerMapping::ServerSet& removedServers,
            const CustomerMapping::ServerSet& limitToServers,
            const CustomerCapabilities&       capabilities,
            unsigned                          threadId
        );

        /**
         * Our server database API.
         */
        Servers* currentServers;

        /**
         * Our regions database API.
         */
        Regions* currentRegions;

        /**
         * Our monitors database API.
         */
        Monitors* currentMonitors;

        /**
         * Our host/schemes database API.
         */
        HostSchemes* currentHostSchemes;

        /**
         * Our customer capabilities database API.
         */
        CustomersCapabilities* currentCustomerCapabilities;

        /**
         * Our customer mapping database Api;
         */
        CustomerMapping* currentMapping;

        /**
         * Our outbound REST API.
         */
        OutboundRestApiFactory* currentOutboundRestApiFactory;

        /**
         * Mutex used to manage access to the services provided by this class.
         */
        QMutex accessMutex;

        /**
         * Flag indicating if we've loaded server information from our database.
         */
        bool loadNeeded;

        /**
         * Hash table of all servers by server ID.
         */
        ServersById serversById;

        /**
         * Hash table used to map a server to a host address.
         */
        ServerIdsByIdentifier serverIdsByIdentifier;

        /**
         * Hash table of active servers by region.
         */
        ServersByServerIdByRegionId activeServersByServerIdByRegionId;

        /**
         * Hash table of inactive servers by region.
         */
        ServersByServerIdByRegionId inactiveServersByServerIdByRegionId;

        /**
         * Hash table of defunct servers by region.
         */
        ServersByServerIdByRegionId defunctServersByServerIdByRegionId;

        /**
         * Hash of region IDs by region index.
         */
        QHash<RegionId, unsigned> regionIndexByRegionIds;
};

#endif
