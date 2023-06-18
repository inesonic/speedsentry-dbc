/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 Inesonic, LLC.
* All rights reserved
********************************************************************************************************************//**
* \file
*
* This header implements the \ref Monitor class.
***********************************************************************************************************************/

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QList>

#include <cstdint>
#include <cstring>

#include "host_scheme.h"
#include "monitor.h"

Monitor::Monitor(
        Monitor::MonitorId        monitorId,
        Monitor::CustomerId       customerId,
        Monitor::HostSchemeId     hostSchemeId,
        Monitor::UserOrdering     userOrdering,
        const QString&            path,
        Monitor::Method           method,
        Monitor::ContentCheckMode contentCheckMode,
        const KeywordList&        keywords,
        Monitor::ContentType      contentType,
        const QString&            userAgent,
        const QByteArray&         postContent
    ):currentMonitorId(
        monitorId
    ),currentCustomerId(
        customerId
    ),currentHostSchemeId(
        hostSchemeId
    ),currentUserOrdering(
        userOrdering
    ),currentPath(
        path
    ),currentMethod(
        method
    ),currentContentCheckMode(
        contentCheckMode
    ),currentKeywords(
        keywords
    ),currentContentType(
        contentType
    ),currentUserAgent(
        userAgent
    ),currentPostContent(
        postContent
    ) {}


Monitor::Monitor() {
    currentMonitorId        = invalidMonitorId;
    currentCustomerId       = invalidCustomerId;
    currentHostSchemeId     = invalidHostSchemeId;
    currentUserOrdering     = 0;
    currentMethod           = Method::GET;
    currentContentCheckMode = ContentCheckMode::NO_CHECK;
    currentContentType      = ContentType::TEXT;
}


Monitor::Monitor(
        const Monitor& other
    ):currentMonitorId(
        other.currentMonitorId
    ),currentCustomerId(
        other.currentCustomerId
    ),currentHostSchemeId(
        other.currentHostSchemeId
    ),currentUserOrdering(
        other.currentUserOrdering
    ),currentPath(
        other.currentPath
    ),currentMethod(
        other.currentMethod
    ),currentContentCheckMode(
        other.currentContentCheckMode
    ),currentKeywords(
        other.currentKeywords
    ),currentContentType(
        other.currentContentType
    ),currentUserAgent(
        other.currentUserAgent
    ),currentPostContent(
        other.currentPostContent
    ) {}


Monitor::Monitor(
        Monitor&& other
    ):currentMonitorId(
        other.currentMonitorId
    ),currentCustomerId(
        other.currentCustomerId
    ),currentHostSchemeId(
        other.currentHostSchemeId
    ),currentUserOrdering(
        other.currentUserOrdering
    ),currentPath(
        other.currentPath
    ),currentMethod(
        other.currentMethod
    ),currentContentCheckMode(
        other.currentContentCheckMode
    ),currentKeywords(
        other.currentKeywords
    ),currentContentType(
        other.currentContentType
    ),currentUserAgent(
        other.currentUserAgent
    ),currentPostContent(
        other.currentPostContent
    ) {}



QString Monitor::toString(Method method) {
    QString result;

    switch (method) {
        case Method::GET:     { result = QString("GET");      break; }
        case Method::HEAD:    { result = QString("HEAD");     break; }
        case Method::POST:    { result = QString("POST");     break; }
        case Method::PUT:     { result = QString("PUT");      break; }
        case Method::DELETE:  { result = QString("DELETE");   break; }
        case Method::OPTIONS: { result = QString("OPTIONS");  break; }
        case Method::PATCH:   { result = QString("PATCH");    break; }
        default:              { Q_ASSERT(false);              break; }
    }

    return result;
}


Monitor::Method Monitor::toMethod(const QString& str, bool* ok) {
    Method result;
    bool   success = true;

    QString l = str.trimmed().toLower();
    if (l == "get") {
        result = Method::GET;
    } else if (l == "head") {
        result = Method::HEAD;
    } else if (l == "post") {
        result = Method::POST;
    } else if (l == "put") {
        result = Method::PUT;
    } else if (l == "delete") {
        result = Method::DELETE;
    } else if (l == "options") {
        result = Method::OPTIONS;
    } else if (l == "patch") {
        result = Method::PATCH;
    } else {
        result  = Method::GET;
        success = false;
    }

    if (ok != nullptr) {
        *ok = success;
    }

    return result;
}


QString Monitor::toString(Monitor::ContentCheckMode contentCheckMode) {
    QString result;

    switch (contentCheckMode) {
        case ContentCheckMode::NO_CHECK:            { result = QString("NO_CHECK");             break; }
        case ContentCheckMode::CONTENT_MATCH:       { result = QString("CONTENT_MATCH");        break; }
        case ContentCheckMode::ALL_KEYWORDS:        { result = QString("ALL_KEYWORDS");         break; }
        case ContentCheckMode::ANY_KEYWORDS:        { result = QString("ANY_KEYWORDS");         break; }
        case ContentCheckMode::SMART_CONTENT_MATCH: { result = QString("SMART_CONTENT_MATCH");  break; }
        default:                                    { Q_ASSERT(false);                          break; }
    }

    return result;
}


Monitor::ContentCheckMode Monitor::toContentCheckMode(const QString& str, bool* ok) {
    ContentCheckMode result;
    bool             success = true;

    QString l = str.trimmed().toLower().replace('-', '_');
    if (l == "no_check") {
        result = ContentCheckMode::NO_CHECK;
    } else if (l == "content_match") {
        result = ContentCheckMode::CONTENT_MATCH;
    } else if (l == "all_keywords") {
        result = ContentCheckMode::ALL_KEYWORDS;
    } else if (l == "any_keywords") {
        result = ContentCheckMode::ANY_KEYWORDS;
    } else if (l == "smart_content_match") {
        result = ContentCheckMode::SMART_CONTENT_MATCH;
    } else {
        result  = ContentCheckMode::NO_CHECK;
        success = false;
    }

    if (ok != nullptr) {
        *ok = success;
    }

    return result;
}


QString Monitor::toString(Monitor::ContentType contentType) {
    QString result;

    switch (contentType) {
        case ContentType::JSON: { result = QString("JSON");   break; }
        case ContentType::TEXT: { result = QString("TEXT");   break; }
        case ContentType::XML:  { result = QString("XML");    break; }
        default:                { Q_ASSERT(false);            break; }
    }

    return result;
}


Monitor::MonitorStatus Monitor::toMonitorStatus(const QString& str, bool* ok) {
    MonitorStatus result;
    bool          success = true;

    QString l = str.trimmed().toLower();
    if (l == "unknown") {
        result = MonitorStatus::UNKNOWN;
    } else if (l == "working") {
        result = MonitorStatus::WORKING;
    } else if (l == "failed") {
        result = MonitorStatus::FAILED;
    } else {
        result = MonitorStatus::UNKNOWN;
        success = false;
    }

    if (ok != nullptr) {
        *ok = success;
    }

    return result;
}


QString Monitor::toString(Monitor::MonitorStatus monitorStatus) {
    QString result;

    switch (monitorStatus) {
        case MonitorStatus::UNKNOWN: { result = QString("UNKNOWN");   break; }
        case MonitorStatus::WORKING: { result = QString("WORKING");   break; }
        case MonitorStatus::FAILED:  { result = QString("FAILED");    break; }
        default:                     { Q_ASSERT(false);               break; }
    }

    return result;
}


Monitor::ContentType Monitor::toContentType(const QString& str, bool* ok) {
    ContentType result;
    bool        success = true;

    QString l = str.trimmed().toLower();
    if (l == "json") {
        result = ContentType::JSON;
    } else if (l == "text") {
        result = ContentType::TEXT;
    } else if (l == "xml") {
        result = ContentType::XML;
    } else {
        result  = ContentType::TEXT;
        success = false;
    }

    if (ok != nullptr) {
        *ok = success;
    }

    return result;
}

QByteArray Monitor::toByteArray(const Monitor::KeywordList& keywordList) {
    unsigned numberKeywords = static_cast<unsigned>(keywordList.size());
    unsigned totalLength    = 2 + 2 * numberKeywords;
    for (unsigned i=0 ; i<numberKeywords ; ++i) {
        totalLength += keywordList.at(i).size();
    }

    QByteArray    result(static_cast<int>(totalLength), 0);
    std::uint8_t* d = reinterpret_cast<std::uint8_t*>(result.data());

    *d++ = static_cast<std::uint8_t>(numberKeywords     );
    *d++ = static_cast<std::uint8_t>(numberKeywords >> 8);

    for (unsigned i=0 ; i<numberKeywords ; ++i) {
        const QByteArray&   keyword       = keywordList.at(i);
        unsigned            keywordLength = static_cast<unsigned>(keyword.size());
        const std::uint8_t* keywordData   = reinterpret_cast<const std::uint8_t*>(keyword.data());

        *d++ = static_cast<std::uint8_t>(keywordLength     );
        *d++ = static_cast<std::uint8_t>(keywordLength >> 8);
        std::memcpy(d, keywordData, keywordLength);

        d += keywordLength;
    }

    return result;
}


Monitor::KeywordList Monitor::toKeywordList(const QByteArray& blob, bool* ok) {
    KeywordList result;

    bool     success    = true;
    unsigned blobLength = static_cast<unsigned>(blob.size());
    if (blobLength >= 2) {
        const std::uint8_t* blobData       = reinterpret_cast<const std::uint8_t*>(blob.data());
        unsigned            numberKeywords = blobData[0] | (blobData[1] << 8);
        unsigned            keywordIndex   = 0;
        unsigned            blobIndex      = 2;
        while (success && keywordIndex < numberKeywords && blobIndex < blobLength) {
            unsigned keywordLength  = blobData[blobIndex] | (blobData[blobIndex+1] << 8);

            blobIndex += 2;
            unsigned bytesRemaining = blobLength - blobIndex;

            if (bytesRemaining >= keywordLength) {
                QByteArray keyword(static_cast<int>(keywordLength), 0);
                std::uint8_t* keywordData = reinterpret_cast<std::uint8_t*>(keyword.data());
                std::memcpy(keywordData, blobData + blobIndex, keywordLength);

                result.append(keyword);

                blobIndex += keywordLength;
                ++keywordIndex;
            } else {
                success = false;
            }
        }

        if (success && blobIndex != blobLength) {
            success = false;
        }
    } else {
        success = false;
    }

    if (ok != nullptr) {
        *ok = success;
    }

    return result;
}


Monitor& Monitor::operator=(const Monitor& other) {
    currentMonitorId        = other.currentMonitorId;
    currentCustomerId       = other.currentCustomerId;
    currentHostSchemeId     = other.currentHostSchemeId;
    currentUserOrdering     = other.currentUserOrdering;
    currentPath             = other.currentPath;
    currentMethod           = other.currentMethod;
    currentContentCheckMode = other.currentContentCheckMode;
    currentKeywords         = other.currentKeywords;
    currentContentType      = other.currentContentType;
    currentUserAgent        = other.currentUserAgent;
    currentPostContent      = other.currentPostContent;

    return *this;
}


Monitor& Monitor::operator=(Monitor&& other) {
    currentMonitorId        = other.currentMonitorId;
    currentCustomerId       = other.currentCustomerId;
    currentHostSchemeId     = other.currentHostSchemeId;
    currentUserOrdering     = other.currentUserOrdering;
    currentPath             = other.currentPath;
    currentMethod           = other.currentMethod;
    currentContentCheckMode = other.currentContentCheckMode;
    currentKeywords         = other.currentKeywords;
    currentContentType      = other.currentContentType;
    currentUserAgent        = other.currentUserAgent;
    currentPostContent      = other.currentPostContent;

    return *this;
}
