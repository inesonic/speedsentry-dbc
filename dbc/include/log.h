/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header defines the \ref logWrite function.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef LOG_H
#define LOG_H

#include <QString>

/**
 * Function you can use to write a log entry.
 *
 * \param[in] message The log message.
 *
 * \param[in] error   If true, the data is reporting an error.
 */
void logWrite(const QString& message, bool error = false);

#endif
