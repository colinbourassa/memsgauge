#include <QSettings>
#include "optionsdialog.h"
#include "serialdevenumerator.h"

/**
 * Constructor; sets up the options-dialog UI and sets settings-file field names.
 */
OptionsDialog::OptionsDialog(QString title, QWidget * parent):QDialog(parent),
m_serialDeviceChanged(false),
m_settingsGroupName("Settings"), m_settingSerialDev("SerialDevice"), m_settingTemperatureUnits("TemperatureUnits")
{
  this->setWindowTitle(title);
  readSettings();
  setupWidgets();
}

/**
 * Instantiates widgets, connects to their signals, and places them on the form.
 */
void OptionsDialog::setupWidgets()
{
  unsigned int row = 0;

  m_grid = new QGridLayout(this);

  m_serialDeviceLabel = new QLabel("Serial device name:", this);
  m_serialDeviceBox = new QComboBox(this);

  m_temperatureUnitsLabel = new QLabel("Temperature units:", this);
  m_temperatureUnitsBox = new QComboBox(this);

  m_horizontalLineA = new QFrame(this);
  m_horizontalLineA->setFrameShape(QFrame::HLine);
  m_horizontalLineA->setFrameShadow(QFrame::Sunken);

  m_okButton = new QPushButton("OK", this);
  m_cancelButton = new QPushButton("Cancel", this);

  SerialDevEnumerator serialDevs;

  m_serialDeviceBox->addItems(serialDevs.getSerialDevList(m_serialDeviceName));
  m_serialDeviceBox->setEditable(true);
  m_serialDeviceBox->setMinimumWidth(150);

  m_temperatureUnitsBox->setEditable(false);
  m_temperatureUnitsBox->addItem("Fahrenheit");
  m_temperatureUnitsBox->addItem("Celsius");
  m_temperatureUnitsBox->setCurrentIndex((int)m_tempUnits);

  m_grid->addWidget(m_serialDeviceLabel, row, 0);
  m_grid->addWidget(m_serialDeviceBox, row++, 1);

  m_grid->addWidget(m_temperatureUnitsLabel, row, 0);
  m_grid->addWidget(m_temperatureUnitsBox, row++, 1);

  m_grid->addWidget(m_horizontalLineA, row++, 0, 1, 2);

  m_grid->addWidget(m_okButton, row, 0);
  m_grid->addWidget(m_cancelButton, row++, 1);

  connect(m_okButton, SIGNAL(clicked()), this, SLOT(accept()));
  connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

/**
 * Reads the new settings from the form controls.
 */
void OptionsDialog::accept()
{
  QString newSerialDeviceName = m_serialDeviceBox->currentText();

  // set a flag if the serial device has been changed;
  // the main application needs to know if it should
  // reconnect to the ECU
  if (m_serialDeviceName.compare(newSerialDeviceName) != 0)
  {
    m_serialDeviceName = newSerialDeviceName;
    m_serialDeviceChanged = true;
  }
  else
  {
    m_serialDeviceChanged = false;
  }

  m_tempUnits = (TemperatureUnits) (m_temperatureUnitsBox->currentIndex());

  writeSettings();
  done(QDialog::Accepted);
}

/**
 * Reads values for all the settings (either from the settings file, or by return defaults.)
 */
void OptionsDialog::readSettings()
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope, PROJECTNAME);

  settings.beginGroup(m_settingsGroupName);
  m_serialDeviceName = settings.value(m_settingSerialDev, "").toString();
  m_tempUnits = (TemperatureUnits) (settings.value(m_settingTemperatureUnits, Fahrenheit).toInt());

  settings.endGroup();
}

/**
 * Writes settings out to a file on disk.
 */
void OptionsDialog::writeSettings()
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope, PROJECTNAME);

  settings.beginGroup(m_settingsGroupName);
  settings.setValue(m_settingSerialDev, m_serialDeviceName);
  settings.setValue(m_settingTemperatureUnits, m_tempUnits);

  settings.endGroup();
}

/**
 * Returns the name of the serial device.
 */
QString OptionsDialog::getSerialDeviceName()
{
#ifdef WIN32
  return QString("\\\\.\\%1").arg(m_serialDeviceName);
#else
  return m_serialDeviceName;
#endif
}
