#include <QThread>
#include <QDateTime>
#include <QCoreApplication>
#include <string.h>
#include "memsinterface.h"

/**
 * Constructor. Sets the serial device and measurement units.
 * @param device Name of (or path to) the serial device used to comminucate
 *  with the ECU.
 */
MEMSInterface::MEMSInterface(QString device, QObject * parent):
QObject(parent), m_deviceName(device), m_stopPolling(false), m_shutdownThread(false), m_initComplete(false)
{
  memset(&m_data, 0, sizeof(mems_data));
  memset(m_d0_response_buffer, 0, 4);
}

/**
 * Destructor.
 */
MEMSInterface::~MEMSInterface()
{
}

/**
 * Clears the block of fault codes.
 */
void MEMSInterface::onFaultCodesClearRequested()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (mems_clear_faults(&m_memsinfo))
    {
      emit faultCodesClearSuccess();
    }
    else
    {
      emit errorSendingCommand();
    }
  }
  else
  {
    emit notConnected();
  }
}

/**
 * Responds to a signal requesting that the idle air control valve be moved.
 */
void MEMSInterface::onIdleAirControlMovementRequest(int desiredPos)
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (!mems_move_iac(&m_memsinfo, desiredPos))
    {
      emit errorSendingCommand();
    }
  }
  else
  {
    emit notConnected();
  }

  emit moveIACComplete();
}

/**
 * Attempts to open the serial device that is connected to the ECU.
 * @return True if serial device was opened successfully and the
 *  ECU is responding to commands; false otherwise.
 */
bool MEMSInterface::connectToECU()
{
  bool status = mems_connect(&m_memsinfo, m_deviceName.toStdString().c_str()) &&
    mems_init_link(&m_memsinfo, m_d0_response_buffer);
  if (status)
  {
    emit gotEcuId(m_d0_response_buffer);
  }
  return status;
}

/**
 * Sets a flag that will cause us to stop polling and disconnect from the serial device.
 */
void MEMSInterface::disconnectFromECU()
{
  m_stopPolling = true;
}

/**
 * Cleans up and exits the worker thread.
 */
void MEMSInterface::onShutdownThreadRequest()
{
  if (m_serviceLoopRunning)
  {
    m_shutdownThread = true;
  }
  else
  {
    QThread::currentThread()->quit();
  }
}

/**
 * Indicates whether the serial device is currently open/connected.
 * @return True when the device is connected; false otherwise.
 */
bool MEMSInterface::isConnected()
{
  return (m_initComplete && mems_is_connected(&m_memsinfo));
}

/**
 * Responds to the parent thread being started by initializing the library
 * struct and emitting a signal indicating that the interface is ready.
 */
void MEMSInterface::onParentThreadStarted()
{
  // Initialize the interface state info struct here, so that
  // it's in the context of the thread that will use it.
  if (!m_initComplete)
  {
    mems_init(&m_memsinfo);
    m_initComplete = true;
  }

  emit interfaceThreadReady();
}

/**
 * Responds to a signal to start polling the ECU.
 */
void MEMSInterface::onStartPollingRequest()
{
  if (connectToECU())
  {
    emit connected();

    m_stopPolling = false;
    m_shutdownThread = false;
    runServiceLoop();
  }
  else
  {
#ifdef WIN32
    QString simpleDeviceName = m_deviceName;

    if (simpleDeviceName.indexOf("\\\\.\\") == 0)
    {
      simpleDeviceName.remove(0, 4);
    }
    emit failedToConnect(simpleDeviceName);
#else
    emit failedToConnect(m_deviceName);
#endif
  }
}

/**
 * Calls the library in a loop until commanded to stop.
 */
void MEMSInterface::runServiceLoop()
{
  bool connected = mems_is_connected(&m_memsinfo);

  m_serviceLoopRunning = true;
  while (!m_stopPolling && !m_shutdownThread && connected)
  {
    if (mems_read(&m_memsinfo, &m_data))
    {
      emit readSuccess();
      emit dataReady();
    }
    else
    {
      emit readError();
    }
    QCoreApplication::processEvents();
  }
  m_serviceLoopRunning = false;

  if (connected)
  {
    mems_disconnect(&m_memsinfo);
  }
  emit disconnected();

  if (m_shutdownThread)
  {
    QThread::currentThread()->quit();
  }
}

bool MEMSInterface::actuatorOnOffDelayTest(actuator_cmd onCmd, actuator_cmd offCmd)
{
  bool status = false;

  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (mems_test_actuator(&m_memsinfo, onCmd, NULL))
    {
      QThread::sleep(2);
      if (mems_test_actuator(&m_memsinfo, offCmd, NULL))
      {
        status = true;
      }
    }

    if (!status)
    {
      emit errorSendingCommand();
    }
  }
  else
  {
    emit notConnected();
  }

  return status;
}

void MEMSInterface::onFuelPumpTest()
{
  actuatorOnOffDelayTest(MEMS_FuelPumpOn, MEMS_FuelPumpOff);
  emit fuelPumpTestComplete();
}

void MEMSInterface::onPTCRelayTest()
{
  actuatorOnOffDelayTest(MEMS_PTCRelayOn, MEMS_PTCRelayOff);
  emit ptcRelayTestComplete();
}

void MEMSInterface::onACRelayTest()
{
  actuatorOnOffDelayTest(MEMS_ACRelayOn, MEMS_ACRelayOff);
  emit acRelayTestComplete();
}

void MEMSInterface::onIgnitionCoilTest()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (!mems_test_actuator(&m_memsinfo, MEMS_FireCoil, NULL))
    {
      emit errorSendingCommand();
    }
  }
}

void MEMSInterface::onFuelInjectorTest()
{
  if (m_initComplete && mems_is_connected(&m_memsinfo))
  {
    if (!mems_test_actuator(&m_memsinfo, MEMS_TestInjectors, NULL))
    {
      emit errorSendingCommand();
    }
  }
}
