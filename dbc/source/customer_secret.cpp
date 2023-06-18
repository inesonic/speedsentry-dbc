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
