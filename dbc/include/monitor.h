/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref Monitor class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef MONITOR_H
#define MONITOR_H

#include <QString>
#include <QByteArray>
#include <QList>

#include <cstdint>

#include "customer_capabilities.h"
#include "host_scheme.h"

class Monitors;
class PostSetting;

/**
 * Class used to hold information about a single customer monitor.
 */
class Monitor {
    friend class Monitors;

    public:
        /**
         * Value used to represent a host/scheme ID.
         */
        typedef std::uint32_t MonitorId;

        /**
         * Type used to represent a customer ID.
         */
        typedef CustomerCapabilities::CustomerId CustomerId;

        /**
         * Type used to represent a host/scheme ID.
         */
        typedef HostScheme::HostSchemeId HostSchemeId;

        /**
         * Value used to indicate user ordering in the GUI.
         */
        typedef std::uint16_t UserOrdering;

        /**
         * Enumeration of supported access methods.
         */
        enum class Method {
            /**
             * Indicates access is via GET.
             */
            GET,

            /**
             * Indicates access is via HEAD.
             */
            HEAD,

            /**
             * Indicates access is via POST.
             */
            POST,

            /**
             * Indicates access is via PUT.
             */
            PUT,

            /**
             * Indicates access is via DELETE.
             */
            DELETE,

            /**
             * Indicates access is via OPTIONS.
             */
            OPTIONS,

            /**
             * Indicates access is via PATCH.
             */
            PATCH
        };

        /**
         * Enumeration of content check modes.
         */
        enum class ContentCheckMode {
            /**
             * Indicates no content checking.
             */
            NO_CHECK,

            /**
             * Indicates check for content matching (via MD5 sum or similar)
             */
            CONTENT_MATCH,

            /**
             * Indicates check for any of the supplied keywords.
             */
            ANY_KEYWORDS,

            /**
             * Indicates check for all of the supplied keywords.
             */
            ALL_KEYWORDS,

            /**
             * Indicates smart(er) content checking.
             */
            SMART_CONTENT_MATCH
        };

        /**
         * Enumeration of supported POST content types.
         */
        enum class ContentType {
            /**
             * Indicates JSON content.
             */
            JSON,

            /**
             * Indicates XML content.
             */
            XML,

            /**
             * Indicates text content.
             */
            TEXT
        };

        /**
         * Enumeration of monitor status values.
         */
        enum class MonitorStatus {
            /**
             * Indicates that the status of the monitor is unknown.
             */
            UNKNOWN,

            /**
             * Indicates that the monitor appears to be working.
             */
            WORKING,

            /**
             * Indicates that the monitor appears to have failed.
             */
            FAILED
        };

        /**
         * Type used to represent a list of keywords.
         */
        typedef QByteArrayList KeywordList;

        /**
         * Value used to indicate an invalid monitor ID.
         */
        static constexpr MonitorId invalidMonitorId = 0;

        /**
         * Value used to indicate an invalid host/scheme ID.
         */
        static constexpr HostSchemeId invalidHostSchemeId = HostScheme::invalidHostSchemeId;

        /**
         * Value used to indicate an invalid customer ID.
         */
        static constexpr CustomerId invalidCustomerId = HostScheme::invalidCustomerId;

    private:
        /**
         * Constructor.
         *
         * \param[in] monitorId        The globally unique monitor ID.
         *
         * \param[in] customerId       The customer ID of the customer owning this monitor.
         *
         * \param[in] hostSchemeId     The host/scheme ID indicating the server and scheme this monitor is tied to.
         *
         * \param[in] userOrdering     A zero based numerical user ordering.  The value is intended to assist user
         *                             interfaces.
         *
         * \param[in] path             The path under the scheme and host to be checked.
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
        Monitor(
            MonitorId          monitorId,
            CustomerId         customerId,
            HostSchemeId       hostSchemeId,
            UserOrdering       userOrdering,
            const QString&     path,
            Method             method,
            ContentCheckMode   contentCheckMode,
            const KeywordList& keywords,
            ContentType        contentType,
            const QString&     userAgent,
            const QByteArray&  postContent
        );

    public:
        Monitor();

        /**
         * Copy constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        Monitor(const Monitor& other);

        /**
         * Move constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        Monitor(Monitor&& other);

        ~Monitor() = default;

        /**
         * Method you can use to determine if this monitor is valid.
         *
         * \return Returns true if the monitor is valid.  Returns false if the monitor is invalid.
         */
        inline bool isValid() const {
            return currentMonitorId != invalidMonitorId;
        }

        /**
         * Method you can use to determine if this monitor is invalid.
         *
         * \return Returns true if the monitor is invalid.  Returns false if the monitor is valid.
         */
        inline bool isInvalid() const {
            return !isValid();
        }

        /**
         * Method you can use to obtain the monitor ID.
         *
         * \return Returns the monitor ID.
         */
        inline MonitorId monitorId() const {
            return currentMonitorId;
        }

        /**
         * Method you can use to obtain the customer ID of the customer that owns this monitor.
         *
         * \return Returns the customer ID of the owner customer.
         */
        inline CustomerId customerId() const {
            return currentCustomerId;
        }

        /**
         * Method you can use to change the customer ID of the customer that owns this monitor.
         *
         * \param[in] newCustomerId The new customer ID.
         */
        inline void setCustomerId(CustomerId newCustomerId) {
            currentCustomerId = newCustomerId;
        }

        /**
         * Method you can use to obtain the host/scheme this monitor is tied to.
         *
         * \return Returns the host/scheme ID this monitor is tied to.
         */
        inline HostSchemeId hostSchemeId() const {
            return currentHostSchemeId;
        }

        /**
         * Method you can use to obtain the user ordering for this monitor.
         *
         * \return Returns the user ordering for this monitor.
         */
        inline UserOrdering userOrdering() const {
            return currentUserOrdering;
        }

        /**
         * Method you can use to update the user ordering value for this monitor.
         *
         * \param[in] newUserOrdering The new user ordering value for this monitor.
         */
        inline void setUserOrdering(UserOrdering newUserOrdering) {
            currentUserOrdering = newUserOrdering;
        }

        /**
         * Method you can use to obtain the current path under the host.  Value is as entered by the customer.
         *
         * \return Returns the path under the host and scheme for this monitor.
         */
        inline const QString& path() const {
            return currentPath;
        }

        /**
         * Method you can use to change the current path under the host.
         *
         * \param[in] newPath The new path to apply to this monitor.
         */
        inline void setPath(const QString& newPath) {
            currentPath = newPath;
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
         * Method you can use to update the method used to access this endpoint.
         *
         * \param[in] newMethod The new access method.
         */
        inline void setMethod(Method newMethod) {
            currentMethod = newMethod;
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
         * Method you can use to change the content check mode.
         *
         * \param[in] newContentCheckMode The new content check mode to be used.
         */
        inline void setContentCheckMode(ContentCheckMode newContentCheckMode) {
            currentContentCheckMode = newContentCheckMode;
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
         * Method you can use to change the content check keyword list.
         *
         * \param[in] newKeywordList The new list of content check keywords.
         */
        inline void setKeywords(const KeywordList& newKeywordList) {
            currentKeywords = newKeywordList;
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
         * Method you can use to change the content type.
         *
         * \param[in] newContentType The new content type.
         */
        inline void setContentType(ContentType newContentType) {
            currentContentType = newContentType;
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
         * Method you can use to change the user agent string.
         *
         * \param[in] newUserAgent The new user agent string.
         */
        inline void setUserAgent(const QString& newUserAgent) {
            currentUserAgent = newUserAgent;
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
         * Method you can use to change the post content.
         *
         * \param[in] newPostContent The new post content.
         */
        inline void setPostContent(const QByteArray& newPostContent) {
            currentPostContent = newPostContent;
        }

        /**
         * Method you can use to convert a method value to a string.
         *
         * \param[in] method The method to be converted.
         *
         * \return Returns a string representation of the method.
         */
        static QString toString(Method method);

        /**
         * Method you can use to convert a string to a method value.
         *
         * \param[in]  str The string to be converted.
         *
         * \param[out] ok  An optional boolean value that will be set to true on success or false on error.
         *
         * \return Returns the converted method value.
         */
        static Method toMethod(const QString& str, bool* ok = nullptr);

        /**
         * Method you can use to convert a content check mode value to a string.
         *
         * \param[in] contentCheckMode The value to be converted.
         *
         * \return Returns a string representation of the content check mode.
         */
        static QString toString(ContentCheckMode contentCheckMode);

        /**
         * Method you can use to convert a string to a content check mode value.
         *
         * \param[in]  str The string to be converted.
         *
         * \param[out] ok  An optional boolean value that will be set to true on success or false on error.
         *
         * \return Returns the converted content check mode value.
         */
        static ContentCheckMode toContentCheckMode(const QString& str, bool* ok = nullptr);

        /**
         * Method you can use to convert a content type value to a string.
         *
         * \param[in] contentType The value to be converted.
         *
         * \return Returns a string representation of the content type value.
         */
        static QString toString(ContentType contentType);

        /**
         * Method you can use to convert a string to a monitor status value.
         *
         * \param[in]  str The string to be converted.
         *
         * \param[out] ok  An optional boolean value that will be set to true on success or false on error.
         *
         * \return returns the converted monitor status value.
         */
        static MonitorStatus toMonitorStatus(const QString& str, bool* ok = nullptr);

        /**
         * Method you can use to convert a monitor status value to a string.
         *
         * \param[in] monitorStatus The monitor status value to be converted.
         *
         * \return Returns a string representation of the monitor status value.
         */
        static QString toString(MonitorStatus monitorStatus);

        /**
         * Method you can use to convert a string to a content type value.
         *
         * \param[in]  str The string to be converted.
         *
         * \param[out] ok  An optional boolean value that will be set to true on success or false on error.
         *
         * \return Returns the converted content type value.
         */
        static ContentType toContentType(const QString& str, bool* ok = nullptr);

        /**
         * Method you can use to convert a keyword list into a binary blob.
         *
         * \param[in] keywordList The keyword list to be converted.
         *
         * \return Returns the keyword list converted to a binary blob.
         */
        static QByteArray toByteArray(const KeywordList& keywordList);

        /**
         * Method you can use to convert a binary blob to a keyword list.
         *
         * \param[in]  blob The string to be converted.
         *
         * \param[out] ok   An optional boolean value that will be set to true on success or false on error.
         *
         * \return Returns the converted content type value.
         */
        static KeywordList toKeywordList(const QByteArray& blob, bool* ok = nullptr);

        /**
         * Assignment operator
         *
         * \param[in] other The instance to assign to this instance.
         *
         * \return Returns a reference to this instance.
         */
        Monitor& operator=(const Monitor& other);

        /**
         * Assignment operator (move semantics)
         *
         * \param[in] other The instance to assign to this instance.
         */
        Monitor& operator=(Monitor&& other);

    private:
        /**
         * The monitor database ID.
         */
        MonitorId currentMonitorId;

        /**
         * The customer who owns this monitor.
         */
        CustomerId currentCustomerId;

        /**
         * The host/scheme indicating the server this monitor is checking.
         */
        HostSchemeId currentHostSchemeId;

        /**
         * The zero based user ordering on the website and/or GUI.
         */
        UserOrdering currentUserOrdering;

        /**
         * The current path under the host.  Value is as entered by the customer.
         */
        QString currentPath;

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

#endif
