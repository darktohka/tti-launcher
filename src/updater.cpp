#include "updater.h"
#include "launcherconstants.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QCryptographicHash>
#include <QDateTime>
#include <QCoreApplication>

#include <bzlib.h>

namespace {

// Error codes used by the original launcher.
constexpr int kErrConnect = 900;   // could not reach the account server
constexpr int kErrInvalid = 901;   // invalid response from the account server
constexpr int kErrWrite   = 902;   // could not open file for write
constexpr int kErrRead    = 903;   // could not open file for read
constexpr int kErrExtract = 904;   // could not extract archive

QByteArray percentEncode(const QString &s)
{
    return QUrl::toPercentEncoding(s);
}

// Joins the download base, the distribution channel and a relative path into
// a full URL: "<downloadBase>/<distribution>/<path>".
QUrl channelUrl(const QString &downloadBase, const QString &distribution, const QString &path)
{
    QString base = downloadBase;
    if (base.endsWith(QLatin1Char('/')))
        base.chop(1);
    const QString url = base + QLatin1Char('/') + distribution + QLatin1Char('/') + path;
    return QUrl(url);
}

bool containsPlatform(const QJsonValue &platforms, const QString &platform)
{
    if (!platforms.isArray())
        return true;    // absent "platforms" means "all platforms"
    const QJsonArray array = platforms.toArray();
    for (const QJsonValue &value : array) {
        if (value.toString() == platform)
            return true;
    }
    return false;
}

bool fileUpToDate(const QString &path, const FileEntry &entry)
{
    QFile file(path);
    if (!file.exists())
        return false;
    if (file.size() != entry.size)
        return false;
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QCryptographicHash hash(QCryptographicHash::Md5);
    QByteArray buffer;
    buffer.resize(8192);
    while (true) {
        const qint64 n = file.read(buffer.data(), buffer.size());
        if (n <= 0)
            break;
        hash.addData(QByteArrayView(buffer.constData(), int(n)));
    }
    file.close();

    return QString::fromLatin1(hash.result().toHex()) == entry.hash;
}

bool extractBz2(const QString &archivePath, const QString &outputPath)
{
    FILE *in = fopen(archivePath.toLocal8Bit().constData(), "rb");
    if (!in)
        return false;

    FILE *out = fopen(outputPath.toLocal8Bit().constData(), "wb");
    if (!out) {
        fclose(in);
        return false;
    }

    int error = BZ_OK;
    BZFILE *bz = BZ2_bzReadOpen(&error, in, 0, 0, nullptr, 0);
    if (error != BZ_OK) {
        fclose(out);
        fclose(in);
        return false;
    }

    char buffer[4096];
    bool ok = true;
    while (true) {
        const int n = BZ2_bzRead(&error, bz, buffer, sizeof(buffer));
        if (error == BZ_STREAM_END) {
            if (n > 0)
                fwrite(buffer, 1, size_t(n), out);
            break;
        }
        if (error != BZ_OK || n <= 0) {
            ok = false;
            break;
        }
        if (fwrite(buffer, 1, size_t(n), out) != size_t(n)) {
            ok = false;
            break;
        }
    }

    BZ2_bzReadClose(&error, bz);
    fclose(out);
    fclose(in);
    return ok;
}

} // namespace

Updater::Updater(QNetworkAccessManager *nam, QObject *parent)
    : QObject(parent)
    , m_nam(nam)
    , downloadBase(launcher::kDownloadBase.toString())
{
}

Updater::~Updater()
{
    if (m_manifestReply) m_manifestReply->deleteLater();
    if (m_loginReply)    m_loginReply->deleteLater();
    if (m_downloadReply) m_downloadReply->deleteLater();
    delete m_downloadFile;
}

QString Updater::installDir()
{
    return QDir::currentPath();
}

// ---------------------------------------------------------------------------
// Login
// ---------------------------------------------------------------------------

void Updater::login(const QString &username, const QString &password,
                    const QString &distribution)
{
    if (m_loginReply) {
        m_loginReply->abort();
        m_loginReply->deleteLater();
        m_loginReply = nullptr;
    }

    QByteArray body;
    body += "username=" + percentEncode(username);
    body += "&password=" + percentEncode(password);
    body += "&distribution=" + percentEncode(distribution);

    QNetworkRequest request(launcher::kLoginEndpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded"));
    request.setHeader(QNetworkRequest::UserAgentHeader, launcher::userAgent());

    m_loginReply = m_nam->post(request, body);
    connect(m_loginReply, &QNetworkReply::finished,
            this, &Updater::onLoginReplyFinished);
}

void Updater::onLoginReplyFinished()
{
    QNetworkReply *reply = m_loginReply;
    m_loginReply = nullptr;

    LoginResponse response;

    if (reply->error() != QNetworkReply::NoError) {
        response.success = false;
        response.code = kErrConnect;
        response.detail = launcher::kCouldNotConnect;
        emit loginFinished(response);
        reply->deleteLater();
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    reply->deleteLater();
    if (!doc.isObject()) {
        response.success = false;
        response.code = kErrInvalid;
        response.detail = launcher::kInvalidResponse;
        emit loginFinished(response);
        return;
    }

    const QJsonObject obj = doc.object();
    response.success = obj.value(QStringLiteral("success")).toBool(false);

    if (response.success) {
        response.cookie = obj.value(QStringLiteral("cookie")).toString();
        response.port   = obj.value(QStringLiteral("port")).toInt(0);
    } else {
        response.code   = obj.value(QStringLiteral("code")).toInt(kErrInvalid);
        response.detail = obj.value(QStringLiteral("detail")).toString();
    }

    emit loginFinished(response);
}

// ---------------------------------------------------------------------------
// Manifest
// ---------------------------------------------------------------------------

void Updater::fetchManifest()
{
    if (m_manifestReply) {
        m_manifestReply->abort();
        m_manifestReply->deleteLater();
        m_manifestReply = nullptr;
    }

    const QUrl url = channelUrl(downloadBase, launcher::kDistribution, launcher::kManifestName);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, launcher::userAgent());

    m_manifestReply = m_nam->get(request);
    connect(m_manifestReply, &QNetworkReply::finished,
            this, &Updater::onManifestReplyFinished);
}

void Updater::onManifestReplyFinished()
{
    QNetworkReply *reply = m_manifestReply;
    m_manifestReply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        emit manifestFetched(false);
        reply->deleteLater();
        return;
    }

    parseManifest(reply->readAll());
    reply->deleteLater();
    emit manifestFetched(true);
}

void Updater::parseManifest(const QByteArray &body)
{
    const QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        emit manifestFetched(false);
        return;
    }

    const QJsonObject root = doc.object();
    launcherVersion = root.value(QStringLiteral("launcher-version")).toString();
    gameVersion     = root.value(QStringLiteral("game-version")).toString();
    loginEndpoint   = root.value(QStringLiteral("login-endpoint")).toString();
    gameServer      = root.value(QStringLiteral("game-server")).toString();

    m_files.clear();
    const QJsonObject tree = root.value(QStringLiteral("tree")).toObject();
    parseTree(tree, QString());
}

void Updater::parseTree(const QJsonObject &object, const QString &parentPath)
{
    if (object.isEmpty())
        return;

    // Skip subtrees that do not target our platform.
    if (!containsPlatform(object.value(QStringLiteral("platforms")), launcher::kPlatform))
        return;

    const QJsonArray directories = object.value(QStringLiteral("directories")).toArray();
    for (const QJsonValue &value : directories) {
        const QJsonObject dir = value.toObject();
        const QString name = dir.value(QStringLiteral("name")).toString();
        const QString path = parentPath.isEmpty()
            ? name
            : parentPath + QStringLiteral("/") + name;
        parseTree(dir, path);
    }

    const QJsonArray filesArray = object.value(QStringLiteral("files")).toArray();
    for (const QJsonValue &value : filesArray) {
        const QJsonObject file = value.toObject();
        if (!containsPlatform(file.value(QStringLiteral("platforms")), launcher::kPlatform))
            continue;

        FileEntry entry;
        const QString name = file.value(QStringLiteral("name")).toString();
        entry.path = parentPath.isEmpty()
            ? name
            : parentPath + QStringLiteral("/") + name;
        entry.size = file.value(QStringLiteral("size")).toVariant().toLongLong();
        entry.hash = file.value(QStringLiteral("hash")).toString().toLower();
        m_files.append(entry);
    }
}

// ---------------------------------------------------------------------------
// Update / patch
// ---------------------------------------------------------------------------

void Updater::startUpdate()
{
    m_currentIndex = 0;
    m_totalFiles = m_files.size();
    processNextFile();
}

void Updater::processNextFile()
{
    while (m_currentIndex < m_files.size()) {
        const FileEntry &entry = m_files.at(m_currentIndex);
        const QString finalPath = QDir(installDir()).filePath(entry.path);

        if (fileUpToDate(finalPath, entry)) {
            ++m_currentIndex;
            continue;
        }

        startFileDownload(entry, m_currentIndex, m_totalFiles);
        return;
    }

    emit updateFinished();
}

void Updater::startFileDownload(const FileEntry &entry, int index, int total)
{
    const QString relative = entry.path + QStringLiteral(".bz2");
    const QUrl url = channelUrl(downloadBase, launcher::kDistribution, relative);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, launcher::userAgent());

    const QString finalPath = QDir(installDir()).filePath(entry.path);
    const QString archivePath = finalPath + QStringLiteral(".bz2");
    QDir().mkpath(QFileInfo(archivePath).absolutePath());

    delete m_downloadFile;
    m_downloadFile = new QFile(archivePath, this);
    if (!m_downloadFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        reportError(kErrWrite, launcher::kCouldNotOpenWrite.arg(finalPath));
        return;
    }

    m_lastBytesRead = 0;
    m_lastTimeMs = QDateTime::currentMSecsSinceEpoch();

    m_downloadReply = m_nam->get(request);
    connect(m_downloadReply, &QNetworkReply::readyRead,
            this, &Updater::onDownloadReadyRead);
    connect(m_downloadReply, &QNetworkReply::downloadProgress,
            this, [this, index, total](qint64 bytesReceived, qint64 bytesTotal) {
                emit downloadProgress(bytesReceived, bytesTotal,
                                      formatProgress(bytesReceived, bytesTotal, index, total));
            });
    connect(m_downloadReply, &QNetworkReply::finished,
            this, &Updater::onDownloadFinished);
}

void Updater::onDownloadReadyRead()
{
    if (m_downloadReply && m_downloadFile)
        m_downloadFile->write(m_downloadReply->readAll());
}

void Updater::onDownloadFinished()
{
    QNetworkReply *reply = m_downloadReply;
    m_downloadReply = nullptr;

    const bool networkOk = (reply->error() == QNetworkReply::NoError);
    reply->deleteLater();

    m_downloadFile->flush();
    m_downloadFile->close();

    if (!networkOk) {
        reportError(kErrConnect, launcher::kCouldNotConnect);
        return;
    }

    const FileEntry &entry = m_files.at(m_currentIndex);
    const QString finalPath = QDir(installDir()).filePath(entry.path);
    const QString archivePath = finalPath + QStringLiteral(".bz2");

    if (!extractBz2(archivePath, finalPath)) {
        reportError(kErrExtract, launcher::kCouldNotExtract.arg(finalPath));
        return;
    }

    ++m_currentIndex;
    processNextFile();
}

QString Updater::formatProgress(qint64 bytesRead, qint64 bytesTotal,
                                int index, int total)
{
    // Format a byte count with a B/kB/MB suffix and one decimal place.
    auto formatSize = [](qint64 bytes) -> QPair<QString, QString> {
        double value = double(bytes);
        QString unit = QStringLiteral("B");
        if (bytes >= 1024 * 1024) {
            value = value / (1024.0 * 1024.0);
            unit = QStringLiteral("MB");
        } else if (bytes >= 1024) {
            value = value / 1024.0;
            unit = QStringLiteral("kB");
        }
        return { QString::number(value, 'f', 1), unit };
    };

    // Transfer speed, in bytes per second.
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 dt = qMax<qint64>(1, now - m_lastTimeMs);
    const double speed = double(bytesRead - m_lastBytesRead) * 1000.0 / double(dt);
    m_lastBytesRead = bytesRead;
    m_lastTimeMs = now;

    double speedValue = speed;
    QString speedUnit = QStringLiteral("B/s");
    if (speed >= 1024.0 * 1024.0) {
        speedValue /= 1024.0 * 1024.0;
        speedUnit = QStringLiteral("MB/s");
    } else if (speed >= 1024.0) {
        speedValue /= 1024.0;
        speedUnit = QStringLiteral("kB/s");
    }

    const auto read = formatSize(bytesRead);
    const auto totalSize = formatSize(bytesTotal);

    return launcher::kUpdatingFile
        .arg(index + 1)
        .arg(total)
        .arg(read.first, totalSize.first)
        .arg(totalSize.second)
        .arg(QString::number(speedValue, 'f', 1))
        .arg(speedUnit);
}

void Updater::reportError(int code, const QString &message)
{
    emit downloadError(code, message);
    setStatus(message);
}

void Updater::setStatus(const QString &status)
{
    emit statusChanged(status);
}