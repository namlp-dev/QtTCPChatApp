#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QTcpSocket>
#include "chatmessage.h"

class ClientConnection : public QObject
{
    Q_OBJECT
public:
    explicit ClientConnection(qintptr socketDescriptor, QObject *parent = nullptr);
    ~ClientConnection() override;

    QString username() const;
    qintptr socketDescriptor() const;
    bool isRegistered() const;

    // Send operations
    void sendJson(const QJsonObject &msg);
    void sendChatMessage(const ChatMessage &message);

public slots:
    void disconnectClient(const QString &reason = QString());

signals:
    void messageReceived(const QString &from, const QString &to, const QString &text);
    void disconnected(const QString &username);
    void registered(const QString &username, ClientConnection *connection);
    void chatHistoryRequested(const QString &requester, const QString &withUser);
    void logMessage(const QString &msg);

private slots:
    void onReadyRead();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    Q_DISABLE_COPY(ClientConnection)

    void processJson(const QJsonObject &obj);
    void handleRegistration(const QJsonObject &obj);
    void handleChatMessage(const QJsonObject &obj);
    void handleChatHistoryRequest(const QJsonObject &obj);

    QPointer<QTcpSocket> m_socket;
    QString m_username;
    qintptr m_socketDescriptor;
    QByteArray m_readBuffer;
    bool m_registered;
};

#endif // CLIENTCONNECTION_H
