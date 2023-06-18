/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This file contains the main entry point for the Zoran monitor.
***********************************************************************************************************************/

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QString>
#include <QFile>
#include <QByteArray>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "metatypes.h"
#include "log.h"
#include "dbc.h"

#include <iostream>

#include "active_resources.h"

int main(int argumentCount, char* argumentValues[]) {
    int exitStatus = 0;

    QApplication application(argumentCount, argumentValues);
    QApplication::setApplicationName("Inesonic Database Controller Tool");
    QApplication::setApplicationVersion("1.0");

    registerMetaTypes();

    if (argumentCount == 2) {
        QString configurationFilename = QString::fromLocal8Bit(argumentValues[1]);

        DbC dbController(configurationFilename);
        exitStatus = application.exec();
    } else {
        logWrite(QString("Invalid command line.  Include path to the configuration file."), true);
        exitStatus = 1;
    }

    return exitStatus;
}
