#include "chatmessage.h"
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

ChatMessage::ChatMessage()
    : m_type(Private)
{}

ChatMessage::ChatMessage(const QString &from,
                         const QString &to,
                         const QString &text,
                         MessageType type,
                         const QDateTime &timestamp)
    : m_from(from)
    , m_to(to)
    , m_text(text)
    , m_timestamp(timestamp)
    , m_type(type)
{}

QString ChatMessage::from() const
{
    return m_from;
}

QString ChatMessage::to() const
{
    return m_to;
}

QString ChatMessage::text() const
{
    return m_text;
}

QDateTime ChatMessage::timestamp() const
{
    return m_timestamp;
}

ChatMessage::MessageType ChatMessage::type() const
{
    return m_type;
}

void ChatMessage::setFrom(const QString &from)
{
    m_from = from;
}

void ChatMessage::setTo(const QString &to)
{
    m_to = to;
}

void ChatMessage::setText(const QString &text)
{
    m_text = text;
}

void ChatMessage::setTimestamp(const QDateTime &timestamp)
{
    m_timestamp = timestamp;
}

void ChatMessage::setType(MessageType type)
{
    m_type = type;
}

QJsonObject ChatMessage::toJson() const
{
    QJsonObject obj;
    obj["from"] = m_from;
    obj["to"] = m_to;
    obj["text"] = m_text;
    obj["timestamp"] = m_timestamp.toString(Qt::ISODate);
    obj["messageType"] = static_cast<int>(m_type); // Changed key to avoid confusion
    return obj;
}

ChatMessage ChatMessage::fromJson(const QJsonObject &obj)
{
    ChatMessage msg;
    msg.m_from = obj["from"].toString();
    msg.m_to = obj["to"].toString();
    msg.m_text = obj["text"].toString();
    msg.m_timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
    msg.m_type = static_cast<MessageType>(obj["messageType"].toInt(Private)); // Match the key
    return msg;
}

bool ChatMessage::saveMessages(const QList<ChatMessage> &messages, const QString &filePath)
{
    QJsonArray arr;
    for (const auto &msg : messages) {
        arr.append(msg.toJson());
    }

    QJsonDocument doc(arr);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }

    file.write(doc.toJson());
    file.close();
    return true;
}

QList<ChatMessage> ChatMessage::loadMessages(const QString &filePath)
{
    QList<ChatMessage> messages;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return messages;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) {
        return messages;
    }

    QJsonArray arr = doc.array();
    for (const auto &val : arr) {
        if (val.isObject()) {
            messages.append(ChatMessage::fromJson(val.toObject()));
        }
    }

    return messages;
}

QDataStream &operator<<(QDataStream &out, const ChatMessage &m)
{
    out << m.m_from << m.m_to << m.m_text << m.m_timestamp << static_cast<int>(m.m_type);
    return out;
}

QDataStream &operator>>(QDataStream &in, ChatMessage &m)
{
    int type;
    in >> m.m_from >> m.m_to >> m.m_text >> m.m_timestamp >> type;
    m.m_type = static_cast<ChatMessage::MessageType>(type);
    return in;
}

bool ChatMessage::operator==(const ChatMessage &other) const
{
    return m_from == other.m_from && m_to == other.m_to && m_text == other.m_text
           && m_timestamp == other.m_timestamp && m_type == other.m_type;
}

QString ChatMessage::conversationId(const QString &user1, const QString &user2)
{
    // Ensure consistent ordering for conversation IDs
    QStringList users = {user1, user2};
    users.sort();
    return users.join("_");
}
