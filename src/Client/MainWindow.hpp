#pragma once

#include <QtCore/QString>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QProgressDialog>

#include "Common/Protocol.hpp"
#include "Net/TcpClient.hpp"

#include "PersistentProgressDialog.hpp"

namespace Ui { class MainWindow; }


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Net::LoginData loginData, QWidget *parent = nullptr);
    ~MainWindow();

private:
    TcpClient* m_client;

    bool m_isAwaitingCancel = false;
    bool m_isAwaitingTask = false;
    PersistentProgressDialog* m_progressDialog = nullptr;

    const QString m_datetimeFormat{"[yyyy.MM.dd-hh:mm:ss.zzz]"};

    uint m_regId_general = 0;
    std::function<void(QString)> f_logGeneral = [](QString msg) { qInfo(qUtf8Printable(QDateTime::currentDateTimeUtc().toString(QStringLiteral("[yyyy.MM.dd-hh:mm:ss.zzz]")) + msg)); };
    std::function<void(QString)> f_logError = [](QString msg) { qWarning(qUtf8Printable(QDateTime::currentDateTimeUtc().toString(QStringLiteral("[yyyy.MM.dd-hh:mm:ss.zzz]")) + msg)); };

private:
    void loadSettings();
    void saveSettings();
    void showErrorMessage(QString errorText);
    void sendRequestToServer(const Protocol::Request* req);
    void resetAwaitingState();

private slots:
    void closeEvent(QCloseEvent* event) final;

    void onAppSettings();
    void onClear();
    void onSendRequest();
    void onSendCancel();
    void onCorruptedMessage(QByteArray msg, QString errorText = QString{});

    void onEquationTypeChanged();

    void onGenerateArray();

    void parseResponse(QByteArray msg, NetConnection* const, Net::AddressPort addrPort);

    void indicateLastUpdated();
    void indicateConnectionState(const QAbstractSocket::SocketState& socketState);
    void onSocketStateChanged(const QAbstractSocket::SocketState& socketState);

private:
    Ui::MainWindow* ui;
};
