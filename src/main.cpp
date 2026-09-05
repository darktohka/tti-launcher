#include "launcherwindow.h"
#include "launcherconstants.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Toontown Infinite"));
    QApplication::setApplicationVersion(launcher::kVersion);

    // The PNG provides a fallback on platforms where the ICO decoder is unavailable.
    QIcon appIcon;
    appIcon.addFile(QStringLiteral(":/icons/launcher.ico"));
    appIcon.addFile(QStringLiteral(":/icons/launcher_256.png"));
    QApplication::setWindowIcon(appIcon);

    LauncherWindow window;
    window.setWindowIcon(appIcon);
    window.show();

    return app.exec();
}
