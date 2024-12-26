/***************************************************************************
 *   Copyright (C) 2024 by AryProgram                                       *
 *   Copyright (C) 2024 by Stephen Lyons - slysven@virginmedia.com         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "CrashHandler.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>
#include <QSysInfo>
#include "post_guard.h"

// Replace with your actual Sentry DSN
const char* SENTRY_DSN = "https://your-project-key@sentry.io/your-project-id";

CrashHandler::CrashHandler()
: mInitialized(false)
, mUserConsented(false)
, mStartTime(QDateTime::currentDateTime())
{
    mDatabasePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/crash-reports";
    QDir().mkpath(mDatabasePath);
}

CrashHandler::~CrashHandler()
{
    shutdown();
}

CrashHandler& CrashHandler::instance()
{
    static CrashHandler instance;
    return instance;
}

void CrashHandler::initialize()
{
    if (mInitialized || !mUserConsented) {
        return;
    }

    initializeSentry();
    setupPlatformHandlers();
    
    mInitialized = true;
    addBreadcrumb("Crash handler initialized", "system");
}

void CrashHandler::initializeSentry()
{
    sentry_options_t* options = sentry_options_new();
    
    sentry_options_set_dsn(options, getSentryDsn().toUtf8().constData());
    sentry_options_set_database_path(options, mDatabasePath.toUtf8().constData());
    sentry_options_set_release(options, QString(APP_VERSION APP_BUILD).toUtf8().constData());
    sentry_options_set_environment(options, getEnvironmentName().toUtf8().constData());
    
#ifdef QT_DEBUG
    sentry_options_set_debug(options, 1);
#endif

    if (sentry_init(options) == 0) {
        setCommonContext();
        qDebug() << "Sentry initialized successfully";
    } else {
        qWarning() << "Failed to initialize Sentry";
    }
}

void CrashHandler::setupPlatformHandlers()
{
#ifdef Q_OS_WIN
    SetUnhandledExceptionFilter(handleWindowsException);
#else
    struct sigaction sa;
    sa.sa_handler = handleCrash;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND;
    
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
#endif
}

#ifdef Q_OS_WIN
LONG WINAPI CrashHandler::handleWindowsException(EXCEPTION_POINTERS* exceptionInfo)
{
    sentry_value_t event = sentry_value_new_message_event(
        SENTRY_LEVEL_FATAL,
        "crash",
        "Windows exception occurred"
    );
    
    sentry_value_set_by_key(event, "exception_code",
        sentry_value_new_int32(exceptionInfo->ExceptionRecord->ExceptionCode));
    
    sentry_capture_event(event);
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

void CrashHandler::handleCrash(int signal)
{
    sentry_value_t event = sentry_value_new_message_event(
        SENTRY_LEVEL_FATAL,
        "crash",
        "Application crashed"
    );
    
    sentry_value_set_by_key(event, "signal",
        sentry_value_new_int32(signal));
    
    sentry_capture_event(event);
    
    // Restore default handler and re-raise signal
    signal(signal, SIG_DFL);
    raise(signal);
}

void CrashHandler::setCommonContext()
{
    sentry_value_t context = sentry_value_new_object();
    
    // App info
    sentry_value_set_by_key(context, "app_version", 
        sentry_value_new_string(QString(APP_VERSION).toUtf8().constData()));
    sentry_value_set_by_key(context, "app_build",
        sentry_value_new_string(QString(APP_BUILD).toUtf8().constData()));
    sentry_value_set_by_key(context, "qt_version",
        sentry_value_new_string(qVersion()));
    
    // System info
    sentry_value_set_by_key(context, "os_version",
        sentry_value_new_string(QSysInfo::prettyProductName().toUtf8().constData()));
    sentry_value_set_by_key(context, "os_kernel",
        sentry_value_new_string(QSysInfo::kernelVersion().toUtf8().constData()));
    sentry_value_set_by_key(context, "cpu_arch",
        sentry_value_new_string(QSysInfo::currentCpuArchitecture().toUtf8().constData()));
    
    sentry_set_context("app", context);
}

QString CrashHandler::getSentryDsn() const
{
    const char* envDsn = getenv("MUDLET_SENTRY_DSN");
    return envDsn ? QString::fromUtf8(envDsn) : QString::fromUtf8(SENTRY_DSN);
}

QString CrashHandler::getEnvironmentName() const
{
#ifdef QT_DEBUG
    return QStringLiteral("development");
#else
    QString build = QString::fromUtf8(APP_BUILD);
    if (build.contains("-ptb")) {
        return QStringLiteral("preview");
    } else if (build.contains("-dev")) {
        return QStringLiteral("development");
    }
    return QStringLiteral("production");
#endif
}

void CrashHandler::shutdown()
{
    if (mInitialized) {
        addBreadcrumb("Crash handler shutting down", "system");
        sentry_shutdown();
        mInitialized = false;
    }
}

void CrashHandler::setUserConsent(bool enabled)
{
    mUserConsented = enabled;
    
    if (enabled && !mInitialized) {
        initialize();
    } else if (!enabled && mInitialized) {
        shutdown();
    }
}

void CrashHandler::captureMessage(const QString& message)
{
    if (!mInitialized || !mUserConsented) {
        return;
    }

    sentry_value_t event = sentry_value_new_message_event(
        SENTRY_LEVEL_INFO,
        "message",
        message.toUtf8().constData()
    );
    
    sentry_capture_event(event);
}

void CrashHandler::captureError(const QString& error, const QString& function)
{
    if (!mInitialized || !mUserConsented) {
        return;
    }

    sentry_value_t event = sentry_value_new_message_event(
        SENTRY_LEVEL_ERROR,
        "error",
        error.toUtf8().constData()
    );
    
    if (!function.isEmpty()) {
        sentry_value_set_by_key(event, "function",
            sentry_value_new_string(function.toUtf8().constData()));
    }
    
    sentry_capture_event(event);
}

void CrashHandler::addBreadcrumb(const QString& message, const QString& category)
{
    if (!mInitialized || !mUserConsented) {
        return;
    }

    sentry_value_t crumb = sentry_value_new_breadcrumb(nullptr, message.toUtf8().constData());
    sentry_value_set_by_key(crumb, "category",
        sentry_value_new_string(category.toUtf8().constData()));
    
    sentry_add_breadcrumb(crumb);
}