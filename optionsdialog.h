#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QString>
#include <QHash>
#include <QFrame>
#include "commonunits.h"

class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    OptionsDialog(QString title, QWidget *parent = 0);
    QString getSerialDeviceName();
    bool getSerialDeviceChanged() { return m_serialDeviceChanged; }
    TemperatureUnits getTemperatureUnits() { return m_tempUnits; }

protected:
    void accept();

private:
    QGridLayout *m_grid;
    QLabel *m_serialDeviceLabel;
    QComboBox *m_serialDeviceBox;

    QLabel *m_temperatureUnitsLabel;
    QComboBox *m_temperatureUnitsBox;

    QFrame *m_horizontalLineA;

    QCheckBox *m_refreshFuelMapCheckbox;

    QPushButton *m_okButton;
    QPushButton *m_cancelButton;

    QString m_serialDeviceName;
    TemperatureUnits m_tempUnits;

    bool m_serialDeviceChanged;

    const QString m_settingsFileName;
    const QString m_settingsGroupName;

    const QString m_settingSerialDev;
    const QString m_settingTemperatureUnits;

    void setupWidgets();
    void readSettings();
    void writeSettings();
};

#endif // OPTIONSDIALOG_H

