#ifndef MEMSINTERFACE_H
#define MEMSINTERFACE_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QByteArray>
#include <QHash>
#include "mems.h"
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

    mems_data getData()          { return m_data; }
    libmems_version getVersion() { return libmems_version(); }

    void cancelRead();

public slots:
    void onParentThreadStarted();
    void onFaultCodesClearRequested();
    void onStartPollingRequest();
    void onShutdownThreadRequest();
    void onFuelPumpOnRequest();
    void onFuelPumpOffRequest();
    void onIdleAirControlMovementRequest(int direction);

signals:
    void dataReady();
    void connected();
    void disconnected();
    void readError();
    void readSuccess();
    void faultCodesReadFailed();
    void faultCodesClearSuccess();
    void faultCodesClearFailure();
    void failedToConnect(QString dev);
    void interfaceReadyForPolling();
    void notConnected();

private:
    mems_data m_data;
    QString m_deviceName;
    mems_info m_memsinfo;
    bool m_stopPolling;
    bool m_shutdownThread;
    bool m_initComplete;

    void runServiceLoop();
    bool connectToECU();
};

#endif // CUXINTERFACE_H
