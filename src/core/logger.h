#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QMutex>

class Logger
{
public:
    static void init(const QString& filePath);
    static void shutdown();
    static QString logFilePath();

private:
    static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
    static bool shouldFilter(const QString& msg, const char* category);

    static QFile* s_file;
    static QMutex s_mutex;
    static QString s_filePath;
    static QtMessageHandler s_originalHandler;
};

#endif // LOGGER_H
