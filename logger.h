#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include "memsinterface.h"

class Logger
{
public:
    Logger(MEMSInterface *memsiface);
    bool openLog(QString fileName);
    void closeLog();
    void logData();
    QString getLogPath();

private:
    MEMSInterface *m_mems;
    QString m_logExtension;
    QString m_logDir;
    QFile m_logFile;
    QTextStream m_logFileStream;
    QString m_lastAttemptedLog;
};

#endif // LOGGER_H

