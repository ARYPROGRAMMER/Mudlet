/***************************************************************************
 *   Copyright (C) 2024 by AryProgram                                       *
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

#ifndef MUDLET_CRASHHANDLER_H
#define MUDLET_CRASHHANDLER_H

#include "pre_guard.h"
#include <QObject>
#include <QString>
#include <QDateTime>
#include <sentry.h>
#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <signal.h>
#endif
#include "post_guard.h"

class CrashHandler : public QObject
{
    Q_OBJECT

public:
    static CrashHandler& instance();
    
    void initialize();
    void shutdown();
    
    void setUserConsent(bool enabled);
    bool hasUserConsent() const { return mUserConsented; }
    bool isInitialized() const { return mInitialized; }
    
    void captureMessage(const QString& message);
    void captureError(const QString& error, const QString& function = QString());
    void addBreadcrumb(const QString& message, const QString& category = "default");

private:
    CrashHandler();
    ~CrashHandler();
    CrashHandler(const CrashHandler&) = delete;
    CrashHandler& operator=(const CrashHandler&) = delete;

    void setupPlatformHandlers();
    void initializeSentry();
    QString getSentryDsn() const;
    QString getEnvironmentName() const;
    void setCommonContext();

    static void handleCrash(int signal);
#ifdef Q_OS_WIN
    static LONG WINAPI handleWindowsException(EXCEPTION_POINTERS* exceptionInfo);
#endif

    bool mInitialized;
    bool mUserConsented;
    QString mDatabasePath;
    QDateTime mStartTime;
};

#endif // MUDLET_CRASHHANDLER_H