/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref ServerAdministrator class.
***********************************************************************************************************************/

#include <QObject>
#include <QString>
#include <QMutex>
#include <QMutexLocker>
#include <QHash>
#include <QJsonObject>
#include <QJsonArray>

#include "log.h"
#include "region.h"
#include "regions.h"
#include "server.h"
#include "servers.h"
#include "monitor.h"
#include "monitors.h"
#include "host_scheme.h"
#include "host_schemes.h"
#include "customer_capabilities.h"
#include "customers_capabilities.h"
#include "customer_mapping.h"
#include "outbound_rest_api_factory.h"
#include "server_administrator.h"

const QString ServerAdministrator::pollingServerStateInactiveEndpoint("/state/inactive");
const QString ServerAdministrator::pollingServerRegionChangeEndpoint("/region/change");
const QString ServerAdministrator::pollingServerCustomerPauseEndpoint("/customer/pause");

ServerAdministrator::ServerAdministrator(
        Servers*                serverDatabaseApi,
        Regions*                regionDatabaseApi,
        Monitors*               monitorDatabaseApi,
        HostSchemes*            hostSchemesDatabaseApi,
        CustomersCapabilities*  customersCapabilitiesDatabaseApi,
        CustomerMapping*        customerMappingDatabaseApi,
        OutboundRestApiFactory* outboundRestApiFactory,
        QObject*                parent
    ):QObject(
        parent
    ),currentServers(
        serverDatabaseApi
    ),currentRegions(
        regionDatabaseApi
    ),currentMonitors(
        monitorDatabaseApi
    ),currentHostSchemes(
        hostSchemesDatabaseApi
    ),currentCustomerCapabilities(
        customersCapabilitiesDatabaseApi
    ),currentMapping(
        customerMappingDatabaseApi
    ),currentOutboundRestApiFactory(
        outboundRestApiFactory
    ) {
    loadNeeded = true;
}


ServerAdministrator::~ServerAdministrator() {}


Server ServerAdministrator::getServer(ServerId serverId, unsigned threadId) {
    QMutexLocker locker(&accessMutex);

    if (loadNeeded) {
        updateLocalCache(threadId);
    }

    return serversById.value(serverId);
}


Server ServerAdministrator::getServer(const QString& identifier, unsigned threadId) {
    QMutexLocker locker(&accessMutex);

    if (loadNeeded) {
        updateLocalCache(threadId);
    }

    ServerId serverId = serverIdsByIdentifier.value(identifier, Server::invalidServerId);
    return serverId != Server::invalidServerId ? serversById.value(serverId) : Server();
}


ServerAdministrator::ServerList ServerAdministrator::getServers(
        RegionId regionId,
        Status   status,
        unsigned threadId
    ) {
    QMutexLocker locker(&accessMutex);

    if (loadNeeded) {
        updateLocalCache(threadId);
    }

    ServerList result;
    for (ServersById::const_iterator it=serversById.constBegin(),end=serversById.constEnd() ; it!=end ; ++it) {
        const Server& server = it.value();
        if (status == Status::ALL_UNKNOWN || server.status() == status) {
            if (regionId == Region::invalidRegionId || server.regionId() == regionId) {
                result.append(server);
            }
        }
    }

    return result;
}


const ServerAdministrator::ServersById& ServerAdministrator::getServersById(unsigned threadId) {
    QMutexLocker locker(&accessMutex);

    if (loadNeeded) {
        updateLocalCache(threadId);
    }

    return serversById;
}



Server ServerAdministrator::createServer(
        ServerAdministrator::RegionId regionId,
        const QString&                identifier,
        ServerAdministrator::Status   status,
        unsigned                      threadId
    ) {
    Server result;

    accessMutex.lock();

    if (loadNeeded) {
        updateLocalCache(threadId);
    }

    accessMutex.unlock();

    Region region = currentRegions->getRegion(regionId, threadId);
    if (region.isValid()) {
        result = currentServers->createServer(regionId, identifier, status, threadId);

        accessMutex.lock();
        serversById.insert(result.serverId(), result);
        addServer(result);
        accessMutex.unlock();

        sendGoInactive(result);
    } else {
        logWrite(QString("Invalid region ID %1 when creating server %2").arg(regionId).arg(identifier), true);
    }

    return result;
}


bool ServerAdministrator::modifyServer(const Server& server, unsigned threadId) {
    QMutexLocker locker(&accessMutex);

    if (loadNeeded) {
        updateLocalCache(threadId);
    }

    return modifyServerHelper(server, threadId, false);
}


bool ServerAdministrator::deleteServer(ServerId serverId, unsigned threadId) {
    bool success;

    QMutexLocker locker(&accessMutex);

    if (loadNeeded) {
        updateLocalCache(threadId);
    }

    Server   oldServer = serversById.value(serverId);
    if (oldServer.isValid() && oldServer.status() == Status::DEFUNCT) {
        success = currentServers->deleteServer(oldServer, threadId);
        if (success) {
            serversById.remove(serverId);
            serverIdsByIdentifier.remove(oldServer.identifier());
            removeFromRegionTable(oldServer);
        }
    } else {
        logWrite(QString("Attempt to delete server that is not defunct, server ID %1").arg(serverId), true);
        success = false;
    }

    return success;
}


void ServerAdministrator::sendGoActive(unsigned threadId) {
    QMutexLocker locker(&accessMutex);

    if (loadNeeded) {
        updateLocalCache(threadId);
    }

    globalUpdateRegionData();
}


bool ServerAdministrator::changeServerStatus(
        ServerAdministrator::ServerId serverId,
        ServerAdministrator::Status   newStatus,
        unsigned                      threadId
    ) {
    bool success;

    QMutexLocker locker(&accessMutex);

    if (loadNeeded) {
        updateLocalCache(threadId);
    }

    Server server = serversById.value(serverId);
    if (server.isValid() && server.status() != newStatus) {
        unsigned oldNumberActiveRegions = static_cast<unsigned>(activeServersByServerIdByRegionId.size());

        server.setStatus(newStatus);
        success = modifyServerHelper(server, threadId, true);

        if (success) {
            unsigned newNumberActiveRegions = static_cast<unsigned>(activeServersByServerIdByRegionId.size());
            bool     updatedServerStatus    = false;
            if (oldNumberActiveRegions != newNumberActiveRegions) {
                globalUpdateRegionData();
                updatedServerStatus = (newStatus == Status::ACTIVE);
            }

            if (!updatedServerStatus) {
                if (newStatus == Status::ACTIVE) {
                    RegionId regionId    = server.regionId();
                    unsigned regionIndex = regionIndexByRegionIds.value(
                        regionId,
                        std::numeric_limits<unsigned>::max()
                    );

                    if (regionIndex == std::numeric_limits<unsigned>::max()) {
                        globalUpdateRegionData();
                    } else {
                        sendGoActive(server, regionIndex, newNumberActiveRegions);
                    }
                } else if (newStatus == Status::INACTIVE) {
                    sendGoInactive(server);
                }
            }
        }
    } else {
        logWrite(
            QString("Attempt to change status of invalid server, server ID %1 to %2")
            .arg(serverId)
            .arg(Server::toString(newStatus)),
            true
        );
        success = false;
    }

    return success;
}


bool ServerAdministrator::startServer(ServerAdministrator::ServerId serverId, unsigned threadId) {
    bool success = true;

    QMutexLocker locker(&accessMutex);

    if (loadNeeded) {
        updateLocalCache(threadId);
    }

    Server server = serversById.value(serverId);
    if (server.isValid() && server.status() == Status::ACTIVE) {
        RegionId regionId    = server.regionId();
        unsigned regionIndex = regionIndexByRegionIds.value(regionId, static_cast<unsigned>(-1));
        if (regionIndex != static_cast<unsigned>(-1)) {
            CustomerMapping::MappingsByCustomerId mappings  = currentMapping->mappings(serverId, threadId);
            QList<CustomerId>                     customers = mappings.keys();

            CustomerMapping::ServerSet limitToServer;
            limitToServer.insert(serverId);

            QList<CustomerId>::const_iterator customerIterator    = customers.constBegin();
            QList<CustomerId>::const_iterator customerEndIterator = customers.constEnd();
            while (success && customerIterator != customerEndIterator) {
                CustomerId           customerId   = *customerIterator;
                CustomerCapabilities capabilities = currentCustomerCapabilities->getCustomerCapabilities(
                    customerId,
                    false,
                    threadId
                );

                if (capabilities.isValid()) {
                    CustomerMapping::Mapping mapping = currentMapping->mapping(customerId, threadId);
                    applyCustomerActivation(
                        mapping,
                        CustomerMapping::ServerSet(),
                        limitToServer,
                        capabilities,
                        threadId
                    );

                    ++customerIterator;
                } else {
                    success = false;
                }
            }

            if (success) {
                unsigned numberActiveRegions = static_cast<unsigned>(activeServersByServerIdByRegionId.size());
                sendGoActive(server, regionIndex, numberActiveRegions);

                logWrite(QString("Starting server %1.").arg(serverId), true);
            }
        } else {
            success = false;
        }
    } else {
        success = false;
    }

    return success;
}


bool ServerAdministrator::activateCustomer(ServerAdministrator::CustomerId customerId, unsigned threadId) {
    bool success;

    QMutexLocker locker(&accessMutex);

    if (loadNeeded) {
        updateLocalCache(threadId);
    }

    CustomerCapabilities capabilities = currentCustomerCapabilities->getCustomerCapabilities(
        customerId,
        false,
        threadId
    );

    if (capabilities.isValid()) {
        QPair<CustomerMapping::Mapping, CustomerMapping::ServerSet> mappingData = assignServersToCustomer(
            customerId,
            false,
            capabilities.multiRegionChecking(),
            ServerExclusionList(),
            threadId
        );

        const CustomerMapping::Mapping   newMapping        = mappingData.first;
        const CustomerMapping::ServerSet removeFromServers = mappingData.second;

        currentMapping->updateMapping(customerId, newMapping, threadId);
        applyCustomerActivation(newMapping, removeFromServers, CustomerMapping::ServerSet(), capabilities, threadId);

        success = true;
    } else {
        success = false;
    }

    return success;
}


bool ServerAdministrator::deactivateCustomer(ServerAdministrator::CustomerId customerId, unsigned threadId) {
    bool success;

    QMutexLocker locker(&accessMutex);

    if (loadNeeded) {
        updateLocalCache(threadId);
    }

    CustomerMapping::Mapping mapping = currentMapping->mapping(customerId, threadId);
    success = currentMapping->updateMapping(customerId, CustomerMapping::Mapping(), threadId);

    if (success) {
        applyCustomerDeactivation(customerId, mapping);
    }

    return success;
}


bool ServerAdministrator::setPaused(CustomerId customerId, bool nowPaused, unsigned threadId) {
    bool success = true;

    QMutexLocker locker(&accessMutex);

    if (loadNeeded) {
        updateLocalCache(threadId);
    }

    CustomerCapabilities capabilities = currentCustomerCapabilities->getCustomerCapabilities(
        customerId,
        false,
        threadId
    );

    if (capabilities.isValid() && capabilities.supportsMaintenanceMode()) {
        CustomerMapping::Mapping                 mapping           = currentMapping->mapping(customerId, threadId);
        CustomerMapping::Mapping::const_iterator serverIterator    = mapping.constBegin();
        CustomerMapping::Mapping::const_iterator serverEndIterator = mapping.constEnd();

        while (success && serverIterator != serverEndIterator) {
            ServersById::iterator serverInstance = serversById.find(*serverIterator);
            if (serverInstance != serversById.end()) {
                const Server&  server     = serverInstance.value();
                const QString& identifier = server.identifier();
                QJsonObject    message;

                message.insert("customer_id", static_cast<double>(customerId));
                message.insert("pause", nowPaused);

                currentOutboundRestApiFactory->postMessage(
                    identifier,
                    pollingServerCustomerPauseEndpoint,
                    QJsonDocument(message),
                    QString("Customer %1 pause state changed to %2").arg(customerId).arg(nowPaused ? "true" : "false")
                );

                ++serverIterator;
            } else {
                logWrite(
                    QString("Unable to locate server %1, while changing customer %2 pause state")
                    .arg(*serverIterator)
                    .arg(customerId),
                    true
                );
                success = false;
            }
        }

        if (success) {
            capabilities.setPaused(nowPaused);
            success = currentCustomerCapabilities->updateCustomerCapabilities(capabilities, threadId);
        }

        if (success) {
            if (nowPaused) {
                logWrite(QString("Paused customer %1").arg(customerId), false);
            } else {
                logWrite(QString("Resumed customer %1").arg(customerId), false);
            }
        }
    } else {
        success = false;
    }

    return success;
}


bool ServerAdministrator::reassignWorkload(
        ServerAdministrator::ServerId            fromServerId,
        const ServerAdministrator::CustomerList& customers,
        ServerAdministrator::ServerId            toServerId,
        unsigned                                 threadId
    ) {
    bool success = true;

    if (toServerId != invalidServerId) {
        Server toServer = getServer(toServerId, threadId);
        if (toServer.isInvalid() || toServer.status() == Server::Status::ACTIVE) {
            success = false;
        }
    }

    if (success) {
        Server       fromServer = getServer(fromServerId, threadId);
        CustomerList customersOnServer;

        if (customers.isEmpty()) {
            fromServer.setStatus(Server::Status::INACTIVE);
            success = modifyServer(fromServer, threadId);

            CustomerMapping::MappingsByCustomerId mappingsByCustomerId = currentMapping->mappings(fromServerId);
            customersOnServer = mappingsByCustomerId.keys();
        } else {
            customersOnServer = customers;
        }

        if (success) {
            unsigned long numberCustomers = static_cast<unsigned long>(customersOnServer.size());
            for (unsigned long customerIndex=0 ; customerIndex<numberCustomers ; ++customerIndex) {
                accessMutex.lock();

                CustomerId           customerId   = customersOnServer.at(customerIndex);
                CustomerCapabilities capabilities = currentCustomerCapabilities->getCustomerCapabilities(
                    customerId,
                    false,
                    threadId
                );

                if (capabilities.isValid()) {
                    CustomerMapping::Mapping   newMapping;
                    CustomerMapping::ServerSet removeFromServers;

                    if (toServerId != invalidServerId) {
                        QPair<CustomerMapping::Mapping, CustomerMapping::ServerSet>
                            mappingData = assignServersToCustomer(
                                customerId,
                                false,
                                capabilities.multiRegionChecking(),
                                ServerExclusionList(),
                                threadId
                        );

                        newMapping        = mappingData.first;
                        removeFromServers = mappingData.second;

                        if (newMapping.remove(fromServerId)) {
                            removeFromServers.insert(fromServerId);
                            newMapping.insert(toServerId);
                        }
                    } else {
                        QPair<CustomerMapping::Mapping, CustomerMapping::ServerSet>
                            mappingData = assignServersToCustomer(
                                customerId,
                                false,
                                capabilities.multiRegionChecking(),
                                ServerExclusionList() << fromServerId,
                                threadId
                        );

                        newMapping        = mappingData.first;
                        removeFromServers = mappingData.second;
                    }

                    currentMapping->updateMapping(customerId, newMapping, threadId);
                    applyCustomerActivation(
                        newMapping,
                        removeFromServers,
                        CustomerMapping::ServerSet(),
                        capabilities,
                        threadId
                    );

                    if (toServerId == invalidServerId) {
                        logWrite(QString("Moved customer %1 from server %2").arg(customerId).arg(fromServerId), false);
                    } else {
                        logWrite(
                            QString("Moved customer %1 from server %2 to %3")
                            .arg(customerId)
                            .arg(fromServerId)
                            .arg(toServerId),
                            false
                        );
                    }
                }

                accessMutex.unlock();
            }
        }
    }

    return success;
}


QPair<CustomerMapping::Mapping, CustomerMapping::ServerSet> ServerAdministrator::assignServersToCustomer(
        ServerAdministrator::CustomerId                 customerId,
        bool                                            ignoreExisting,
        bool                                            multiRegion,
        const ServerAdministrator::ServerExclusionList& exclusionList,
        unsigned                                        threadId
    ) {
    CustomerMapping::Mapping mapping =   ignoreExisting
                                       ? CustomerMapping::Mapping()
                                       : currentMapping->mapping(customerId, threadId);

    // Remove servers that are excluded, inactive, or redundant within the same region.  Also identify the least loaded
    // server in the customer's deployment fleet.

    CustomerMapping::Mapping::iterator mappingIterator    = mapping.begin();
    CustomerMapping::Mapping::iterator mappingEndIterator = mapping.end();

    QSet<RegionId>             assignedRegions;
    CustomerMapping::ServerSet removedServers;
    ServerId                   leastLoadedServerId = invalidServerId;
    float                      bestCpuLoading      = std::numeric_limits<float>::max();
    while (mappingIterator != mappingEndIterator) {
        ServerId      serverId = *mappingIterator;
        if (exclusionList.contains(serverId)) {
            removedServers.insert(serverId);
            mappingIterator = mapping.erase(mappingIterator);
        } else {
            const Server& server = serversById.value(*mappingIterator);
            if (server.status() != Server::Status::ACTIVE) {
                removedServers.insert(serverId);
                mappingIterator = mapping.erase(mappingIterator);
            } else {
                RegionId regionId = server.regionId();
                if (assignedRegions.contains(regionId)) {
                    removedServers.insert(serverId);
                    mappingIterator = mapping.erase(mappingIterator);
                } else {
                    assignedRegions.insert(server.regionId());

                    if (server.cpuLoading() < bestCpuLoading) {
                        leastLoadedServerId = serverId;
                        bestCpuLoading      = server.cpuLoading();
                    }
                }

                ++mappingIterator;
            }
        }
    }

    if (multiRegion) {
        unsigned numberActiveRegions = static_cast<unsigned>(activeServersByServerIdByRegionId.size());
        if (static_cast<unsigned>(mapping.size()) != numberActiveRegions) {
            // Customers have at least one region without a server.

            QList<RegionId> activeRegionList = activeServersByServerIdByRegionId.keys();
            QSet<RegionId>  allActiveRegions(activeRegionList.constBegin(), activeRegionList.constEnd());

            QSet<RegionId>  unassignedRegions = allActiveRegions;
            unassignedRegions.subtract(assignedRegions);

            for (  QSet<RegionId>::const_iterator regionIterator    = unassignedRegions.constBegin(),
                                                  regionEndIterator = unassignedRegions.constEnd()
                 ; regionIterator != regionEndIterator
                 ; ++regionIterator
                ) {
                RegionId           regionId          = *regionIterator;
                const ServersById& serversThisRegion = activeServersByServerIdByRegionId.value(regionId);

                Server server = leastLoadedServer(serversThisRegion, exclusionList);
                if (server.isValid()) {
                    mapping.insert(server.serverId());

                    if (server.cpuLoading() < bestCpuLoading) {
                        leastLoadedServerId = server.serverId();
                        bestCpuLoading      = server.cpuLoading();
                    }
                }
            }
        }
    } else {
        if (mapping.size() != 1) {
            Server bestServer = leastLoadedServer(serversById, exclusionList);
            leastLoadedServerId = bestServer.serverId();
            bestCpuLoading      = bestServer.cpuLoading();

            CustomerMapping::ServerSet oldServers = mapping;
            oldServers.remove(bestServer.serverId());

            removedServers.unite(oldServers);

            mapping = CustomerMapping::Mapping(
                leastLoadedServerId,
                CustomerMapping::ServerSet() << leastLoadedServerId
            );
        }
    }

    if (!mapping.contains(mapping.primaryServerId())) {
        mapping.setPrimaryServer(leastLoadedServerId);
    }

    if (ignoreExisting) {
        removedServers = currentMapping->mapping(customerId, threadId);
        removedServers.subtract(mapping);
    }

    return QPair<CustomerMapping::Mapping, CustomerMapping::ServerSet>(mapping, removedServers);
}


Server ServerAdministrator::leastLoadedServer(
        const ServerAdministrator::ServersById&         servers,
        const ServerAdministrator::ServerExclusionList& exclusionList
    ) {
    ServerId bestServerid   = invalidServerId;
    float    bestCpuLoading = std::numeric_limits<float>::max();
    for (ServersById::const_iterator it=servers.constBegin(),end=servers.constEnd() ; it!=end ; ++it) {
        const Server& server   = it.value();
        ServerId      serverId = server.serverId();
        if (server.status() == Server::Status::ACTIVE && !exclusionList.contains(serverId)) {
            float         cpuLoading = server.cpuLoading();

            if (cpuLoading < bestCpuLoading) {
                bestServerid   = server.serverId();
                bestCpuLoading = cpuLoading;
            }
        }
    }

    return servers.value(bestServerid);
}


void ServerAdministrator::updateLocalCache(unsigned threadId) {
    serversById = currentServers->getServersById(threadId);

    serverIdsByIdentifier.clear();
    activeServersByServerIdByRegionId.clear();
    inactiveServersByServerIdByRegionId.clear();
    defunctServersByServerIdByRegionId.clear();
    regionIndexByRegionIds.clear();

    for (ServersById::const_iterator it=serversById.constBegin(),end=serversById.constEnd() ; it!=end ; ++it) {
        addServer(it.value());
    }

    loadNeeded = false;
}


void ServerAdministrator::globalUpdateRegionData() {
    unsigned numberActiveRegions = static_cast<unsigned>(activeServersByServerIdByRegionId.size());
    regionIndexByRegionIds.clear();
    unsigned regionIndex = 0;
    for (  ServersByServerIdByRegionId::const_iterator
               regionIterator    = activeServersByServerIdByRegionId.constBegin(),
               regionEndIterator = activeServersByServerIdByRegionId.constEnd()
         ; regionIterator != regionEndIterator
         ; ++regionIterator
        ) {
        const ServersById& serversById = regionIterator.value();
        for (  ServersById::const_iterator serverIterator    = serversById.constBegin(),
                                           serverEndIterator = serversById.constEnd()
             ; serverIterator != serverEndIterator
             ; ++serverIterator
            ) {
            sendGoActive(serverIterator.value(), regionIndex, numberActiveRegions);
        }

        regionIndexByRegionIds.insert(regionIterator.key(), regionIndex);
        ++regionIndex;
    }
}


bool ServerAdministrator::modifyServerHelper(const Server& server, unsigned int threadId, bool forceModify) {
    bool success;

    Server oldServer = serversById.value(server.serverId());
    if (forceModify || oldServer.status() != Status::ACTIVE) {
        success = currentServers->modifyServer(server, threadId);
        if (success) {
            serversById.insert(server.serverId(), server);

            if (oldServer.identifier() != server.identifier()) {
                serverIdsByIdentifier.remove(oldServer.identifier());
                serverIdsByIdentifier.insert(server.identifier(), server.serverId());
            }

            if (oldServer.status() != server.status() || oldServer.regionId() != server.regionId()) {
                removeFromRegionTable(oldServer);
                addToRegionTable(server);
            }
        }
    } else {
        logWrite(
            QString("Can-not modify a server while it's active, server ID %1").arg(server.serverId()),
            true
        );
        success = false;
    }

    return success;
}


void ServerAdministrator::addServer(const Server& server) {
    ServerId serverId    = server.serverId();
    QString  hostAddress = server.identifier();

    serverIdsByIdentifier.insert(hostAddress, serverId);
    addToRegionTable(server);
}


void ServerAdministrator::removeServer(const Server& server) {
    const QString& identifier = server.identifier();
    serverIdsByIdentifier.remove(identifier);
    removeFromRegionTable(server);
}


ServerAdministrator::ServersByServerIdByRegionId* ServerAdministrator::getRegionTable(
        ServerAdministrator::Status status
    ) {
    ServersByServerIdByRegionId* result = nullptr;

    switch (status) {
        case Status::ALL_UNKNOWN:   { result = nullptr;                                break; }
        case Status::ACTIVE:        { result = &activeServersByServerIdByRegionId;     break; }
        case Status::INACTIVE:      { result = &inactiveServersByServerIdByRegionId;   break; }
        case Status::DEFUNCT:       { result = &defunctServersByServerIdByRegionId;    break; }
        case Status::NUMBER_VALUES: {
            Q_ASSERT(false);
            break;
        }

        default: {
            Q_ASSERT(false);
            break;
        }
    }

    return result;
}


void ServerAdministrator::addToRegionTable(const Server& server) {
    Status                       status          = server.status();
    ServersByServerIdByRegionId* serversByRegion = getRegionTable(status);
    if (serversByRegion != nullptr) {
        RegionId regionId = server.regionId();
        ServerId serverId = server.serverId();
        (*serversByRegion)[regionId].insert(serverId, server);
    }
}


void ServerAdministrator::removeFromRegionTable(const Server& server) {
    Status                       status          = server.status();
    ServersByServerIdByRegionId* serversByRegion = getRegionTable(status);
    if (serversByRegion != nullptr) {
        RegionId                              regionId = server.regionId();
        ServersByServerIdByRegionId::iterator it       = serversByRegion->find(regionId);
        if (it != serversByRegion->end()) {
            ServersById& serversById = it.value();

            ServerId serverId = server.serverId();
            serversById.remove(serverId);

            if (serversById.isEmpty()) {
                serversByRegion->erase(it);
            }
        }
    }
}


void ServerAdministrator::sendGoInactive(const Server& server) {
    const QString& identifier = server.identifier();
    currentOutboundRestApiFactory->postMessage(
        identifier,
        pollingServerStateInactiveEndpoint,
        QString("Server going inactive.")
    );
}


void ServerAdministrator::sendGoActive(const Server& server, unsigned regionIndex, unsigned numberRegions) {
    const QString& identifier = server.identifier();
    QJsonObject    message;

    message.insert("region_index", static_cast<int>(regionIndex));
    message.insert("number_regions", static_cast<int>(numberRegions));

    currentOutboundRestApiFactory->postMessage(
        identifier,
        pollingServerRegionChangeEndpoint,
        QJsonDocument(message),
        QString("Server going active")
    );
}


QJsonObject ServerAdministrator::buildCustomerMessage(
        unsigned                           pollingInterval,
        bool                               supportsPingTesting,
        bool                               supportsSslExpirationTesting,
        bool                               multiRegion,
        bool                               supportsLatencyMeasurements,
        const HostSchemes::HostSchemeHash& hostSchemesById,
        const Monitors::MonitorsById&      monitorsById
    ) {
    QJsonObject result;

    QHash<HostScheme::HostSchemeId, QList<const Monitor*>> monitorsListByHostScheme;
    for (  Monitors::MonitorsById::const_iterator monitorsIterator = monitorsById.constBegin(),
                                                  monitorsEndIterator = monitorsById.constEnd()
         ; monitorsIterator != monitorsEndIterator
         ; ++monitorsIterator
        ) {
        const Monitor& monitor = monitorsIterator.value();
        monitorsListByHostScheme[monitor.hostSchemeId()].append(&monitor);
    }

    QJsonObject hostSchemesObject;
    for (  HostSchemes::HostSchemeHash::const_iterator hostSchemeIterator = hostSchemesById.constBegin(),
                                                       hostSchemeEndIterator = hostSchemesById.constEnd()
         ; hostSchemeIterator != hostSchemeEndIterator
         ; ++hostSchemeIterator
        ) {
        const HostScheme&            hostScheme   = hostSchemeIterator.value();
        HostScheme::HostSchemeId     hostSchemeId = hostScheme.hostSchemeId();
        const QList<const Monitor*>& monitorsList = monitorsListByHostScheme.value(hostSchemeId);
        unsigned                     numberMonitors = static_cast<unsigned>(monitorsList.size());

        if (numberMonitors > 0) {
            QJsonObject monitorsObject;
            for (unsigned monitorIndex=0 ; monitorIndex<numberMonitors ; ++monitorIndex) {
                const Monitor& monitor = *monitorsList.at(monitorIndex);
                QJsonObject    monitorObject;

                monitorObject.insert("uri", monitor.path());

                if (monitor.method() != Monitor::Method::GET) {
                    monitorObject.insert("method", Monitor::toString(monitor.method()).toLower());
                }

                if (monitor.contentCheckMode() != Monitor::ContentCheckMode::NO_CHECK) {
                    monitorObject.insert("content_check_mode", Monitor::toString(monitor.contentCheckMode()).toLower());
                }

                const Monitor::KeywordList& keywords       = monitor.keywords();
                unsigned                    numberKeywords = static_cast<unsigned>(keywords.size());
                if (numberKeywords > 0) {
                    QJsonArray keywordsArray;
                    for (unsigned keywordsIndex=0 ; keywordsIndex<numberKeywords ; ++keywordsIndex) {
                        const QByteArray& keyword = keywords.at(keywordsIndex);
                        keywordsArray.append(QString::fromLatin1(keyword.toBase64()));
                    }

                    monitorObject.insert("keywords", keywordsArray);
                }

                if (monitor.contentType() != Monitor::ContentType::TEXT) {
                    monitorObject.insert("post_content_type", Monitor::toString(monitor.contentType()).toLower());
                }

                if (!monitor.userAgent().isEmpty()) {
                    monitorObject.insert("post_user_agent", monitor.userAgent());
                }

                const QByteArray& postContent = monitor.postContent();
                if (!postContent.isEmpty()) {
                    monitorObject.insert("post_content", QString::fromLatin1(postContent.toBase64()));
                }

                monitorsObject.insert(QString::number(monitor.monitorId()), monitorObject);
            }

            QJsonObject hostSchemeObject;
            hostSchemeObject.insert("url", hostScheme.url().toString());
            hostSchemeObject.insert("monitors", monitorsObject);

            hostSchemesObject.insert(QString::number(hostSchemeId), hostSchemeObject);
        }
    }

    result.insert("polling_interval", static_cast<int>(pollingInterval));
    result.insert("ping", supportsPingTesting);
    result.insert("ssl_expiration", supportsSslExpirationTesting);
    result.insert("latency", supportsLatencyMeasurements);
    result.insert("multi_region", multiRegion);

    result.insert("host_schemes", hostSchemesObject);

    return result;
}


void ServerAdministrator::applyCustomerDeactivation(CustomerId customerId, const CustomerMapping::ServerSet& servers) {
    QJsonObject requestObject;
    requestObject.insert("customer_id", static_cast<double>(customerId));

    for (  CustomerMapping::ServerSet::const_iterator serverIterator    = servers.constBegin(),
                                                      serverEndIterator = servers.constEnd()
         ; serverIterator != serverEndIterator
         ; ++serverIterator
        ) {
        ServerId serverId = *serverIterator;
        const Server& server = serversById.value(serverId);
        if (server.isValid()) {
            currentOutboundRestApiFactory->postMessage(
                server.identifier(),
                QString("/customer/remove"),
                requestObject,
                QString("Deactivated customer %1").arg(customerId)
            );
        }
    }
}


void ServerAdministrator::applyCustomerActivation(
        const CustomerMapping::Mapping&   mapping,
        const CustomerMapping::ServerSet& removedServers,
        const CustomerMapping::ServerSet& limitToServers,
        const CustomerCapabilities&       capabilities,
        unsigned                          threadId
    ) {
    CustomerId customerId = capabilities.customerId();

    HostSchemes::HostSchemeHash hostSchemesById = currentHostSchemes->getHostSchemes(customerId, threadId);
    Monitors::MonitorsById      monitorsById    = currentMonitors->getMonitorsByCustomerId(customerId, threadId);

    unsigned perServerPollingInterval = capabilities.pollingInterval();
    QJsonObject messageObject = buildCustomerMessage(
        perServerPollingInterval,
        false,
        false,
        capabilities.multiRegionChecking(),
        capabilities.supportsLatencyTracking(),
        hostSchemesById,
        monitorsById
    );

    QJsonObject requestObject;
    requestObject.insert(QString::number(customerId), messageObject);

    for (  CustomerMapping::Mapping::const_iterator serverIterator    = mapping.constBegin(),
                                                    serverEndIterator = mapping.constEnd()
         ; serverIterator != serverEndIterator
         ; ++serverIterator
        ) {
        ServerId serverId = *serverIterator;
        if (serverId != mapping.primaryServerId()) {
            const Server& server = serversById.value(serverId);
            if (server.isValid() && (limitToServers.isEmpty() || limitToServers.contains(serverId))) {
                currentOutboundRestApiFactory->postMessage(
                    server.identifier(),
                    QString("/customer/add"),
                    requestObject,
                    QString("Updated settings for customer %1").arg(customerId)
                );

                if (capabilities.paused()) {
                    QJsonObject pauseMessage;

                    pauseMessage.insert("customer_id", static_cast<double>(customerId));
                    pauseMessage.insert("pause", true);

                    currentOutboundRestApiFactory->postMessage(
                        server.identifier(),
                        pollingServerCustomerPauseEndpoint,
                        QJsonDocument(pauseMessage),
                        QString("Customer %1 pause state set to true").arg(customerId)
                    );
                }
            }
        }
    }

    ServerId primaryServerId = mapping.primaryServerId();
    if (limitToServers.isEmpty() || limitToServers.contains(primaryServerId)) {
        messageObject.insert("ping", capabilities.supportsPingBasedPolling());
        messageObject.insert("ssl_expiration", capabilities.supportsSslExpirationChecking());

        requestObject.insert(QString::number(customerId), messageObject);

        const Server& server = serversById.value(mapping.primaryServerId());
        if (server.isValid()) {
            currentOutboundRestApiFactory->postMessage(
                server.identifier(),
                QString("/customer/add"),
                requestObject,
                QString("Updated settings for customer %1 - primary server").arg(customerId)
            );

            if (capabilities.paused()) {
                QJsonObject pauseMessage;

                pauseMessage.insert("customer_id", static_cast<double>(customerId));
                pauseMessage.insert("pause", true);

                currentOutboundRestApiFactory->postMessage(
                    server.identifier(),
                    pollingServerCustomerPauseEndpoint,
                    QJsonDocument(pauseMessage),
                    QString("Customer %1 pause state set to true").arg(customerId)
                );
            }
        }
    }

    applyCustomerDeactivation(customerId, removedServers);
}
