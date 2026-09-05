#pragma once

#include "draggablewindow.h"

#include <QMainWindow>
#include <QUrl>

class QLineEdit;
class QPushButton;
class QProgressBar;
class QLabel;
class QNetworkAccessManager;
class Updater;

// Main launcher window.
class LauncherWindow : public DraggableWindow {
  Q_OBJECT

public:
  explicit LauncherWindow(QWidget *parent = nullptr);
  ~LauncherWindow() override;

private slots:
  void on_push_button_close_clicked();
  void on_push_button_minimize_clicked();
  void on_push_button_home_page_clicked();
  void on_push_button_report_a_bug_clicked();
  void on_push_button_play_clicked();
  void on_line_edit_username_returnPressed();
  void on_line_edit_password_returnPressed();

  void onManifestFetched(bool ok);
  void onLoginFinished(const struct LoginResponse &response);
  void onDownloadError(int error, const QString &errorString);
  void onDownloadProgress(qint64 bytesRead, qint64 bytesTotal,
                          const QString &status);
  void onUpdateFinished();
  void onStatusChanged(const QString &status);

private:
  void setupUi();
  void startGame();
  void setStatus(const QString &status);

  QWidget *m_window = nullptr;
  QPushButton *m_closeButton = nullptr;
  QPushButton *m_minimizeButton = nullptr;
  QLineEdit *m_usernameEdit = nullptr;
  QLineEdit *m_passwordEdit = nullptr;
  QPushButton *m_reportBugButton = nullptr;
  QPushButton *m_homePageButton = nullptr;
  QPushButton *m_playButton = nullptr;
  QProgressBar *m_progressBar = nullptr;
  QLabel *m_statusLabel = nullptr;
  QLabel *m_gameVersionLabel = nullptr;
  QLabel *m_launcherVersionLabel = nullptr;
  QPushButton *m_contentPacksButton = nullptr;
  QPushButton *m_optionsButton = nullptr;

  QNetworkAccessManager *m_nam = nullptr;
  Updater *m_updater = nullptr;

  QString m_cookie;
};