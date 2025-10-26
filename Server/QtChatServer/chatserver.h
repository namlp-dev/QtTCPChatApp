#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QList>
#include <QMap>
#include <QPointer>
#include <QStringList>
#include <QTcpServer>
#include "chatmessage.h"

class ClientConnection;

class ChatServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit ChatServer(QObject *parent = nullptr);
    ~ChatServer() override;

    // Server lifecycle
    bool startServer(quint16 port);
    void stopServer();
    bool isRunning() const;
    quint16 serverPort() const;

    // Client management
    QStringList clientList() const;
    void kickClient(const QString &username, const QString &reason = "Kicked by server");

    // Messaging
    void sendMessageToUser(const QString &username, const QJsonObject &msg);
    void broadcastMessage(const QString &text);
    void broadcastJson(const QJsonObject &msg);

    // History management
    QList<ChatMessage> getChatHistory(const QString &user1, const QString &user2);

signals:
    void started(quint16 port);
    void stopped();
    void clientConnected(const QString &username);
    void clientDisconnected(const QString &username);
    void messageReceived(const QString &from, const QString &to, const QString &text);
    void logMessage(const QString &msg);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void handleClientMessage(const QString &from, const QString &to, const QString &text);
    void handleClientDisconnected(const QString &username);
    void handleClientRegistered(const QString &username, ClientConnection *connection);
    void handleChatHistoryRequest(const QString &requester, const QString &withUser);

private:
    Q_DISABLE_COPY(ChatServer)

    void notifyUserListUpdate();
    void saveMessageToHistory(const ChatMessage &message);
    QString getHistoryFilePath(const QString &user1, const QString &user2);

    QMap<QString, ClientConnection *> m_clients;
    QMap<qintptr, ClientConnection *> m_pendingConnections;

    quint16 m_port;
    bool m_running;
};

#endif // CHATSERVER_H
