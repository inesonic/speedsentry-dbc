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
* This header defines the \ref MonitorUpdater class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef MONITOR_UPDATER_H
#define MONITOR_UPDATER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QMap>
#include <QHash>
#include <QSet>
#include <QMutex>
#include <QUrl>

#include <cstdint>

#include "host_scheme.h"
#include "host_schemes.h"
#include "scheme_host.h"
#include "scheme_host_path.h"
#include "monitor.h"
#include "monitors.h"
#include "sql_helpers.h"

class QTimer;
class CustomerCapabilities;
class DatabaseManager;
class ServerAdministrator;

/**
 * Class you can use to update the database after receiving a set of new monitor settings.
 */
class MonitorUpdater:public QObject {
    Q_OBJECT

    public:
        /**
         * Value used to represent a host/scheme ID.
         */
        typedef Monitor::MonitorId MonitorId;

        /**
         * Type used to represent a customer ID.
         */
        typedef HostScheme::CustomerId CustomerId;

        /**
         * Type used to represent a host/scheme ID.
         */
        typedef HostScheme::HostSchemeId HostSchemeId;

        /**
         * Value used to indicate user ordering in the GUI.
         */
        typedef Monitor::UserOrdering UserOrdering;

        /**
         * Enumeration of supported access methods.
         */
        typedef Monitor::Method Method;

        /**
         * Enumeration of content check modes.
         */
        typedef Monitor::ContentCheckMode ContentCheckMode;

        /**
         * Enumeration of supported POST content types.
         */
        typedef Monitor::ContentType ContentType;

        /**
         * Type used to represent a list of keywords.
         */
        typedef Monitor::KeywordList KeywordList;

        /**
         * Value used to indicate an invalid monitor ID.
         */
        static constexpr MonitorId invalidMonitorId = Monitor::invalidMonitorId;

        /**
         * Value used to indicate an invalid host/scheme ID.
         */
        static constexpr HostSchemeId invalidHostSchemeId = HostScheme::invalidHostSchemeId;

        /**
         * Value used to indicate an invalid customer ID.
         */
        static constexpr CustomerId invalidCustomerId = HostScheme::invalidCustomerId;

        /**
         * Type used to represent a collection of monitors.
         */
        typedef QList<Monitor> MonitorList;

        /**
         * Type used to represent a collection of monitors.
         */
        typedef QHash<SchemeHostPath, Monitor> MonitorsBySchemeHostPath;

        /**
         * Class that encapsulates a received monitor entry to be processed.
         */
        class Entry {
            public:
                inline Entry() {
                    currentUserOrdering     = 0;
                    currentMethod           = Method::GET;
                    currentContentCheckMode = ContentCheckMode::NO_CHECK;
                    currentContentType      = ContentType::TEXT;
                }

                /**
                 * Constructor.
                 *
                 * \param[in] userOrdering     A zero based numerical user ordering.  The value is intended to assist
                 *                             user interfaces.
                 *
                 * \param[in] uri              The URI, either full or partial to be checked.  A partial URI will use
                 *                             the host and scheme of the previous entry.
                 *
                 * \param[in] method           The method used to check the URL.
                 *
                 * \param[in] contentCheckMode The content check mode for this monitor.
                 *
                 * \param[in] keywords         The list of content check keywords for this monitor.
                 *
                 * \param[in] contentType      The type of content to supply as part of a POST message.
                 *
                 * \param[in] userAgent        The user-agent string to be reported during a POST message.
                 *
                 * \param[in] postContent      An array of byte data to be sent as the post message.
                 */
                inline Entry(
                        UserOrdering       userOrdering,
                        const QUrl&        uri,
                        Method             method,
                        ContentCheckMode   contentCheckMode,
                        const KeywordList& keywords,
                        ContentType        contentType,
                        const QString&     userAgent,
                        const QByteArray&  postContent
                    ):currentUserOrdering(
                        userOrdering
                    ),currentUri(
                        uri
                    ),currentMethod(
                        method
                    ),currentContentCheckMode(
                        contentCheckMode
                    ),currentKeywords(
                        keywords
                    ),currentContentType(
                        contentType
                    ),currentUserAgent(
                        userAgent
                    ),currentPostContent(
                        postContent
                    ) {}

                /**
                 * Copy constructor
                 *
                 * \param[in] other The instance to assign to this instance.
                 */
                inline Entry(
                        const Entry& other
                    ):currentUserOrdering(
                        other.currentUserOrdering
                    ),currentUri(
                        other.currentUri
                    ),currentMethod(
                        other.currentMethod
                    ),currentContentCheckMode(
                        other.currentContentCheckMode
                    ),currentKeywords(
                        other.currentKeywords
                    ),currentContentType(
                        other.currentContentType
                    ),currentUserAgent(
                        other.currentUserAgent
                    ),currentPostContent(
                        other.currentPostContent
                    ) {}

                /**
                 * Move constructor
                 *
                 * \param[in] other The instance to assign to this instance.
                 */
                inline Entry(
                        Entry&& other
                    ):currentUserOrdering(
                        other.currentUserOrdering
                    ),currentUri(
                        other.currentUri
                    ),currentMethod(
                        other.currentMethod
                    ),currentContentCheckMode(
                        other.currentContentCheckMode
                    ),currentKeywords(
                        other.currentKeywords
                    ),currentContentType(
                        other.currentContentType
                    ),currentUserAgent(
                        other.currentUserAgent
                    ),currentPostContent(
                        other.currentPostContent
                    ) {}

                ~Entry() = default;

                /**
                 * Method you can use to obtain the user ordering for this monitor.
                 *
                 * \return Returns the user ordering for this monitor.
                 */
                inline UserOrdering userOrdering() const {
                    return currentUserOrdering;
                }

                /**
                 * Method you can use to update the user ordering for this entry.
                 *
                 * \poram[in] newUserOrdering The new user ordering value.
                 */
                inline void setUserOrdering(UserOrdering newUserOrdering) {
                    currentUserOrdering = newUserOrdering;
                }

                /**
                 * Method you can use to obtain the current URI for this monitor, either full or partial.  A URI that
                 * is missing the host and scheme should use the host and scheme of the previous entry.
                 *
                 * \return Returns the URI for this monitor.
                 */
                inline const QUrl& uri() const {
                    return currentUri;
                }

                /**
                 * Method you can use to obtain the method used to access this endpoint.
                 *
                 * \return Returns the method used to access this endpoint.
                 */
                inline Method method() const {
                    return currentMethod;
                }

                /**
                 * Method you can use to determine the desired content check mode for this endpoint.
                 *
                 * \return Returns the endpoint content check mode.
                 */
                inline ContentCheckMode contentCheckMode() const {
                    return currentContentCheckMode;
                }

                /**
                 * Method you can use to obtain the content check keyword list.
                 *
                 * \return Returns the content check keyword list.
                 */
                inline const KeywordList& keywords() const {
                    return currentKeywords;
                }

                /**
                 * Method you can use to get the content type.
                 *
                 * \return Returns the content type for this POST.
                 */
                inline ContentType contentType() const {
                    return currentContentType;
                }

                /**
                 * Method you can use to obtain the user agent string.
                 *
                 * \return Returns the user agent string.
                 */
                inline const QString& userAgent() const {
                    return currentUserAgent;
                }

                /**
                 * Method you can use to obtain the post content.
                 *
                 * \return Returns the post content.
                 */
                inline const QByteArray& postContent() const {
                    return currentPostContent;
                }

                /**
                 * Assignment operator
                 *
                 * \param[in] other The instance to assign to this instance.
                 *
                 * \return Returns a reference to this instance.
                 */
                inline Entry& operator=(const Entry& other) {
                    currentUserOrdering     = other.currentUserOrdering;
                    currentUri              = other.currentUri;
                    currentMethod           = other.currentMethod;
                    currentContentCheckMode = other.currentContentCheckMode;
                    currentKeywords         = other.currentKeywords;
                    currentContentType      = other.currentContentType;
                    currentUserAgent        = other.currentUserAgent;
                    currentPostContent      = other.currentPostContent;

                    return *this;
                }

                /**
                 * Assignment operator (move semantics)
                 *
                 * \param[in] other The instance to assign to this instance.
                 */
                inline Entry& operator=(Entry&& other) {
                    currentUserOrdering     = other.currentUserOrdering;
                    currentUri              = other.currentUri;
                    currentMethod           = other.currentMethod;
                    currentContentCheckMode = other.currentContentCheckMode;
                    currentKeywords         = other.currentKeywords;
                    currentContentType      = other.currentContentType;
                    currentUserAgent        = other.currentUserAgent;
                    currentPostContent      = other.currentPostContent;

                    return *this;
                }

            private:
                /**
                 * The zero based user ordering on the website and/or GUI.
                 */
                UserOrdering currentUserOrdering;

                /**
                 * The current URI, either full or partial.
                 */
                QUrl currentUri;

                /**
                 * The method used to access this endpoint.
                 */
                Method currentMethod;

                /**
                 * The content check mode.
                 */
                ContentCheckMode currentContentCheckMode;

                /**
                 * The list of content check keywords.
                 */
                KeywordList currentKeywords;

                /**
                 * The current content type for this POST.
                 */
                ContentType currentContentType;

                /**
                 * the current user agent string.
                 */
                QString currentUserAgent;

                /**
                 * The current post content.
                 */
                QByteArray currentPostContent;
        };

        /**
         * Trivial class used to report errors during updates.
         */
        class Error {
            public:
                Error():currentUserOrdering(0xFFFF) {}

                /**
                 * Constructor
                 *
                 * \param[in] userOrdering The user ordering where the error was located.
                 *
                 * \param[in] errorMessage A textual error message.
                 */
                inline Error(
                        UserOrdering   userOrdering,
                        const QString& errorMessage
                    ):currentUserOrdering(
                        userOrdering
                    ),currentErrorMessage(
                        errorMessage
                    ) {}

                /**
                 * Copy constructor.
                 *
                 * \param[in] other The instance to assign to this instance.
                 */
                inline Error(
                        const Error& other
                    ):currentUserOrdering(
                        other.currentUserOrdering
                    ),currentErrorMessage(
                        other.currentErrorMessage
                    ) {}

                /**
                 * Move constructor.
                 *
                 * \param[in] other The instance to assign to this instance.
                 */
                inline Error(
                        Error&& other
                    ):currentUserOrdering(
                        other.currentUserOrdering
                    ),currentErrorMessage(
                        other.currentErrorMessage
                    ) {}

                /**
                 * Method you can use to determine the location of this error.
                 *
                 * \return Returns the user ordering where the error was found.
                 */
                inline UserOrdering userOrdering() const {
                    return currentUserOrdering;
                }

                /**
                 * Method you can use to obtain a textual error message.
                 *
                 * \return Returns a textual description of the error.
                 */
                const QString& errorMessage() const {
                    return currentErrorMessage;
                }

                /**
                 * Assignment operator.
                 *
                 * \param[in] other The instance to assign to this instance.
                 *
                 * \return Returns a reference to this instance.
                 */
                inline Error& operator=(const Error& other) {
                    currentUserOrdering = other.currentUserOrdering;
                    currentErrorMessage = other.currentErrorMessage;

                    return *this;
                }

                /**
                 * Assignment operator.
                 *
                 * \param[in] other The instance to assign to this instance.
                 *
                 * \return Returns a reference to this instance.
                 */
                inline Error& operator=(Error&& other) {
                    currentUserOrdering = other.currentUserOrdering;
                    currentErrorMessage = other.currentErrorMessage;

                    return *this;
                }

            private:
                /**
                 * The user ordering where the error was located.
                 */
                UserOrdering currentUserOrdering;

                /**
                 * A descriptive error message.
                 */
                QString currentErrorMessage;
        };

        /**
         * Type used to represent a list of monitor entries.
         */
        typedef QList<Entry> MonitorEntries;

        /**
         * Type used to represent a collection of errors during update.
         */
        typedef QList<Error> Errors;

        /**
         * Constructor
         *
         * \param[in] hostSchemes         The host/schemes database API.
         *
         * \param[in] monitors            The monitors database API.
         *
         * \param[in] serverAdministrator The server administration engine.
         */
        MonitorUpdater(HostSchemes* hostSchemes, Monitors* monitors, ServerAdministrator* serverAdministrator);

        ~MonitorUpdater() override;

        /**
         * Method you can use to update a host scheme in the database.
         *
         * \param[in] hostScheme The host/scheme to be updated.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool modifyHostScheme(const HostScheme& hostScheme, unsigned threadId = 0);

        /**
         * Method you can use to delete a single monitor from the database.  This method will also check the host
         * schemes and remove any host schemes that no longer have monitors tied to them.  Finally this method will
         * schedule the backend polling servers to be updated.
         *
         * \param[in] monitor  The monitor to be deleted.
         *
         * \param[in] threadId An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool deleteMonitor(const Monitor& monitor, unsigned threadId = 0);

        /**
         * Method you can use to delete all monitors tied to a specific host/scheme.  This method will also schedule
         * the backend polling servers to be updated.
         *
         * \param[in] hostScheme The monitor to be deleted.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool deleteHostScheme(const HostScheme& hostScheme, unsigned threadId = 0);

        /**
         * Method you can use to delete all monitors and host/scheme entries tied to a specific customer.  This method
         * will also schedule the backend polling servers to be updated.
         *
         * \param[in] customerId The monitor to be deleted.
         *
         * \param[in] threadId   An optional thread ID used to maintain independent per-thread database instances.
         *
         * \return Returns true on success.  Returns false on error.
         */
        bool deleteCustomer(Monitor::CustomerId customerId, unsigned threadId = 0);

        /**
         * Method you can use to process a list of monitor entries in order to update the database for a given
         * customer.
         *
         * \param[in]     customerCapabilities The customer capabilities instance used to discern both the customer ID
         *                                     and the features the customer can use.
         *
         * \param[in,out] newMonitorEntries    The new monitor entries to be processed for this customer.  The user
         *                                     ordering will be updated on exit.
         *
         * \param[in]     threadId             An optional thread ID used to maintain independent per-thread database
         *                                     instances.
         *
         * \return Returns error data.  An empty list of errors means no errors were found.
         */
        Errors update(
            const CustomerCapabilities& customerCapabilities,
            MonitorEntries&             newMonitorEntries,
            unsigned                    threadId = 0
        );

    signals:
        /**
         * Signal that is emitted to schedules a customer update.
         *
         * \param[in] customerId The zero based ID of the customer with pending updates.
         *
         * \param[in] deactivate If true, we should deactivate this customer.  If false, we should activate or
         *                       reactivate this customer.
         */
        void schedulePollingServerUpdate(unsigned long customerId, bool deactivate);

    private slots:
        /**
         * Slot that is triggered to schedules a customer update.
         *
         * \param[in] customerId The zero based ID of the customer with pending updates.
         *
         * \param[in] deactivate If true, we should deactivate this customer.  If false, we should activate or
         *                       reactivate this customer.
         */
        void performSchedulePollingServerUpdate(unsigned long customerId, bool deactivate);

        /**
         * Slot that is triggered to perform customer updates.
         */
        void performUpdates();

    private:
        /**
         * Delay before updates start.
         */
        static const unsigned updateDelaySeconds = 10; // 5 * 60;

        /**
         * The thread ID to be used for the timer.
         */
        static const unsigned timerThreadId = static_cast<unsigned>(-3);

        /**
         * Type used to track host schemes by value.
         */
        typedef QHash<SchemeHost, const HostScheme*> HostSchemesBySchemeHost;

        /**
         * Type used to store sorted monitor entries.
         */
        typedef QMap<UserOrdering, Entry*> SortedEntries;

        /**
         * Type used to track pending customer updates.
         */
        typedef QHash<Monitor::CustomerId, bool> PendingUpdates;

        /**
         * Type used to track pending updates by timestamp.
         */
        typedef QMap<unsigned long long, PendingUpdates> PendingUpdatesByTimestamp;

        /**
         * Type used to track the timestamp for pending customer updates.
         */
        typedef QHash<Monitor::CustomerId, unsigned long long> UpdateTimestampByCustomerId;

        /**
         * Method that sorts a set of incoming monitor entries for errors and sorts the entries.
         *
         * \param[out] errors               The resulting errors from the sorting and checking operation.
         *
         * \param[in]  customerCapabilities The customer capabilities used to validate the provided monitor entries.
         *
         * \param[in]  newMonitorEntries    The list of entries to be sorted and checked.  The entries user ordering
         *                                  will also be adjusted to remove gaps.
         *
         * \return Returns a list of sorted entries.
         */
        static SortedEntries sortAndCheckEntries(
            Errors&                     errors,
            const CustomerCapabilities& customerCapabilities,
            MonitorEntries&             newMonitorEntries
        );

        /**
         * Method that hashes a list of known host/scheme instances for fast lookup.
         *
         * \param[in] hostSchemes The hash of host/scheme instances to be processed.
         *
         * \return Returns a hash of host/scheme instances by SchemeHost.
         */
        static HostSchemesBySchemeHost hashHostSchemesBySchemeHost(const HostSchemes::HostSchemeHash& hostSchemes);

        /**
         * Method that converts a URL to a slug, that is path plus query string.
         *
         * \param[in] url The URL to be converted.
         *
         * \return Returns the slug.
         */
        static QString urlToSlug(const QUrl& url);

        /**
         * Method that creates a new monitor instance on the heap from provided data.
         *
         * \param[in] customerId   The customer ID to tie to this monitor.
         *
         * \param[in] hostSchemeId The host/scheme ID to tie to this monitor.
         *
         * \param[in] slug         The slug to assign to this monitor.
         *
         * \param[in] entry        The monitor entry.
         *
         * \param[in] threadId     The thread ID used to select a database instance.
         *
         * \return Returns a pointer to the newly created monitor.
         */
        Monitor* createMonitor(
            CustomerId     customerId,
            HostSchemeId   hostSchemeId,
            const QString& slug,
            const Entry&   entry,
            unsigned       threadId
        );

        /**
         * Method that update a monitor from a monitor entry.
         *
         * \param[in,out] monitor      The monitor to be updated, if needed.
         *
         * \param[in]     monitorEntry The monitor entry to use as a source for changes.
         *
         * \return Returns true if the monitor was modified.  Returns false if the monitor was not modified.
         */
        static bool updateMonitor(Monitor* monitor, const Entry& monitorEntry);

        /**
         * Method that identifies and deletes old monitors from the database.
         *
         * \param[in,out] errors           A list of errors to be updated should new errors be found.
         *
         * \param[in]     existingMonitors A hash holding the existing monitors.
         *
         * \param[in]     threadId         A thread ID used to select a database instance.
         */
        void deleteUnusedMonitors(
            Errors&                                   errors,
            const Monitors::MonitorsBySchemeHostPath& existingMonitors,
            unsigned                                  threadId
        );

        /**
         * Method that identifies and deletes old host/schemes from the database.
         *
         * \param[in,out] errors              A list of errors to be updated should new errors be found.
         *
         * \param[in]     existingHostSchemes A hash holding the existing host/schemes.
         *
         * \param[in]     usedHostSchemes     A set holding the host/schemes that are still being used.
         *
         * \param[in]     threadId            A thread ID used to select a database instance.
         */
        void deleteUnusedHostSchemes(
            Errors&                            errors,
            const HostSchemes::HostSchemeHash& existingHostSchemes,
            const QSet<const HostScheme*>&     usedHostSchemes,
            unsigned                           threadId
        );

        /**
         * Template function that deletes entries from an iterable container.
         *
         * \param[in] container The container to purge all entries from.
         */
        template<typename T> void deleteContents(T& container) {
            for (typename T::iterator it=container.begin(),end=container.end() ; it!=end ; ++it) {
                delete *it;
            }
        }

        /**
         * The current host/schemes database API.
         */
        HostSchemes* currentHostSchemes;

        /**
         * The current monitors database API.
         */
        Monitors* currentMonitors;

        /**
         * The server administrator used to push updates.
         */
        ServerAdministrator* currentServerAdministrator;

        /**
         * Timer used to perform deferred updates.
         */
        QTimer* updateTimer;

        /**
         * Map of customers requiring updates by update time.
         */
        PendingUpdatesByTimestamp pendingUpdatesByTimestamp;

        /**
         * Map of customers with update times requiring updates.
         */
        UpdateTimestampByCustomerId updateTimestampByCustomerId;
};

#endif
