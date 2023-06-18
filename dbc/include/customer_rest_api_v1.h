/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref CustomerRestApi class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef CUSTOMER_REST_API_H
#define CUSTOMER_REST_API_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QJsonDocument>

#include <rest_api_in_v1_server.h>
#include <rest_api_in_v1_json_response.h>
#include <rest_api_in_v1_binary_response.h>
#include <rest_api_in_v1_inesonic_customer_rest_handler.h>
#include <rest_api_in_v1_inesonic_customer_binary_rest_handler.h>

#include "rest_helpers.h"

class CustomerAuthenticator;
class CustomersCapabilities;
class HostSchemes;
class Monitors;
class Regions;
class Servers;
class Events;
class EventProcessor;
class LatencyInterfaceManager;
class Resoures;
class LatencyPlotter;
class ResourcePlotter;
class ServerAdministrator;

/**
 * Class that supports the customer REST API.
 */
class CustomerRestApiV1:public QObject {
    Q_OBJECT

    public:
        /**
         * Path used to get information about a specific customer's capabilities.
         */
        static const QString capabilitiesGetPath;

        /**
         * Path used to get information about a single customer host server and server status.
         */
        static const QString hostsGetPath;

        /**
         * Path used to get information about all customer host servers and server status.
         */
        static const QString hostsListPath;

        /**
         * Path used to get information about a single monitor.
         */
        static const QString monitorsGetPath;

        /**
         * Path used to get information about all monitors.
         */
        static const QString monitorsListPath;

        /**
         * Path used to update monitor state.
         */
        static const QString monitorsUpdatePath;

        /**
         * Path used to get information about a single region.
         */
        static const QString regionsGetPath;

        /**
         * Path used to get all regions.
         */
        static const QString regionsListPath;

        /**
         * Path used to get information about a single event.
         */
        static const QString eventsGetPath;

        /**
         * Path used to get all events or events of a given timeframe.
         */
        static const QString eventsListPath;

        /**
         * Path used to generate a custom event.
         */
        static const QString eventsCreatePath;

        /**
         * Path used to get status information about a single monitor.
         */
        static const QString statusGetPath;

        /**
         * Path used to get status information about all monitors.
         */
        static const QString statusListPath;

        /**
         * Path used to get information needed by the WordPress plug-in.
         */
        static const QString multipleListPath;

        /**
         * Path used to get latency data.
         */
        static const QString latencyListPath;

        /**
         * Path used to get latency plots.
         */
        static const QString latencyPlotPath;

        /**
         * Path used to request operations be paused or resumed.
         */
        static const QString customerPausePath;

        /**
         * Path used to check if resource data is available for this customer.
         */
        static const QString resourceAvailablePath;

        /**
         * Path used to add new resource data.
         */
        static const QString resourceCreatePath;

        /**
         * Path use to obtain resource data.
         */
        static const QString resourceListPath;

        /**
         * Path used to obtain a plot of resource data.
         */
        static const QString resourcePlotPath;

        /**
         * Constructor
         *
         * \param[in] restApiServer                    The REST API server instance.
         *
         * \param[in] wordPressCustomerAuthenticator   The customer authentication API -- WordPress subset.
         *
         * \param[in] restCustomerAuthenticator        The customer authentication API -- WordPress + REST.
         *
         * \param[in] customersCapabilitiesDatabaseApi Class used to manage customer's capabilities in the database.
         *
         * \param[in] hostSchemeDatabaseApi            Class used to manage hosts and schemes.
         *
         * \param[in] monitorDatabaseApi               Class used to manage the customer's monitors.
         *
         * \param[in] monitorUpdater                   Class used to update and change customer monitor settings.
         *
         * \param[in] regionDatabaseApi                Class used to manage region data.
         *
         * \param[in] serverDatabaseApi                Class used to manager server data.
         *
         * \param[in] eventsDatabaseApi                Class used to access events data.
         *
         * \param[in] eventProcessor                   Class used to report events.
         *
         * \param[in] latencyInterfaceManager          The latency interface manager, used to get latency data.
         *
         * \param[in] latencyPlotter                   Plotter used to generate latency plots.
         *
         * \param[in] resourceDatabaseApi              Class used to record resource data.
         *
         * \param[in] resourcePlotter                  Plotter used to generate resource plots.
         *
         * \param[in] serverAdministrator              The server administration instance.
         *
         * \param[in] parent                           Pointer to the parent object.
         */
        CustomerRestApiV1(
            RestApiInV1::Server*     restApiServer,
            CustomerAuthenticator*   wordPressCustomerAuthenticator,
            CustomerAuthenticator*   restCustomerAuthenticator,
            CustomersCapabilities*   customersCapabilitiesDatabaseApi,
            HostSchemes*             hostSchemeDatabaseApi,
            Monitors*                monitorDatabaseApi,
            MonitorUpdater*          monitorUpdater,
            Regions*                 regionDatabaseApi,
            Servers*                 serverDatabaseApi,
            Events*                  eventDatabaseApi,
            EventProcessor*          eventProcessor,
            LatencyInterfaceManager* latencyInterfaceManager,
            LatencyPlotter*          latencyPlotter,
            Resources*               resourceDatabaseApi,
            ResourcePlotter*         resourcePlotter,
            ServerAdministrator*     ServerAdministrator,
            QObject*                 parent = nullptr
        );

        ~CustomerRestApiV1() override;

    private:
        /**
         * The v1/capabilities/get handler.
         */
        class CapabilitiesGet:public RestApiInV1::InesonicCustomerRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator           Class used to authenticate a customer.
                 *
                 * \param[in] customerCapabilitiesDatabaseApi Class used to manage customer capabilities in the
                 *                                            database.
                 */
                CapabilitiesGet(
                    CustomerAuthenticator* customerAuthenticator,
                    CustomersCapabilities* customerCapabilitiesDatabaseApi
                );

                ~CapabilitiesGet() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current customer capabilities database API.
                 */
                CustomersCapabilities* customersCapabilities;
        };

        /**
         * The v1/hosts/get handler.
         */
        class HostsGet:public RestApiInV1::InesonicCustomerRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator Class used to authenticate a customer.
                 *
                 * \param[in] hostSchemeDatabaseApi Class used to manage hosts and schemes.
                 */
                HostsGet(
                    CustomerAuthenticator* customerAuthenticator,
                    HostSchemes*           hostSchemeDatabaseApi
                );

                ~HostsGet() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current host/scheme database API.
                 */
                HostSchemes* currentHostSchemes;
        };

        /**
         * The v1/hosts/list handler.
         */
        class HostsList:public RestApiInV1::InesonicCustomerRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator Class used to authenticate a customer.
                 *
                 * \param[in] hostSchemeDatabaseApi Class used to manage hosts and schemes.
                 */
                HostsList(
                    CustomerAuthenticator* customerAuthenticator,
                    HostSchemes*           hostSchemeDatabaseApi
                );

                ~HostsList() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current host/scheme database API.
                 */
                HostSchemes* currentHostSchemes;
        };

        /**
         * The v1/monitors/get handler.
         */
        class MonitorsGet:public RestApiInV1::InesonicCustomerRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator Class used to authenticate a customer.
                 *
                 * \param[in] monitorDatabaseApi    Class used to manage the customer's monitors.
                 */
                MonitorsGet(CustomerAuthenticator* customerAuthenticator, Monitors* monitorDatabaseApi);

                ~MonitorsGet() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current monitors database API.
                 */
                Monitors* currentMonitors;
        };

        /**
         * The v1/monitors/list handler.
         */
        class MonitorsList:public RestApiInV1::InesonicCustomerRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator Class used to authenticate a customer.
                 *
                 * \param[in] monitorDatabaseApi    Class used to manage the customer's monitors.
                 *
                 * \param[in] hostSchemeDatabaseApi Class used to manage hosts and schemes.
                 */
                MonitorsList(
                    CustomerAuthenticator* customerAuthenticator,
                    Monitors*              monitorDatabaseApi,
                    HostSchemes*           hostSchemeDatabaseApi
                );

                ~MonitorsList() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current monitors database API.
                 */
                Monitors* currentMonitors;

                /**
                 * The current host/scheme database API.
                 */
                HostSchemes* currentHostSchemes;
        };

        /**
         * The v1/monitors/update handler.
         */
        class MonitorsUpdate:public RestApiInV1::InesonicCustomerRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator            Class used to authenticate a customer.
                 *
                 * \param[in] customersCapabilitiesDatabaseApi Class used to manage customer's capabilities in the
                 *                                             database.
                 *
                 * \param[in] monitorUpdater                   Class used to update and change customer monitor
                 *                                             settings.
                 */
                MonitorsUpdate(
                    CustomerAuthenticator* customerAuthenticator,
                    CustomersCapabilities* customersCapabilitiesDatabaseApi,
                    MonitorUpdater*        monitorUpdater
                );

                ~MonitorsUpdate() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The customer capabilities class instance.
                 */
                CustomersCapabilities* currentCustomersCapabilities;

                /**
                 * The current monitor updater instance.
                 */
                MonitorUpdater* currentMonitorUpdater;
        };

        /**
         * The v1/regions/get handler.
         */
        class RegionsGet:public RestApiInV1::InesonicCustomerRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator Class used to authenticate a customer.
                 *
                 * \param[in] regionDatabaseApi     Class used to manage regions.
                 */
                RegionsGet(
                    CustomerAuthenticator* customerAuthenticator,
                    Regions*               regionDatabaseApi
                );

                ~RegionsGet() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current region database API.
                 */
                Regions* currentRegions;
        };

        /**
         * The v1/regions/list handler.
         */
        class RegionsList:public RestApiInV1::InesonicCustomerRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator Class used to authenticate a customer.
                 *
                 * \param[in] regionDatabaseApi     Class used to manage regions.
                 */
                RegionsList(
                    CustomerAuthenticator* customerAuthenticator,
                    Regions*               regionDatabaseApi
                );

                ~RegionsList() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current region database API.
                 */
                Regions* currentRegions;
        };

        /**
         * The v1/events/get handler.
         */
        class EventsGet:public RestApiInV1::InesonicCustomerRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator Class used to authenticate a customer.
                 *
                 * \param[in] eventDatabaseApi      Class used to manage events.
                 */
                EventsGet(
                    CustomerAuthenticator* customerAuthenticator,
                    Events*                eventDatabaseApi
                );

                ~EventsGet() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current event database API.
                 */
                Events* currentEvents;
        };

        /**
         * The v1/events/list handler.
         */
        class EventsList:public RestApiInV1::InesonicCustomerRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator Class used to authenticate a customer.
                 *
                 * \param[in] eventDatabaseApi      Class used to manage events.
                 */
                EventsList(
                    CustomerAuthenticator* customerAuthenticator,
                    Events*                eventDatabaseApi
                );

                ~EventsList() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current event database API.
                 */
                Events* currentEvents;
        };

        /**
         * The v1/events/create handler.
         */
        class EventsCreate:public RestApiInV1::InesonicCustomerRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator Class used to authenticate a customer.
                 *
                 * \param[in] monitorDatabaseApi    Class used to manage the customer's monitors.
                 *
                 * \param[in] eventProcessor        Class used to report events.
                 */
                EventsCreate(
                    CustomerAuthenticator* customerAuthenticator,
                    Monitors*              monitorDatabaseApi,
                    EventProcessor*        eventProcessor
                );

                ~EventsCreate() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current monitors database API.
                 */
                Monitors* currentMonitors;

                /**
                 * The event processor used to send event notifications.
                 */
                EventProcessor* currentEventProcessor;
        };

        /**
         * The v1/status/get handler.
         */
        class StatusGet:public RestApiInV1::InesonicCustomerRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator Class used to authenticate a customer.
                 *
                 * \param[in] monitorDatabaseApi    Class used to manage the customer's monitors.
                 *
                 * \param[in] eventDatabaseApi      Class used to manage events and status.
                 */
                StatusGet(
                    CustomerAuthenticator* customerAuthenticator,
                    Monitors*              monitorDatabaseApi,
                    Events*                eventDatabaseApi
                );

                ~StatusGet() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current monitors database API.
                 */
                Monitors* currentMonitors;

                /**
                 * The current event database API.
                 */
                Events* currentEvents;
        };

        /**
         * The v1/status/list handler.
         */
        class StatusList:public RestApiInV1::InesonicCustomerRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator Class used to authenticate a customer.
                 *
                 * \param[in] eventDatabaseApi      Class used to manage events and status.
                 */
                StatusList(
                    CustomerAuthenticator* customerAuthenticator,
                    Events*                eventDatabaseApi
                );

                ~StatusList() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current event database API.
                 */
                Events* currentEvents;
        };

        /**
         * The v1/multiple/list handler.
         */
        class MultipleList:public RestApiInV1::InesonicCustomerRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator Class used to authenticate a customer.
                 *
                 * \param[in] hostSchemeDatabaseApi Class used to manage hosts and schemes.
                 *
                 * \param[in] monitorDatabaseApi    Class used to manage the customer's monitors.
                 *
                 * \param[in] eventDatabaseApi      Class used to manage events and status.
                 */
                MultipleList(
                    CustomerAuthenticator* customerAuthenticator,
                    HostSchemes*           hostSchemeDatabaseApi,
                    Monitors*              monitorDatabaseApi,
                    Events*                eventDatabaseApi
                );

                ~MultipleList() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current host/scheme database API.
                 */
                HostSchemes* currentHostSchemes;

                /**
                 * The current monitors database API.
                 */
                Monitors* currentMonitors;

                /**
                 * The current event database API.
                 */
                Events* currentEvents;
        };

        /**
         * The v1/latency/list handler.
         */
        class LatencyList:public RestApiInV1::InesonicCustomerRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator   Class used to authenticate a customer.
                 *
                 * \param[in] latencyInterfaceManager The latency interface manager, used to get latency data.
                 *
                 * \param[in] serverDatabaseApi       Class used to manager server data.
                 */
                LatencyList(
                    CustomerAuthenticator*   customerAuthenticator,
                    LatencyInterfaceManager* latencyInterfaceManager,
                    Servers*                 serverDatabaseApi
                );

                ~LatencyList() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current latency database API.
                 */
                LatencyInterfaceManager* currentLatencyInterfaceManager;

                /**
                 * The current servers database API.
                 */
                Servers* currentServers;
        };

        /**
         * The v1/latency/plot handler.
         */
        class LatencyPlot:public RestApiInV1::InesonicCustomerBinaryRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator Class used to authenticate a customer.
                 *
                 * \param[in] latencyPlotter        The plotter used to generate latency plots.
                 */
                LatencyPlot(CustomerAuthenticator* customerAuthenticator, LatencyPlotter* latencyPlotter);

                ~LatencyPlot() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::BinaryResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current latency plotter.
                 */
                LatencyPlotter* currentLatencyPlotter;
        };

        /**
         * The v1/customer/pause handler.
         */
        class CustomerPause:public RestApiInV1::InesonicCustomerRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator Class used to authenticate a customer.
                 *
                 * \param[in] serverAdministrator   The server administration class.
                 */
                CustomerPause(
                    CustomerAuthenticator* customerAuthenticator,
                    ServerAdministrator*   ServerAdministrator
                );

                ~CustomerPause() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current server administration instance.
                 */
                ServerAdministrator* currentServerAdministrator;
        };

        /**
         * The v1/resource/available handler.
         */
        class ResourceAvailable:public RestApiInV1::InesonicCustomerRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator Class used to authenticate a customer.
                 *
                 * \param[in] resourceDatabaseApi   Class used to store new resource data.
                 */
                ResourceAvailable(CustomerAuthenticator* customerAuthenticator, Resources* resourceDatabaseApi);

                ~ResourceAvailable() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current resource database API.
                 */
                Resources* currentResources;
        };

        /**
         * The v1/resource/create handler.
         */
        class ResourceCreate:public RestApiInV1::InesonicCustomerRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator Class used to authenticate a customer.
                 *
                 * \param[in] resourceDatabaseApi   Class used to store new resource data.
                 */
                ResourceCreate(CustomerAuthenticator* customerAuthenticator, Resources* resourceDatabaseApi);

                ~ResourceCreate() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current resource database API.
                 */
                Resources* currentResources;
        };

        /**
         * The v1/resource/list handler.
         */
        class ResourceList:public RestApiInV1::InesonicCustomerRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator Class used to authenticate a customer.
                 *
                 * \param[in] resourceDatabaseApi   Class used to store new resource data.
                 */
                ResourceList(CustomerAuthenticator* customerAuthenticator, Resources* resourceDatabaseApi);

                ~ResourceList() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::JsonResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current resource database API.
                 */
                Resources* currentResources;
        };

        /**
         * The v1/resource/plot handler.
         */
        class ResourcePlot:public RestApiInV1::InesonicCustomerBinaryRestHandler, private RestHelpers {
            public:
                /**
                 * Constructor
                 *
                 * \param[in] customerAuthenticator Class used to authenticate a customer.
                 *
                 * \param[in] resourcePlotter       The plotter used to generate resource plots.
                 */
                ResourcePlot(CustomerAuthenticator* customerAuthenticator, ResourcePlotter* resourcePlotter);

                ~ResourcePlot() override;

            protected:
                /**
                 * Method you can overload to receive a request and send a return response.  This method will only be
                 * triggered if the message meets the authentication requirements.
                 *
                 * \param[in] path       The request path.
                 *
                 * \param[in] customerId The customer Id of the customer making the request.
                 *
                 * \param[in] request    The request data encoded as a JSON document.
                 *
                 * \param[in] threadId   The ID used to uniquely identify this thread while in flight.
                 *
                 * \return The response to return, also encoded as a JSON document.
                 */
                RestApiInV1::BinaryResponse processAuthenticatedRequest(
                    const QString&       path,
                    unsigned long        customerId,
                    const QJsonDocument& request,
                    unsigned             threadId
                ) override;

            private:
                /**
                 * The current latency plotter.
                 */
                ResourcePlotter* currentResourcePlotter;
        };

        /**
         * The v1/capabilities/get handler.
         */
        CapabilitiesGet capabilitiesGet;

        /**
         * The v1/hosts/get handler.
         */
        HostsGet hostsGet;

        /**
         * The v1/hosts/list handler.
         */
        HostsList hostsList;

        /**
         * The v1/monitors/get handler.
         */
        MonitorsGet monitorsGet;

        /**
         * The v1/monitors/list handler.
         */
        MonitorsList monitorsList;

        /**
         * The v1/monitors/update handler.
         */
        MonitorsUpdate monitorsUpdate;

        /**
         * The v1/regions/get handler.
         */
        RegionsGet regionsGet;

        /**
         * The v1/regions/list handler.
         */
        RegionsList regionsList;

        /**
         * The v1/events/get handler.
         */
        EventsGet eventsGet;

        /**
         * The v1/events/list handler.
         */
        EventsList eventsList;

        /**
         * The v1/events/create handler.
         */
        EventsCreate eventsCreate;

        /**
         * The v1/status/get handler.
         */
        StatusGet statusGet;

        /**
         * The v1/status/list handler.
         */
        StatusList statusList;

        /**
         * The v1/multiple/list handler.
         */
        MultipleList multipleList;

        /**
         * The v1/latency/list handler.
         */
        LatencyList latencyList;

        /**
         * The v1/latency/plot handler.
         */
        LatencyPlot latencyPlot;

        /**
         * The v1/customer/pause handler.
         */
        CustomerPause customerPause;

        /**
         * The v1/resource/available handler.
         */
        ResourceAvailable resourceAvailable;

        /**
         * The v1/resource/create handler.
         */
        ResourceCreate resourceCreate;

        /**
         * The v1/resource/list handler.
         */
        ResourceList resourceList;

        /**
         * The v1/resource/plot handler.
         */
        ResourcePlot resourcePlot;
};

#endif
