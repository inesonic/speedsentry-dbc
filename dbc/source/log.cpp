/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref Monitor class.
***********************************************************************************************************************/

#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>

#include <iostream>

#include "log.h"

QMutex loggingMutex;

void logWrite(const QString& message, bool error) {
    QMutexLocker mutexLocker(&loggingMutex);

    QString dateTime = QDateTime::currentDateTime().toString(Qt::DateFormat::ISODate);

    if (error) {
        QString logEntry = QString("%1: *** %2").arg(dateTime, message);
        std::cerr << logEntry.toLocal8Bit().data() << std::endl;
    } else {
        QString logEntry = QString("%1: %2").arg(dateTime, message);
        std::cout << logEntry.toLocal8Bit().data() << std::endl;

    }
}
