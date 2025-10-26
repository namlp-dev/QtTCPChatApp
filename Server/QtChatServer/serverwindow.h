#ifndef SERVERWINDOW_H
#define SERVERWINDOW_H

#include <QMainWindow>
#include <QScopedPointer>

// Forward declarations
class QTextEdit;
class QListWidget;
class QPushButton;
class QLineEdit;
class QSpinBox;
class QLabel;
class QListWidgetItem;
class ChatServer;

class ServerWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit ServerWindow(QWidget *parent = nullptr);
    ~ServerWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onStartClicked();
    void onStopClicked();
    void onBroadcastClicked();
    void onKickClicked();
    void onClientItemDoubleClicked(QListWidgetItem *item);
    void onClientConnected(const QString &username);
    void onClientDisconnected(const QString &username);
    void onMessageReceived(const QString &from, const QString &to, const QString &text);
    void onLogMessage(const QString &msg);

private:
    void setupUi();
    void appendLog(const QString &msg);
    void updateClientList();
    void updateServerState(bool running);

    QScopedPointer<ChatServer> m_server;

    // UI Widgets
    QSpinBox *m_portSpin;
    QPushButton *m_startButton;
    QPushButton *m_stopButton;
    QLabel *m_statusLabel;

    QListWidget *m_clientList;
    QPushButton *m_kickButton;

    QLineEdit *m_broadcastEdit;
    QPushButton *m_broadcastButton;

    QTextEdit *m_logEdit;
};

#endif // SERVERWINDOW_H
