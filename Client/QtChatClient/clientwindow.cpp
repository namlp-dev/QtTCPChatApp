#include "clientwindow.h"
#include <QCloseEvent>
#include <QDir>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStandardPaths>
#include <QTextEdit>
#include <QVBoxLayout>
#include "chatclient.h"
#include "chatmessage.h"

ClientWindow::ClientWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_client(new ChatClient(this))
    , m_settings("QtChatApp", "ChatClient")
{
    setupUi();
    loadSettings();

    connect(m_client.data(), &ChatClient::connected, this, &ClientWindow::onConnected);
    connect(m_client.data(), &ChatClient::disconnected, this, &ClientWindow::onDisconnected);
    connect(m_client.data(), &ChatClient::messageReceived, this, &ClientWindow::onMessageReceived);
    connect(m_client.data(),
            &ChatClient::chatHistoryReceived,
            this,
            &ClientWindow::onChatHistoryReceived);
    connect(m_client.data(), &ChatClient::userListUpdated, this, &ClientWindow::onUserListUpdated);
    connect(m_client.data(), &ChatClient::logMessage, this, &ClientWindow::onLogMessage);
    connect(m_client.data(), &ChatClient::errorOccurred, this, &ClientWindow::onErrorOccurred);
    connect(m_client.data(), &ChatClient::kicked, this, &ClientWindow::onKicked);

    updateConnectionState(false);
}

ClientWindow::~ClientWindow() {}

void ClientWindow::setupUi()
{
    setWindowTitle("Qt Chat Client");
    resize(1000, 700);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // Connection group
    QGroupBox *connGroup = new QGroupBox("Connection", this);
    QHBoxLayout *connLayout = new QHBoxLayout(connGroup);

    connLayout->addWidget(new QLabel("Host:", this));
    m_serverHostEdit = new QLineEdit("127.0.0.1", this);
    connLayout->addWidget(m_serverHostEdit);

    connLayout->addWidget(new QLabel("Port:", this));
    m_serverPortEdit = new QLineEdit("12345", this);
    m_serverPortEdit->setMaximumWidth(80);
    connLayout->addWidget(m_serverPortEdit);

    connLayout->addWidget(new QLabel("Username:", this));
    m_usernameEdit = new QLineEdit(this);
    connLayout->addWidget(m_usernameEdit);

    m_connectButton = new QPushButton("Connect", this);
    connLayout->addWidget(m_connectButton);

    m_disconnectButton = new QPushButton("Disconnect", this);
    connLayout->addWidget(m_disconnectButton);

    mainLayout->addWidget(connGroup);

    // Chat area
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

    // User list
    QWidget *userWidget = new QWidget(this);
    QVBoxLayout *userLayout = new QVBoxLayout(userWidget);
    userLayout->addWidget(new QLabel("Online Users:", this));
    m_userList = new QListWidget(this);
    userLayout->addWidget(m_userList);
    splitter->addWidget(userWidget);

    // Chat view
    QWidget *chatWidget = new QWidget(this);
    QVBoxLayout *chatLayout = new QVBoxLayout(chatWidget);

    m_chatWithLabel = new QLabel("Select a user to chat", this);
    m_chatWithLabel->setStyleSheet("font-weight: bold; color: #2196F3;");
    chatLayout->addWidget(m_chatWithLabel);

    m_chatView = new QTextEdit(this);
    m_chatView->setReadOnly(true);
    chatLayout->addWidget(m_chatView);

    QHBoxLayout *msgLayout = new QHBoxLayout();
    m_messageEdit = new QLineEdit(this);
    m_messageEdit->setPlaceholderText("Type your message...");
    msgLayout->addWidget(m_messageEdit);

    m_sendButton = new QPushButton("Send", this);
    msgLayout->addWidget(m_sendButton);

    chatLayout->addLayout(msgLayout);
    splitter->addWidget(chatWidget);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);

    mainLayout->addWidget(splitter);

    // Log area
    QGroupBox *logGroup = new QGroupBox("Log", this);
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumHeight(120);
    logLayout->addWidget(m_logEdit);
    mainLayout->addWidget(logGroup);

    // Connections
    connect(m_connectButton, &QPushButton::clicked, this, &ClientWindow::onConnectButtonClicked);
    connect(m_disconnectButton,
            &QPushButton::clicked,
            this,
            &ClientWindow::onDisconnectButtonClicked);
    connect(m_sendButton, &QPushButton::clicked, this, &ClientWindow::onSendButtonClicked);
    connect(m_messageEdit,
            &QLineEdit::returnPressed,
            this,
            &ClientWindow::onMessageEditReturnPressed);
    connect(m_userList, &QListWidget::itemDoubleClicked, this, &ClientWindow::onUserDoubleClicked);
}

void ClientWindow::loadSettings()
{
    m_serverHostEdit->setText(m_settings.value("server/host", "127.0.0.1").toString());
    m_serverPortEdit->setText(m_settings.value("server/port", "12345").toString());
    m_usernameEdit->setText(m_settings.value("user/username", "").toString());
}

void ClientWindow::saveSettings()
{
    m_settings.setValue("server/host", m_serverHostEdit->text());
    m_settings.setValue("server/port", m_serverPortEdit->text());
    m_settings.setValue("user/username", m_usernameEdit->text());
}

void ClientWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    if (m_client->isConnected()) {
        m_client->disconnectFromServer();
    }
    event->accept();
}

void ClientWindow::onConnectButtonClicked()
{
    QString username = m_usernameEdit->text().trimmed();
    if (username.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter a username");
        return;
    }

    QString host = m_serverHostEdit->text().trimmed();
    bool ok;
    quint16 port = m_serverPortEdit->text().toUShort(&ok);

    if (!ok || port == 0) {
        QMessageBox::warning(this, "Error", "Invalid port number");
        return;
    }

    m_client->setUsername(username);
    m_client->connectToServer(host, port);
}

void ClientWindow::onDisconnectButtonClicked()
{
    m_client->disconnectFromServer();
}

void ClientWindow::onSendButtonClicked()
{
    QString text = m_messageEdit->text().trimmed();
    if (text.isEmpty() || m_currentChatUser.isEmpty()) {
        return;
    }

    m_client->sendMessage(m_currentChatUser, text);
    m_messageEdit->clear();

    // Add to local history
    ChatMessage msg(m_client->username(),
                    m_currentChatUser,
                    text,
                    ChatMessage::Private,
                    QDateTime::currentDateTime());
    m_chatHistories[m_currentChatUser].append(msg);
    // appendMessageToView(msg);
    saveLocalChatHistory(m_currentChatUser);
}

void ClientWindow::onMessageEditReturnPressed()
{
    onSendButtonClicked();
}

void ClientWindow::onUserDoubleClicked(QListWidgetItem *item)
{
    QString username = item->text();
    if (username == m_client->username()) {
        return;
    }
    switchToUser(username);
}

void ClientWindow::onConnected()
{
    updateConnectionState(true);
    appendLog("Connected to server successfully");
}

void ClientWindow::onDisconnected()
{
    updateConnectionState(false);
    m_userList->clear();
    m_currentChatUser.clear();
    m_chatWithLabel->setText("Select a user to chat");
    m_chatView->clear();
    appendLog("Disconnected from server");
}

void ClientWindow::onMessageReceived(const ChatMessage &message)
{
    // Handle broadcast/server messages - show in current chat view
    if (message.type() == ChatMessage::Broadcast || message.type() == ChatMessage::ServerAlert) {
        QString style = (message.type() == ChatMessage::ServerAlert)
                            ? "color: red; font-weight: bold;"
                            : "color: blue; font-weight: bold;";

        QString time = message.timestamp().toString("hh:mm:ss");
        QString prefix = (message.type() == ChatMessage::ServerAlert) ? "[SERVER ALERT]"
                                                                      : "[SERVER BROADCAST]";

        m_chatView->append(
            QString("<div style='%1'><span style='color: gray;'>[%2]</span> %3 %4: %5</div>")
                .arg(style)
                .arg(time)
                .arg(prefix)
                .arg(message.from().toHtmlEscaped())
                .arg(message.text().toHtmlEscaped()));

        appendLog(QString("Broadcast from %1: %2").arg(message.from()).arg(message.text()));
        return;
    }

    // Handle private messages
    QString otherUser = (message.from() == m_client->username()) ? message.to() : message.from();

    if (!m_chatHistories.contains(otherUser)) {
        m_chatHistories[otherUser] = QList<ChatMessage>();
    }

    m_chatHistories[otherUser].append(message);

    if (m_currentChatUser == otherUser) {
        appendMessageToView(message);
    }

    saveLocalChatHistory(otherUser);
}

void ClientWindow::onChatHistoryReceived(const QString &withUser, const QList<ChatMessage> &messages)
{
    m_chatHistories[withUser] = messages;

    if (m_currentChatUser == withUser) {
        m_chatView->clear();
        for (const auto &msg : messages) {
            appendMessageToView(msg);
        }
    }

    saveLocalChatHistory(withUser);
    appendLog(QString("Loaded %1 messages with %2").arg(messages.size()).arg(withUser));
}

void ClientWindow::onUserListUpdated(const QStringList &users)
{
    m_onlineUsers = users;
    m_userList->clear();

    for (const QString &user : users) {
        if (user != m_client->username()) {
            m_userList->addItem(user);
        }
    }

    appendLog(QString("Online users: %1").arg(users.join(", ")));
}

void ClientWindow::onLogMessage(const QString &msg)
{
    appendLog(msg);
}

void ClientWindow::onErrorOccurred(const QString &error)
{
    appendLog(QString("ERROR: %1").arg(error));
    QMessageBox::critical(this, "Error", error);
}

void ClientWindow::onKicked(const QString &reason)
{
    QMessageBox::warning(this, "Kicked", reason);
    appendLog(QString("Kicked: %1").arg(reason));
}

void ClientWindow::updateConnectionState(bool connected)
{
    m_connectButton->setEnabled(!connected);
    m_disconnectButton->setEnabled(connected);
    m_sendButton->setEnabled(connected);
    m_messageEdit->setEnabled(connected);

    m_serverHostEdit->setEnabled(!connected);
    m_serverPortEdit->setEnabled(!connected);
    m_usernameEdit->setEnabled(!connected);
}

void ClientWindow::appendMessageToView(const ChatMessage &message)
{
    QString time = message.timestamp().toString("hh:mm:ss");
    QString from = message.from();
    QString text = message.text().toHtmlEscaped();

    QString color = (from == m_client->username()) ? "#4CAF50" : "#2196F3";

    m_chatView->append(QString("<div><span style='color: gray;'>[%1]</span> "
                               "<span style='color: %2; font-weight: bold;'>%3:</span> %4</div>")
                           .arg(time)
                           .arg(color)
                           .arg(from.toHtmlEscaped())
                           .arg(text));
}

void ClientWindow::appendLog(const QString &msg)
{
    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_logEdit->append(QString("[%1] %2").arg(time).arg(msg));
}

void ClientWindow::switchToUser(const QString &username)
{
    if (username == m_currentChatUser) {
        return;
    }

    m_currentChatUser = username;
    m_chatWithLabel->setText(QString("Chatting with: %1").arg(username));
    m_chatView->clear();

    // Load local history first
    loadLocalChatHistory(username);

    // Display cached messages
    if (m_chatHistories.contains(username)) {
        for (const auto &msg : m_chatHistories[username]) {
            appendMessageToView(msg);
        }
    }

    // Request server history
    m_client->requestChatHistory(username);
}

void ClientWindow::loadLocalChatHistory(const QString &withUser)
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);

    QString convId = ChatMessage::conversationId(m_client->username(), withUser);
    QString filePath = QString("%1/%2.json").arg(dataPath).arg(convId);

    QList<ChatMessage> messages = ChatMessage::loadMessages(filePath);
    if (!messages.isEmpty()) {
        m_chatHistories[withUser] = messages;
    }
}

void ClientWindow::saveLocalChatHistory(const QString &withUser)
{
    if (!m_chatHistories.contains(withUser)) {
        return;
    }

    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);

    QString convId = ChatMessage::conversationId(m_client->username(), withUser);
    QString filePath = QString("%1/%2.json").arg(dataPath).arg(convId);

    ChatMessage::saveMessages(m_chatHistories[withUser], filePath);
}
