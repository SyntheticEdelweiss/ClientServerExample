#pragma once

#include <QtCore/QString>
#include <QtWidgets/QMainWindow>

#include "Net/TcpClient.hpp"

namespace Ui { class MainWindow; }


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    TcpClient* m_client;

    const QString m_datetimeFormat{"[yyyy.MM.dd-hh:mm:ss.zzz]"};

private:
    void loadSettings();

private slots:
    void closeEvent(QCloseEvent* event) final;

    void onAppSettings();
    void onSendRequest();

    void onEquationTypeChanged();

    void onGenerateArray();

    void parseResponse(QByteArray msg, NetConnection* const, Net::AddressPort addrPort);

    void indicateLastUpdated();
    void indicateConnectionState(const QAbstractSocket::SocketState& socketState);
    void onSocketStateChanged(const QAbstractSocket::SocketState& socketState);

private:
    Ui::MainWindow* ui;
};
