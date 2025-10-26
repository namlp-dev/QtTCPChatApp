#ifndef CLIENTWINDOW_H
#define CLIENTWINDOW_H

#include <QList>
#include <QMainWindow>
#include <QMap>
#include <QScopedPointer>
#include <QSettings>
#include "chatmessage.h"

// Forward declarations
class QLineEdit;
class QPushButton;
class QTextEdit;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QSplitter;
class ChatClient;

class ClientWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit ClientWindow(QWidget *parent = nullptr);
    ~ClientWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // UI actions
    void onConnectButtonClicked();
    void onDisconnectButtonClicked();
    void onSendButtonClicked();
    void onUserDoubleClicked(QListWidgetItem *item);
    void onMessageEditReturnPressed();

    // ChatClient signals
    void onConnected();
    void onDisconnected();
    void onMessageReceived(const ChatMessage &message);
    void onChatHistoryReceived(const QString &withUser, const QList<ChatMessage> &messages);
    void onUserListUpdated(const QStringList &users);
    void onLogMessage(const QString &msg);
    void onErrorOccurred(const QString &error);
    void onKicked(const QString &reason);

private:
    void setupUi();
    void loadSettings();
    void saveSettings();
    void updateConnectionState(bool connected);
    void appendMessageToView(const ChatMessage &message);
    void appendLog(const QString &msg);
    void switchToUser(const QString &username);
    void loadLocalChatHistory(const QString &withUser);
    void saveLocalChatHistory(const QString &withUser);

    QScopedPointer<ChatClient> m_client;

    // UI Widgets
    QLineEdit *m_serverHostEdit;
    QLineEdit *m_serverPortEdit;
    QLineEdit *m_usernameEdit;
    QPushButton *m_connectButton;
    QPushButton *m_disconnectButton;

    QListWidget *m_userList;
    QTextEdit *m_chatView;
    QLineEdit *m_messageEdit;
    QPushButton *m_sendButton;
    QLabel *m_chatWithLabel;

    QTextEdit *m_logEdit;

    // State management
    QString m_currentChatUser;                         // Currently chatting with
    QMap<QString, QList<ChatMessage>> m_chatHistories; // Per-user history cache
    QStringList m_onlineUsers;

    // Settings persistence
    QSettings m_settings;
};

#endif // CLIENTWINDOW_H
