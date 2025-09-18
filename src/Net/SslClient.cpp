#include "SslClient.hpp"

SslClient::SslClient(const quint8 _connType, const QString _connTypeName, QObject* parent)
    : TcpClient(_connType, _connTypeName, parent)
{
    connect(m_pSslSocket, qOverload<const QList<QSslError>&>(&QSslSocket::sslErrors), this, &SslClient::onSslErrors);
}

void SslClient::setSslConfiguration(const QSslConfiguration& configuration)
{
    if (m_connectionState == Net::ConnectionState::Created)
    {
        f_logGeneral(QString("%1: called setSslConfiguration() while connection is open - action forbidden").arg(nameId()));
        return;
    }
    m_pSslSocket->setSslConfiguration(configuration);
}

void SslClient::tryConnectToHost()
{
    m_pSslSocket->connectToHostEncrypted(m_connectionSettings.ipDestination.toString(), m_connectionSettings.portOut, QIODevice::ReadWrite);
    bool isConnected = m_pTcpSocket->waitForConnected(m_waitForConnectedInterval);
    if (isConnected)
    {
        emit openedConnection(true);
        m_reconnectTimer->stop();
        authorize();
    }
    else
    {
        emit openedConnection(false);
        if (m_isReconnectEnabled)
            m_reconnectTimer->start(m_reconnectInterval);
    }
    return;
}

void SslClient::onSslErrors(const QList<QSslError>& errors)
{
    for (const auto& error : errors)
        f_logError(QString("%1: SSL error occured: %2 (code %3)").arg(nameId()).arg(error.errorString()).arg(error.error()));

    if (m_isIgnoreSslErrors)
        m_pSslSocket->ignoreSslErrors();
}
