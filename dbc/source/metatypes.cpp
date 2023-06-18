/*-*-c++-*-*************************************************************************************************************
* Copyright 2016 Inesonic, LLC.
* All rights reserved.
********************************************************************************************************************//**
* \file
*
* This file implements the \ref registerMetaTypes function.
***********************************************************************************************************************/

#include <QtGlobal>
#include <QHostAddress>
#include <QJsonDocument>

#include "metatypes.h"

void registerMetaTypes() {
    qRegisterMetaType<QHostAddress>();
    qRegisterMetaType<QJsonDocument>();
}
