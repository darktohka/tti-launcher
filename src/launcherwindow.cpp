#include "launcherwindow.h"
#include "launcherconstants.h"
#include "updater.h"

#include <QApplication>
#include <QDesktopServices>
#include <QFont>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPixmap>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace {
QFont makeBoldFont(const char *family, int pointSize) {
  QFont font;
  font.setFamily(QString::fromLatin1(family));
  font.setPointSize(pointSize);
  font.setWeight(QFont::Bold);
  return font;
}
} // namespace

LauncherWindow::LauncherWindow(QWidget *parent)
    : DraggableWindow(parent), m_nam(new QNetworkAccessManager(this)),
      m_updater(new Updater(m_nam, this)) {
  setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
  setupUi();

  // Initial update check (fetch manifest, verify launcher version).
  connect(m_updater, &Updater::manifestFetched, this,
          &LauncherWindow::onManifestFetched);
  connect(m_updater, &Updater::loginFinished, this,
          &LauncherWindow::onLoginFinished);
  connect(m_updater, &Updater::downloadError, this,
          &LauncherWindow::onDownloadError);
  connect(m_updater, &Updater::downloadProgress, this,
          &LauncherWindow::onDownloadProgress);
  connect(m_updater, &Updater::updateFinished, this,
          &LauncherWindow::onUpdateFinished);
  connect(m_updater, &Updater::statusChanged, this,
          &LauncherWindow::onStatusChanged);

  connect(m_usernameEdit, &QLineEdit::returnPressed, this,
          &LauncherWindow::on_line_edit_username_returnPressed);
  connect(m_passwordEdit, &QLineEdit::returnPressed, this,
          &LauncherWindow::on_line_edit_password_returnPressed);

  // The frameless window is drawn as a rounded rectangle on top of the
  // background image, so the corners must be transparent.
  setAttribute(Qt::WA_TranslucentBackground);
  m_usernameEdit->setFocus(Qt::OtherFocusReason);

  m_updater->fetchManifest();
}

LauncherWindow::~LauncherWindow() = default;

// ---------------------------------------------------------------------------
// UI
// ---------------------------------------------------------------------------

void LauncherWindow::setupUi() {
  setObjectName(QStringLiteral("launcher_window"));
  resize(800, 600);
  setFixedSize(800, 600);

  // The background is painted in paintEvent() with rounded corners; the
  // window itself stays translucent so the corners are transparent.
  m_background = QPixmap(QStringLiteral(":/JPEG/assets/BACKGROUND.jpg"));

  setFont(makeBoldFont("Comic Sans MS", 10));

  QPalette blackText = palette();
  blackText.setColor(QPalette::WindowText, Qt::black);
  blackText.setColor(QPalette::Text, Qt::black);
  blackText.setColor(QPalette::ButtonText, Qt::black);
  setPalette(blackText);

  m_closeButton = new QPushButton(this);
  m_closeButton->setObjectName(QStringLiteral("push_button_close"));
  m_closeButton->setGeometry(751, 10, 34, 34);
  m_closeButton->setFixedSize(35, 35);
  m_closeButton->setCursor(Qt::PointingHandCursor);
  m_closeButton->setStyleSheet(
      QStringLiteral("QPushButton#push_button_close {\n"
                     "    border: none;\n"
                     "    background-image: url(:/JPEG/assets/CLOSE_NORMAL.jpg);\n"
                     "    background-repeat: none;\n"
                     "}\n"
                     "\n"
                     "QPushButton#push_button_close:hover {\n"
                     "    background-image: url(:/JPEG/assets/CLOSE_ROLLOVER.jpg);\n"
                     "}\n"
                     "\n"
                     "QPushButton#push_button_close:pressed {\n"
                     "    background-image: url(:/JPEG/assets/CLOSE_DOWN.jpg);\n"
                     "}"));
  connect(m_closeButton, &QPushButton::clicked, this,
          &LauncherWindow::on_push_button_close_clicked);

  m_minimizeButton = new QPushButton(this);
  m_minimizeButton->setObjectName(QStringLiteral("push_button_minimize"));
  m_minimizeButton->setGeometry(706, 10, 34, 34);
  m_minimizeButton->setFixedSize(35, 35);
  m_minimizeButton->setCursor(Qt::PointingHandCursor);
  m_minimizeButton->setStyleSheet(QStringLiteral(
      "QPushButton#push_button_minimize {\n"
      "    border: none;\n"
      "    background-image: url(:/JPEG/assets/MINIMIZE_NORMAL.jpg);\n"
      "    background-repeat: none;\n"
      "}\n"
      "\n"
      "QPushButton#push_button_minimize:hover {\n"
      "    background-image: url(:/JPEG/assets/MINIMIZE_ROLLOVER.jpg);\n"
      "}\n"
      "\n"
      "QPushButton#push_button_minimize:pressed {\n"
      "    background-image: url(:/JPEG/assets/MINIMIZE_DOWN.jpg);\n"
      "}"));
  connect(m_minimizeButton, &QPushButton::clicked, this,
          &LauncherWindow::on_push_button_minimize_clicked);

  m_usernameEdit = new QLineEdit(this);
  m_usernameEdit->setObjectName(QStringLiteral("line_edit_username"));
  m_usernameEdit->setGeometry(144, 283, 168, 16);
  m_usernameEdit->setFixedSize(169, 17);
  m_usernameEdit->setFont(makeBoldFont("Microsoft Sans Serif", 10));
  m_usernameEdit->setMaxLength(59);
  m_usernameEdit->setFrame(false);
  m_usernameEdit->setAlignment(Qt::AlignCenter);
  m_usernameEdit->setStyleSheet(
      QStringLiteral("QLineEdit#line_edit_username {\n"
                     "    border: none;\n"
                     "    background: none;\n"
                     "    background-color: rgb(255, 255, 255);\n"
                     "}"));

  m_passwordEdit = new QLineEdit(this);
  m_passwordEdit->setObjectName(QStringLiteral("line_edit_password"));
  m_passwordEdit->setGeometry(144, 319, 168, 16);
  m_passwordEdit->setFixedSize(169, 17);
  m_passwordEdit->setFont(makeBoldFont("Microsoft Sans Serif", 10));
  m_passwordEdit->setMaxLength(59);
  m_passwordEdit->setAlignment(Qt::AlignCenter);
  m_passwordEdit->setEchoMode(QLineEdit::Password);
  m_passwordEdit->setStyleSheet(
      QStringLiteral("QLineEdit#line_edit_password {\n"
                     "    border: none;\n"
                     "    background: none;\n"
                     "    background-color: rgb(255, 255, 255);\n"
                     "}"));

  m_reportBugButton = new QPushButton(this);
  m_reportBugButton->setObjectName(QStringLiteral("push_button_report_a_bug"));
  m_reportBugButton->setGeometry(306, 548, 119, 39);
  m_reportBugButton->setFixedSize(120, 40);
  m_reportBugButton->setCursor(Qt::PointingHandCursor);
  m_reportBugButton->setStyleSheet(QStringLiteral(
      "QPushButton#push_button_report_a_bug {\n"
      "    border: none;\n"
      "    background-image: url(:/JPEG/assets/REPORT_A_BUG_NORMAL.jpg);\n"
      "    background-repeat: none;\n"
      "}\n"
      "\n"
      "QPushButton#push_button_report_a_bug:hover {\n"
      "    background-image: url(:/JPEG/assets/REPORT_A_BUG_ROLLOVER.jpg);\n"
      "}\n"
      "\n"
      "QPushButton#push_button_report_a_bug:pressed {\n"
      "    background-image: url(:/JPEG/assets/REPORT_A_BUG_DOWN.jpg);\n"
      "}\n"
      "\n"
      "QPushButton#push_button_report_a_bug:pressed {\n"
      "    background-image: url(:/JPEG/assets/REPORT_A_BUG_DISABLED.jpg);\n"
      "}"));
  connect(m_reportBugButton, &QPushButton::clicked, this,
          &LauncherWindow::on_push_button_report_a_bug_clicked);

  m_homePageButton = new QPushButton(this);
  m_homePageButton->setObjectName(QStringLiteral("push_button_home_page"));
  m_homePageButton->setGeometry(170, 548, 119, 39);
  m_homePageButton->setFixedSize(120, 40);
  m_homePageButton->setCursor(Qt::PointingHandCursor);
  m_homePageButton->setStyleSheet(QStringLiteral(
      "QPushButton#push_button_home_page {\n"
      "    border: none;\n"
      "    background-image: url(:/JPEG/assets/HOME_PAGE_NORMAL.jpg);\n"
      "    background-repeat: none;\n"
      "}\n"
      "\n"
      "QPushButton#push_button_home_page:hover {\n"
      "    background-image: url(:/JPEG/assets/HOME_PAGE_ROLLOVER.jpg);\n"
      "}\n"
      "\n"
      "QPushButton#push_button_home_page:pressed {\n"
      "    background-image: url(:/JPEG/assets/HOME_PAGE_DOWN.jpg);\n"
      "}\n"
      "\n"
      "QPushButton#push_button_home_page:disabled {\n"
      "    background-image: url(:/JPEG/assets/HOME_PAGE_DISABLED.jpg);\n"
      "}"));
  connect(m_homePageButton, &QPushButton::clicked, this,
          &LauncherWindow::on_push_button_home_page_clicked);

  m_playButton = new QPushButton(this);
  m_playButton->setObjectName(QStringLiteral("push_button_play"));
  m_playButton->setGeometry(368, 397, 84, 81);
  m_playButton->setFixedSize(85, 82);
  m_playButton->setCursor(Qt::PointingHandCursor);
  m_playButton->setStyleSheet(QStringLiteral(
      "QPushButton#push_button_play {\n"
      "    border: none;\n"
      "    background-image: url(:/JPEG/assets/PLAY_NORMAL.jpg);\n"
      "    background-repeat: none;\n"
      "}\n"
      "\n"
      "QPushButton#push_button_play:hover {\n"
      "    background-image: url(:/JPEG/assets/PLAY_ROLLOVER.jpg);\n"
      "}\n"
      "\n"
      "QPushButton#push_button_play:pressed {\n"
      "    background-image: url(:/JPEG/assets/PLAY_DOWN.jpg);\n"
      "}\n"
      "\n"
      "QPushButton#push_button_play:disabled {\n"
      "    background-image: url(:/JPEG/assets/PLAY_DISABLED.jpg);\n"
      "}"));
  connect(m_playButton, &QPushButton::clicked, this,
          &LauncherWindow::on_push_button_play_clicked);

  m_progressBar = new QProgressBar(this);
  m_progressBar->setObjectName(QStringLiteral("progress_bar"));
  m_progressBar->setGeometry(32, 257, 290, 11);
  m_progressBar->setMinimumSize(5, 5);
  m_progressBar->setMaximumSize(500, 500);
  m_progressBar->setStyleSheet(QStringLiteral(
      "QProgressBar#progress_bar {\n"
      "    border: none;\n"
      "    background: none;\n"
      "    background-color: rgb(255, 255, 255);\n"
      "}\n"
      "\n"
      "QProgressBar::chunk#progress_bar {\n"
      "    border: none;\n"
      "    border-radius: 2px;\n"
      "    background-color: rgb(255, 140, 0);\n"
      "}"));
  m_progressBar->setValue(0);
  m_progressBar->setTextVisible(false);

  m_statusLabel = new QLabel(this);
  m_statusLabel->setObjectName(QStringLiteral("label_status"));
  m_statusLabel->setGeometry(32, 257, 290, 11);
  m_statusLabel->setMinimumSize(291, 12);
  m_statusLabel->setMaximumSize(291, 12);
  m_statusLabel->setFont(makeBoldFont("Comic Sans MS", 7));
  m_statusLabel->setTextFormat(Qt::PlainText);
  m_statusLabel->setAlignment(Qt::AlignCenter);
  m_statusLabel->setWordWrap(true);
  m_statusLabel->setStyleSheet(QStringLiteral(
      "QLabel#label_status {\n    border: none;\n    background: none;\n}"));

  m_gameVersionLabel = new QLabel(this);
  m_gameVersionLabel->setObjectName(QStringLiteral("label_game_version"));
  m_gameVersionLabel->setGeometry(610, 580, 174, 19);
  m_gameVersionLabel->setMinimumSize(175, 20);
  m_gameVersionLabel->setMaximumSize(175, 20);
  m_gameVersionLabel->setFont(makeBoldFont("Comic Sans MS", 8));
  m_gameVersionLabel->setTextFormat(Qt::PlainText);
  m_gameVersionLabel->setWordWrap(true);
  m_gameVersionLabel->setStyleSheet(QStringLiteral(
      "QLabel#label_game_version {\n    border: none;\n    background: none;\n}"));
  m_gameVersionLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);

  m_launcherVersionLabel = new QLabel(this);
  m_launcherVersionLabel->setObjectName(
      QStringLiteral("label_launcher_version"));
  m_launcherVersionLabel->setGeometry(610, 567, 174, 19);
  m_launcherVersionLabel->setMinimumSize(175, 20);
  m_launcherVersionLabel->setMaximumSize(175, 20);
  m_launcherVersionLabel->setFont(makeBoldFont("Comic Sans MS", 8));
  m_launcherVersionLabel->setWordWrap(true);
  m_launcherVersionLabel->setStyleSheet(QStringLiteral(
      "QLabel#label_launcher_version {\n    border: none;\n    background: none;\n}"));
  m_launcherVersionLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);

  m_contentPacksButton = new QPushButton(this);
  m_contentPacksButton->setObjectName(
      QStringLiteral("push_button_content_packs"));
  m_contentPacksButton->setGeometry(32, 548, 119, 39);
  m_contentPacksButton->setFixedSize(120, 40);
  m_contentPacksButton->setEnabled(false);
  m_contentPacksButton->setCursor(Qt::PointingHandCursor);
  m_contentPacksButton->setStyleSheet(QStringLiteral(
      "QPushButton#push_button_content_packs {\n"
      "    border: none;\n"
      "    background-image: url(:/JPEG/assets/CONTENT_PACKS_NORMAL.jpg);\n"
      "    background-repeat: none;\n"
      "}\n"
      "\n"
      "QPushButton#push_button_content_packs:hover {\n"
      "    background-image: url(:/JPEG/assets/CONTENT_PACKS_ROLLOVER.jpg);\n"
      "}\n"
      "\n"
      "QPushButton#push_button_content_packs:pressed {\n"
      "    background-image: url(:/JPEG/assets/CONTENT_PACKS_DOWN.jpg);\n"
      "}\n"
      "\n"
      "QPushButton#push_button_content_packs:disabled {\n"
      "    background-image: url(:/JPEG/assets/CONTENT_PACKS_DISABLED.jpg);\n"
"}"));

  m_optionsButton = new QPushButton(this);
  m_optionsButton->setObjectName(QStringLiteral("push_button_options"));
  m_optionsButton->setGeometry(444, 548, 119, 39);
  m_optionsButton->setFixedSize(120, 40);
  m_optionsButton->setEnabled(false);
  m_optionsButton->setCursor(Qt::PointingHandCursor);
  m_optionsButton->setStyleSheet(QStringLiteral(
      "QPushButton#push_button_options {\n"
      "    border: none;\n"
      "    background-image: url(:/JPEG/assets/OPTIONS_NORMAL.jpg);\n"
      "    background-repeat: none;\n"
      "}\n"
      "\n"
      "QPushButton#push_button_options:hover {\n"
      "    background-image: url(:/JPEG/assets/OPTIONS_ROLLOVER.jpg);\n"
      "}\n"
      "\n"
      "QPushButton#push_button_options:pressed {\n"
      "    background-image: url(:/JPEG/assets/OPTIONS_DOWN.jpg);\n"
      "}\n"
      "\n"
      "QPushButton#push_button_options:disabled {\n"
      "    background-image: url(:/JPEG/assets/OPTIONS_DISABLED.jpg);\n"
      "}"));

  setWindowTitle(QStringLiteral("Toontown Infinite"));
  m_statusLabel->setText(QStringLiteral("Login"));
  m_launcherVersionLabel->setText(QStringLiteral("N/A"));
  m_gameVersionLabel->setText(QStringLiteral("N/A"));
}

void LauncherWindow::paintEvent(QPaintEvent *) {
  // Paint the rounded corners manually.
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  QPainterPath path;
  path.addRoundedRect(QRectF(rect()), 25, 25);
  painter.setClipPath(path);
  painter.drawPixmap(rect(), m_background);
}

// ---------------------------------------------------------------------------
// Button handlers
// ---------------------------------------------------------------------------

void LauncherWindow::on_push_button_close_clicked() { close(); }

void LauncherWindow::on_push_button_minimize_clicked() { showMinimized(); }

void LauncherWindow::on_push_button_home_page_clicked() {
  QDesktopServices::openUrl(launcher::kSiteUrl);
}

void LauncherWindow::on_push_button_report_a_bug_clicked() {
  QDesktopServices::openUrl(launcher::kLaunchpadUrl);
}

void LauncherWindow::on_push_button_play_clicked() {
  if (!m_playButton->isEnabled())
    return;
  if (m_usernameEdit->text().isEmpty() || m_passwordEdit->text().isEmpty())
    return;

  // Disable the login controls while credentials are being verified so the
  // flow cannot be re-entered (matches the original launcher).
  m_playButton->setEnabled(false);
  m_usernameEdit->setEnabled(false);
  m_passwordEdit->setEnabled(false);

  setStatus(launcher::kVerifyingCredentials);
  m_updater->login(m_usernameEdit->text(), m_passwordEdit->text(),
                   launcher::kDistribution);
}

void LauncherWindow::on_line_edit_username_returnPressed() {
  if (m_playButton->isEnabled()) {
    on_push_button_play_clicked();
  } else if (!m_usernameEdit->text().isEmpty()) {
    m_passwordEdit->setFocus();
  }
}

void LauncherWindow::on_line_edit_password_returnPressed() {
  if (m_playButton->isEnabled()) {
    on_push_button_play_clicked();
  } else if (!m_passwordEdit->text().isEmpty() &&
             m_usernameEdit->text().isEmpty()) {
    m_usernameEdit->setFocus();
  }
}

// ---------------------------------------------------------------------------
// Network callbacks
// ---------------------------------------------------------------------------

void LauncherWindow::onManifestFetched(bool ok) {
  if (!ok)
    return;

  // Compare the server's launcher-version with the built-in one. If the
  // launcher is outdated, inform the user and exit.
  if (!m_updater->launcherVersion.isEmpty() &&
      m_updater->launcherVersion != launcher::kVersion) {
    QMessageBox::information(this, launcher::kUpdateLauncherTitle,
                             launcher::kUpdateLauncherBody);
    QApplication::exit(1);
    return;
  }

  m_launcherVersionLabel->setText(m_updater->launcherVersion.isEmpty()
                                      ? QStringLiteral("N/A")
                                      : m_updater->launcherVersion);
  m_gameVersionLabel->setText(m_updater->gameVersion.isEmpty()
                                  ? QStringLiteral("N/A")
                                  : m_updater->gameVersion);
}

void LauncherWindow::onLoginFinished(const LoginResponse &response) {
  if (!response.success) {
    setStatus(QString::number(response.code) + QStringLiteral(": ") +
              response.detail);
    m_playButton->setEnabled(true);
    m_usernameEdit->setEnabled(true);
    m_passwordEdit->setEnabled(true);
    m_usernameEdit->setFocus();
    return;
  }

  m_cookie = response.cookie;

  // Patch game files, then start the game when finished.
  m_updater->startUpdate();
}

void LauncherWindow::onUpdateFinished() { startGame(); }

void LauncherWindow::onDownloadError(int error, const QString &errorString) {
  setStatus(QString::number(error) + QStringLiteral(": ") + errorString);
  m_playButton->setEnabled(true);
  m_usernameEdit->setEnabled(true);
  m_passwordEdit->setEnabled(true);
}

void LauncherWindow::onDownloadProgress(qint64 bytesRead, qint64 bytesTotal,
                                        const QString &status) {
  m_progressBar->setRange(0, bytesTotal);
  m_progressBar->setValue(bytesRead);
  setStatus(status);
}

void LauncherWindow::onStatusChanged(const QString &status) {
  setStatus(status);
}

void LauncherWindow::setStatus(const QString &status) {
  m_statusLabel->setText(status);
}

// ---------------------------------------------------------------------------
// Game launch
// ---------------------------------------------------------------------------

void LauncherWindow::startGame() {
  setStatus(launcher::kStartingGame);

  // Set the environment variables consumed by the game.
  qputenv("TTI_PLAYCOOKIE", m_cookie.toUtf8());
  qputenv("TTI_GAMESERVER", m_updater->gameServer.toUtf8());

  // Launch the game from the install directory.
  const QString exe =
      Updater::installDir() + QStringLiteral("/") + launcher::kGameExecutable;
  QProcess::startDetached(exe);

  close();
}
