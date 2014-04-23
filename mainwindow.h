#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QLabel>
#include <QThread>
#include <QHash>
#include <QPair>
#include <QTimer>
#include <analogwidgets/manometer.h>
#include <qledindicator/qledindicator.h>
#include "optionsdialog.h"
#include "memsinterface.h"
#include "aboutbox.h"
#include "logger.h"
#include "fueltrimbar.h"
#include "commonunits.h"
#include "helpviewer.h"

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{    
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void onDataReady();
    void onConnect();
    void onDisconnect();
    void onReadError();
    void onReadSuccess();
    void onFailedToConnect(QString dev);
    void onInterfaceReady();
    void onNotConnected();

signals:
    void requestToStartPolling();
    void requestThreadShutdown();

    void fuelPumpControl(bool);
    void ptcRelayControl(bool);
    void acRelayControl(bool);
    void injectorTest();
    void coilTest();

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *m_ui;

    QTimer *m_fuelPumpTestTimer;
    QTimer *m_acRelayTestTimer;
    QTimer *m_ptcRelayTestTimer;

    QThread *m_memsThread;
    MEMSInterface *m_mems;
    OptionsDialog *m_options;
    AboutBox *m_aboutBox;
    QMessageBox *m_pleaseWaitBox;
    HelpViewer *m_helpViewerDialog;

    Logger *m_logger;

    static const float mapGaugeMaxPsi = 20.0;
    static const float mapGaugeMaxKPa = 160.0;

    QHash<PressureUnits,QString> *m_pressureUnitSuffix;
    QHash<TemperatureUnits,QString> *m_tempUnitSuffix;
    QHash<TemperatureUnits,QPair<int,int> > *m_tempRange;
    QHash<TemperatureUnits,QPair<int,int> > *m_tempLimits;

    void buildSpeedAndTempUnitTables();
    void setupWidgets();
    void setGearLabel(bool gearReading);    
    int convertPressure(int pressurePsi);
    int convertTemperature(int tempF);

private slots:
    void onExitSelected();
    void onEditOptionsClicked();
    void onHelpContentsClicked();
    void onHelpAboutClicked();
    void onConnectClicked();
    void onDisconnectClicked();
    void onStartLogging();
    void onStopLogging();

    void onTestFuelPumpRelayClicked();
    void onTestACRelayClicked();
    void onTestPTCRelayClicked();

    void onFuelPumpTestTimeout();
    void onACRelayTestTimeout();
    void onPTCRelayTestTimeout();
};

#endif // MAINWINDOW_H

