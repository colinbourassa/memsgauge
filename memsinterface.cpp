#include <QThread>
#include <QDateTime>
#include <QCoreApplication>
#include <string.h>
#include "memsinterface.h"

/**
 * Constructor. Sets the serial device and measurement units.
 * @param device Name of (or path to) the serial device used to comminucate
 *  with the 14CUX.
 * @param sUnits Units to be used when expressing road speed
 * @param tUnits Units to be used when expressing coolant/fuel temperature
 */
MEMSInterface::MEMSInterface(QString device, QObject *parent) :
    QObject(parent),
    m_deviceName(device),
    m_stopPolling(false),
    m_shutdownThread(false),
    m_initComplete(false)
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
            emit faultCodesClearFailure();
        }
    }
    else
    {
        emit notConnected();
    }
}

/**
 * Responds to a signal requesting that the idle air control valve be moved.
 * @param direction Direction of travel for the idle air control valve;
 *  0 to open and 1 to close
 * @param steps Number of steps to move the valve in the specified direction
 */
void MEMSInterface::onIdleAirControlMovementRequest(int direction)
{
    uint8_t new_pos = 0;
    if (m_initComplete && mems_is_connected(&m_memsinfo))
    {
        mems_move_idle_bypass_motor(&m_memsinfo, (bool)direction, &new_pos);
    }
    else
    {
        emit notConnected();
    }
}

/**
 * Attempts to open the serial device that is connected to the 14CUX.
 * @return True if serial device was opened successfully; false otherwise.
 */
bool MEMSInterface::connectToECU()
{
    bool status = mems_connect(&m_memsinfo, m_deviceName.toStdString().c_str()) &&
                  mems_init_link(&m_memsinfo, m_d0_response_buffer);

    if (status)
    {
        emit connected(m_d0_response_buffer);
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
    // If we're currently connected, just set a flag to let the polling loop
    // shut the thread down. Otherwise, shut it down here.
    if (m_initComplete && mems_is_connected(&m_memsinfo))
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

    emit interfaceReadyForPolling();
}

/**
 * Responds to a signal to start polling the ECU.
 */
void MEMSInterface::onStartPollingRequest()
{
    if (connectToECU())
    {
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

void MEMSInterface::onFuelPumpControl(bool state)
{
    if (m_initComplete && mems_is_connected(&m_memsinfo))
    {
        mems_fuel_pump_control(&m_memsinfo, state);
    }
}

void MEMSInterface::onPTCRelayControl(bool state)
{
    if (m_initComplete && mems_is_connected(&m_memsinfo))
    {
        mems_ptc_relay_control(&m_memsinfo, state);
    }
}

void MEMSInterface::onACRelayControl(bool state)
{
    if (m_initComplete && mems_is_connected(&m_memsinfo))
    {
        mems_ac_relay_control(&m_memsinfo, state);
    }
}

void MEMSInterface::onIgnitionCoilTest()
{
    if (m_initComplete && mems_is_connected(&m_memsinfo))
    {
        mems_test_coil(&m_memsinfo);
    }
}

void MEMSInterface::onFuelInjectorTest()
{
    if (m_initComplete && mems_is_connected(&m_memsinfo))
    {
        mems_test_injectors(&m_memsinfo);
    }
}
