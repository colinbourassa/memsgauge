#include <QCloseEvent>
#include <QMessageBox>
#include <QList>
#include <QDateTime>
#include <QDir>
#include <QHBoxLayout>
#include <QThread>
#include <QFileDialog>
#include <QDesktopWidget>
#include <QCryptographicHash>
#include <QGraphicsOpacityEffect>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "faultcodedialog.h"

/**
 * Constructor; sets up main UI
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_ui(new Ui::MainWindow),
      m_memsThread(0),
      m_mems(0),
      m_options(0),
      m_aboutBox(0),
      m_pleaseWaitBox(0),
      m_helpViewerDialog(0)
{
    buildSpeedAndTempUnitTables();
    m_ui->setupUi(this);
    this->setWindowTitle("RoverGauge");

    m_options = new OptionsDialog(this->windowTitle(), this);
    m_mems = new MEMSInterface(m_options->getSerialDeviceName());

    m_iacDialog = new IdleAirControlDialog(this->windowTitle(), this);
    connect(m_iacDialog, SIGNAL(requestIdleAirControlMovement(int)),
            m_mems, SLOT(onIdleAirControlMovementRequest(int)));

    m_logger = new Logger(m_mems);

    m_fuelPumpCycleTimer = new QTimer(this);

    connect(m_mems, SIGNAL(dataReady()), this, SLOT(onDataReady()));
    connect(m_mems, SIGNAL(connected()), this, SLOT(onConnect()));
    connect(m_mems, SIGNAL(disconnected()), this, SLOT(onDisconnect()));
    connect(m_mems, SIGNAL(readError()), this, SLOT(onReadError()));
    connect(m_mems, SIGNAL(readSuccess()), this, SLOT(onReadSuccess()));
    connect(m_mems, SIGNAL(failedToConnect(QString)), this, SLOT(onFailedToConnect(QString)));
    connect(m_mems, SIGNAL(interfaceReadyForPolling()), this, SLOT(onInterfaceReady()));
    connect(m_mems, SIGNAL(notConnected()), this, SLOT(onNotConnected()));
    connect(m_fuelPumpCycleTimer, SIGNAL(timeout()), m_mems, SLOT(onFuelPumpOffRequest()));
    connect(this, SIGNAL(requestToStartPolling()), m_mems, SLOT(onStartPollingRequest()));
    connect(this, SIGNAL(requestThreadShutdown()), m_mems, SLOT(onShutdownThreadRequest()));
    connect(this, SIGNAL(requestFuelPumpOn()), m_mems, SLOT(onFuelPumpOnRequest()));
    connect(this, SIGNAL(requestFuelPumpOff()), m_mems, SLOT(onFuelPumpOffRequest()));

    setWindowIcon(QIcon(":/icons/key.png"));

    setupWidgets();
}

/**
 * Destructor; cleans up instance of 14CUX communications library
 *  and miscellaneous data storage
 */
MainWindow::~MainWindow()
{
    delete m_tempLimits;
    delete m_tempRange;
    delete m_speedUnitSuffix;
    delete m_tempUnitSuffix;
    delete m_aboutBox;
    delete m_options;
    delete m_mems;
    delete m_memsThread;
}

/**
 * Populates hash tables with unit-of-measure suffixes and temperature thresholds
 */
void MainWindow::buildSpeedAndTempUnitTables()
{
    m_speedUnitSuffix = new QHash<SpeedUnits,QString>();
    m_speedUnitSuffix->insert(MPH, " MPH");
    m_speedUnitSuffix->insert(KPH, " km/h");

    m_tempUnitSuffix = new QHash<TemperatureUnits,QString>;
    m_tempUnitSuffix->insert(Fahrenheit, " F");
    m_tempUnitSuffix->insert(Celcius, " C");

    m_tempRange = new QHash<TemperatureUnits,QPair<int, int> >;
    m_tempRange->insert(Fahrenheit, qMakePair(-40, 280));
    m_tempRange->insert(Celcius, qMakePair(-40, 140));

    m_tempLimits = new QHash<TemperatureUnits,QPair<int, int> >;
    m_tempLimits->insert(Fahrenheit, qMakePair(180, 210));
    m_tempLimits->insert(Celcius, qMakePair(80, 98));
}

/**
 * Instantiates widgets used in the main window.
 */
void MainWindow::setupWidgets()
{
    // set menu and button icons
    m_ui->m_saveROMImageAction->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_ui->m_exitAction->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    m_ui->m_showFaultCodesAction->setIcon(style()->standardIcon(QStyle::SP_DialogNoButton));
    m_ui->m_editSettingsAction->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    m_ui->m_helpContentsAction->setIcon(style()->standardIcon(QStyle::SP_DialogHelpButton));
    m_ui->m_helpAboutAction->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
    m_ui->m_startLoggingButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_ui->m_stopLoggingButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));

    // connect menu item signals
    connect(m_ui->m_exitAction, SIGNAL(triggered()), this, SLOT(onExitSelected()));
    connect(m_ui->m_idleAirControlAction, SIGNAL(triggered()), this, SLOT(onIdleAirControlClicked()));
    connect(m_ui->m_editSettingsAction, SIGNAL(triggered()), this, SLOT(onEditOptionsClicked()));
    connect(m_ui->m_helpContentsAction, SIGNAL(triggered()), this, SLOT(onHelpContentsClicked()));
    connect(m_ui->m_helpAboutAction, SIGNAL(triggered()), this, SLOT(onHelpAboutClicked()));

    // connect button signals
    connect(m_ui->m_connectButton, SIGNAL(clicked()), this, SLOT(onConnectClicked()));
    connect(m_ui->m_disconnectButton, SIGNAL(clicked()), this, SLOT(onDisconnectClicked()));
    connect(m_ui->m_fuelPumpOneshotButton, SIGNAL(clicked()), this, SLOT(onFuelPumpOneshot()));
    connect(m_ui->m_fuelPumpContinuousButton, SIGNAL(clicked()), this, SLOT(onFuelPumpContinuous()));
    connect(m_ui->m_startLoggingButton, SIGNAL(clicked()), this, SLOT(onStartLogging()));
    connect(m_ui->m_stopLoggingButton, SIGNAL(clicked()), this, SLOT(onStopLogging()));

    // set the LED colors
    m_ui->m_milLed->setOnColor1(QColor(255, 0, 0));
    m_ui->m_milLed->setOnColor2(QColor(176, 0, 2));
    m_ui->m_milLed->setOffColor1(QColor(20, 0, 0));
    m_ui->m_milLed->setOffColor2(QColor(90, 0, 2));
    m_ui->m_milLed->setDisabled(true);
    m_ui->m_commsGoodLed->setOnColor1(QColor(102, 255, 102));
    m_ui->m_commsGoodLed->setOnColor2(QColor(82, 204, 82));
    m_ui->m_commsGoodLed->setOffColor1(QColor(0, 102, 0));
    m_ui->m_commsGoodLed->setOffColor2(QColor(0, 51, 0));
    m_ui->m_commsGoodLed->setDisabled(true);
    m_ui->m_commsBadLed->setOnColor1(QColor(255, 0, 0));
    m_ui->m_commsBadLed->setOnColor2(QColor(176, 0, 2));
    m_ui->m_commsBadLed->setOffColor1(QColor(20, 0, 0));
    m_ui->m_commsBadLed->setOffColor2(QColor(90, 0, 2));
    m_ui->m_commsBadLed->setDisabled(true);
    m_ui->m_idleModeLed->setOnColor1(QColor(102, 255, 102));
    m_ui->m_idleModeLed->setOnColor2(QColor(82, 204, 82));
    m_ui->m_idleModeLed->setOffColor1(QColor(0, 102, 0));
    m_ui->m_idleModeLed->setOffColor2(QColor(0, 51, 0));
    m_ui->m_idleModeLed->setDisabled(true);
    m_ui->m_fuelPumpRelayStateLed->setOnColor1(QColor(102, 255, 102));
    m_ui->m_fuelPumpRelayStateLed->setOnColor2(QColor(82, 204, 82));
    m_ui->m_fuelPumpRelayStateLed->setOffColor1(QColor(0, 102, 0));
    m_ui->m_fuelPumpRelayStateLed->setOffColor2(QColor(0, 51, 0));
    m_ui->m_fuelPumpRelayStateLed->setDisabled(true);

    m_ui->m_logFileNameBox->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss"));

    SpeedUnits speedUnit = m_options->getSpeedUnits();

    m_ui->m_speedo->setMinimum(0.0);
    m_ui->m_speedo->setMaximum((m_options->getSpeedUnits() == MPH) ? speedometerMaxMPH : speedometerMaxKPH);
    m_ui->m_speedo->setSuffix(m_speedUnitSuffix->value(speedUnit));
    m_ui->m_speedo->setNominal(1000.0);
    m_ui->m_speedo->setCritical(1000.0);

    m_ui->m_revCounter->setMinimum(0.0);
    m_ui->m_revCounter->setMaximum(8000);
    m_ui->m_revCounter->setSuffix(" RPM");
    m_ui->m_revCounter->setNominal(100000.0);
    m_ui->m_revCounter->setCritical(8000);

    TemperatureUnits tempUnits = m_options->getTemperatureUnits();
    int tempMin = m_tempRange->value(tempUnits).first;
    int tempMax = m_tempRange->value(tempUnits).second;

    m_ui->m_waterTempGauge->setValue(tempMin);
    m_ui->m_waterTempGauge->setMaximum(tempMax);
    m_ui->m_waterTempGauge->setMinimum(tempMin);
    m_ui->m_waterTempGauge->setSuffix(m_tempUnitSuffix->value(tempUnits));
    m_ui->m_waterTempGauge->setNominal(m_tempLimits->value(tempUnits).first);
    m_ui->m_waterTempGauge->setCritical(m_tempLimits->value(tempUnits).second);

    m_ui->m_airTempGauge->setValue(tempMin);
    m_ui->m_airTempGauge->setMaximum(tempMax);
    m_ui->m_airTempGauge->setMinimum(tempMin);
    m_ui->m_airTempGauge->setSuffix(m_tempUnitSuffix->value(tempUnits));
    m_ui->m_airTempGauge->setNominal(10000.0);
    m_ui->m_airTempGauge->setCritical(10000.0);
}

/**
 * Attempts to open the serial device connected to the 14CUX,
 * and starts updating the display with data if successful.
 */
void MainWindow::onConnectClicked()
{
    // If the worker thread hasn't been created yet, do that now.
    if (m_memsThread == 0)
    {
        m_memsThread = new QThread(this);
        m_mems->moveToThread(m_memsThread);
        connect(m_memsThread, SIGNAL(started()), m_mems, SLOT(onParentThreadStarted()));
    }

    // If the worker thread is alreay running, ask it to start polling the ECU.
    // Otherwise, start the worker thread, but don't ask it to begin polling
    // yet; it'll signal us when it's ready.
    if (m_memsThread->isRunning())
    {
        emit requestToStartPolling();
    }
    else
    {
        m_memsThread->start();
    }
}

/**
 * Responds to the signal from the worker thread which indicates that it's
 * ready to start polling the ECU. This routine emits the "start polling"
 * command signal.
 */
void MainWindow::onInterfaceReady()
{
    emit requestToStartPolling();
}

/**
 * Sets a flag in the worker thread that tells it to disconnect from the
 * serial device.
 */
void MainWindow::onDisconnectClicked()
{
    m_ui->m_disconnectButton->setEnabled(false);
    m_mems->disconnectFromECU();
}

/**
 * Closes the main window and terminates the application.
 */
void MainWindow::onExitSelected()
{
    this->close();
}

/**
 * Converts speed in miles per hour to the desired units.
 * @param speedMph Speed in miles per hour
 * @return Speed in the desired units
 */
int MainWindow::convertSpeed(int speedMph)
{
    double speed = speedMph;

    switch (m_options->getSpeedUnits())
    {
    case KPH:
        speed *= 1.609344;
        break;
    default:
        break;
    }

    return (int)speed;
}

/**
 * Converts temperature in Fahrenheit degrees to the desired units.
 * @param tempF Temperature in Fahrenheit degrees
 * @return Temperature in the desired units
 */
int MainWindow::convertTemperature(int tempF)
{
    double temp = tempF;

    switch (m_options->getTemperatureUnits())
    {
    case Celcius:
        temp = (temp - 32) * (0.5555556);
        break;
    case Fahrenheit:
    default:
        break;
    }

    return (int)temp;
}


/**
 * Updates the gauges and indicators with the latest data available from
 * the ECU.
 */
void MainWindow::onDataReady()
{
    mems_data data = m_mems->getData();

    //m_ui->m_milLed->setChecked(m_mems->isMILOn());

    m_ui->m_throttleBar->setValue((float)data.throttle_pot_voltage / 5.10 * 100);
    m_ui->m_mapValue->setText(QString::number(data.map_psi));
    m_ui->m_idleBypassPosBar->setValue((float)data.iac_position / 255.0 * 100);
    m_ui->m_revCounter->setValue(data.engine_rpm);
    m_ui->m_waterTempGauge->setValue(convertTemperature(data.coolant_temp_f));
    m_ui->m_airTempGauge->setValue(convertTemperature(data.intake_air_temp_f));
    m_ui->m_voltage->setText(QString::number(data.battery_voltage, 'f', 1) + "VDC");

    /*
        int targetIdleSpeedRPM = m_mems->getTargetIdleSpeed();
        if (targetIdleSpeedRPM > 0)
            m_ui->m_targetIdle->setText(QString::number(targetIdleSpeedRPM));
        else
            m_ui->m_targetIdle->setText("");
    */

    m_ui->m_idleModeLed->setChecked(data.idle_switch);

    setGearLabel(data.park_neutral_switch);

    m_logger->logData();
}

/**
 * Sets the label indicating the current gear selection
 */
void MainWindow::setGearLabel(bool gearReading)
{
    if (gearReading)
    {
        m_ui->m_gear->setText("Park/Neutral");
    }
    else
    {
        m_ui->m_gear->setText("Drive/Reverse");
    }
}

/**
 * Opens the settings dialog, where the user may change the
 * serial device, timer interval, gauge settings, and
 * correction factors.
 */
void MainWindow::onEditOptionsClicked()
{
    // if the user doesn't cancel the options dialog...
    if (m_options->exec() == QDialog::Accepted)
    {
        // update the speedo appropriately
        SpeedUnits speedUnits = m_options->getSpeedUnits();
        if (speedUnits == MPH)
        {
            m_ui->m_speedo->setMaximum(speedometerMaxMPH);
        }
        else
        {
            m_ui->m_speedo->setMaximum(speedometerMaxKPH);
        }
        m_ui->m_speedo->setSuffix(m_speedUnitSuffix->value(speedUnits));
        m_ui->m_speedo->repaint();

        TemperatureUnits tempUnits = m_options->getTemperatureUnits();
        QString tempUnitStr = m_tempUnitSuffix->value(tempUnits);

        int tempMin = m_tempRange->value(tempUnits).first;
        int tempMax = m_tempRange->value(tempUnits).second;
        int tempNominal = m_tempLimits->value(tempUnits).first;
        int tempCritical = m_tempLimits->value(tempUnits).second;

        m_ui->m_airTempGauge->setSuffix(tempUnitStr);
        m_ui->m_airTempGauge->setValue(tempMin);
        m_ui->m_airTempGauge->setMaximum(tempMax);
        m_ui->m_airTempGauge->setMinimum(tempMin);
        m_ui->m_airTempGauge->repaint();

        m_ui->m_waterTempGauge->setSuffix(tempUnitStr);
        m_ui->m_waterTempGauge->setValue(tempMin);
        m_ui->m_waterTempGauge->setMaximum(tempMax);
        m_ui->m_waterTempGauge->setMinimum(tempMin);
        m_ui->m_waterTempGauge->setNominal(tempNominal);
        m_ui->m_waterTempGauge->setCritical(tempCritical);
        m_ui->m_waterTempGauge->repaint();

        // if the user changed the serial device name and/or the polling
        // interval, stop the timer, re-connect to the 14CUX (if neccessary),
        // and restart the timer
        if (m_options->getSerialDeviceChanged())
        {
            if (m_mems->isConnected())
            {
                m_mems->disconnectFromECU();
            }
            m_mems->setSerialDevice(m_options->getSerialDeviceName());
        }
    }
}

/**
 * Responds to a 'close' event on the main window by first shutting down
 * the other thread.
 * @param event The event itself.
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    m_logger->closeLog();

    if ((m_memsThread != 0) && m_memsThread->isRunning())
    {
        emit requestThreadShutdown();
        m_memsThread->wait(2000);
    }

    event->accept();
}

/**
 * Reponds to the "connect" signal from the MEMSInterface by enabling/disabling
 * the appropriate buttons and setting a message in the status bar.
 */
void MainWindow::onConnect()
{
    m_ui->m_connectButton->setEnabled(false);
    m_ui->m_disconnectButton->setEnabled(true);
    m_ui->m_commsGoodLed->setChecked(false);
    m_ui->m_commsBadLed->setChecked(false);
    m_ui->m_fuelPumpOneshotButton->setEnabled(true);
    m_ui->m_fuelPumpContinuousButton->setEnabled(true);
}

/**
 * Reponds to the "disconnect" signal from the MEMSInterface by enabling/disabling
 * the appropriate buttons and setting a message in the status bar.
 */
void MainWindow::onDisconnect()
{
    m_ui->m_connectButton->setEnabled(true);
    m_ui->m_disconnectButton->setEnabled(false);
    m_ui->m_milLed->setChecked(false);
    m_ui->m_commsGoodLed->setChecked(false);
    m_ui->m_commsBadLed->setChecked(false);
    m_ui->m_fuelPumpOneshotButton->setEnabled(false);
    m_ui->m_fuelPumpContinuousButton->setEnabled(false);
    m_ui->m_tuneRevNumberLabel->setText("Tune:");
    m_ui->m_identLabel->setText("Ident:");
    m_ui->m_checksumFixerLabel->setText("Checksum fixer:");

    m_ui->m_speedo->setValue(0.0);
    m_ui->m_revCounter->setValue(0.0);
    m_ui->m_waterTempGauge->setValue(m_ui->m_waterTempGauge->minimum());
    m_ui->m_airTempGauge->setValue(m_ui->m_airTempGauge->minimum());
    m_ui->m_throttleBar->setValue(0);
    m_ui->m_mapValue->setText("");
    m_ui->m_idleBypassPosBar->setValue(0);
    m_ui->m_idleModeLed->setChecked(false);
    m_ui->m_targetIdle->setText("");
    m_ui->m_voltage->setText("");
    m_ui->m_gear->setText("");
    m_ui->m_fuelPumpRelayStateLed->setChecked(false);
    m_ui->m_oddFuelTrimBar->setValue(0);
    if (m_ui->m_oddFuelTrimBar->isVisible())
        m_ui->m_oddFuelTrimBarLabel->setText("+0%");
    else
        m_ui->m_oddFuelTrimBarLabel->setText("0.0VDC");

    m_ui->m_oddFuelTrimBar->repaint();
}

/**
 * Responds to the "read error" signal from the worker thread by turning
 * on a red lamp.
 */
void MainWindow::onReadError()
{
    m_ui->m_commsGoodLed->setChecked(false);
    m_ui->m_commsBadLed->setChecked(true);
}

/**
 * Responds to the "read success" signal from the worker thread by turning
 * on a green lamp.
 */
void MainWindow::onReadSuccess()
{
    m_ui->m_commsGoodLed->setChecked(true);
    m_ui->m_commsBadLed->setChecked(false);
}

/**
 * Opens the log file for writing.
 */
void MainWindow::onStartLogging()
{
    if (m_logger->openLog(m_ui->m_logFileNameBox->text()))
    {
        m_ui->m_startLoggingButton->setEnabled(false);
        m_ui->m_stopLoggingButton->setEnabled(true);
    }
    else
    {
        QMessageBox::warning(this, "Error",
            "Failed to open log file (" + m_logger->getLogPath() + ")", QMessageBox::Ok);
    }
}

/**
 * Closes the open log file.
 */
void MainWindow::onStopLogging()
{
    m_logger->closeLog();
    m_ui->m_stopLoggingButton->setEnabled(false);
    m_ui->m_startLoggingButton->setEnabled(true);
}

/**
 * Displays an dialog box with information about the program.
 */
void MainWindow::onHelpAboutClicked()
{
    if (m_aboutBox == 0)
    {
        m_aboutBox = new AboutBox(style(), this->windowTitle(), m_mems->getVersion(), this);
    }
    m_aboutBox->exec();
}

/**
 * Displays the online help.
 */
void MainWindow::onHelpContentsClicked()
{
    if (m_helpViewerDialog == 0)
    {
        m_helpViewerDialog = new HelpViewer(this->windowTitle(), this);
    }
    m_helpViewerDialog->show();
}

/**
 * Displays a message box indicating that an error in connecting to the
 * serial device.
 * @param Name of the serial device that the software attempted to open
 */
void MainWindow::onFailedToConnect(QString dev)
{
    if (dev.isEmpty() || dev.isNull())
    {
        QMessageBox::warning(this, "Error",
            QString("Error connecting to 14CUX. No serial port name specified.\n\n") +
            QString("Set a serial device using \"Options\" --> \"Edit Settings\""),
            QMessageBox::Ok);
    }
    else
    {
        QMessageBox::warning(this, "Error",
            "Error connecting to 14CUX. Could not open serial device: " + dev,
            QMessageBox::Ok);
    }
}

/**
 * Displays a message box indicating that an action cannot be completed due
 * to the software not being connected to the ECU.
 */
void MainWindow::onNotConnected()
{
    if (m_pleaseWaitBox != 0)
    {
        m_pleaseWaitBox->hide();
    }

    QMessageBox::warning(this, "Error",
        "This requires that the software first be connected to the ECU (using the \"Connect\" button.)",
        QMessageBox::Ok);
}

/**
 * Starts a timer that periodically re-sends the signal to run the fuel
 * pump, thus keeping the pump running continuously.
 */
void MainWindow::onFuelPumpContinuous()
{
    if (m_ui->m_fuelPumpContinuousButton->isChecked())
    {
        emit requestFuelPumpOn();
        m_ui->m_fuelPumpOneshotButton->setEnabled(false);
    }
    else
    {
        emit requestFuelPumpOff();
        m_ui->m_fuelPumpOneshotButton->setEnabled(true);
    }
}

void MainWindow::onFuelPumpOneshot()
{
    emit requestFuelPumpOn();
    m_fuelPumpCycleTimer->start(2000);
}

/**
 * Displays the idle-air-control dialog.
 */
void MainWindow::onIdleAirControlClicked()
{
    m_iacDialog->show();
}
