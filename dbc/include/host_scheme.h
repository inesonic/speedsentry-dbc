/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref HostScheme class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef HOST_SCHEME_H
#define HOST_SCHEME_H

#include <QString>
#include <QUrl>

#include <cstdint>

class HostSchemes;

/**
 * Trivial class used to hold information about a customer's server and scheme.
 */
class HostScheme {
    friend class HostSchemes;

    public:
        /**
         * Value used to represent a host/scheme ID.
         */
        typedef std::uint32_t HostSchemeId;

        /**
         * Value used to represent a customer ID.
         */
        typedef std::uint32_t CustomerId;

        /**
         * Value used to indicate an invalid host/scheme ID.
         */
        static constexpr HostSchemeId invalidHostSchemeId = 0;

        /**
         * Value used to indicate an invalid customer ID.
         */
        static constexpr CustomerId invalidCustomerId = 0;

        /**
         * Value indicating an invalid or unknown SSL expiration timestamp.
         */
        static constexpr unsigned long long invalidSslExpirationTimestamp = 0;

    private:
        /**
         * Constructor.
         *
         * \param[in] hostSchemeId           The ID used to identify this host and scheme.
         *
         * \param[in] customerId             The ID used to identify the customer tied to this host.
         *
         * \param[in] url                    The server URL.
         *
         * \param[in] sslExpirationTimestamp The timestamp when the SSL certificate is expected to expire.
         */
        inline HostScheme(
                HostSchemeId  hostSchemeId,
                CustomerId    customerId,
                const QUrl&   url,
                unsigned long sslExpirationTimestamp
            ):currentHostSchemeId(
                hostSchemeId
            ),currentCustomerId(
                customerId
            ),currentUrl(
                url
            ),currentSslExpirationTimestamp(
                sslExpirationTimestamp
            ) {}

    public:
        inline HostScheme() {
            currentHostSchemeId = invalidHostSchemeId;
            currentCustomerId   = 0;
            currentSslExpirationTimestamp = invalidSslExpirationTimestamp;
        }

        /**
         * Copy constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        inline HostScheme(
                const HostScheme& other
            ):currentHostSchemeId(
                other.currentHostSchemeId
            ),currentCustomerId(
                other.currentCustomerId
            ),currentUrl(
                other.currentUrl
            ),currentSslExpirationTimestamp(
                other.currentSslExpirationTimestamp
            ) {}

        /**
         * Move constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        inline HostScheme(
                HostScheme&& other
            ):currentHostSchemeId(
                other.currentHostSchemeId
            ),currentCustomerId(
                other.currentCustomerId
            ),currentUrl(
                other.currentUrl
            ),currentSslExpirationTimestamp(
                other.currentSslExpirationTimestamp
            ) {}

        ~HostScheme() = default;

        /**
         * Method you can use to determine if this host/scheme ID is valid.
         *
         * \return Returns True if the host/scheme instance is valid.  Returns false if the host/scheme instance is
         *         invalid.
         */
        inline bool isValid() const {
            return currentHostSchemeId != invalidHostSchemeId;
        }

        /**
         * Method you can use to determine if this host/scheme ID is invalid.
         *
         * \return Returns True if the host/scheme instance is invalid.  Returns false if the host/scheme instance is
         *         valid.
         */
        inline bool isInvalid() const {
            return !isValid();
        }

        /**
         * Method you can use to obtain the host/scheme ID.
         *
         * \return Returns the region ID for this region.
         */
        inline HostSchemeId hostSchemeId() const {
            return currentHostSchemeId;
        }

        /**
         * Method you can use to obtain the customer ID of the customer tied to this host/scheme.
         *
         * \return Returns the customer ID.
         */
        inline CustomerId customerId() const {
            return currentCustomerId;
        }

        /**
         * Method you can use to change the stored customer ID.
         *
         * \param[in[ newCustomerId The new customer ID for this host/scheme.
         */
        inline void setCustomerId(CustomerId newCustomerId) {
            currentCustomerId = newCustomerId;
        }

        /**
         * Method you can use to obtain the URL for this host/scheme.
         *
         * \return Returns the URL.
         */
        inline const QUrl& url() const {
            return currentUrl;
        }

        /**
         * Method you can use to change the URL for this host/scheme.
         *
         * \param[in[ newUrl The new customer ID for this host/scheme.
         */
        inline void setUrl(const QUrl& newUrl) {
            currentUrl = newUrl;
        }

        /**
         * Method you can use to obtain the SSL certificate expiration timestamp.
         *
         * \return Returns the SSL expiration timestamp.
         */
        inline unsigned long long sslExpirationTimestamp() const {
            return currentSslExpirationTimestamp;
        }

        /**
         * Method you can use to change the SSL expiration timestamp.
         *
         * \param[in[ newSslExpirationTimestamp The new SSL expiration timestamp.
         */
        inline void setSslExpirationTimestamp(unsigned long long newSslExpirationTimestamp) {
            currentSslExpirationTimestamp = newSslExpirationTimestamp;
        }

        /**
         * Assignment operator.
         *
         * \param[in] other The instance to assign to this instance.
         *
         * \return Returns a reference to this instance.
         */
        inline HostScheme& operator=(const HostScheme& other) {
            currentHostSchemeId           = other.currentHostSchemeId;
            currentCustomerId             = other.currentCustomerId;
            currentUrl                    = other.currentUrl;
            currentSslExpirationTimestamp = other.currentSslExpirationTimestamp;

            return *this;
        }

        /**
         * Assignment operator (move semantics).
         *
         * \param[in] other The instance to assign to this instance.
         *
         * \return Returns a reference to this instance.
         */
        inline HostScheme& operator=(HostScheme&& other) {
            currentHostSchemeId           = other.currentHostSchemeId;
            currentCustomerId             = other.currentCustomerId;
            currentUrl                    = other.currentUrl;
            currentSslExpirationTimestamp = other.currentSslExpirationTimestamp;

            return *this;
        }

    private:
        /**
         * The current host/scheme ID.
         */
        HostSchemeId currentHostSchemeId;

        /**
         * The current customer ID of the customer associated with this host/scheme.
         */
        CustomerId currentCustomerId;

        /**
         * The URL used to access the server.  The URL will always exclude paths, query strings, and fragments.
         */
        QUrl currentUrl;

        /**
         * The last reported SSL expiration timestamp.
         */
        unsigned long currentSslExpirationTimestamp;
};

#endif
