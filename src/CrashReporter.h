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

#ifndef MUDLET_CRASHREPORTER_H
#define MUDLET_CRASHREPORTER_H

#include "pre_guard.h"
#include <QObject>
#include <QString>
#include <QMutex>
#include <QDateTime>
#include <QNetworkAccessManager>
#include "post_guard.h"

class CrashReporter : public QObject
{
    Q_OBJECT

public:
    static CrashReporter& instance();

    void initialize();
    void uploadCrashReport(const QString& crashId);
    void checkPendingReports();
    bool hasValidReport(const QString& crashId) const;
    void cleanOldReports(int daysToKeep = 30);

signals:
    void crashReportUploaded(const QString& crashId, bool success);
    void uploadProgress(const QString& crashId, qint64 bytesSent, qint64 bytesTotal);

private:
    CrashReporter();
    ~CrashReporter() = default;
    CrashReporter(const CrashReporter&) = delete;
    CrashReporter& operator=(const CrashReporter&) = delete;

    QString getReportPath(const QString& crashId) const;
    void handleUploadFinished(QNetworkReply* reply, const QString& crashId);
    void prepareUploadData(const QString& crashId, QHttpMultiPart* multiPart);
    void addMetadata(QHttpMultiPart* multiPart, const QString& crashId);
    QString generateMinidumpId() const;

    QNetworkAccessManager* mNetworkManager;
    QString mUploadUrl;
    QMutex mUploadMutex;
    bool mInitialized;
    QDateTime mLastCleanup;
};

#endif // MUDLET_CRASHREPORTER_H