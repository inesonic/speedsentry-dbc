/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref CustomerSecret class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef CUSTOMER_SECRET_H
#define CUSTOMER_SECRET_H

#include <QString>

#include <cstdint>

#include "region.h"

class Servers;

/**
 * Trivial class used to hold the current customer secret.
 */
class CustomerSecret {
    public:
        /**
         * Value used to represent a server ID.
         */
        typedef std::uint32_t CustomerId;

//FIXME    private:
        /**
         * Constructor.
         *
         * \param[in] customerId     The customer ID.
         *
         * \param[in] customerSecret The current customer secret without padding.
         */
        CustomerSecret(CustomerId customerId, const QByteArray& customerSecret);

    public:
        /**
         * Value used to indicate the required length for the customer secret without padding.
         */
        static constexpr unsigned secretLength = 56;

        /**
         * Value used to indicate the required length of the padded customer secret.
         */
        static constexpr unsigned paddedSecretLength = 64;

        /**
         * Value used to indicate an invalid customer ID.
         */
        static constexpr CustomerId invalidCustomerId = 0;

        CustomerSecret();

        /**
         * Copy constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        CustomerSecret(const CustomerSecret& other);

        /**
         * Move constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        CustomerSecret(CustomerSecret&& other);

        ~CustomerSecret();

        /**
         * Method you can use to determine if this customer entry is valid.
         *
         * \return Returns True if the server instance is valid.  Returns false if the server instance is invalid.
         */
        inline bool isValid() const {
            return currentCustomerId != invalidCustomerId;
        }

        /**
         * Method you can use to determine if this server instance is invalid.
         *
         * \return Returns True if the server instance is invalid.  Returns false if the server instance is valid.
         */
        inline bool isInvalid() const {
            return !isValid();
        }

        /**
         * Method you can use to get the customer ID.
         *
         * \return Returns the customer ID.
         */
        inline CustomerId customerId() const {
            return currentCustomerId;
        }

        /**
         * Method you can use to get the current unpadded customer secret.
         *
         * \return Returns the current unpadded customer secret.
         */
        inline QByteArray customerSecret() const {
            return currentCustomerSecret.left(secretLength);
        }

        /**
         * Method you can use to get the current padded customer secret.
         *
         * \return Returns the current padded customer secret.
         */
        inline const QByteArray& padddedCustomerSecret() const {
            return currentCustomerSecret;
        }

        /**
         * Method you can use to generate a new customer secret.
         */
        void generateNewSecret();

        /**
         * Assignment operator.
         *
         * \param[in] other The instance to assign to this instance.
         *
         * \return Returns a reference to this instance.
         */
        CustomerSecret& operator=(const CustomerSecret& other);

        /**
         * Assignment operator.
         *
         * \param[in] other The instance to assign to this instance.
         *
         * \return Returns a reference to this instance.
         */
        CustomerSecret& operator=(CustomerSecret&& other);

    private:
        /**
         * The current customer ID.
         */
        CustomerId currentCustomerId;

        /**
         * The current padded customer secret.
         */
        QByteArray currentCustomerSecret;
};

#endif
