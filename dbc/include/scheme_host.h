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
* This header defines the \ref SchemeHost class.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef SCHEME_HOST_H
#define SCHEME_HOST_H

#include <QString>
#include <QHash>
#include <QUrl>

#include <cstdint>

#include "host_scheme.h"

/**
 * Trivial class that tracks two strings holding a scheme and host.
 */
class SchemeHost {
    public:
        /**
         * Constructor
         *
         * \param[in] scheme A string holding the scheme.
         *
         * \param[in] host   A string holding the host name.
         */
        inline SchemeHost(
                const QString& scheme,
                const QString& host
            ):currentScheme(
                scheme.toLower()
            ),currentHost(
                host.toLower()
            ) {}

        /**
         * Constructor
         *
         * \param[in] url The URL to convert to a scheme/host.
         */
        SchemeHost(
                const QUrl& url
            ):currentScheme(
                url.scheme()
            ),currentHost(
                url.authority()
            ) {}

        /**
         * Copy constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        inline SchemeHost(
                const SchemeHost& other
            ):currentScheme(
                other.currentScheme
            ),currentHost(
                other.currentHost
            ) {}

        /**
         * Move constructor
         *
         * \param[in] other The instance to assign to this instance.
         */
        inline SchemeHost(
                SchemeHost&& other
            ):currentScheme(
                other.currentScheme
            ),currentHost(
                other.currentHost
            ) {}

        ~SchemeHost() = default;

        /**
         * Method you can use to get the current scheme.
         *
         * \return Returns the current scheme.
         */
        inline const QString& scheme() const {
            return currentScheme;
        }

        /**
         * Method you can use to get current host.
         *
         * \return Returns the current host.
         */
        inline const QString& host() const {
            return currentHost;
        }

        /**
         * Comparison operator
         *
         * \param[in] other The instance to compare against this instance.
         *
         * \return Returns true if the instances are equal.  Returns false if the instances are not equal.
         */
        inline bool operator==(const SchemeHost& other) const {
            return currentScheme == other.currentScheme && currentHost == other.currentHost;
        }

        /**
         * Comparison operator
         *
         * \param[in] other The instance to compare against this instance.
         *
         * \return Returns true if the instances are not equal.  Returns false if the instances are equal.
         */
        inline bool operator!=(const SchemeHost& other) const {
            return currentScheme != other.currentScheme || currentHost != other.currentHost;
        }

        /**
         * Comparison operator
         *
         * \param[in] other The instance to compare against this instance.
         *
         * \return Returns true if this instance should precede the other instance.  Returns false if the instances
         *         are equivalent or this instance should follow the other instance.
         */
        inline bool operator<(const SchemeHost& other) const {
            return (
                   currentScheme < other.currentScheme
                || (   currentScheme == other.currentScheme
                    && currentHost < other.currentHost
                   )
            );
        }

        /**
         * Comparison operator
         *
         * \param[in] other The instance to compare against this instance.
         *
         * \return Returns true if this instance should follow the other instance.  Returns false if the instances
         *         are equivalent or this instance should precede the other instance.
         */
        inline bool operator>(const SchemeHost& other) const {
            return (
                   currentScheme > other.currentScheme
                || (   currentScheme == other.currentScheme
                    && currentHost > other.currentHost
                   )
            );
        }

        /**
         * Comparison operator
         *
         * \param[in] other The instance to compare against this instance.
         *
         * \return Returns true if this instance should precede the other instance or is equivalent to the other
         *         instance.  Returns false if this instance should follow the other instance.
         */
        inline bool operator<=(const SchemeHost& other) const {
            return !operator>(other);
        }

        /**
         * Comparison operator
         *
         * \param[in] other The instance to compare against this instance.
         *
         * \return Returns true if this instance should follow the other instance or is equivalent to the other
         *         instance.  Returns false if this instance should precede the other instance.
         */
        inline bool operator>=(const SchemeHost& other) const {
            return !operator<(other);
        }

        /**
         * Assignment operator
         *
         * \param[in] other The instance to assign to this instance.
         */
        inline SchemeHost& operator=(const SchemeHost& other) {
            currentScheme = other.currentScheme;
            currentHost   = other.currentHost;

            return *this;
        }

        /**
         * Assignment operator (move semantics)
         *
         * \param[in] other The instance to assign to this instance.
         */
        inline SchemeHost& operator=(SchemeHost&& other) {
            currentScheme = other.currentScheme;
            currentHost   = other.currentHost;

            return *this;
        }

    private:
        /**
         * The currently tracked scheme.
         */
        QString currentScheme;

        /**
         * The currently tracked host.
         */
        QString currentHost;
};

/**
 * Hash function for the \ref SchemeHostPath class.
 *
 * \param[in] value The value to be hashed.
 *
 * \param[in] seed  An optional seed to apply to the hash.
 *
 * \return Returns a numerical hash of the \ref SchemeHostPath instance.
 */
inline unsigned qHash(const SchemeHost& value, unsigned seed = 0) {
    return qHash(value.scheme(), seed) ^ qHash(value.host(), seed);
}

#endif
