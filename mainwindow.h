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
    void onInterfaceThreadReady();
    void onNotConnected();
    void onEcuIdReceived(uint8_t* id);
    void onFuelPumpTestComplete();
    void onACRelayTestComplete();
    void onPTCRelayTestComplete();
    void onMoveIACComplete();
    void onCommandError();
    void onFaultCodeClearComplete();

signals:
    void requestToStartPolling();
    void requestThreadShutdown();

    void fuelPumpTest();
    void ptcRelayTest();
    void acRelayTest();
    void injectorTest();
    void coilTest();
    void moveIAC(int desiredPos);

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *m_ui;

    QThread *m_memsThread;
    MEMSInterface *m_mems;
    OptionsDialog *m_options;
    AboutBox *m_aboutBox;
    QMessageBox *m_pleaseWaitBox;
    HelpViewer *m_helpViewerDialog;

    Logger *m_logger;

    static const float mapGaugeMaxPsi = 16.0;
    static const float mapGaugeMaxKPa = 160.0;

    bool m_actuatorTestsEnabled;

    QHash<TemperatureUnits,QString> *m_tempUnitSuffix;
    QHash<TemperatureUnits,QPair<int,int> > *m_tempRange;
    QHash<TemperatureUnits,QPair<int,int> > *m_tempLimits;

    void buildSpeedAndTempUnitTables();
    void setupWidgets();
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
    void onMoveIACClicked();
    void onTestFuelPumpRelayClicked();
    void onTestACRelayClicked();
    void onTestPTCRelayClicked();    

    void setActuatorTestsEnabled(bool enabled);
};

#endif // MAINWINDOW_H

