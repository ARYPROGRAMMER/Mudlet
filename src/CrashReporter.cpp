/***************************************************************************
 *   Copyright (C) 2024 by ARYPROGRAMMER                                    *
 *   Copyright (C) 2024 by Stephen Lyons - slysven@virginmedia.com         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.            *
 ***************************************************************************/

#include "CrashReporter.h"
#include "CrashHandler.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QDebug>
#include <QStandardPaths>
#include <QUuid>
#include <QDateTime>
#include "post_guard.h"

CrashReporter::CrashReporter()
: mNetworkManager(new QNetworkAccessManager(this))
, mInitialized(false)
, mLastCleanup(QDateTime::currentDateTime())
{
    mUploadUrl = QString("https://sentry.io/api/%1/minidump/?sentry_key=%2")
        .arg(SENTRY_PROJECT_ID)
        .arg(SENTRY_PUBLIC_KEY);
}

CrashReporter& CrashReporter::instance()
{
    static CrashReporter instance;
    return instance;
}

void CrashReporter::initialize()
{
    if (mInitialized) {
        return;
    }

    cleanOldReports();
    checkPendingReports();
    
    mInitialized = true;
}

void CrashReporter::uploadCrashReport(const QString& crashId)
{
    QMutexLocker locker(&mUploadMutex);
    
    if (!hasValidReport(crashId)) {
        qWarning() << "Invalid crash report ID:" << crashId;
        emit crashReportUploaded(crashId, false);
        return;
    }

    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    prepareUploadData(crashId, multiPart);
    
    QNetworkRequest request(QUrl(mUploadUrl));
    QNetworkReply* reply = mNetworkManager->post(request, multiPart);
    multiPart->setParent(reply);
    
    connect(reply, &QNetworkReply::uploadProgress, this, 
            [this, crashId](qint64 bytesSent, qint64 bytesTotal) {
        emit uploadProgress(crashId, bytesSent, bytesTotal);
    });
    
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, crashId]() {
        handleUploadFinished(reply, crashId);
    });
}

void CrashReporter::handleUploadFinished(QNetworkReply* reply, const QString& crashId)
{
    bool success = (reply->error() == QNetworkReply::NoError);
    
    if (success) {
        QString reportPath = getReportPath(crashId);
        QFile::remove(reportPath);
        qDebug() << "Successfully uploaded crash report:" << crashId;
    } else {
        qWarning() << "Failed to upload crash report:" << crashId
                  << "Error:" << reply->errorString();
    }
    
    emit crashReportUploaded(crashId, success);
    reply->deleteLater();
}

void CrashReporter::prepareUploadData(const QString& crashId, QHttpMultiPart* multiPart)
{
    QString reportPath = getReportPath(crashId);
    QFile reportFile(reportPath);
    
    if (!reportFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open crash report file:" << reportPath;
        return;
    }
    
    QHttpPart crashPart;
    crashPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
    crashPart.setHeader(QNetworkRequest::ContentDispositionHeader, 
        QVariant("form-data; name=\"upload_file_minidump\"; filename=\"" + crashId + ".dmp\""));
    crashPart.setBody(reportFile.readAll());
    
    multiPart->append(crashPart);
    addMetadata(multiPart, crashId);
}

void CrashReporter::addMetadata(QHttpMultiPart* multiPart, const QString& crashId)
{
    QHttpPart metadataPart;
    metadataPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    metadataPart.setHeader(QNetworkRequest::ContentDispositionHeader, 
        QVariant("form-data; name=\"sentry\""));
    
    QJsonObject metadata;
    metadata["platform"] = "native";
    metadata["release"] = QString(APP_VERSION APP_BUILD);
    metadata["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    metadata["event_id"] = crashId;
    
    metadataPart.setBody(QJsonDocument(metadata).toJson());
    multiPart->append(metadataPart);
}

QString CrashReporter::getReportPath(const QString& crashId) const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) 
        + "/crash-reports/" 
        + crashId 
        + ".dmp";
}

bool CrashReporter::hasValidReport(const QString& crashId) const
{
    return QFile::exists(getReportPath(crashId));
}

void CrashReporter::checkPendingReports()
{
    QString crashDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) 
        + "/crash-reports";
    
    QDir dir(crashDir);
    QStringList filters;
    filters << "*.dmp";
    
    for (const QString& file : dir.entryList(filters, QDir::Files)) {
        QString crashId = file;
        crashId.chop(4); // Remove .dmp extension
        uploadCrashReport(crashId);
    }
}

void CrashReporter::cleanOldReports(int daysToKeep)
{
    QString crashDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) 
        + "/crash-reports";
    
    QDir dir(crashDir);
    QStringList filters;
    filters << "*.dmp";
    
    QDateTime cutoffDate = QDateTime::currentDateTime().addDays(-daysToKeep);
    
    for (const QString& file : dir.entryList(filters, QDir::Files)) {
        QFileInfo fileInfo(dir.filePath(file));
        if (fileInfo.lastModified() < cutoffDate) {
            QFile::remove(fileInfo.filePath());
        }
    }
    
    mLastCleanup = QDateTime::currentDateTime();
}

QString CrashReporter::generateMinidumpId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}