#include "AppSettings.hpp"

#include "ui_AppSettings.h"


AppSettings::AppSettings(QWidget* parent)
    : QDialog(parent, Qt::Window)
    , ui(new Ui::AppSettings)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("Application settings"));

    // https://stackoverflow.com/questions/5284147/validating-ipv4-addresses-with-regexp
    const QString ipValidationString("((25[0-5]|(2[0-4]|1\\d|[1-9]|)\\d)\\.){3}(25[0-5]|(2[0-4]|1\\d|[1-9]|)\\d)");
    QRegularExpression ipValidationRegexp(ipValidationString, QRegularExpression::DontCaptureOption);
    ui->lineEdit_ipLocal->setValidator(new QRegularExpressionValidator(ipValidationRegexp, ui->lineEdit_ipLocal));
    ui->lineEdit_ipDestination->setValidator(new QRegularExpressionValidator(ipValidationRegexp, ui->lineEdit_ipDestination));

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AppSettings::onAccept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &AppSettings::onReject);
}

AppSettings::~AppSettings() {}

void AppSettings::onAccept()
{
    m_netSettings.ipLocal       = QHostAddress(ui->lineEdit_ipLocal->text());
    m_netSettings.ipDestination = QHostAddress(ui->lineEdit_ipDestination->text());
    m_netSettings.portIn        = ui->spinBox_portIn->value();
    m_netSettings.portOut       = ui->spinBox_portOut->value();
    QDialog::accept();
}

void AppSettings::onReject()
{
    QDialog::reject();
}

void AppSettings::onReset()
{
    init(m_netSettings);
}

void AppSettings::init(Net::ConnectionSettings a_netSettings)
{
    m_netSettings = a_netSettings;
    ui->lineEdit_ipLocal->setText(m_netSettings.ipLocal.toString());
    ui->lineEdit_ipDestination->setText(m_netSettings.ipDestination.toString());
    ui->spinBox_portIn->setValue(m_netSettings.portIn);
    ui->spinBox_portOut->setValue(m_netSettings.portOut);
}
