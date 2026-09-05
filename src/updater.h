#pragma once

#include <QObject>
#include <QUrl>
#include <QList>
#include <QJsonObject>

class QNetworkAccessManager;
class QNetworkReply;
class QFile;

// One file entry from the manifest "tree".
// Replicates the per-file record: relative path, expected size, expected MD5 hash.
struct FileEntry
{
    QString path;
    qint64  size = 0;
    QString hash;
};

// Parsed result of a login request.
// success -> cookie + port are valid; failure -> code + detail describe the error.
struct LoginResponse
{
    bool    success = false;
    int     code    = 0;
    int     port    = 0;
    QString cookie;
    QString detail;
};

// Update engine. Downloads the manifest, verifies the launcher version, and
// downloads/patch-extracts game files, reporting progress as it goes.
//
// Mirrors the "Updater" QObject of the original launcher. Files whose local
// size or MD5 hash differs from the manifest are re-downloaded as "<path>.bz2"
// (from "<downloadBase>/<distribution>/<path>.bz2") and decompressed in place.
class Updater : public QObject
{
    Q_OBJECT

public:
    explicit Updater(QNetworkAccessManager *nam, QObject *parent = nullptr);
    ~Updater() override;

    // Base of the download server, e.g. "http://download.toontowninfinite.com/".
    QString downloadBase;

    // Manifest values (set after fetchManifest() succeeds).
    QString launcherVersion;
    QString gameVersion;
    QString loginEndpoint;
    QString gameServer;

    QList<FileEntry> files() const { return m_files; }

    // Sends the login request. The result is delivered through loginFinished().
    void login(const QString &username, const QString &password,
               const QString &distribution);

    // Fetches and parses manifest.json from the download server. The result is
    // reported via manifestFetched().
    void fetchManifest();

    // Starts (or continues) patching every file that is missing or outdated.
    void startUpdate();

    // Root directory game files are installed into (the launcher directory).
    static QString installDir();

signals:
    void manifestFetched(bool ok);
    void loginFinished(const LoginResponse &response);
    void downloadError(int error, const QString &errorString);
    void downloadProgress(qint64 bytesRead, qint64 bytesTotal, const QString &status);
    void updateFinished();
    void statusChanged(const QString &status);

private slots:
    void onManifestReplyFinished();
    void onLoginReplyFinished();
    void onDownloadReadyRead();
    void onDownloadFinished();

private:
    void parseTree(const QJsonObject &object, const QString &parentPath);
    void parseManifest(const QByteArray &body);
    void processNextFile();
    void startFileDownload(const FileEntry &entry, int index, int total);
    QString formatProgress(qint64 bytesRead, qint64 bytesTotal, int index, int total);
    void reportError(int code, const QString &message);
    void setStatus(const QString &status);

    QNetworkAccessManager *m_nam = nullptr;
    QNetworkReply *m_manifestReply = nullptr;
    QNetworkReply *m_loginReply = nullptr;
    QNetworkReply *m_downloadReply = nullptr;
    QFile *m_downloadFile = nullptr;

    QList<FileEntry> m_files;
    int m_currentIndex = 0;
    int m_totalFiles = 0;

    // State for transfer-speed calculation.
    qint64 m_lastBytesRead = 0;
    qint64 m_lastTimeMs = 0;
};