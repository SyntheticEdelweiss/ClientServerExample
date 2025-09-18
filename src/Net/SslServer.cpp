#include "SslServer.hpp"

// #include <QtNetwork/QSslCertificate>
// #include <QtNetwork/QSslCertificateExtension>
// #include <QtNetwork/QSslCipher>
// #include <QtNetwork/QSslConfiguration>
// #include <QtNetwork/QSslDiffieHellmanParameters>
// #include <QtNetwork/QSslEllipticCurve>
// #include <QtNetwork/QSslError>
// #include <QtNetwork/QSslKey>
// #include <QtNetwork/QSslPreSharedKeyAuthenticator>
// #include <QtNetwork/QSslSocket>

SslServerInternal::SslServerInternal(SslServer* parent)
    : QTcpServer(static_cast<QObject*>(parent))
    , m_pServer(parent)
{}

void SslServerInternal::incomingConnection(qintptr socketDescriptor)
{
    QSslSocket* pSocket = new QSslSocket;
    if (pSocket->setSocketDescriptor(socketDescriptor))
    {
        addPendingConnection(pSocket);
        if (pSocket->state() != QAbstractSocket::ConnectedState) // if socket wasn't rejected and aborted
            return;
        pSocket->setSslConfiguration(m_pServer->m_sslConfiguration);
        connect(pSocket, qOverload<const QList<QSslError>&>(&QSslSocket::sslErrors), m_pServer, &SslServer::onSslErrors);
        pSocket->startServerEncryption();
    }
    else
    {
        delete pSocket;
    }
}

void SslServer::setSslConfiguration(const QSslConfiguration& configuration)
{
    if (m_connectionState == Net::ConnectionState::Created)
    {
        f_logGeneral(QString("%1: called setSslConfiguration() while connection is open - action forbidden").arg(nameId()));
        return;
    }
    m_sslConfiguration = configuration;
}

void SslServer::onSslErrors(const QList<QSslError>& errors)
{
    auto* pSocket = qobject_cast<QSslSocket*>(QObject::sender());
    QString errorText = QStringLiteral("%1: client %2:%3 (local %4:%5) sockd:%6: SSL errors occured:")
            .arg(nameId())
            .arg(pSocket->peerAddress().toString())
            .arg(pSocket->peerPort())
            .arg(pSocket->localAddress().toString())
            .arg(pSocket->localPort())
            .arg(pSocket->socketDescriptor())
            .arg(pSocket->errorString())
            .arg(pSocket->error());
    for (const auto& error : errors)
        errorText.append(QStringLiteral("\n%1 (code %2)").arg(error.errorString()).arg(error.error()));
    f_logError(errorText);

    if (m_isIgnoreSslErrors)
        pSocket->ignoreSslErrors();
}
