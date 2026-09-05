#pragma once

#include <QString>
#include <QUrl>

// Constants replicated from launcher.exe (Toontown Infinite launcher v1.2.0).
// These values were recovered from the original binary's static string pool.
namespace launcher {

const QString kVersion       = QStringLiteral("1.2.0");
const QString kPlatform      = QStringLiteral("win32");
const QString kDistribution  = QStringLiteral("live");

const QString kManifestName  = QStringLiteral("manifest.json");
const QUrl    kHomepageUrl   = QStringLiteral("https://launchpad.net/toontowninfinite");
const QUrl    kSiteUrl       = QStringLiteral("https://toontowninfinite.com");
const QUrl    kLoginEndpoint = QStringLiteral("https://toontowninfinite.com/api/login/");
const QUrl    kDownloadBase  = QStringLiteral("http://download.toontowninfinite.com/");

const QString kGameExecutable = QStringLiteral("infinite.exe");

// HTTP User-Agent template: "TTI-Launcher/%1 (%2/%3)" -> (version, distribution, platform)
inline QString userAgent()
{
    return QStringLiteral("TTI-Launcher/%1 (%2/%3)")
        .arg(kVersion, kDistribution, kPlatform);
}

// Status / dialog messages.
const QString kUpdateLauncherTitle = QStringLiteral("Update your launcher!");
const QString kUpdateLauncherBody  =
    QStringLiteral("Your launcher appears to be out of date. Please download and install the most recent one via the Toontown Infinite website.");
const QString kCouldNotConnect      = QStringLiteral("Couldn't connect to the account server.");
const QString kInvalidResponse      = QStringLiteral("Received an invalid response from the account server.");
const QString kCouldNotOpenWrite    = QStringLiteral("Couldn't open file for write: %1");
const QString kCouldNotOpenRead     = QStringLiteral("Couldn't open file for read: %1");
const QString kCouldNotExtract      = QStringLiteral("Couldn't extract archive: %1");
const QString kVerifyingCredentials = QStringLiteral("Verifying account credentials...");
const QString kUpdatingFile         = QStringLiteral("Updating file %1 of %2... [%3/%4%5 @ %6%7]");
const QString kStartingGame         = QStringLiteral("Starting the game...");

} // namespace launcher