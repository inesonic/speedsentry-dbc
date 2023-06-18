/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref OutboundRestApi class.
***********************************************************************************************************************/

/* .. sphinx-project polling_server */

#ifndef OUTBOUND_REST_API_H
#define OUTBOUND_REST_API_H

#include <QObject>
#include <QUrl>
#include <QQueue>
#include <QTimer>
#include <QString>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <rest_api_out_v1_server.h>
#include <rest_api_out_v1_inesonic_rest_handler.h>

/**
 * Class that provides a generic outbound REST API to a single server.  You can use this class to issue messages to
 * remote endpoints on a server in a controlled fashion.  This function merges an outbound server instance with
 * ephemeral REST API instances.
 *
 * To use, instantiate an instance and then call the \ref startReporting method (or trigger it).
 */
class OutboundRestApi:public RestApiOutV1::Server {
    Q_OBJECT

    public:
        /**
         * The time to allow an instance to be idle before garbage collecting.
         */
        static constexpr unsigned maximumIdleTimeSeconds = 3600;

        /**
         * Retry interval in seconds.
         */
        static constexpr unsigned retryInterval = 60;

        /**
         * Constructor
         *
         * \param[in] networkAccessManager The network settings manager.
         *
         * \param[in] authority            The authority to send messages to.
         *
         * \param[in] timeDeltaSlug        The URL used to determine time deltas.
         *
         * \param[in] garbageCollect       If true, then this server should deallocate itself after an extended
         *                                 period of non-use.
         *
         * \param[in] parent               Pointer to the parent object.
         */
        OutboundRestApi(
            QNetworkAccessManager* networkAccessManager,
            const QUrl&            authority,
            const QString&         timeDeltaSlug = defaultTimeDeltaSlug,
            bool                   garbageCollect = false,
            QObject*               parent = Q_NULLPTR
        );

        ~OutboundRestApi() override;

        /**
         * Method you can use to send a message to a remote host.
         *
         * \param[in] endpoint The endpoint to send the message to.
         *
         * \param[in] message  The message to be sent.
         *
         * \param[in] logText  Text to be dumped to stdout on successful completion.
         */
        void postMessage(
            const QString&       endpoint,
            const QJsonDocument& message,
            const QString&       logText
        );

        /**
         * Method you can use to send a message to a remote host.
         *
         * \param[in] endpoint The endpoint to send the message to.
         *
         * \param[in] message  The message to be sent.
         *
         * \param[in] logText  Text to be dumped to stdout on successful completion.
         */
        void postMessage(
            const QString&      endpoint,
            const QJsonObject&  message,
            const QString&      logText
        );

        /**
         * Method you can use to send a message to a remote host.  This version will trigger a callback to an old
         * style Qt slot.
         *
         * \param[in] endpoint The endpoint to send the message to.
         *
         * \param[in] message  The message to be sent.
         *
         * \param[in] logText  Text to be dumped to stdout on successful completion.
         *
         * \param[in] context  An optional context to include with the response.
         *
         * \param[in] receiver The class receiving the response.
         *
         * \param[in] slot     The receiver slot.
         */
        void postMessage(
            const QString&       endpoint,
            const QJsonDocument& message,
            const QString&       logText,
            void*                context,
            QObject*             receiver,
            const char*          slot
        );

        /**
         * Method you can use to send a message to a remote host.  This version will trigger a callback to an old
         * style Qt slot.
         *
         * \param[in] endpoint The endpoint to send the message to.
         *
         * \param[in] message  The message to be sent.
         *
         * \param[in] logText  Text to be dumped to stdout on successful completion.
         *
         * \param[in] context  An optional context to include with the response.
         *
         * \param[in] receiver The class receiving the response.
         *
         * \param[in] slot     The receiver slot.
         */
        void postMessage(
            const QString&     endpoint,
            const QJsonObject& message,
            const QString&     logText,
            void*              context,
            QObject*           receiver,
            const char*        slot
        );

        /**
         * Method you can use to send a message to a remote host.
         *
         * \param[in] endpoint The endpoint to send the message to.
         *
         * \param[in] logText  Text to be dumped to stdout on successful completion.
         */
        void postMessage(const QString& endpoint, const QString& logText);

        /**
         * Method you can use to send a message to a remote host.  This version will trigger a callback to an old
         * style Qt slot.
         *
         * \param[in] endpoint The endpoint to send the message to.
         *
         * \param[in] logText  Text to be dumped to stdout on successful completion.
         *
         * \param[in] context  An optional context to include with the response.
         *
         * \param[in] receiver The class receiving the response.
         *
         * \param[in] slot     The receiver slot.
         */
        void postMessage(
            const QString& endpoint,
            const QString& logText,
            void*          context,
            QObject*       receiver,
            const char*    slot
        );

    signals:
        /**
         * Signal that is emitted when it's time to garbage collect this instance.
         *
         * \param[out] thisInstance A pointer to this instance.
         */
        void performGarbageCollection(OutboundRestApi* thisInstance);

        /**
         * Signal used internally to trigger a message to a remote host with a callback.  The use of the signal allows
         * us to queue messages across threads.
         *
         * \param[out] endpoint The endpoint to send the message to.
         *
         * \param[out] message  The message to be sent.
         *
         * \param[out] logText  Text to be dumped to stdout on successful completion.
         *
         * \param[out] context  A context to include in the response slot.
         *
         * \param[out] receiver The class receiving the response.
         *
         * \param[in] slot      The receiver slot.
         */
        void sendMessage(
            const QString&       endpoint,
            const QJsonDocument& message,
            const QString&       logText,
            void*                context,
            QObject*             receiver,
            const QString&       slot
        );

        /**
         * Signal used internally to trigger a callback after a message has been sent successfully.
         *
         * \param[in] context The context to be sent.
         */
        void sendCallback(void* context);

    private slots:
        /**
         * Slot that send messages to remote hosts.  This slot is triggered by the \ref sendMessage signal.
         *
         * \param[in] endpoint    The endpoint to send the message to.
         *
         * \param[in] message     The message to be sent.
         *
         * \param[in] logText     Text to be dumped to stdout on successful completion.
         *
         * \param[in] context     A context to include in the response slot.
         *
         * \param[in] receiver    The class receiving the response.
         *
         * \param[in] slot        The receiver slot.
         */
        void processSendMessage(
            const QString&       endpoint,
            const QJsonDocument& message,
            const QString&       logText,
            void*                context,
            QObject*             receiver,
            const QString&       slot
        );

        /**
         * Slot that is triggered when an event times out.
         */
        void startNextAction();

        /**
         * Slot that is triggered on successful transmission of a request.
         *
         * \param[in] jsonData The JSON response data.
         */
        void jsonResponse(const QJsonDocument& jsonData);

        /**
         * Slot that is triggered when a transmission fails.
         *
         * \param[in] errorString a string providing an error message.
         */
        void requestFailed(const QString& errorString);

    private:
        /**
         * Class that contains work for this server to perform.
         */
        class Request {
            public:
                Request():
                    currentContext(nullptr),
                    currentReceiver(nullptr) {}

                /**
                 * Constructor.
                 *
                 * \param[in] endpoint The end point to send this request message to.
                 *
                 * \param[in] message  The message to be sent.
                 *
                 * \param[in] logText  Log text to show on successful completion.
                 *
                 * \param[in] context  Context data send to the receiver.
                 *
                 * \param[in] receiver A receiver object to receive notification upon completion.
                 *
                 * \param[in] slot     A slot to send a notification to.
                 */
                Request(
                        const QString&       endpoint,
                        const QJsonDocument& message,
                        const QString&       logText,
                        void*                context = nullptr,
                        QObject*             receiver = nullptr,
                        const QString&       slot = QString()
                    ):currentEndpoint(
                        endpoint
                    ),currentMessage(
                        message
                    ),currentLogText(
                        logText
                    ),currentContext(
                        context
                    ),currentReceiver(
                        receiver
                    ),currentSlot(
                        slot
                    ) {}

                /**
                 * Copy constructor
                 *
                 * \param[in] other The instance to assign to this instance.
                 */
                Request(
                        const Request& other
                    ):currentEndpoint(
                        other.currentEndpoint
                    ),currentMessage(
                        other.currentMessage
                    ),currentLogText(
                        other.currentLogText
                    ),currentContext(
                        other.currentContext
                    ),currentReceiver(
                        other.currentReceiver
                    ),currentSlot(
                        other.currentSlot
                    ) {}

                /**
                 * Move constructor
                 *
                 * \param[in] other The instance to assign to this instance.
                 */
                Request(
                        Request&& other
                    ):currentEndpoint(
                        other.currentEndpoint
                    ),currentMessage(
                        other.currentMessage
                    ),currentLogText(
                        other.currentLogText
                    ),currentContext(
                        other.currentContext
                    ),currentReceiver(
                        other.currentReceiver
                    ),currentSlot(
                        other.currentSlot
                    ) {}

                /**
                 * Method you can use to determine the endpoint for this request.
                 *
                 * \return Returns the endpoint for this request.
                 */
                const QString& endpoint() const {
                    return currentEndpoint;
                }

                /**
                 * Method you can use to determine the message to be sent.
                 *
                 * \return Returns the message content to be sent.
                 */
                const QJsonDocument& message() const {
                    return currentMessage;
                }

                /**
                 * Method you can use to determine the log text to show on successful completion.
                 *
                 * \return Returns the log text to be shown on completion.
                 */
                const QString& logText() const {
                    return currentLogText;
                }

                /**
                 * Method you can use to obtain the context for responses.
                 *
                 * \return Returns a void pointer holding an optional context.
                 */
                void* context() const {
                    return currentContext;
                }

                /**
                 * Method you can use to obtain the receiver for a response notification.
                 *
                 * \return Returns the receiver to send a response notification to.
                 */
                QObject* receiver() const {
                    return currentReceiver;
                }

                /**
                 * Method you can use to obtain the slot name for the slot to send a response message to.
                 *
                 * \return Returns the slot name.
                 */
                const QString& slot() const {
                    return currentSlot;
                }

                /**
                 * Assignment operator.
                 *
                 * \param[in] other The instance to assign to this instance.
                 *
                 * \return Returns a reference to this instance.
                 */
                Request& operator=(const Request& other) {
                    currentEndpoint = other.currentEndpoint;
                    currentMessage  = other.currentMessage;
                    currentLogText  = other.currentLogText;
                    currentContext  = other.currentContext;
                    currentReceiver = other.currentReceiver;
                    currentSlot     = other.currentSlot;

                    return *this;
                }

                /**
                 * Assignment operator (move semantics).
                 *
                 * \param[in] other The instance to assign to this instance.
                 *
                 * \return Returns a reference to this instance.
                 */
                Request& operator=(Request&& other) {
                    currentEndpoint = other.currentEndpoint;
                    currentMessage  = other.currentMessage;
                    currentLogText  = other.currentLogText;
                    currentContext  = other.currentContext;
                    currentReceiver = other.currentReceiver;
                    currentSlot     = other.currentSlot;

                    return *this;
                }

            private:
                /**
                 * The endpoint we're sending this message to.
                 */
                QString currentEndpoint;

                /**
                 * The message to be sent to the endpoint.
                 */
                QJsonDocument currentMessage;

                /**
                 * Log text to display on successful completion.
                 */
                QString currentLogText;

                /**
                 * An optional context to send to a slot on completion.
                 */
                void* currentContext;

                /**
                 * An optional receiver object to receive notificaiton on successful completion.
                 */
                QObject* currentReceiver;

                /**
                 * The slot to send the notification to.
                 */
                QString currentSlot;
        };

        /**
         * Enumeration of timer actions.
         */
        enum class TimerAction {
            /**
             * Indicates no pending action.
             */
            NONE,

            /**
             * Indicates we should retry our request.
             */
            RETRY,

            /**
             * Indicates we should cleanup our request.
             */
            CLEANUP,

            /**
             * Indicates we should be garbage collected.
             */
            GARBAGE_COLLECTION
        };

        /**
         * Method that starts the next request.
         */
        void startSend(const Request& request);

        /**
         * Method that pops the head from the pending requests queue and begins the next action.
         */
        void startNext();

        /**
         * Queue of pending requests.
         */
        QQueue<Request> pendingRequests;

        /**
         * Pointer to the in-flight request.
         */
        RestApiOutV1::InesonicRestHandler* activeRequest;

        /**
         * Flag that indicates if we should perform garbage collection.
         */
        bool currentPerformGarbageCollection;

        /**
         * The next timer action to be performed.
         */
        TimerAction timerAction;

        /**
         * Timer used to manage retries and garbage collection.
         */
        QTimer eventTimer;
};

#endif
