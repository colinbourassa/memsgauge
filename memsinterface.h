#ifndef MEMSINTERFACE_H
#define MEMSINTERFACE_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QByteArray>
#include <QHash>
#include "rosco.h"
#include "commonunits.h"

class MEMSInterface : public QObject
{
    Q_OBJECT
public:
    explicit MEMSInterface(QString device, QObject *parent = 0);
    ~MEMSInterface();

    void setSerialDevice(QString device) { m_deviceName = device; }

    QString getSerialDevice() { return m_deviceName; }
    int getIntervalMsecs();

    bool isConnected();
    void disconnectFromECU();

    mems_data* getData()          { return &m_data; }
    librosco_version getVersion() { return mems_get_lib_version(); }

    void cancelRead();

public slots:
    void onParentThreadStarted();
    void onFaultCodesClearRequested();
    void onStartPollingRequest();
    void onShutdownThreadRequest();

    void onFuelPumpTest();
    void onPTCRelayTest();
    void onACRelayTest();
    void onIgnitionCoilTest();
    void onFuelInjectorTest();
    void onIdleAirControlMovementRequest(int desiredPos);

signals:
    void dataReady();
    void connected();
    void disconnected();
    void readError();
    void readSuccess();
    void faultCodesClearSuccess();
    void failedToConnect(QString dev);
    void interfaceThreadReady();
    void notConnected();
    void gotEcuId(uint8_t* id_buffer);
    void errorSendingCommand();
    void fuelPumpTestComplete();
    void ptcRelayTestComplete();
    void acRelayTestComplete();
    void moveIACComplete();

private:
    mems_data m_data;
    QString m_deviceName;
    mems_info m_memsinfo;
    bool m_stopPolling;
    bool m_shutdownThread;
    bool m_initComplete;
    bool m_serviceLoopRunning;
    uint8_t m_d0_response_buffer[4];

    void runServiceLoop();
    bool connectToECU();
    bool actuatorOnOffDelayTest(actuator_cmd onCmd, actuator_cmd offCmd);
};

#endif // CUXINTERFACE_H
