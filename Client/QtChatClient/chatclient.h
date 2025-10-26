#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <QByteArray>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QTcpSocket>
#include "chatmessage.h"

class ChatClient : public QObject
{
    Q_OBJECT
public:
    explicit ChatClient(QObject *parent = nullptr);
    ~ChatClient() override;

    // Connection management
    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    // Authentication
    void setUsername(const QString &username);
    QString username() const;

    // Messaging
    void sendMessage(const QString &to, const QString &text);
    void requestChatHistory(const QString &withUser);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);
    void messageReceived(const ChatMessage &message);
    void chatHistoryReceived(const QString &withUser, const QList<ChatMessage> &messages);
    void userListUpdated(const QStringList &users);
    void logMessage(const QString &msg);
    void kicked(const QString &reason);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onSocketErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    Q_DISABLE_COPY(ChatClient)

    void sendJson(const QJsonObject &obj);
    void processIncomingJson(const QJsonObject &obj);
    void handleChatHistoryResponse(const QJsonObject &obj);

    QPointer<QTcpSocket> m_socket;
    QString m_username;
    QByteArray m_readBuffer;
};

#endif // CHATCLIENT_H
