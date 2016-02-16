#include <QDir>
#include <QDateTime>
#include "logger.h"

/**
 * Constructor. Sets the interface class pointer as
 * well as log directory and log file extension.
 */
Logger::Logger(MEMSInterface* memsiface):
m_logExtension(".txt"), m_logDir("logs")
{
  m_mems = memsiface;
}

/**
 * Attempts to open a log file with the name specified.
 * @return True on success, false otherwise
 */
bool Logger::openLog(QString fileName)
{
  bool success = false;

  m_lastAttemptedLog = m_logDir + QDir::separator() + fileName + m_logExtension;

  // if the 'logs' directory exists, or if we're able to create it...
  if (!m_logFile.isOpen() && (QDir(m_logDir).exists() || QDir().mkdir(m_logDir)))
  {
    // set the name of the log file and open it for writing
    bool alreadyExists = QFileInfo(m_lastAttemptedLog).exists();

    m_logFile.setFileName(m_lastAttemptedLog);
    if (m_logFile.open(QFile::WriteOnly | QFile::Append))
    {
      m_logFileStream.setDevice(&m_logFile);

      if (!alreadyExists)
      {
        m_logFileStream << "#time,engineSpeed,waterTemp,intakeAirTemp," <<
          "throttleVoltage,manifoldPressure,idleBypassPos,mainVoltage," <<
          "idleswitch,closedloop,lambdaVoltage_mV" << endl;
      }

      success = true;
    }
  }

  return success;
}

/**
 * Close the log file.
 */
void Logger::closeLog()
{
  m_logFile.close();
}

/**
 * Converts degrees F to degrees C if necessary
 */
uint8_t Logger::convertTemp(uint8_t degrees)
{
  if (m_tempUnits == Celsius)
  {
    return ((degrees - 32) * 0.555556);
  }
  else
  {
    return degrees;
  }
}

/**
 * Commands the logger to query the interface for the currently
 * buffered data, and write it to the file.
 */
void Logger::logData()
{
  mems_data* data = m_mems->getData();

  if (m_logFile.isOpen() && (m_logFileStream.status() == QTextStream::Ok))
  {
    m_logFileStream << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "," <<
      data->engine_rpm << "," <<
      convertTemp(data->coolant_temp_f) << "," <<
      convertTemp(data->intake_air_temp_f) << "," <<
      data->throttle_pot_voltage << "," <<
      data->map_kpa << "," <<
      data->iac_position << "," <<
      data->battery_voltage << "," <<
      data->idle_switch << "," <<
      data->closed_loop << "," <<
      data->lambda_voltage_mv << endl;
  }
}

/**
 * Returns the full path to the last log that we attempted to open.
 * @return Full path to last log file
 */
QString Logger::getLogPath()
{
  return m_lastAttemptedLog;
}
