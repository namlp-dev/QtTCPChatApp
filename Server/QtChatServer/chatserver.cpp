#include "chatserver.h"
#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include "chatmessage.h"
#include "clientconnection.h"

ChatServer::ChatServer(QObject *parent)
    : QTcpServer(parent)
    , m_port(0)
    , m_running(false)
{}

ChatServer::~ChatServer()
{
    stopServer();
}

bool ChatServer::startServer(quint16 port)
{
    if (m_running) {
        emit logMessage("Server already running");
        return false;
    }

    if (!listen(QHostAddress::Any, port)) {
        emit logMessage(QString("Failed to start server: %1").arg(errorString()));
        return false;
    }

    m_port = port;
    m_running = true;
    emit started(m_port);
    emit logMessage(QString("Server started on port %1").arg(m_port));
    return true;
}

void ChatServer::stopServer()
{
    if (!m_running) {
        return;
    }

    // Disconnect all clients
    for (auto *conn : m_clients) {
        conn->disconnectClient("Server shutting down");
    }
    m_clients.clear();
    m_pendingConnections.clear();

    close();
    m_running = false;
    emit stopped();
    emit logMessage("Server stopped");
}

bool ChatServer::isRunning() const
{
    return m_running;
}

quint16 ChatServer::serverPort() const
{
    return m_port;
}

QStringList ChatServer::clientList() const
{
    return m_clients.keys();
}

void ChatServer::kickClient(const QString &username, const QString &reason)
{
    if (!m_clients.contains(username)) {
        return;
    }

    ClientConnection *conn = m_clients[username];

    // Send kick notification
    QJsonObject kickMsg;
    kickMsg["type"] = "kick";
    kickMsg["reason"] = reason;
    conn->sendJson(kickMsg);

    // Disconnect
    conn->disconnectClient(reason);

    emit logMessage(QString("Kicked user: %1 - Reason: %2").arg(username).arg(reason));
}

void ChatServer::sendMessageToUser(const QString &username, const QJsonObject &msg)
{
    if (m_clients.contains(username)) {
        m_clients[username]->sendJson(msg);
    }
}

void ChatServer::broadcastMessage(const QString &text)
{
    ChatMessage msg("SERVER", "", text, ChatMessage::Broadcast);

    // Don't save broadcasts to history (they're not private conversations)
    // saveMessageToHistory(msg);

    // Send to all clients
    QJsonObject obj = msg.toJson();
    obj["type"] = "chat"; // Message type in protocol
    // Note: msg.toJson() already includes the MessageType enum value

    broadcastJson(obj);

    emit logMessage(QString("Broadcast: %1").arg(text));
    emit messageReceived("SERVER", "", text);
}

void ChatServer::broadcastJson(const QJsonObject &msg)
{
    for (auto *conn : m_clients) {
        conn->sendJson(msg);
    }
}

void ChatServer::incomingConnection(qintptr socketDescriptor)
{
    ClientConnection *conn = new ClientConnection(socketDescriptor, this);
    m_pendingConnections[socketDescriptor] = conn;

    connect(conn, &ClientConnection::registered, this, &ChatServer::handleClientRegistered);
    connect(conn, &ClientConnection::messageReceived, this, &ChatServer::handleClientMessage);
    connect(conn, &ClientConnection::disconnected, this, &ChatServer::handleClientDisconnected);
    connect(conn,
            &ClientConnection::chatHistoryRequested,
            this,
            &ChatServer::handleChatHistoryRequest);
    connect(conn, &ClientConnection::logMessage, this, &ChatServer::logMessage);

    emit logMessage(QString("New connection: descriptor %1").arg(socketDescriptor));
}

void ChatServer::handleClientRegistered(const QString &username, ClientConnection *connection)
{
    // Check if username already exists
    if (m_clients.contains(username)) {
        QJsonObject errorMsg;
        errorMsg["type"] = "error";
        errorMsg["message"] = "Username already taken";
        connection->sendJson(errorMsg);
        connection->disconnectClient("Username taken");
        return;
    }

    // Remove from pending and add to active clients
    m_pendingConnections.remove(connection->socketDescriptor());
    m_clients[username] = connection;

    emit clientConnected(username);
    emit logMessage(QString("Client registered: %1").arg(username));

    // Send user list update to all clients
    notifyUserListUpdate();
}

void ChatServer::handleClientMessage(const QString &from, const QString &to, const QString &text)
{
    ChatMessage msg(from, to, text, ChatMessage::Private);

    // Save to history
    saveMessageToHistory(msg);

    // Forward to recipient
    if (m_clients.contains(to)) {
        m_clients[to]->sendChatMessage(msg);
    }

    // Echo back to sender (for confirmation)
    if (m_clients.contains(from)) {
        m_clients[from]->sendChatMessage(msg);
    }

    emit messageReceived(from, to, text);
    emit logMessage(QString("Message: %1 -> %2: %3").arg(from).arg(to).arg(text));
}

void ChatServer::handleClientDisconnected(const QString &username)
{
    if (username.isEmpty()) {
        // Pending connection disconnected
        return;
    }

    m_clients.remove(username);
    emit clientDisconnected(username);
    emit logMessage(QString("Client disconnected: %1").arg(username));

    // Notify all remaining clients
    notifyUserListUpdate();
}

void ChatServer::handleChatHistoryRequest(const QString &requester, const QString &withUser)
{
    QList<ChatMessage> history = getChatHistory(requester, withUser);

    QJsonArray arr;
    for (const auto &msg : history) {
        arr.append(msg.toJson());
    }

    QJsonObject response;
    response["type"] = "chat_history";
    response["with"] = withUser;
    response["messages"] = arr;

    sendMessageToUser(requester, response);

    emit logMessage(QString("Sent %1 history messages to %2 (conversation with %3)")
                        .arg(history.size())
                        .arg(requester)
                        .arg(withUser));
}

void ChatServer::notifyUserListUpdate()
{
    QJsonArray arr;
    for (const QString &user : m_clients.keys()) {
        arr.append(user);
    }

    QJsonObject msg;
    msg["type"] = "user_list";
    msg["users"] = arr;

    broadcastJson(msg);
}

void ChatServer::saveMessageToHistory(const ChatMessage &message)
{
    // Don't save broadcasts to individual histories
    if (message.type() == ChatMessage::Broadcast || message.type() == ChatMessage::ServerAlert) {
        return;
    }

    QString filePath = getHistoryFilePath(message.from(), message.to());

    // Load existing messages
    QList<ChatMessage> messages = ChatMessage::loadMessages(filePath);
    messages.append(message);

    // Save updated list
    ChatMessage::saveMessages(messages, filePath);
}

QString ChatServer::getHistoryFilePath(const QString &user1, const QString &user2)
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    dataPath += "/server_history";
    QDir().mkpath(dataPath);

    QString convId = ChatMessage::conversationId(user1, user2);
    return QString("%1/%2.json").arg(dataPath).arg(convId);
}

QList<ChatMessage> ChatServer::getChatHistory(const QString &user1, const QString &user2)
{
    QString filePath = getHistoryFilePath(user1, user2);
    return ChatMessage::loadMessages(filePath);
}
