#include "logger.h"
#include <QDateTime>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>

QFile* Logger::s_file = nullptr;
QMutex Logger::s_mutex;
QString Logger::s_filePath;
QtMessageHandler Logger::s_originalHandler = nullptr;

void Logger::init(const QString& filePath)
{
    QMutexLocker lock(&s_mutex);

    if (s_file) {
        return; // Already initialized
    }

    s_filePath = filePath;

    // Ensure directory exists
    QFileInfo fi(filePath);
    QDir().mkpath(fi.absolutePath());

    s_file = new QFile(filePath);
    if (!s_file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        delete s_file;
        s_file = nullptr;
        return;
    }

    // Write header
    QTextStream stream(s_file);
    stream << "\n========================================\n";
    stream << "Log started: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
    stream << "========================================\n";
    s_file->flush();

    // Install our message handler to capture all qDebug() etc.
    s_originalHandler = qInstallMessageHandler(messageHandler);
}

void Logger::shutdown()
{
    QMutexLocker lock(&s_mutex);

    // Restore original handler
    if (s_originalHandler) {
        qInstallMessageHandler(s_originalHandler);
        s_originalHandler = nullptr;
    }

    if (s_file) {
        s_file->close();
        delete s_file;
        s_file = nullptr;
    }
}

QString Logger::logFilePath()
{
    return s_filePath;
}

bool Logger::shouldFilter(const QString& msg, const char* category)
{
    // Filter out Windows Bluetooth driver noise
    if (msg.contains("Windows.Devices.Bluetooth") ||
        msg.contains("ReturnHr") ||
        msg.contains("LogHr")) {
        return true;
    }

    // Filter out Android QtBluetoothGatt noise
    QString cat = category ? QString::fromLatin1(category) : QString();
    if (cat.contains("QtBluetoothGatt") ||
        msg.contains("Perform next BTLE IO") ||
        msg.contains("Performing queued job") ||
        msg.contains("BluetoothGatt")) {
        return true;
    }

    return false;
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    // Filter noisy messages
    if (shouldFilter(msg, context.category)) {
        return;
    }

    // Format: [HH:mm:ss.zzz] LEVEL: message
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    QString level;
    switch (type) {
        case QtDebugMsg:    level = "DEBUG"; break;
        case QtInfoMsg:     level = "INFO"; break;
        case QtWarningMsg:  level = "WARN"; break;
        case QtCriticalMsg: level = "ERROR"; break;
        case QtFatalMsg:    level = "FATAL"; break;
    }

    QString line = QString("[%1] %2: %3").arg(timestamp, level, msg);

    // Write to file
    {
        QMutexLocker lock(&s_mutex);
        if (s_file && s_file->isOpen()) {
            QTextStream stream(s_file);
            stream << line << "\n";
            s_file->flush();
        }
    }

    // Also pass to original handler (for logcat/console)
    if (s_originalHandler) {
        s_originalHandler(type, context, msg);
    }
}
