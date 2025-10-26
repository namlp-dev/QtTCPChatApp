#include "serverwindow.h"
#include <QCloseEvent>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include "chatserver.h"

ServerWindow::ServerWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_server(new ChatServer(this))
{
    setupUi();

    connect(m_server.data(), &ChatServer::started, this, [this](quint16 port) {
        updateServerState(true);
        appendLog(QString("Server started on port %1").arg(port));
    });

    connect(m_server.data(), &ChatServer::stopped, this, [this]() {
        updateServerState(false);
        appendLog("Server stopped");
    });

    connect(m_server.data(), &ChatServer::clientConnected, this, &ServerWindow::onClientConnected);
    connect(m_server.data(),
            &ChatServer::clientDisconnected,
            this,
            &ServerWindow::onClientDisconnected);
    connect(m_server.data(), &ChatServer::messageReceived, this, &ServerWindow::onMessageReceived);
    connect(m_server.data(), &ChatServer::logMessage, this, &ServerWindow::onLogMessage);

    updateServerState(false);
}

ServerWindow::~ServerWindow() {}

void ServerWindow::setupUi()
{
    setWindowTitle("Qt Chat Server");
    resize(800, 600);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // Server control group
    QGroupBox *controlGroup = new QGroupBox("Server Control", this);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlGroup);

    controlLayout->addWidget(new QLabel("Port:", this));
    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1024, 65535);
    m_portSpin->setValue(12345);
    controlLayout->addWidget(m_portSpin);

    m_startButton = new QPushButton("Start Server", this);
    controlLayout->addWidget(m_startButton);

    m_stopButton = new QPushButton("Stop Server", this);
    controlLayout->addWidget(m_stopButton);

    m_statusLabel = new QLabel("Status: Stopped", this);
    m_statusLabel->setStyleSheet("font-weight: bold;");
    controlLayout->addWidget(m_statusLabel);

    controlLayout->addStretch();

    mainLayout->addWidget(controlGroup);

    // Client management group
    QGroupBox *clientGroup = new QGroupBox("Connected Clients", this);
    QVBoxLayout *clientLayout = new QVBoxLayout(clientGroup);

    m_clientList = new QListWidget(this);
    clientLayout->addWidget(m_clientList);

    m_kickButton = new QPushButton("Kick Selected Client", this);
    clientLayout->addWidget(m_kickButton);

    mainLayout->addWidget(clientGroup);

    // Broadcast group
    QGroupBox *broadcastGroup = new QGroupBox("Broadcast Message", this);
    QHBoxLayout *broadcastLayout = new QHBoxLayout(broadcastGroup);

    m_broadcastEdit = new QLineEdit(this);
    m_broadcastEdit->setPlaceholderText("Type message to broadcast to all clients...");
    broadcastLayout->addWidget(m_broadcastEdit);

    m_broadcastButton = new QPushButton("Broadcast", this);
    broadcastLayout->addWidget(m_broadcastButton);

    mainLayout->addWidget(broadcastGroup);

    // Log group
    QGroupBox *logGroup = new QGroupBox("Server Log", this);
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);

    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    logLayout->addWidget(m_logEdit);

    mainLayout->addWidget(logGroup);

    // Connect signals
    connect(m_startButton, &QPushButton::clicked, this, &ServerWindow::onStartClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &ServerWindow::onStopClicked);
    connect(m_broadcastButton, &QPushButton::clicked, this, &ServerWindow::onBroadcastClicked);
    connect(m_kickButton, &QPushButton::clicked, this, &ServerWindow::onKickClicked);
    connect(m_clientList,
            &QListWidget::itemDoubleClicked,
            this,
            &ServerWindow::onClientItemDoubleClicked);
    connect(m_broadcastEdit, &QLineEdit::returnPressed, this, &ServerWindow::onBroadcastClicked);
}

void ServerWindow::closeEvent(QCloseEvent *event)
{
    if (m_server->isRunning()) {
        auto reply = QMessageBox::question(this,
                                           "Confirm Exit",
                                           "Server is still running. Stop and exit?",
                                           QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            m_server->stopServer();
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}

void ServerWindow::onStartClicked()
{
    quint16 port = m_portSpin->value();
    if (!m_server->startServer(port)) {
        QMessageBox::critical(this, "Error", "Failed to start server");
    }
}

void ServerWindow::onStopClicked()
{
    m_server->stopServer();
    m_clientList->clear();
}

void ServerWindow::onBroadcastClicked()
{
    QString text = m_broadcastEdit->text().trimmed();
    if (text.isEmpty()) {
        return;
    }

    m_server->broadcastMessage(text);
    m_broadcastEdit->clear();
    appendLog(QString("Broadcasted: %1").arg(text));
}

void ServerWindow::onKickClicked()
{
    QListWidgetItem *item = m_clientList->currentItem();
    if (!item) {
        QMessageBox::information(this, "No Selection", "Please select a client to kick");
        return;
    }

    QString username = item->text();
    auto reply
        = QMessageBox::question(this,
                                "Confirm Kick",
                                QString("Are you sure you want to kick user '%1'?").arg(username),
                                QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_server->kickClient(username, "Kicked by administrator");
        appendLog(QString("Kicked client: %1").arg(username));
    }
}

void ServerWindow::onClientItemDoubleClicked(QListWidgetItem *item)
{
    QString username = item->text();
    QMessageBox::information(this,
                             "Client Info",
                             QString("Client: %1\nStatus: Connected").arg(username));
}

void ServerWindow::onClientConnected(const QString &username)
{
    updateClientList();
    appendLog(QString("Client connected: %1").arg(username));
}

void ServerWindow::onClientDisconnected(const QString &username)
{
    updateClientList();
    appendLog(QString("Client disconnected: %1").arg(username));
}

void ServerWindow::onMessageReceived(const QString &from, const QString &to, const QString &text)
{
    appendLog(QString("Message [%1 -> %2]: %3").arg(from).arg(to).arg(text));
}

void ServerWindow::onLogMessage(const QString &msg)
{
    appendLog(msg);
}

void ServerWindow::updateClientList()
{
    m_clientList->clear();
    QStringList clients = m_server->clientList();

    for (const QString &client : clients) {
        m_clientList->addItem(client);
    }

    m_statusLabel->setText(QString("Status: Running - %1 client(s) connected").arg(clients.size()));
}

void ServerWindow::appendLog(const QString &msg)
{
    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_logEdit->append(QString("[%1] %2").arg(time).arg(msg));
}

void ServerWindow::updateServerState(bool running)
{
    m_startButton->setEnabled(!running);
    m_stopButton->setEnabled(running);
    m_portSpin->setEnabled(!running);
    m_broadcastButton->setEnabled(running);
    m_broadcastEdit->setEnabled(running);
    m_kickButton->setEnabled(running);

    if (running) {
        m_statusLabel->setText("Status: Running - 0 client(s) connected");
        m_statusLabel->setStyleSheet("font-weight: bold; color: green;");
    } else {
        m_statusLabel->setText("Status: Stopped");
        m_statusLabel->setStyleSheet("font-weight: bold; color: red;");
        m_clientList->clear();
    }
}
