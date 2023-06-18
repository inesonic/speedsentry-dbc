/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref CustomerAuthenticator class.
***********************************************************************************************************************/

#include <QObject>
#include <QRandomGenerator>

#include <cstdint>
#include <cstring>

#include "customer_secret.h"
#include "customer_secrets.h"
#include "customer_capabilities.h"
#include "customers_capabilities.h"
#include "customer_authenticator.h"

CustomerAuthenticator::CustomerAuthenticator(
        bool                   wordPressApi,
        bool                   restApi,
        CustomerSecrets*       customerSecretsDatabaseApi,
        CustomersCapabilities* customersCapabilitiesDatabaseApi
    ):allowWordPressApi(
        wordPressApi
    ),allowRestApi(
        restApi
    ),customerSecrets(
        customerSecretsDatabaseApi
    ),customersCapabilities(
        customersCapabilitiesDatabaseApi
    ) {}


CustomerAuthenticator::~CustomerAuthenticator() {}


unsigned long CustomerAuthenticator::customerId(const QString& customerIdentifier, unsigned threadId) {
    unsigned long customerId = 0;

    bool          ok;
    std::uint64_t identifierValue = customerIdentifier.toULongLong(&ok, 16);

    if (ok) {
        customerId = customerSecrets->toCustomerId(identifierValue);
        if (customerId != 0) {
            CustomerCapabilities capabilities = customersCapabilities->getCustomerCapabilities(
                static_cast<CustomerCapabilities::CustomerId>(customerId),
                false,
                threadId
            );

            if (!capabilities.isValid()                       ||
                ((!allowWordPressApi                ||
                  !capabilities.supportsWordPress()    ) &&
                 (!allowRestApi                   ||
                  !capabilities.supportsRestApi()    )      )    ) {
                customerId = 0;
            }
        }
    }

    return customerId;
}


QByteArray CustomerAuthenticator::customerSecret(unsigned long customerId, unsigned threadId) {
    CustomerSecret secret = customerSecrets->getCustomerSecret(
        static_cast<CustomerCapabilities::CustomerId>(customerId),
        false,
        threadId
    );

    return secret.padddedCustomerSecret();
}
