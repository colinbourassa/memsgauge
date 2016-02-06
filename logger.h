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
    void setTemperatureUnits(TemperatureUnits type) { m_tempUnits = type; }

private:
    uint8_t convertTemp(uint8_t degrees);

    MEMSInterface *m_mems;
    QString m_logExtension;
    QString m_logDir;
    QFile m_logFile;
    QTextStream m_logFileStream;
    QString m_lastAttemptedLog;
    TemperatureUnits m_tempUnits;
};

#endif // LOGGER_H

