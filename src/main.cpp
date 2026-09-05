#include "launcherwindow.h"
#include "launcherconstants.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Toontown Infinite"));
    QApplication::setApplicationVersion(launcher::kVersion);

    LauncherWindow window;
    window.show();

    return app.exec();
}