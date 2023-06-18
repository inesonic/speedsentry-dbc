/*-*-c++-*-*************************************************************************************************************
* Copyright 2016 Inesonic, LLC.
* All rights reserved.
********************************************************************************************************************//**
* \file
*
* This header defines the \ref registerMetaTypes function as well as other features used to register and manage various
* application specific metatypes.
***********************************************************************************************************************/

/* .. sphinx-project db_controller */

#ifndef METATYPES_H
#define METATYPES_H

#include <QMetaType>
#include <QHostAddress>
#include <QJsonDocument>

/**
 * Function you can call during application start-up to register metatypes required by the application.
 */
void registerMetaTypes();

/*
 * Declare the various metatypes below.
 */

Q_DECLARE_METATYPE(QHostAddress)
Q_DECLARE_METATYPE(QJsonDocument)

#endif
