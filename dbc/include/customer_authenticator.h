/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref CustomerAuthenticator class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef CUSTOMER_AUTHENTICATOR_H
#define CUSTOMER_AUTHENTICATOR_H

#include <QObject>
#include <QString>
#include <QByteArray>

#include <cstdint>

#include <rest_api_in_v1_customer_data.h>

class CustomerSecrets;
class CustomersCapabilities;

/**
 * Class used to authenticate customers and provide access to customer REST APIs.
 */
class CustomerAuthenticator:public QObject, public RestApiInV1::CustomerData {
    public:
        /**
         * Constructor
         *
         * \param[in] wordPressApi                     If true, then this class will allow access if the customer
         *                                             can use the WordPress API.
         *
         * \param[in] restApi                          If true, then this class will allow access if the customer can
         *                                             use the generic REST API.
         *
         * \param[in] customerSecretsDatabaseApi       The customer secrets database API.
         *
         * \param[in] customersCapabilitiesDatabaseApi The customer capabilities database API.
         */
        CustomerAuthenticator(
            bool                   wordPressApi,
            bool                   restApi,
            CustomerSecrets*       customerSecretsDatabaseApi,
            CustomersCapabilities* customersCapabilitiesDatabaseApi
        );

        ~CustomerAuthenticator() override;


        /**
         * Method you should overload to map customer identifiers to an internal numeric customer ID.
         *
         * \param[in] customerIdentifier The customer identifier to be mapped.
         *
         * \param[in] threadId           The thread ID of the thread we're executing under.
         *
         * \return Returns the internal customer ID associated with the customer identifier.  Returning
         *         a customer ID of 0 will cause a 401 UNAUTHORIZED to be returned.
         */
        unsigned long customerId(const QString& customerIdentifier, unsigned threadId) final;

        /**
         * Method you should overload to map customer IDs to customer secrets.
         *
         * \param[in] customerId The internal customer ID to be mapped.
         *
         * \param[in] threadId   The thread ID of the thread we're executing under.
         *
         * \return Returns the secret associated with the customer identifier.  The secret should be
         *         padded to \ref paddedSecretLength bytes.
         */
        QByteArray customerSecret(unsigned long customerId, unsigned threadId) final;

    private:
        /**
         * Value that holds true if this class instance should allow access to the WordPress API.
         */
        bool allowWordPressApi;

        /**
         * Value that holds true if this class instance should allow access to the generic REST API.
         */
        bool allowRestApi;

        /**
         * The customer secrets database API.
         */
        CustomerSecrets* customerSecrets;

        /**
         * The customer capabilities database API.
         */
        CustomersCapabilities* customersCapabilities;
};

#endif
