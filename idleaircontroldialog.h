#ifndef IDLEAIRCONTROLDIALOG_H
#define IDLEAIRCONTROLDIALOG_H

#include <QString>
#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QRadioButton>
#include <QPushButton>
#include "memsinterface.h"

class IdleAirControlDialog : public QDialog
{
    Q_OBJECT
public:
    explicit IdleAirControlDialog(QString title, QWidget *parent = 0);

signals:
    void requestIdleAirControlMovement(int direction);

private:
    QGridLayout *m_iacGrid;
    QSpinBox *m_stepCountBox;
    QLabel *m_stepCountLabel;
    QRadioButton *m_closeValveButton;
    QRadioButton *m_openValveButton;
    QPushButton *m_sendCommandButton;
    QLabel *m_noteLabel;
    QPushButton *m_closeButton;

private slots:
    void onSendCommand();

};

#endif // IDLEAIRCONTROLDIALOG_H
