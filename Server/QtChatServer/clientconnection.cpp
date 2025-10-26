#include "clientconnection.h"
#include <QDataStream>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include "chatmessage.h"

ClientConnection::ClientConnection(qintptr socketDescriptor, QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_socketDescriptor(socketDescriptor)
    , m_registered(false)
{
    if (!m_socket->setSocketDescriptor(socketDescriptor)) {
        emit logMessage(QString("Failed to set socket descriptor: %1").arg(m_socket->errorString()));
        deleteLater();
        return;
    }

    connect(m_socket, &QTcpSocket::readyRead, this, &ClientConnection::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientConnection::onDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &ClientConnection::onSocketError);

    emit logMessage(QString("ClientConnection created for descriptor %1").arg(socketDescriptor));
}

ClientConnection::~ClientConnection()
{
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
}

QString ClientConnection::username() const
{
    return m_username;
}

qintptr ClientConnection::socketDescriptor() const
{
    return m_socketDescriptor;
}

bool ClientConnection::isRegistered() const
{
    return m_registered;
}

QString ClientConnection::peerAddress() const
{
    if (m_socket) {
        QHostAddress addr = m_socket->peerAddress();

        // Convert IPv4-mapped IPv6 to IPv4 for cleaner display
        if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
            QHostAddress ipv4Addr(addr.toIPv4Address());
            if (!ipv4Addr.isNull()) {
                return ipv4Addr.toString(); // Returns clean "127.0.0.1"
            }
        }

        return addr.toString();
    }
    return QString();
}

quint16 ClientConnection::peerPort() const
{
    if (m_socket) {
        return m_socket->peerPort();
    }
    return 0;
}

QString ClientConnection::connectionInfo() const
{
    if (m_socket) {
        return QString("%1 (%2:%3)")
            .arg(m_username.isEmpty() ? "Unregistered" : m_username)
            .arg(m_socket->peerAddress().toString())
            .arg(m_socket->peerPort());
    }
    return m_username.isEmpty() ? "Unregistered" : m_username;
}

void ClientConnection::sendJson(const QJsonObject &msg)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    QJsonDocument doc(msg);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    // Length-prefixed protocol
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);
    stream << static_cast<quint32>(data.size());
    packet.append(data);

    m_socket->write(packet);
    m_socket->flush();
}

void ClientConnection::sendChatMessage(const ChatMessage &message)
{
    QJsonObject obj = message.toJson();
    obj["type"] = "chat";
    sendJson(obj);
}

void ClientConnection::disconnectClient(const QString &reason)
{
    Q_UNUSED(reason)
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
}

void ClientConnection::onReadyRead()
{
    m_readBuffer.append(m_socket->readAll());

    while (m_readBuffer.size() >= static_cast<int>(sizeof(quint32))) {
        QDataStream stream(&m_readBuffer, QIODevice::ReadOnly);
        stream.setVersion(QDataStream::Qt_6_0);

        quint32 msgSize;
        stream >> msgSize;

        if (m_readBuffer.size() < static_cast<int>(sizeof(quint32) + msgSize)) {
            break;
        }

        m_readBuffer.remove(0, sizeof(quint32));
        QByteArray jsonData = m_readBuffer.left(msgSize);
        m_readBuffer.remove(0, msgSize);

        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        if (doc.isObject()) {
            processJson(doc.object());
        }
    }
}

void ClientConnection::onDisconnected()
{
    emit logMessage(
        QString("Client disconnected: %1").arg(m_username.isEmpty() ? "unknown" : m_username));
    emit disconnected(m_username);
    deleteLater();
}

void ClientConnection::onSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    emit logMessage(QString("Socket error for %1: %2")
                        .arg(m_username.isEmpty() ? "unknown" : m_username)
                        .arg(m_socket->errorString()));
}

void ClientConnection::processJson(const QJsonObject &obj)
{
    QString type = obj["type"].toString();

    if (type == "register") {
        handleRegistration(obj);
    } else if (type == "chat") {
        handleChatMessage(obj);
    } else if (type == "request_history") {
        handleChatHistoryRequest(obj);
    }
}

void ClientConnection::handleRegistration(const QJsonObject &obj)
{
    if (m_registered) {
        emit logMessage("Client already registered");
        return;
    }

    QString username = obj["username"].toString().trimmed();
    if (username.isEmpty()) {
        QJsonObject errorMsg;
        errorMsg["type"] = "error";
        errorMsg["message"] = "Invalid username";
        sendJson(errorMsg);
        disconnectClient("Invalid username");
        return;
    }

    m_username = username;
    m_registered = true;
    emit registered(m_username, this);
}

void ClientConnection::handleChatMessage(const QJsonObject &obj)
{
    if (!m_registered) {
        emit logMessage("Received message from unregistered client");
        return;
    }

    QString from = obj["from"].toString();
    QString to = obj["to"].toString();
    QString text = obj["text"].toString();

    if (from != m_username) {
        emit logMessage(
            QString("Username mismatch: claimed %1, registered as %2").arg(from).arg(m_username));
        return;
    }

    emit messageReceived(from, to, text);
}

void ClientConnection::handleChatHistoryRequest(const QJsonObject &obj)
{
    if (!m_registered) {
        return;
    }

    QString withUser = obj["with"].toString();
    emit chatHistoryRequested(m_username, withUser);
}
