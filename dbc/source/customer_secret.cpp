/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref CustomerSecret class.
***********************************************************************************************************************/

#include <QString>
#include <QRandomGenerator>

#include <cstdint>
#include <cstring>

#include "customer_secret.h"

CustomerSecret::CustomerSecret(
        CustomerId        customerId,
        const QByteArray& customerSecret
    ):currentCustomerId(
        customerId
    ),currentCustomerSecret(
        customerSecret
    ) {
    currentCustomerSecret.resize(paddedSecretLength);
}


CustomerSecret::CustomerSecret():currentCustomerId(CustomerSecret::invalidCustomerId) {}


CustomerSecret::CustomerSecret(
        const CustomerSecret& other
    ):currentCustomerId(
        other.currentCustomerId
    ),currentCustomerSecret(
        other.currentCustomerSecret
    ) {}


CustomerSecret::CustomerSecret(
        CustomerSecret&& other
    ):currentCustomerId(
        other.currentCustomerId
    ),currentCustomerSecret(
        other.currentCustomerSecret
    ) {}


CustomerSecret::~CustomerSecret() {
    std::uint8_t* d = reinterpret_cast<std::uint8_t*>(currentCustomerSecret.data());
    std::memset(d, 0, currentCustomerSecret.size());

}


void CustomerSecret::generateNewSecret() {
    currentCustomerSecret.resize(paddedSecretLength);
    std::uint8_t* d = reinterpret_cast<std::uint8_t*>(currentCustomerSecret.data());
    std::uint64_t r = 0;
    for (unsigned i=0 ; i<paddedSecretLength ; ++i) {
        if ((i % 8) == 0) {
            r = QRandomGenerator::global()->generate64();
        }

        *d = static_cast<std::uint8_t>(r);
        ++d;
        r >>= 8;
    }
}


CustomerSecret& CustomerSecret::operator=(const CustomerSecret& other) {
    currentCustomerId     = other.currentCustomerId;
    currentCustomerSecret = other.currentCustomerSecret;

    return *this;
}


CustomerSecret& CustomerSecret::operator=(CustomerSecret&& other) {
    currentCustomerId     = other.currentCustomerId;
    currentCustomerSecret = other.currentCustomerSecret;

    return *this;
}
