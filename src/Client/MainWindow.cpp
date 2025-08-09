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
#include "Common/RegLogger.hpp"
#include "Common/Utils.hpp"
#include "Net/NetHeaders.hpp"

#include "./ui_MainWindow.h"
#include "AppSettings.hpp"

using namespace std;
using namespace Net;
using namespace Protocol;

QT_CHARTS_USE_NAMESPACE

// should correspond to tabWidget index in ui
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
    RegLoggerThreadWorker::instantiateRegLogger(qApp->applicationDirPath());
    m_regId_general = RegLogger::instance().addFile("log_client");

    auto log_lambda = [this](QString msg){
        RegLogger::instance().logData(this->m_regId_general, msg);
        qWarning(qUtf8Printable(QDateTime::currentDateTimeUtc().toString("[yyyy.MM.dd-hh:mm:ss.zzz]") + msg));
    };
    f_logGeneral = log_lambda;
    f_logError = f_logGeneral;

    ui->setupUi(this);

    // setup QChartView
    {
        auto series = new QLineSeries;
        auto chart = new QChart;
        chart->addSeries(series);
        auto axisX = new QValueAxis(chart);
        auto axisY = new QValueAxis(chart);
        chart->addAxis(axisX, Qt::AlignBottom);
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisX);
        series->attachAxis(axisY);
        // Connect series updates to auto-scaling slots
        connect(series, &QXYSeries::pointsReplaced, this, [axisX, axisY, series] {
            auto pts = series->pointsVector();
            if (pts.isEmpty())
                return;
            auto [minX, maxX] = std::minmax_element(pts.begin(), pts.end(), [](auto& a, auto& b) { return a.x() < b.x(); });
            auto [minY, maxY] = std::minmax_element(pts.begin(), pts.end(), [](auto& a, auto& b) { return a.y() < b.y(); });
            axisX->setRange(minX->x(), maxX->x());
            axisY->setRange(minY->y(), maxY->y());
        });
        chart->legend()->setVisible(true);
        chart->legend()->setAlignment(Qt::AlignBottom);
        chart->setAnimationOptions(QChart::NoAnimation);
        ui->chartView_funcGraph_chart->setChart(chart);
        ui->chartView_funcGraph_chart->setRenderHint(QPainter::Antialiasing);
    }

    // setup progress dialog
    {
        m_progressDialog = new PersistentProgressDialog(this);
        QObject::connect(m_progressDialog, &QProgressDialog::canceled, this, &MainWindow::onSendCancel);
    }

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
    m_client->setLoggingFunctions(f_logGeneral, f_logError);

    loadSettings();

    connect(m_client, &NetConnection::socketStateChanged, this, &MainWindow::onSocketStateChanged);

    connect(ui->action_settings, &QAction::triggered, this, &MainWindow::onAppSettings);

    connect(ui->pushButton_arraySorting_generate, &QPushButton::pressed, this, &MainWindow::onGenerateArray);

    connect(ui->pushButton_sendRequest, &QPushButton::pressed, this, &MainWindow::onSendRequest);
    connect(ui->pushButton_clear, &QPushButton::pressed, this, &MainWindow::onClear);
    connect(ui->comboBox_funcGraph_equation, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::onEquationTypeChanged);

    QMetaObject::invokeMethod(this, [this](){
        Net::reopenWaitThreadedConnection(m_client, m_client->getConnectionSettings());
    }, Qt::QueuedConnection);
    onEquationTypeChanged();
}

MainWindow::~MainWindow()
{
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

void MainWindow::saveSettings()
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
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveSettings();
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

void MainWindow::onClear()
{
    switch (static_cast<TaskIndex>(ui->tabWidget_tasks->currentIndex()))
    {
    case TaskIndex::ArraySorting:
    {
        ui->plainTextEdit_arraySorting_initialData->clear();
        ui->plainTextEdit_arraySorting_resultData->clear();
        break;
    }
    case TaskIndex::PrimeNumbers:
    {
        ui->plainTextEdit_primeNumbers_resultData->clear();
        break;
    }
    case TaskIndex::FunctionGraph:
    {
        auto chart = ui->chartView_funcGraph_chart->chart();
        auto series = qobject_cast<QLineSeries*>(chart->series().value(0));
        series->replace(QVector<QPointF>{});
        chart->setTitle(QString{});
        break;
    }
    default: { return; }
    }
}

void MainWindow::onSendRequest()
{
    if (m_client->getSocketState() != QAbstractSocket::ConnectedState)
    {
        showErrorMessage(QObject::tr("No connection to server"));
        return;
    }
    // Don't spam requests
    if (m_isAwaitingCancel || m_isAwaitingTask)
    {
        showErrorMessage(QObject::tr("Can't send request - still awaiting response."));
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
        // Validate input array by silently removing everything that is not number
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
        request = make_unique<Request_FindPrimeNumbers>();
        auto req = static_cast<Request_FindPrimeNumbers*>(request.get());
        req->x_from = ui->spinBox_primeNumbers_from->value();
        req->x_to = ui->spinBox_primeNumbers_to->value();
        // Validation
        if (req->x_from > req->x_to)
        {
            showErrorMessage(QObject::tr("Check input data - data invalid"));
            return;
        }
        break;
    }
    case TaskIndex::FunctionGraph:
    {
        request = make_unique<Request_CalculateFunction>();
        auto req = static_cast<Request_CalculateFunction*>(request.get());
        req->equationType = ui->comboBox_funcGraph_equation->currentData(Qt::UserRole).value<EquationType>();
        req->x_from = ui->spinBox_funcGraph_from->value();
        req->x_to = ui->spinBox_funcGraph_to->value();
        req->x_step = ui->spinBox_funcGraph_step->value();
        req->a = ui->spinBox_funcGraph_constA->value();
        req->b = ui->spinBox_funcGraph_constB->value();
        req->c = ui->spinBox_funcGraph_constC->value();
        // Validation
        if (req->x_from > req->x_to || req->x_step < 1 || (req->a | req->b | req->c) == 0)
        {
            showErrorMessage(QObject::tr("Check input data - data invalid"));
            return;
        }
        break;
    }
    default:
    {
        Q_CRITICAL_UNREACHABLE();
        Q_ASSERT(false);
        return;
    }
    }
    sendRequestToServer(request.get());

    // Block button while awaiting Task completion
    ui->pushButton_sendRequest->setEnabled(false);
}

void MainWindow::onSendCancel()
{
    if (m_client->getSocketState() != QAbstractSocket::ConnectedState)
    {
        showErrorMessage(QObject::tr("No connection to server"));
        return;
    }
    // Don't spam requests
    if (m_isAwaitingCancel)
        return;

    m_isAwaitingCancel = true;
    Request_CancelCurrentTask req;
    sendRequestToServer(&req);
}

void MainWindow::onEquationTypeChanged()
{
    auto lambda_etString = [](Protocol::EquationType data){
        switch (data)
        {
        case EquationType::Linear: { return "a*x + b"; }
        case EquationType::Quadratic: { return "a*x^2 + b*x + c"; }
        default: { return "Invalid"; }
        }
    };
    ui->label_funcGraph_equation->setText(lambda_etString(ui->comboBox_funcGraph_equation->currentData(Qt::UserRole).value<EquationType>()));
}

void MainWindow::onGenerateArray()
{
    static std::mt19937_64 gen{static_cast<std::mt19937_64::result_type>(std::chrono::system_clock::now().time_since_epoch().count())};
    auto x_from = ui->spinBox_arraySorting_from->value();
    auto x_to = ui->spinBox_arraySorting_to->value();
    auto x_count = ui->spinBox_arraySorting_count->value();

    // Validation
    if (x_from >= x_to || x_count < 2)
    {
        showErrorMessage(QObject::tr("Can't generate array - invalid parameters"));
        return;
    }

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

void MainWindow::parseResponse(QByteArray msg, NetConnection* const, Net::AddressPort)
{
    Request request;
    MAKE_QDATASTREAM_NET(streamMsg, &msg, QIODevice::ReadOnly);
    streamMsg >> request;
    if (streamMsg.status() != QDataStream::Ok)
    {
        onCorruptedMessage(msg);
        return;
    }

    // There's gonna be a lot of progress updates, no need to mention them
    if (request.type != RequestType::ProgressRange && request.type != RequestType::ProgressValue)
        f_logGeneral(QStringLiteral("Received %1 response").arg(toQString(request.type)));

    switch (request.type)
    {
    case RequestType::InvalidRequest:
    {
        Request_InvalidRequest req;
        streamMsg.device()->seek(0);
        streamMsg >> req;
        if (streamMsg.status() != QDataStream::Ok)
        {
            onCorruptedMessage(msg);
            return;
        }

        // Should be a better way to determine if reset should be called. Maybe prompt user and then force cancel or restart connection?
        if (req.errorCode != Protocol::ErrorCode::AlreadyRunningTask)
            resetAwaitingState();
        showErrorMessage(req.errorText.isEmpty() ? toQString(req.errorCode) : req.errorText);
        break;

    }
    case RequestType::SortArray:
    {
        Request_SortArray req;
        streamMsg.device()->seek(0);
        streamMsg >> req;
        if (streamMsg.status() != QDataStream::Ok)
        {
            onCorruptedMessage(msg);
            return;
        }

        QString text;
        text.reserve(6 * req.numbers.size());
        for (auto item : req.numbers)
            text.append(QString::number(item) + ' ');
        text.chop(1);
        ui->plainTextEdit_arraySorting_resultData->setPlainText(text);
        resetAwaitingState();
        break;
    }
    case RequestType::FindPrimeNumbers:
    {
        Request_FindPrimeNumbers req;
        streamMsg.device()->seek(0);
        streamMsg >> req;
        if (streamMsg.status() != QDataStream::Ok)
        {
            onCorruptedMessage(msg);
            return;
        }

        QString text;
        text.reserve(6 * req.primeNumbers.size());
        for (auto item : req.primeNumbers)
            text.append(QString::number(item) + ' ');
        text.chop(1);
        ui->plainTextEdit_primeNumbers_resultData->setPlainText(text);
        resetAwaitingState();
        break;
    }
    case RequestType::CalculateFunction:
    {        
        Request_CalculateFunction req;
        streamMsg.device()->seek(0);
        streamMsg >> req;
        if (streamMsg.status() != QDataStream::Ok)
        {
            onCorruptedMessage(msg);
            return;
        }

        QVector<QPointF> points; // should really just use QPointF in Request
        points.reserve(req.points.size());
        for (auto const& point : qAsConst(req.points))
            points.push_back(QPointF{point});

        auto chart = ui->chartView_funcGraph_chart->chart();
        auto series = qobject_cast<QLineSeries*>(chart->series().value(0));
        series->replace(points);
        chart->setTitle(QStringLiteral("%1 function chart").arg(toQString(req.equationType)));

        resetAwaitingState();
        break;
    }
    case RequestType::ProgressRange:
    {
        Request_ProgressRange req;
        streamMsg.device()->seek(0);
        streamMsg >> req;
        if (streamMsg.status() != QDataStream::Ok)
        {
            onCorruptedMessage(msg);
            return;
        }

        m_progressDialog->setRange(req.minimum, req.maximum);
        break;
    }
    case RequestType::ProgressValue:
    {
        Request_ProgressValue req;
        streamMsg.device()->seek(0);
        streamMsg >> req;
        if (streamMsg.status() != QDataStream::Ok)
        {
            onCorruptedMessage(msg);
            return;
        }

        m_progressDialog->setValue(req.value);
        break;
    }
    case RequestType::CancelCurrentTask:
    {
        resetAwaitingState();
        break;
    }
    default:
    {
        QString errorText(QStringLiteral("Received message with incorrect RequestType"));
        f_logError(QString("%1: %2").arg(errorText).arg(QString::fromLatin1(msg.toHex())));
        return;
    }
    }
}

void MainWindow::onCorruptedMessage(QByteArray msg)
{
    QString errorText(QStringLiteral("Received message with corrupted data"));
    f_logError(QString("%1: %2").arg(errorText).arg(QString::fromLatin1(msg.toHex())));
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
    // Reset awating state in case connection was lost
    if (socketState == QAbstractSocket::UnconnectedState)
    {
        resetAwaitingState();
    }
    indicateConnectionState(socketState);
    indicateLastUpdated();
}

void MainWindow::showErrorMessage(QString errorText)
{
    QMessageBox::critical(this, tr("Error"), errorText);
}

void MainWindow::sendRequestToServer(const Protocol::Request* req)
{
    QByteArray msg;
    MAKE_QDATASTREAM_NET(stream, &msg, QIODevice::WriteOnly);
    stream << *req;
    m_client->sendMessageQueued(msg);
    f_logGeneral(QStringLiteral("Sent %1 request").arg(toQString(req->type)));
}

void MainWindow::resetAwaitingState()
{
    m_progressDialog->reset();
    ui->pushButton_sendRequest->setEnabled(true);
    m_isAwaitingCancel = false;
    m_isAwaitingTask = false;
}
