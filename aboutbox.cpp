#include "aboutbox.h"

/**
 * Constructor. Receives a pointer to the QStyle of the parent form.
 */
AboutBox::AboutBox(QStyle * parentStyle, QString title, librosco_version version, QWidget * parent):
QDialog(parent),
m_urlString(QString("https://github.com/colinbourassa/memsgauge")),
m_urlLibString(QString("https://github.com/colinbourassa/librosco")),
m_aboutString(QString("A graphical interface to the Rover Modular Engine Management System 1.6."))
{
  this->setWindowTitle(title);
  m_ver.major = version.major;
  m_ver.minor = version.minor;
  m_ver.patch = version.patch;
  m_style = parentStyle;
  setupWidgets();
}

/**
 * Builds a dot-separated string with the version number.
 * @maj Major version number
 * @min Minor version number
 * @patch Patch version number
 * @return Dot-separated version string
 */
QString AboutBox::makeVersionString(int maj, int min, int patch)
{
  return (QString::number(maj) + "." + QString::number(min) + "." + QString::number(patch));
}

/**
 * Creates widgets and places them on the form.
 */
void AboutBox::setupWidgets()
{
  m_grid = new QGridLayout();
  this->setLayout(m_grid);

  m_iconLabel = new QLabel(this);
  m_iconLabel->setPixmap(m_style->standardIcon(QStyle::SP_MessageBoxInformation).pixmap(32, 32));

  m_name = new QLabel(PROJECTNAME + QString(" ") + makeVersionString(VER_MAJOR, VER_MINOR, VER_PATCH), this);
  QFont defaultFont = m_name->font();

  defaultFont.setPointSize(14);
  m_name->setFont(defaultFont);

  m_info = new QLabel(m_aboutString, this);
  m_infoLib =
    new QLabel(QString("Using librosco %1.").arg(makeVersionString(m_ver.major, m_ver.minor, m_ver.patch)), this);

  m_url = new QLabel("<a href=\"" + m_urlString + "\">" + m_urlString + "</a>", this);
  m_url->setOpenExternalLinks(true);

  m_urlLib = new QLabel("<a href=\"" + m_urlLibString + "\">" + m_urlLibString + "</a>", this);
  m_urlLib->setOpenExternalLinks(true);

  m_ok = new QPushButton("Close", this);
  connect(m_ok, SIGNAL(clicked()), SLOT(accept()));

  m_grid->addWidget(m_iconLabel, 0, 0, 1, 1, Qt::AlignCenter);
  m_grid->addWidget(m_name, 0, 1, 1, 1, Qt::AlignLeft);
  m_grid->addWidget(m_info, 1, 1, 1, 1, Qt::AlignLeft);
  m_grid->addWidget(m_url, 2, 1, 1, 1, Qt::AlignLeft);
  m_grid->addWidget(m_infoLib, 3, 1, 1, 1, Qt::AlignLeft);
  m_grid->addWidget(m_urlLib, 4, 1, 1, 1, Qt::AlignLeft);
  m_grid->addWidget(m_ok, 5, 1, 1, 1, Qt::AlignRight);
}
