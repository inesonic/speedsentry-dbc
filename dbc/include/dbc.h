/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the database controller main class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef DB_CONTROLLER_H
#define DB_CONTROLLER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QFileSystemWatcher>

namespace RestApiInV1 {
    class Server;
    class TimeDeltaHandler;
}

class QNetworkAccessManager;

class DatabaseManager;
class LatencyInterfaceManager;
class LatencyPlotter;
class ResourcePlotter;
class Regions;
class Servers;
class ServerAdministrator;
class CustomerSecrets;
class CustomersCapabilities;
class HostSchemes;
class Monitors;
class Events;
class CustomerMapping;
class Resources;

class RegionManager;
class ServerManager;
class HostSchemeManager;
class MonitorManager;
class CustomerCapabilitiesManager;
class EventManager;
class CustomerMappingManager;
class LatencyManager;
class MultipleManager;
class ResourceManager;
class CustomerAuthenticator;
class CustomerRestApiV1;
class OutboundRestApi;
class OutboundRestApiFactory;

class MonitorUpdater;
class EventProcessor;

/**
 * The database controller application class.
 */
class DbC:public QObject {
    Q_OBJECT

    public:
        /**
         * Constructor
         *
         * \param[in] configurationFilename The configuration file used to manage database and application settings.
         *                                  Note that this file will be monitored for updates so it can be changed
         *                                  while the application is running.
         *
         * \param[in] parent                Pointer to the parent object.
         */
        DbC(const QString& configurationFilename, QObject* parent = nullptr);

        ~DbC() override;

    private slots:
        /**
         * Method that is triggered whenever the supplied configuration file is changed.
         *
         * \param[in] filePath The path to the changed file.
         */
        void configurationFileChanged(const QString& filePath);

    private:
        /**
         * The REST API key length, in bytes.
         */
        static constexpr unsigned keyLength = 56;

        /**
         * The filesystem watcher used to monitor the configuration file.
         */
        QFileSystemWatcher* fileSystemWatcher;

        /**
         * The path to the current configuration file.
         */
        QString currentConfigurationFilename;

        /**
         * Flag indicating if verbose reporting should be used.
         */
        bool verboseReporting;

        /**
         * The inbound REST API key.
         */
        QByteArray inboundApiKey;

        /**
         * The current list of allowed machine identifiers.
         */
        QList<QString> currentMachineIdentifiers;

        /**
         * The current database manager.
         */
        DatabaseManager* databaseManager;

        /**
         * Interface to obtain and update regions data.
         */
        Regions* currentRegions;

        /**
         * Interface used to obtain and update server data.
         */
        Servers* currentServers;

        /**
         * Interface used to obtain and update customer host/scheme values.
         */
        HostSchemes* currentHostSchemes;

        /**
         * Interface used to obtain and update customer monitor settings.
         */
        Monitors* currentMonitors;

        /**
         * Interface used to obtain and update customer secrets.
         */
        CustomerSecrets* currentCustomerSecrets;

        /**
         * Interface used to obtain and update customer capabilities.
         */
        CustomersCapabilities* currentCustomersCapabilities;

        /**
         * Interface used to obtain and record events.
         */
        Events* currentEvents;

        /**
         * Interface used to obtain customer resource data.
         */
        Resources* currentResources;

        /**
         * Interface used to update and query customer mapping to polling servers.
         */
        CustomerMapping* currentCustomerMapping;

        /**
         * The internal REST API used to obtain and modify regions.
         */
        RegionManager* currentRegionManager;

        /**
         * The internal REST API used to obtain and modify servers.
         */
        ServerManager* currentServerManager;

        /**
         * The internal REST API used to directly obtain and modify host/scheme values.
         */
        HostSchemeManager* currentHostSchemeManager;

        /**
         * The internal REST API used to directly obtain and modify monitors values.
         */
        MonitorManager* currentMonitorManager;

        /**
         * The internal REST API used to directly obtain and modify customer capabilities (and secrets).
         */
        CustomerCapabilitiesManager* currentCustomerCapabilitiesManager;

        /**
         * The internal REST API used to directly obtain and record events as well as monitor status information.
         */
        EventManager* currentEventManager;

        /**
         * The internal REST API used to obtain and modify customer/server mappings.
         */
        CustomerMappingManager* currentCustomerMappingManager;

        /**
         * The internal REST API used to record and report customer latency data.
         */
        LatencyManager* currentLatencyManager;

        /**
         * The internal REST API used to report multiple parameters for a customer simultaneously.
         */
        MultipleManager* currentMultipleManager;

        /**
         * The internal REST API used to manage resource data.
         */
        ResourceManager* currentResourceManager;

        /**
         * Class used to authenticate customers use of our REST API -- WordPress Only.
         */
        CustomerAuthenticator* wordPressCustomerAuthenticator;

        /**
         * Class used to authenticate customers use of our REST API -- WordPress And REST.
         */
        CustomerAuthenticator* restCustomerAuthenticator;

        /**
         * The customer facing REST API, version 1.
         */
        CustomerRestApiV1* customerRestApiV1;

        /**
         * The customer monitor update engine.
         */
        MonitorUpdater* currentMonitorUpdater;

        /**
         * The server administrator class.
         */
        ServerAdministrator* currentServerAdministrator;

        /**
         * The event processor.
         */
        EventProcessor* currentEventProcessor;

        /**
         * The latency interface manager class used to cache and write received customer data.
         */
        LatencyInterfaceManager* latencyInterfaceManager;

        /**
         * The latency plotting engine.
         */
        LatencyPlotter* currentLatencyPlotter;

        /**
         * The resource plotting engine.
         */
        ResourcePlotter* currentResourcePlotter;

        /**
         * The database username.
         */
        QString databaseUsername;

        /**
         * The database password.
         */
        QString databasePassword;

        /**
         * The database server.
         */
        QString databaseServer;

        /**
         * The database name.
         */
        QString databaseName;

        /**
         * Database table used to track our monitor systems.
         */
        QString monitorIdentifierTable;

        /**
         * The inbound REST API inbound server.
         */
        RestApiInV1::Server* inboundRestServer;

        /**
         * The inbound REST API to measure time deltas.
         */
        RestApiInV1::TimeDeltaHandler* timeDeltaHandler;

        /**
         * The network access manager used by the outbound REST API.
         */
        QNetworkAccessManager* networkAccessManager;

        /**
         * The API used to talk to the Python based website infrastructure.
         */
        OutboundRestApi* websiteRestApi;

        /**
         * The multi-server outbound REST API factory.
         */
        OutboundRestApiFactory* outboundRestApiFactory;
};

#endif
