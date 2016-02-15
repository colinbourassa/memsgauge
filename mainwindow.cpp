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

MainWindow::MainWindow(QWidget* parent):QMainWindow(parent),
m_ui(new Ui::MainWindow),
m_memsThread(0),
m_mems(0), m_options(0), m_aboutBox(0), m_pleaseWaitBox(0), m_helpViewerDialog(0), m_actuatorTestsEnabled(false)
{
  buildSpeedAndTempUnitTables();
  m_ui->setupUi(this);
  this->setWindowTitle(PROJECTNAME + QString(" ") +
      QString::number(VER_MAJOR) + "." +
      QString::number(VER_MINOR) + "." +
      QString::number(VER_PATCH));

  m_options = new OptionsDialog(this->windowTitle(), this);
  m_mems = new MEMSInterface(m_options->getSerialDeviceName());
  m_logger = new Logger(m_mems);

  connect(m_mems, SIGNAL(dataReady()), this, SLOT(onDataReady()));
  connect(m_mems, SIGNAL(connected()), this, SLOT(onConnect()));
  connect(m_mems, SIGNAL(disconnected()), this, SLOT(onDisconnect()));
  connect(m_mems, SIGNAL(readError()), this, SLOT(onReadError()));
  connect(m_mems, SIGNAL(readSuccess()), this, SLOT(onReadSuccess()));
  connect(m_mems, SIGNAL(failedToConnect(QString)), this, SLOT(onFailedToConnect(QString)));
  connect(m_mems, SIGNAL(interfaceThreadReady()), this, SLOT(onInterfaceThreadReady()));
  connect(m_mems, SIGNAL(notConnected()), this, SLOT(onNotConnected()));
  connect(m_mems, SIGNAL(gotEcuId(uint8_t *)), this, SLOT(onEcuIdReceived(uint8_t *)));
  connect(m_mems, SIGNAL(errorSendingCommand()), this, SLOT(onCommandError()));

  connect(m_mems, SIGNAL(fuelPumpTestComplete()), this, SLOT(onFuelPumpTestComplete()));
  connect(m_mems, SIGNAL(acRelayTestComplete()), this, SLOT(onACRelayTestComplete()));
  connect(m_mems, SIGNAL(ptcRelayTestComplete()), this, SLOT(onPTCRelayTestComplete()));
  connect(m_mems, SIGNAL(moveIACComplete()), this, SLOT(onMoveIACComplete()));
  connect(this, SIGNAL(moveIAC(int)), m_mems, SLOT(onIdleAirControlMovementRequest(int)));

  connect(this, SIGNAL(fuelPumpTest()), m_mems, SLOT(onFuelPumpTest()));
  connect(this, SIGNAL(acRelayTest()), m_mems, SLOT(onACRelayTest()));
  connect(this, SIGNAL(ptcRelayTest()), m_mems, SLOT(onPTCRelayTest()));

  connect(m_mems, SIGNAL(faultCodesClearSuccess()), this, SLOT(onFaultCodeClearComplete()));

  connect(this, SIGNAL(requestToStartPolling()), m_mems, SLOT(onStartPollingRequest()));
  connect(this, SIGNAL(requestThreadShutdown()), m_mems, SLOT(onShutdownThreadRequest()));

  setWindowIcon(QIcon(":/icons/key.png"));

  setupWidgets();
}

MainWindow::~MainWindow()
{
  delete m_tempLimits;
  delete m_tempRange;
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
  m_tempUnitSuffix = new QHash <TemperatureUnits, QString>;
  m_tempUnitSuffix->insert(Fahrenheit, " F");
  m_tempUnitSuffix->insert(Celsius, " C");

  m_tempRange = new QHash <TemperatureUnits, QPair <int, int> >;

  m_tempRange->insert(Fahrenheit, qMakePair(-40, 280));
  m_tempRange->insert(Celsius, qMakePair(-40, 140));

  m_tempLimits = new QHash <TemperatureUnits, QPair <int, int> >;

  m_tempLimits->insert(Fahrenheit, qMakePair(180, 210));
  m_tempLimits->insert(Celsius, qMakePair(80, 98));
}

/**
 * Instantiates widgets used in the main window.
 */
void MainWindow::setupWidgets()
{
  // set menu and button icons
  m_ui->m_exitAction->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
  m_ui->m_editSettingsAction->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
  m_ui->m_helpContentsAction->setIcon(style()->standardIcon(QStyle::SP_DialogHelpButton));
  m_ui->m_helpAboutAction->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
  m_ui->m_startLoggingButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
  m_ui->m_stopLoggingButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));

  // connect menu item signals
  connect(m_ui->m_exitAction, SIGNAL(triggered()), this, SLOT(onExitSelected()));
  connect(m_ui->m_editSettingsAction, SIGNAL(triggered()), this, SLOT(onEditOptionsClicked()));
  connect(m_ui->m_helpContentsAction, SIGNAL(triggered()), this, SLOT(onHelpContentsClicked()));
  connect(m_ui->m_helpAboutAction, SIGNAL(triggered()), this, SLOT(onHelpAboutClicked()));

  // connect button signals
  connect(m_ui->m_connectButton, SIGNAL(clicked()), this, SLOT(onConnectClicked()));
  connect(m_ui->m_disconnectButton, SIGNAL(clicked()), this, SLOT(onDisconnectClicked()));
  connect(m_ui->m_startLoggingButton, SIGNAL(clicked()), this, SLOT(onStartLogging()));
  connect(m_ui->m_stopLoggingButton, SIGNAL(clicked()), this, SLOT(onStopLogging()));
  connect(m_ui->m_clearFaultsButton, SIGNAL(clicked()), m_mems, SLOT(onFaultCodesClearRequested()));
  connect(m_ui->m_testACRelayButton, SIGNAL(clicked()), this, SLOT(onTestACRelayClicked()));
  connect(m_ui->m_testFuelPumpRelayButton, SIGNAL(clicked()), this, SLOT(onTestFuelPumpRelayClicked()));
  connect(m_ui->m_testPTCRelayButton, SIGNAL(clicked()), this, SLOT(onTestPTCRelayClicked()));
  connect(m_ui->m_testIgnitionCoilButton, SIGNAL(clicked()), m_mems, SLOT(onIgnitionCoilTest()));
  connect(m_ui->m_testFuelInjectorButton, SIGNAL(clicked()), m_mems, SLOT(onFuelInjectorTest()));
  connect(m_ui->m_moveIACButton, SIGNAL(clicked()), this, SLOT(onMoveIACClicked()));

  // set the LED colors
  m_ui->m_commsGoodLed->setOnColor1(QColor(102, 255, 102));
  m_ui->m_commsGoodLed->setOnColor2(QColor(82, 204, 82));
  m_ui->m_commsGoodLed->setOffColor1(QColor(0, 102, 0));
  m_ui->m_commsGoodLed->setOffColor2(QColor(0, 51, 0));
  m_ui->m_commsBadLed->setOnColor1(QColor(255, 0, 0));
  m_ui->m_commsBadLed->setOnColor2(QColor(176, 0, 2));
  m_ui->m_commsBadLed->setOffColor1(QColor(20, 0, 0));
  m_ui->m_commsBadLed->setOffColor2(QColor(90, 0, 2));

  m_ui->m_idleSwitchLed->setOnColor1(QColor(102, 255, 102));
  m_ui->m_idleSwitchLed->setOnColor2(QColor(82, 204, 82));
  m_ui->m_idleSwitchLed->setOffColor1(QColor(0, 102, 0));
  m_ui->m_idleSwitchLed->setOffColor2(QColor(0, 51, 0));
  m_ui->m_neutralSwitchLed->setOnColor1(QColor(102, 255, 102));
  m_ui->m_neutralSwitchLed->setOnColor2(QColor(82, 204, 82));
  m_ui->m_neutralSwitchLed->setOffColor1(QColor(0, 102, 0));
  m_ui->m_neutralSwitchLed->setOffColor2(QColor(0, 51, 0));

  m_ui->m_faultLedATS->setOnColor1(QColor(255, 0, 0));
  m_ui->m_faultLedATS->setOnColor2(QColor(176, 0, 2));
  m_ui->m_faultLedATS->setOffColor1(QColor(20, 0, 0));
  m_ui->m_faultLedATS->setOffColor2(QColor(90, 0, 2));
  m_ui->m_faultLedCTS->setOnColor1(QColor(255, 0, 0));
  m_ui->m_faultLedCTS->setOnColor2(QColor(176, 0, 2));
  m_ui->m_faultLedCTS->setOffColor1(QColor(20, 0, 0));
  m_ui->m_faultLedCTS->setOffColor2(QColor(90, 0, 2));
  m_ui->m_faultLedFuelPump->setOnColor1(QColor(255, 0, 0));
  m_ui->m_faultLedFuelPump->setOnColor2(QColor(176, 0, 2));
  m_ui->m_faultLedFuelPump->setOffColor1(QColor(20, 0, 0));
  m_ui->m_faultLedFuelPump->setOffColor2(QColor(90, 0, 2));
  m_ui->m_faultLedTps->setOnColor1(QColor(255, 0, 0));
  m_ui->m_faultLedTps->setOnColor2(QColor(176, 0, 2));
  m_ui->m_faultLedTps->setOffColor1(QColor(20, 0, 0));
  m_ui->m_faultLedTps->setOffColor2(QColor(90, 0, 2));

  m_ui->m_logFileNameBox->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss"));

  m_ui->m_mapGauge->setMinimum(0.0);
  m_ui->m_mapGauge->setMaximum(140.0);
  m_ui->m_mapGauge->setSuffix("kPa");
  m_ui->m_mapGauge->setNominal(1000.0);
  m_ui->m_mapGauge->setCritical(1000.0);

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
 * Attempts to open the serial device connected to the ECU,
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

void MainWindow::onInterfaceThreadReady()
{
  emit requestToStartPolling();
}

void MainWindow::onEcuIdReceived(uint8_t* id)
{
  char idString[20];

  sprintf(idString, "ECU ID: %02X %02X %02X %02X", id[0], id[1], id[2], id[3]);
  m_ui->m_ecuIdLabel->setText(QString(idString));
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
 * Converts temperature in Fahrenheit degrees to the desired units.
 * @param tempF Temperature in Fahrenheit degrees
 * @return Temperature in the desired units
 */
int MainWindow::convertTemperature(int tempF)
{
  double temp = tempF;

  switch (m_options->getTemperatureUnits())
  {
  case Celsius:
    temp = (temp - 32) * (0.5555556);
    break;
  case Fahrenheit:
  default:
    break;
  }

  return (int)temp;
}

void MainWindow::setActuatorTestsEnabled(bool enabled)
{
  m_ui->m_testACRelayButton->setEnabled(enabled);
  m_ui->m_testFuelInjectorButton->setEnabled(enabled);
  m_ui->m_testFuelPumpRelayButton->setEnabled(enabled);
  m_ui->m_moveIACButton->setEnabled(enabled);
  m_ui->m_testIgnitionCoilButton->setEnabled(enabled);
  m_ui->m_testPTCRelayButton->setEnabled(enabled);
  m_actuatorTestsEnabled = enabled;
}

/**
 * Updates the gauges and indicators with the latest data available from
 * the ECU.
 */
void MainWindow::onDataReady()
{
  mems_data* data = m_mems->getData();
  int corrected_iac = (data->iac_position > IAC_MAXIMUM) ? IAC_MAXIMUM : data->iac_position;
  float corrected_throttle = (data->throttle_pot_voltage > 5.0) ? 5.0 : data->throttle_pot_voltage;

  if ((data->engine_rpm == 0) && !m_actuatorTestsEnabled)
  {
    setActuatorTestsEnabled(true);
  }
  else if ((data->engine_rpm > 0) && m_actuatorTestsEnabled)
  {
    setActuatorTestsEnabled(false);
  }

  m_ui->m_throttleBar->setValue(corrected_throttle / 5.00 * 100);
  m_ui->m_throttlePotVolts->setText(QString::number(data->throttle_pot_voltage, 'f', 2) + "V");
  m_ui->m_idleBypassPosBar->setValue((float)corrected_iac / (float)IAC_MAXIMUM * 100);
  m_ui->m_iacPositionSteps->setText(QString::number(data->iac_position));
  m_ui->m_revCounter->setValue(data->engine_rpm);
  m_ui->m_mapGauge->setValue(data->map_kpa);
  m_ui->m_waterTempGauge->setValue(convertTemperature(data->coolant_temp_f));
  m_ui->m_airTempGauge->setValue(convertTemperature(data->intake_air_temp_f));
  m_ui->m_voltage->setText(QString::number(data->battery_voltage, 'f', 1) + "V");

  m_ui->m_faultLedCTS->setChecked((data->fault_codes & 0x01) != 0);
  m_ui->m_faultLedATS->setChecked((data->fault_codes & 0x02) != 0);
  m_ui->m_faultLedFuelPump->setChecked((data->fault_codes & 0x04) != 0);
  m_ui->m_faultLedTps->setChecked((data->fault_codes & 0x08) != 0);

  m_ui->m_idleSwitchLed->setChecked(data->idle_switch);
  m_ui->m_neutralSwitchLed->setChecked(data->park_neutral_switch);

  m_logger->logData();
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
    TemperatureUnits tempUnits = m_options->getTemperatureUnits();
    QString tempUnitStr = m_tempUnitSuffix->value(tempUnits);

    int tempMin = m_tempRange->value(tempUnits).first;
    int tempMax = m_tempRange->value(tempUnits).second;
    int tempNominal = m_tempLimits->value(tempUnits).first;
    int tempCritical = m_tempLimits->value(tempUnits).second;

    m_logger->setTemperatureUnits(tempUnits);

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
    // interval, stop the timer, re-connect to the ECU (if neccessary),
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
void MainWindow::closeEvent(QCloseEvent* event)
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

  m_ui->m_clearFaultsButton->setEnabled(true);
}

/**
 * Reponds to the "disconnect" signal from the MEMSInterface by enabling/disabling
 * the appropriate buttons and setting a message in the status bar.
 */
void MainWindow::onDisconnect()
{
  m_ui->m_connectButton->setEnabled(true);
  m_ui->m_disconnectButton->setEnabled(false);
  m_ui->m_commsGoodLed->setChecked(false);
  m_ui->m_commsBadLed->setChecked(false);
  m_ui->m_ecuIdLabel->setText("ECU ID:");

  m_ui->m_mapGauge->setValue(0.0);
  m_ui->m_revCounter->setValue(0.0);
  m_ui->m_waterTempGauge->setValue(m_ui->m_waterTempGauge->minimum());
  m_ui->m_airTempGauge->setValue(m_ui->m_airTempGauge->minimum());
  m_ui->m_throttleBar->setValue(0);
  m_ui->m_idleBypassPosBar->setValue(0);
  m_ui->m_idleSwitchLed->setChecked(false);
  m_ui->m_neutralSwitchLed->setChecked(false);
  m_ui->m_throttlePotVolts->setText("0.00V");
  m_ui->m_iacPositionSteps->setText("0");
  m_ui->m_voltage->setText("0.0V");
  m_ui->m_faultLedCTS->setChecked(false);
  m_ui->m_faultLedATS->setChecked(false);
  m_ui->m_faultLedFuelPump->setChecked(false);
  m_ui->m_faultLedTps->setChecked(false);

  setActuatorTestsEnabled(false);

  m_ui->m_clearFaultsButton->setEnabled(false);
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
    QMessageBox::warning(this, "Error", "Failed to open log file (" + m_logger->getLogPath() + ")", QMessageBox::Ok);
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
                         QString("Error connecting to ECU. No serial port name specified.\n\n") +
                         QString("Set a serial device by selecting \"Edit Settings\" from the \"Options\" menu."),
                         QMessageBox::Ok);
  }
  else
  {
    QMessageBox::warning(this, "Error",
                         "Error connecting to ECU on port " + dev + ".\nCheck cable wiring and check that ECU is on.",
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

void MainWindow::onFuelPumpTestComplete()
{
  m_ui->m_testFuelPumpRelayButton->setEnabled(true);
}

void MainWindow::onACRelayTestComplete()
{
  m_ui->m_testACRelayButton->setEnabled(true);
}

void MainWindow::onPTCRelayTestComplete()
{
  m_ui->m_testPTCRelayButton->setEnabled(true);
}

void MainWindow::onTestFuelPumpRelayClicked()
{
  m_ui->m_testFuelPumpRelayButton->setEnabled(false);
  emit fuelPumpTest();
}

void MainWindow::onTestACRelayClicked()
{
  m_ui->m_testACRelayButton->setEnabled(false);
  emit acRelayTest();
}

void MainWindow::onTestPTCRelayClicked()
{
  m_ui->m_testPTCRelayButton->setEnabled(false);
  emit ptcRelayTest();
}

void MainWindow::onMoveIACClicked()
{
  m_ui->m_moveIACButton->setEnabled(false);
  QApplication::setOverrideCursor(Qt::WaitCursor);
  float percent = (float)m_ui->m_iacPositionSlider->value() * 25.0 / 100.0;
  int desiredPos = (int)((float)IAC_MAXIMUM * percent);
  emit moveIAC(desiredPos);
}

void MainWindow::onMoveIACComplete()
{
  QApplication::restoreOverrideCursor();
  m_ui->m_moveIACButton->setEnabled(true);
}

void MainWindow::onCommandError()
{
  QMessageBox::warning(this, "Error", "Error sending command.", QMessageBox::Ok);
}

void MainWindow::onFaultCodeClearComplete()
{
  QMessageBox::information(this, "Complete", "Successfully cleared fault codes.", QMessageBox::Ok);
}
