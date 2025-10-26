#include "chatclient.h"
#include <QDataStream>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "chatmessage.h"

ChatClient::ChatClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::connected, this, &ChatClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &ChatClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &ChatClient::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &ChatClient::onSocketErrorOccurred);
}

ChatClient::~ChatClient()
{
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
}

void ChatClient::connectToServer(const QString &host, quint16 port)
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        emit logMessage("Already connected or connecting");
        return;
    }

    emit logMessage(QString("Connecting to %1:%2...").arg(host).arg(port));
    m_socket->connectToHost(host, port);
}

void ChatClient::disconnectFromServer()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
}

bool ChatClient::isConnected() const
{
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

void ChatClient::setUsername(const QString &username)
{
    m_username = username;
}

QString ChatClient::username() const
{
    return m_username;
}

void ChatClient::sendMessage(const QString &to, const QString &text)
{
    if (!isConnected()) {
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject obj;
    obj["type"] = "chat";
    obj["from"] = m_username;
    obj["to"] = to;
    obj["text"] = text;
    obj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    sendJson(obj);
}

void ChatClient::requestChatHistory(const QString &withUser)
{
    if (!isConnected()) {
        return;
    }

    QJsonObject obj;
    obj["type"] = "request_history";
    obj["from"] = m_username;
    obj["with"] = withUser;

    sendJson(obj);
}

void ChatClient::sendJson(const QJsonObject &obj)
{
    QJsonDocument doc(obj);
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

void ChatClient::onConnected()
{
    emit logMessage("Connected to server");
    m_readBuffer.clear();

    // Send registration
    QJsonObject obj;
    obj["type"] = "register";
    obj["username"] = m_username;
    sendJson(obj);

    emit connected();
}

void ChatClient::onDisconnected()
{
    emit logMessage("Disconnected from server");
    m_readBuffer.clear();
    emit disconnected();
}

void ChatClient::onReadyRead()
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
            processIncomingJson(doc.object());
        }
    }
}

void ChatClient::processIncomingJson(const QJsonObject &obj)
{
    QString type = obj["type"].toString();

    if (type == "chat") {
        ChatMessage msg = ChatMessage::fromJson(obj);
        emit messageReceived(msg);

        // Log for debugging
        if (msg.type() == ChatMessage::Broadcast || msg.type() == ChatMessage::ServerAlert) {
            emit logMessage(QString("Received broadcast/alert from: %1").arg(msg.from()));
        }
    } else if (type == "user_list") {
        QJsonArray arr = obj["users"].toArray();
        QStringList users;
        for (const auto &val : arr) {
            users.append(val.toString());
        }
        emit userListUpdated(users);
    } else if (type == "chat_history") {
        handleChatHistoryResponse(obj);
    } else if (type == "kick") {
        QString reason = obj["reason"].toString("You have been kicked");
        emit kicked(reason);
        disconnectFromServer();
    } else if (type == "error") {
        emit errorOccurred(obj["message"].toString());
    }
}

void ChatClient::handleChatHistoryResponse(const QJsonObject &obj)
{
    QString withUser = obj["with"].toString();
    QJsonArray arr = obj["messages"].toArray();

    QList<ChatMessage> messages;
    for (const auto &val : arr) {
        if (val.isObject()) {
            messages.append(ChatMessage::fromJson(val.toObject()));
        }
    }

    emit chatHistoryReceived(withUser, messages);
}

void ChatClient::onSocketErrorOccurred(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    QString errorMsg = m_socket->errorString();
    emit logMessage(QString("Socket error: %1").arg(errorMsg));
    emit errorOccurred(errorMsg);
}
