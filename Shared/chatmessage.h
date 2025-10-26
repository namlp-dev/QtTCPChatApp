#ifndef CHATMESSAGE_H
#define CHATMESSAGE_H

#include <QDataStream>
#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QString>

class ChatMessage
{
public:
    enum MessageType {
        Private,    // One-to-one chat
        Broadcast,  // Server broadcast to all
        ServerAlert // Server administrative message
    };

    ChatMessage();
    ChatMessage(const QString &from,
                const QString &to,
                const QString &text,
                MessageType type = Private,
                const QDateTime &timestamp = QDateTime::currentDateTime());

    // Accessors
    QString from() const;
    QString to() const;
    QString text() const;
    QDateTime timestamp() const;
    MessageType type() const;

    // Mutators
    void setFrom(const QString &from);
    void setTo(const QString &to);
    void setText(const QString &text);
    void setTimestamp(const QDateTime &timestamp);
    void setType(MessageType type);

    // JSON serialization
    QJsonObject toJson() const;
    static ChatMessage fromJson(const QJsonObject &obj);

    // Local chat history persistence
    static bool saveMessages(const QList<ChatMessage> &messages, const QString &filePath);
    static QList<ChatMessage> loadMessages(const QString &filePath);

    // Binary serialization
    friend QDataStream &operator<<(QDataStream &out, const ChatMessage &m);
    friend QDataStream &operator>>(QDataStream &in, ChatMessage &m);

    bool operator==(const ChatMessage &other) const;

    // Helper: get conversation ID for history grouping
    static QString conversationId(const QString &user1, const QString &user2);

private:
    QString m_from;
    QString m_to;
    QString m_text;
    QDateTime m_timestamp;
    MessageType m_type;
};

Q_DECLARE_METATYPE(ChatMessage)

#endif // CHATMESSAGE_H
