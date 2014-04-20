#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QLabel>
#include <QFile>
#include <QLineEdit>
#include <QTextStream>
#include <QThread>
#include <QFrame>
#include <QTableWidget>
#include <QHash>
#include <QPair>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <analogwidgets/manometer.h>
#include <qledindicator/qledindicator.h>
#include "optionsdialog.h"
#include "idleaircontroldialog.h"
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
    void requestFuelPumpOn();
    void requestFuelPumpOff();

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *m_ui;

    QTimer *m_fuelPumpCycleTimer;
    QThread *m_memsThread;
    MEMSInterface *m_mems;
    OptionsDialog *m_options;
    IdleAirControlDialog *m_iacDialog;
    AboutBox *m_aboutBox;
    QMessageBox *m_pleaseWaitBox;
    HelpViewer *m_helpViewerDialog;

    Logger *m_logger;

    static const float speedometerMaxMPH = 160.0;
    static const float speedometerMaxKPH = 240.0;

    QHash<SpeedUnits,QString> *m_speedUnitSuffix;
    QHash<TemperatureUnits,QString> *m_tempUnitSuffix;
    QHash<TemperatureUnits,QPair<int,int> > *m_tempRange;
    QHash<TemperatureUnits,QPair<int,int> > *m_tempLimits;

    void buildSpeedAndTempUnitTables();
    void setupWidgets();

    void setGearLabel(bool gearReading);
    void setLambdaTrimIndicators(int lambdaTrimOdd, int lambdaTrimEven);

    int convertSpeed(int speedMph);
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
    void onFuelPumpOneshot();
    void onFuelPumpContinuous();
    void onIdleAirControlClicked();
};

#endif // MAINWINDOW_H

