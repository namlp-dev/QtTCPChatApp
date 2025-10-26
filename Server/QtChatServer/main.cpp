#include <QApplication>
#include "serverwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application metadata
    QApplication::setApplicationName("QtChatServer");
    QApplication::setApplicationVersion("1.0");
    QApplication::setOrganizationName("QtChatApp");
    QApplication::setOrganizationDomain("qtchatapp.local");

    ServerWindow window;
    window.show();

    return app.exec();
}
