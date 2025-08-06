#include "MainWindow.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <random>

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCore/QSettings>
#include <QtWidgets/QMessageBox>

#include "Common/Protocol.hpp"
#include "Net/NetHeaders.hpp"

#include "./ui_MainWindow.h"
#include "AppSettings.hpp"

using namespace std;
using namespace Net;
using namespace Protocol;

QT_CHARTS_USE_NAMESPACE

enum class TaskIndex
{
    ArraySorting = 0,
    PrimeNumbers,
    FunctionGraph,
};

const QString g_settingsPath{"./ClientSettings.ini"};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->comboBox_funcGraph_equation->addItem("Linear", QVariant::fromValue(EquationType::Linear));
    ui->comboBox_funcGraph_equation->addItem("Quadratic", QVariant::fromValue(EquationType::Quadratic));

    using namespace placeholders;
    m_client = std::get<0>(Net::instantiateWaitThreadedConnection<TcpClient>());
    // All GUI stuff must be done in GUI thread, so we either split response parsing and rendering, or do everything in one function in GUI thread
    m_client->setCallbackFunction([this](QByteArray arg1, NetConnection* const arg2, Net::AddressPort arg3) {
        QMetaObject::invokeMethod(this, [=](){
            parseResponse(arg1, arg2, arg3);
        }, Qt::QueuedConnection);
    });
    m_client->setAuthorizationEnabled(true);
    m_client->setLoginData(Net::LoginData{QStringLiteral("Chuck"), QStringLiteral("Norris")});

    loadSettings();

    connect(m_client, &NetConnection::socketStateChanged, this, &MainWindow::onSocketStateChanged);

    connect(ui->action_settings, &QAction::triggered, this, &MainWindow::onAppSettings);

    connect(ui->pushButton_arraySorting_generate, &QPushButton::pressed, this, &MainWindow::onGenerateArray);

    connect(ui->pushButton_sendRequest, &QPushButton::pressed, this, &MainWindow::onSendRequest);
    connect(ui->comboBox_funcGraph_equation, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::onEquationTypeChanged);

    QMetaObject::invokeMethod(this, [this](){
        Net::reopenWaitThreadedConnection(m_client, m_client->getConnectionSettings());
    }, Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
    QSettings settingsFile(g_settingsPath, QSettings::IniFormat, this);
    settingsFile.beginGroup("Geometry");
    auto geo = this->geometry();
    settingsFile.setValue("x", geo.x());
    settingsFile.setValue("y", geo.y());
    settingsFile.setValue("width", geo.width());
    settingsFile.setValue("height", geo.height());
    settingsFile.endGroup();

    settingsFile.beginGroup("Network");
    auto ns = m_client->getConnectionSettings();
    settingsFile.setValue("ipLocal", ns.ipLocal.toString());
    settingsFile.setValue("ipDestination", ns.ipDestination.toString());
    settingsFile.setValue("portIn", ns.portIn);
    settingsFile.setValue("portOut", ns.portOut);
    settingsFile.endGroup();

    delete ui;
}

void MainWindow::loadSettings()
{
    QSettings settingsFile(g_settingsPath, QSettings::IniFormat, this);

    settingsFile.beginGroup("Geometry");
    int x = settingsFile.value("x").toInt();
    int y = settingsFile.value("y").toInt();
    int width = settingsFile.value("width").toInt();
    int height = settingsFile.value("height").toInt();
    this->setGeometry(x, y, width, height);
    settingsFile.endGroup();

    settingsFile.beginGroup("Network");
    Net::ConnectionSettings ns;
    ns.ipLocal = QHostAddress(settingsFile.value("ipLocal").toString());
    ns.ipDestination = QHostAddress(settingsFile.value("ipDestination").toString());
    ns.portIn = settingsFile.value("portIn").toUInt();
    ns.portOut = settingsFile.value("portOut").toUInt();
    m_client->setConnectionSettings(ns);
    settingsFile.endGroup();
}


// TODO: shouldn't QMainWindow get deleted anyway without it?
void MainWindow::closeEvent(QCloseEvent* event)
{
    this->deleteLater();
    QMainWindow::closeEvent(event);
}

void MainWindow::onAppSettings()
{
    // TODO: stop whatever we're doing right now if we're doing anything?
    AppSettings asw(this);
    asw.init(m_client->getConnectionSettings());
    auto ret = asw.exec();
    if (ret == QDialog::Accepted)
    {
        m_client->reopenConnectionQueued(asw.m_netSettings);
    }
}

void MainWindow::onSendRequest()
{
    if (m_client->getSocketState() != QAbstractSocket::ConnectedState)
    {
        QMessageBox::critical(this, tr("Error"), QObject::tr("No connection to server"));
        return;
    }

    unique_ptr<Request> request;
    switch (static_cast<TaskIndex>(ui->tabWidget_tasks->currentIndex()))
    {
    case TaskIndex::ArraySorting:
    {
        QVector<int> targetArray;
        QString text = ui->plainTextEdit_arraySorting_initialData->document()->toPlainText();
        QStringList textList = text.split(' ', Qt::SkipEmptyParts);
        bool isTextInvalid = false;
        for (QString const& s : textList)
        {
            bool isOk;
            int val = s.toInt(&isOk, 10);
            if (isOk)
                targetArray.push_back(val);
            else
                isTextInvalid = true;
        }
        if (isTextInvalid)
        {
            QString fixedText;
            fixedText.reserve(text.length());
            for (auto item : targetArray)
                fixedText.append(QString::number(item) + ' ');
            fixedText.chop(1);
            ui->plainTextEdit_arraySorting_initialData->setPlainText(fixedText);
        }

        request = make_unique<Request_SortArray>();
        auto req = static_cast<Request_SortArray*>(request.get());
        req->numbers = std::move(targetArray);
        break;
    }
    case TaskIndex::PrimeNumbers:
    {
        request = make_unique<Request_PrimeNumbers>();
        auto req = static_cast<Request_PrimeNumbers*>(request.get());
        req->x_from = ui->spinBox_primeNumbers_from->value();
        req->x_to = ui->spinBox_primeNumbers_to->value();
        break;
    }
    case TaskIndex::FunctionGraph:
    {
        // TODO: set minimum number of points? Validate input?
        request = make_unique<Request_CalculateFunction>();
        auto req = static_cast<Request_CalculateFunction*>(request.get());
        req->equationType = ui->comboBox_funcGraph_equation->currentData(Qt::UserRole).value<EquationType>();
        req->x_from = ui->spinBox_funcGraph_from->value();
        req->x_to = ui->spinBox_funcGraph_to->value();
        req->x_step = ui->spinBox_funcGraph_step->value();
        req->a = ui->spinBox_funcGraph_constA->value();
        req->b = ui->spinBox_funcGraph_constB->value();
        req->c = ui->spinBox_funcGraph_constC->value();
        break;
    }
    default:
    {
        return;
    }
    }

    QByteArray msg;
    MAKE_QDATASTREAM_NET(stream, &msg, QIODevice::ReadWrite);
    stream << *request;
    m_client->sendMessageQueued(msg);
    // send request
    // block ui?
    // start progress dialog with early cancel button
}

void MainWindow::onEquationTypeChanged()
{
    ui->label_funcGraph_equation->setText(toQString(ui->comboBox_funcGraph_equation->currentData(Qt::UserRole).value<EquationType>()));
}

void MainWindow::onGenerateArray()
{
    static std::mt19937_64 gen{static_cast<std::mt19937_64::result_type>(std::chrono::system_clock::now().time_since_epoch().count())};
    auto x_from = ui->spinBox_arraySorting_from->value();
    auto x_to = ui->spinBox_arraySorting_to->value();
    auto x_count = ui->spinBox_arraySorting_count->value();
    std::uniform_int_distribution<> dist(x_from, x_to);
    QString text;
    text.reserve(x_count * sizeof(x_count));
    for (int i = 0; i < x_count; ++i)
    {
        auto num = dist(gen);
        text.append(QString::number(num) + ' ');
    }
    text.chop(1);
    ui->plainTextEdit_arraySorting_initialData->setPlainText(text);
}

void MainWindow::parseResponse(QByteArray msg, NetConnection* const, Net::AddressPort addrPort)
{
    Request request;
    MAKE_QDATASTREAM_NET(streamMsg, &msg, QIODevice::ReadOnly);
    streamMsg >> request;
    if (streamMsg.status() != QDataStream::Ok)
    {

        return;
    }

    switch (request.type)
    {
    case RequestType::SortArray:
    {
        Request_SortArray req;
        streamMsg.device()->seek(0);
        streamMsg >> req;

        QString text;
        for (auto item : req.numbers)
            text.append(QString::number(item) + ' ');
        text.chop(1);
        ui->plainTextEdit_arraySorting_resultData->setPlainText(text);
        break;
    }
    case RequestType::FindPrimeNumbers:
    {
        Request_PrimeNumbers req;
        streamMsg.device()->seek(0);
        streamMsg >> req;

        QString text;
        for (auto item : req.primeNumbers)
            text.append(QString::number(item) + ' ');
        text.chop(1);
        ui->plainTextEdit_primeNumbers_resultData->setPlainText(text);
        break;
    }
    case RequestType::CalculateFunction:
    {        
        Request_CalculateFunction req;
        streamMsg.device()->seek(0);
        streamMsg >> req;

        QLineSeries* series = new QLineSeries();
        for (auto item : qAsConst(req.points)) // normally, would just use QList<QPointF> instead of QVector<int>
            series->append(QPointF(item));

        QChart* chart = new QChart();
        chart->addSeries(series);
        chart->setTitle(QStringLiteral("%1 function chart").arg(toQString(req.equationType)));
        chart->createDefaultAxes();
        chart->legend()->setVisible(true);
        chart->legend()->setAlignment(Qt::AlignBottom);
        chart->setAnimationOptions(QChart::NoAnimation);

        ui->chartView_funcGraph_chart->setChart(chart);
        ui->chartView_funcGraph_chart->setRenderHint(QPainter::Antialiasing);
        break;
    }
    default:
    {
        return;
    }
    }
}

void MainWindow::indicateLastUpdated()
{
    ui->label_lastUpdatedDatetime->setText(QDateTime::currentDateTimeUtc().toString(m_datetimeFormat));
}

void MainWindow::indicateConnectionState(const QAbstractSocket::SocketState& socketState)
{
    // &#11044; // Black Large Circle // ⬤
    QString indicatorText("<html><head/><body><span style=\"color:%1;\"><p> &#11044; </p></span></body></html>");
    QString connectionText;
    switch (socketState)
    {
    case QAbstractSocket::UnconnectedState:
    {
        indicatorText = indicatorText.arg("red");
        connectionText = QStringLiteral("Unconnected");
        break;
    }
    case QAbstractSocket::HostLookupState:  // [[fallthrough]]
    case QAbstractSocket::ConnectingState:  // [[fallthrough]]
    case QAbstractSocket::ClosingState:
    {
        connectionText = QStringLiteral("Connecting...");
        indicatorText = indicatorText.arg("yellow");
        break;
    }
    case QAbstractSocket::ListeningState:  // [[fallthrough]]
    case QAbstractSocket::ConnectedState:  // [[fallthrough]]
    case QAbstractSocket::BoundState:
    {
        connectionText = QStringLiteral("Connected");
        indicatorText = indicatorText.arg("#11ff11");
        break;
    }
    default:
    {
        connectionText = QStringLiteral("Unconnected");
        indicatorText = indicatorText.arg("black");
        break;
    }
    }
    ui->label_connectionIndicator->setText(indicatorText);
    ui->label_connectionText->setText(connectionText);
}

void MainWindow::onSocketStateChanged(const QAbstractSocket::SocketState& socketState)
{
    indicateConnectionState(socketState);
    indicateLastUpdated();
}
