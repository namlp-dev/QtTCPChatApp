#include <QApplication>
#include "clientwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application metadata
    QApplication::setApplicationName("QtChatClient");
    QApplication::setApplicationVersion("1.0");
    QApplication::setOrganizationName("QtChatApp");
    QApplication::setOrganizationDomain("qtchatapp.local");

    ClientWindow window;
    window.show();

    return app.exec();
}
