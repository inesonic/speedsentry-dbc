/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the database controller main class.
***********************************************************************************************************************/

#include <QObject>
#include <QCoreApplication>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QStringList>
#include <QFileSystemWatcher>
#include <QFile>
#include <QNetworkAccessManager>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <inextea.h>
#include <crypto_aes_cbc_encryptor.h>
#include <crypto_helpers.h>

#include <rest_api_in_v1_server.h>
#include <rest_api_in_v1_handler.h>
#include <rest_api_in_v1_time_delta_handler.h>
#include <rest_api_in_v1_inesonic_binary_rest_handler.h>

#include "log.h"
#include "database_manager.h"
#include "latency_interface_manager.h"
#include "latency_plotter.h"
#include "regions.h"
#include "servers.h"
#include "customer_secrets.h"
#include "customers_capabilities.h"
#include "host_schemes.h"
#include "monitors.h"
#include "events.h"
#include "customer_mapping.h"
#include "region_manager.h"
#include "server_manager.h"
#include "host_scheme_manager.h"
#include "monitor_manager.h"
#include "customer_capabilities_manager.h"
#include "event_manager.h"
#include "customer_mapping_manager.h"
#include "latency_manager.h"
#include "multiple_manager.h"
#include "customer_authenticator.h"
#include "server_administrator.h"
#include "customer_rest_api_v1.h"
#include "outbound_rest_api.h"
#include "outbound_rest_api_factory.h"
#include "monitor_updater.h"
#include "event_processor.h"
#include "resources.h"
#include "resource_plotter.h"
#include "resource_manager.h"
#include "dbc.h"

DbC::DbC(const QString& configurationFilename, QObject* parent):QObject(parent) {
    currentConfigurationFilename = configurationFilename;
    fileSystemWatcher = new QFileSystemWatcher(QStringList() << configurationFilename, this);
    connect(fileSystemWatcher, &QFileSystemWatcher::fileChanged, this, &DbC::configurationFileChanged);

    databaseManager        = new DatabaseManager;
    currentRegions         = new Regions(databaseManager, this);
    currentServers         = new Servers(databaseManager, this);
    currentCustomerSecrets = new CustomerSecrets(
        databaseManager,
        QByteArray(),
        QByteArray(),
        CustomerSecrets::defaultCacheDepth,
        this
    );
    currentCustomersCapabilities = new CustomersCapabilities(
        databaseManager,
        CustomersCapabilities::defaultCacheDepth,
        this
    );
    currentHostSchemes     = new HostSchemes(databaseManager, this);
    currentMonitors        = new Monitors(databaseManager, this);
    currentEvents          = new Events(databaseManager, this);
    currentResources       = new Resources(databaseManager, this);
    currentCustomerMapping = new CustomerMapping(databaseManager, this);

    latencyInterfaceManager  = new LatencyInterfaceManager(databaseManager, this);

    currentLatencyPlotter  = new LatencyPlotter(latencyInterfaceManager, this);
    currentResourcePlotter = new ResourcePlotter(currentResources, this);

    wordPressCustomerAuthenticator = new CustomerAuthenticator(
        true,
        true,
        currentCustomerSecrets,
        currentCustomersCapabilities
    );

    restCustomerAuthenticator = new CustomerAuthenticator(
        false,
        true,
        currentCustomerSecrets,
        currentCustomersCapabilities
    );

    inboundRestServer = new RestApiInV1::Server(this);
    inboundRestServer->setLoggingFunction(&logWrite);
    timeDeltaHandler  = new RestApiInV1::TimeDeltaHandler;
    inboundRestServer->registerHandler(
        timeDeltaHandler,
        RestApiInV1::Handler::Method::POST,
        RestApiInV1::TimeDeltaHandler::defaultEndpoint
    );

    networkAccessManager = new QNetworkAccessManager(this);
    networkAccessManager->setRedirectPolicy(QNetworkRequest::RedirectPolicy::NoLessSafeRedirectPolicy);
    networkAccessManager->setStrictTransportSecurityEnabled(true);

    websiteRestApi = new OutboundRestApi(
        networkAccessManager,
        QUrl(),
        OutboundRestApi::defaultTimeDeltaSlug,
        false,
        this
    );

    currentEventProcessor = new EventProcessor(
        currentMonitors,
        currentHostSchemes,
        currentEvents,
        websiteRestApi,
        this
    );

    outboundRestApiFactory = new OutboundRestApiFactory(
        networkAccessManager,
        OutboundRestApiFactory::Scheme::HTTPS,
        0,
        OutboundRestApiFactory::defaultTimeDeltaSlug,
        this
    );

    currentServerAdministrator = new ServerAdministrator(
        currentServers,
        currentRegions,
        currentMonitors,
        currentHostSchemes,
        currentCustomersCapabilities,
        currentCustomerMapping,
        outboundRestApiFactory,
        this
    );
    currentMonitorUpdater = new MonitorUpdater(currentHostSchemes, currentMonitors, currentServerAdministrator);

    currentRegionManager = new RegionManager(inboundRestServer, currentRegions, QByteArray(), this);
    currentServerManager = new ServerManager(
        inboundRestServer,
        currentServerAdministrator,
        currentRegions,
        QByteArray(),
        this
    );
    currentHostSchemeManager = new HostSchemeManager(
        inboundRestServer,
        currentHostSchemes,
        currentMonitorUpdater,
        QByteArray(),
        this
    );
    currentMonitorManager = new MonitorManager(
        inboundRestServer,
        currentMonitors,
        currentCustomersCapabilities,
        currentMonitorUpdater,
        QByteArray(),
        this
    );
    currentCustomerCapabilitiesManager = new CustomerCapabilitiesManager(
        inboundRestServer,
        currentCustomersCapabilities,
        currentCustomerSecrets,
        currentServerAdministrator,
        QByteArray(),
        this
    );
    currentEventManager = new EventManager(
        inboundRestServer,
        currentEvents,
        currentMonitors,
        currentEventProcessor,
        QByteArray(),
        this
    );
    currentCustomerMappingManager = new CustomerMappingManager(
        inboundRestServer,
        currentCustomerMapping,
        currentServerAdministrator,
        QByteArray(),
        this
    );
    currentLatencyManager = new LatencyManager(
        inboundRestServer,
        latencyInterfaceManager,
        currentServers,
        currentMonitors,
        currentLatencyPlotter,
        QByteArray(),
        this
    );
    currentMultipleManager = new MultipleManager(
        inboundRestServer,
        currentHostSchemes,
        currentMonitors,
        currentEvents,
        currentLatencyPlotter,
        QByteArray(),
        this
    );
    currentResourceManager = new ResourceManager(
        inboundRestServer,
        currentResources,
        currentResourcePlotter,
        QByteArray(),
        this
    );
    customerRestApiV1 = new CustomerRestApiV1(
        inboundRestServer,
        wordPressCustomerAuthenticator,
        restCustomerAuthenticator,
        currentCustomersCapabilities,
        currentHostSchemes,
        currentMonitors,
        currentMonitorUpdater,
        currentRegions,
        currentServers,
        currentEvents,
        currentEventProcessor,
        latencyInterfaceManager,
        currentLatencyPlotter,
        currentResources,
        currentResourcePlotter,
        currentServerAdministrator,
        this
    );

    configurationFileChanged(configurationFilename);

    currentServerAdministrator->sendGoActive(65535);
}


DbC::~DbC() {
    delete timeDeltaHandler;
}


void DbC::configurationFileChanged(const QString& /* filePath */) {
    QFile configurationFile(currentConfigurationFilename);
    bool success = configurationFile.open(QFile::OpenModeFlag::ReadOnly);
    if (success) {
        QByteArray configurationData = configurationFile.readAll();
        configurationFile.close();

        QJsonParseError jsonParseResult;
        QJsonDocument   jsonDocument = QJsonDocument::fromJson(configurationData, &jsonParseResult);

        if (jsonParseResult.error == QJsonParseError::ParseError::NoError) {
            QJsonObject jsonObject            = jsonDocument.object();

            QString    encodedInboundApiKey = jsonObject.value(QString("inbound_api_key")).toString();

            verboseReporting = jsonObject.value("verbose").toBool(false);
            databaseUsername = jsonObject.value("database_username").toString();
            databasePassword = jsonObject.value("database_password").toString();
            databaseServer   = jsonObject.value("database_server").toString();
            databaseName     = jsonObject.value("database_name").toString();

            QString websiteAuthority     = jsonObject.value(QString("website_authority")).toString();
            QString websiteEncodedApiKey = jsonObject.value(QString("website_api_key")).toString();

            QString encodedCustomerSecretsEncryptionKey = jsonObject.value(
                "customer_secrets_encryption_key"
            ).toString();

            QString encodedCustomerIdentifierKey = jsonObject.value("customer_identifier_key").toString();

            double customerSecretsCacheSizeAsDouble = jsonObject.value("customer_secrets_cache_size").toDouble(-1);

            double customerCapabilitiesCacheSizeAsDouble = jsonObject.value(
                "customer_capabilities_cache_size"
            ).toDouble(-1);

            double aggregationAgeAsDouble = jsonObject.value("aggregation_age").toDouble(-1);

            double aggregationSamplePeriodAsDouble = jsonObject.value("aggregation_sample_period").toDouble(-1);

            double expungeAgeAsDouble = jsonObject.value(QString("expunge_age")).toDouble(-1);

            int     inboundPort                  = jsonObject.value(QString("inbound_port")).toInt(
                RestApiInV1::Server::defaultPort
            );
            QString inboundHostAddressStr        = jsonObject.value("inbound_host_address").toString(
                RestApiInV1::Server::defaultHostAddress.toString()
            );
            int     maximumConcurrentConnections = jsonObject.value(
                "maximum_concurrent_connections"
            ).toInt(
                RestApiInV1::Server::defaultMaximumSimultaneousConnections
            );

            QByteArray::FromBase64Result inboundKey = QByteArray::fromBase64Encoding(
                encodedInboundApiKey.toUtf8(),
                QByteArray::Base64Option::Base64Encoding
            );
            if (inboundKey) {
                inboundApiKey = *inboundKey;
                if (static_cast<unsigned>(inboundApiKey.size()) != keyLength) {
                    inboundApiKey.clear();
                    logWrite(QString("Invalid API key length."), true);
                    success = false;
                }
            } else {
                logWrite(QString("Invalid API key length."), true);
                success = false;
            }

            QByteArray                   websiteApiKey;
            QByteArray::FromBase64Result websiteKey = QByteArray::fromBase64Encoding(
                websiteEncodedApiKey.toUtf8(),
                QByteArray::Base64Option::Base64Encoding
            );
            if (websiteKey) {
                websiteApiKey = *websiteKey;
                if (static_cast<unsigned>(websiteApiKey.size()) != keyLength) {
                    inboundApiKey.clear();
                    logWrite(QString("Invalid website API key length."), true);
                    success = false;
                }
            } else {
                logWrite(QString("Invalid website API key length."), true);
                success = false;
            }

            QString    encodedPollingServerApiKey = jsonObject.value("polling_server_api_key").toString();
            QByteArray::FromBase64Result pollingServerKey = QByteArray::fromBase64Encoding(
                encodedPollingServerApiKey.toUtf8(),
                QByteArray::Base64Option::Base64Encoding
            );

            QByteArray pollingServerApiKey;
            if (pollingServerKey) {
                pollingServerApiKey = *pollingServerKey;
                if (static_cast<unsigned>(pollingServerApiKey.size()) != keyLength) {
                    pollingServerApiKey.clear();
                    logWrite(QString("Invalid polling server API key length."), true);
                    success = false;
                }
            } else {
                logWrite(QString("Invalid polling server API key length."), true);
                success = false;
            }

            QString pollingServerSchemeString = jsonObject.value("polling_server_scheme").toString("https").toLower();
            OutboundRestApiFactory::Scheme pollingServerScheme = OutboundRestApiFactory::Scheme::HTTPS;
            if (pollingServerSchemeString == "http") {
                pollingServerScheme = OutboundRestApiFactory::Scheme::HTTP;
            } else if (pollingServerSchemeString == "https") {
                pollingServerScheme = OutboundRestApiFactory::Scheme::HTTPS;
            } else {
                logWrite(QString("Invalid polling server scheme."), true);
                success = false;
            }

            int pollingServerPort = jsonObject.value("polling_server_port").toInt(0);
            if (pollingServerPort < 0 || pollingServerPort > 0xFFFF) {
                logWrite(QString("Invalid polling server port."), true);
                success = false;
            }

            if (success                          &&
                (databaseUsername.isEmpty() ||
                 databasePassword.isEmpty() ||
                 databaseServer.isEmpty()   ||
                 databaseName.isEmpty()        )    ) {
                logWrite(QString("Missing or invalid database settings."), true);
                success = false;
            }

            if (success && aggregationAgeAsDouble <= 0) {
                logWrite(QString("Aggregation age value is invalid."), true);
                success = false;
            }

            if (success && aggregationSamplePeriodAsDouble <= 0) {
                logWrite(QString("Aggregation sample period is invalid."), true);
            }

            if (success && expungeAgeAsDouble <= 0) {
                logWrite(QString("Expunge age is invalid."), true);
            }

            if (success && (inboundPort < 0 || inboundPort > 0xFFFF)) {
                logWrite(QString("Inbound port address is invalid."), true);
                success = false;
            }

            if (success && maximumConcurrentConnections <= 0) {
                logWrite(QString("Invalid maximum number of concurrent connections."), true);
                success = false;
            }

            QByteArray customerSecretsEncryptionKey;
            if (!encodedCustomerSecretsEncryptionKey.isEmpty()) {
                QByteArray::FromBase64Result customerSecretsEncryptionKeyResult = QByteArray::fromBase64Encoding(
                    encodedCustomerSecretsEncryptionKey.toUtf8(),
                    QByteArray::Base64Option::Base64Encoding
                );

                if (customerSecretsEncryptionKeyResult) {
                    customerSecretsEncryptionKey = *customerSecretsEncryptionKeyResult;
                    unsigned encryptionKeyLength = static_cast<unsigned>(customerSecretsEncryptionKey.size());
                    if (encryptionKeyLength != Crypto::AesCbcEncryptor::keyLength) {
                        logWrite(QString("Invalid customer secrets encryption key length."), true);
                        success = false;
                    }
                } else {
                    logWrite(QString("Customer secrets encryption key should be base-64 encoded."), true);
                    success = false;
                }

                Crypto::scrub(*customerSecretsEncryptionKeyResult);
            } else {
                logWrite(QString("Missing or empty \"customer_secrets_encryption_key\" value."), true);
                success = false;
            }

            Crypto::scrub(encodedCustomerSecretsEncryptionKey);

            QByteArray customerIdentifierKey;
            if (!encodedCustomerIdentifierKey.isEmpty()) {
                QByteArray::FromBase64Result customerIdentifierKeyResult = QByteArray::fromBase64Encoding(
                    encodedCustomerIdentifierKey.toUtf8(),
                    QByteArray::Base64Option::Base64Encoding
                );

                if (customerIdentifierKeyResult) {
                    customerIdentifierKey = *customerIdentifierKeyResult;
                    unsigned encryptionKeyLength = static_cast<unsigned>(customerIdentifierKey.size());
                    if (encryptionKeyLength != IneXtea::keyLength) {
                        logWrite(QString("Invalid customer identifier key length."), true);
                        success = false;
                    }
                } else {
                    logWrite(QString("Customer identifier key should be base-64 encoded."), true);
                    success = false;
                }

                Crypto::scrub(*customerIdentifierKeyResult);
            } else {
                logWrite(QString("Missing or empty \"customer_identifier_key\" value."), true);
                success = false;
            }

            Crypto::scrub(encodedCustomerIdentifierKey);

            unsigned long customerSecretsCacheSize = 0;
            if (success) {
                if (customerSecretsCacheSizeAsDouble > 0) {
                    customerSecretsCacheSize = static_cast<unsigned long>(customerSecretsCacheSizeAsDouble);
                } else {
                    logWrite(QString("Invalid customer secrets cache size."), true);
                    success = false;
                }
            }

            unsigned long customerCapabilitiesCacheSize = 0;
            if (success) {
                if (customerCapabilitiesCacheSizeAsDouble > 0) {
                    customerCapabilitiesCacheSize = static_cast<unsigned long>(customerCapabilitiesCacheSizeAsDouble);
                } else {
                    logWrite(QString("Invalid customer secrets cache size."), true);
                    success = false;
                }
            }

            if (success) {
                databaseManager->setDatabaseConnectionSettings(
                    databaseUsername,
                    databasePassword,
                    databaseName,
                    databaseServer
                );

                QHostAddress inboundHostAddress(inboundHostAddressStr);
                success = inboundRestServer->reconfigure(inboundHostAddress, inboundPort);
                if (success) {
                    inboundRestServer->setMaximumSimultaneousConnections(maximumConcurrentConnections);
                } else {
                    logWrite(QString("Invalid inbound server configuration."), true);
                    success = false;
                }
            }

            if (success) {
                websiteRestApi->setDefaultSecret(websiteApiKey);
                websiteRestApi->setSchemeAndHost(websiteAuthority);

                outboundRestApiFactory->setDefaultSecret(pollingServerApiKey);
                outboundRestApiFactory->setScheme(pollingServerScheme);
                outboundRestApiFactory->setPort(pollingServerPort);

                currentRegionManager->setSecret(inboundApiKey);
                currentServerManager->setSecret(inboundApiKey);
                currentHostSchemeManager->setSecret(inboundApiKey);
                currentMonitorManager->setSecret(inboundApiKey);
                currentCustomerCapabilitiesManager->setSecret(inboundApiKey);
                currentEventManager->setSecret(inboundApiKey);
                currentCustomerMappingManager->setSecret(inboundApiKey);
                currentLatencyManager->setSecret(inboundApiKey);
                currentMultipleManager->setSecret(inboundApiKey);
                currentResourceManager->setSecret(inboundApiKey);

                currentCustomerSecrets->setEncryptionKeys(customerSecretsEncryptionKey);
                currentCustomerSecrets->setCustomerIdentifierKey(customerIdentifierKey);
                currentCustomerSecrets->resizeCache(customerSecretsCacheSize);
                currentCustomersCapabilities->resizeCache(customerCapabilitiesCacheSize);

                Crypto::scrub(customerSecretsEncryptionKey);
                Crypto::scrub(customerIdentifierKey);

                latencyInterfaceManager->setParameters(
                    static_cast<unsigned long>(aggregationAgeAsDouble),
                    static_cast<unsigned long>(aggregationSamplePeriodAsDouble),
                    static_cast<unsigned long>(expungeAgeAsDouble),
                    false
                );

                currentResources->setMaximumAge(expungeAgeAsDouble);
            }
        } else {
            logWrite(QString("Invalid JSON formatted configuration file."), true);
            success = false;
        }
    } else {
        logWrite(QString("Could not open configuration file  %1").arg(currentConfigurationFilename), true);
        success = false;
    }

    if (!success) {
        QCoreApplication::instance()->exit(1);
    }
}
