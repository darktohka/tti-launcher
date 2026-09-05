#include "draggablewindow.h"

#include <QMouseEvent>
#include <QWindow>

DraggableWindow::DraggableWindow(QWidget *parent) : QMainWindow(parent) {}

void DraggableWindow::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    // On X11 and Wayland, hand the drag to the window manager/compositor via
    // startSystemMove(). Programmatic QWidget::move() is ignored by Wayland
    // compositors, so it cannot be used as the primary mechanism there.
    if (windowHandle() && windowHandle()->startSystemMove()) {
      event->accept();
      return;
    }
    // Fallback (platforms without startSystemMove support).
    m_dragging = true;
    m_dragOffset =
        event->globalPosition().toPoint() - frameGeometry().topLeft();
    event->accept();
    return;
  }
  QMainWindow::mousePressEvent(event);
}

void DraggableWindow::mouseMoveEvent(QMouseEvent *event) {
  if (m_dragging && (event->buttons() & Qt::LeftButton)) {
    move(event->globalPosition().toPoint() - m_dragOffset);
    event->accept();
    return;
  }
  QMainWindow::mouseMoveEvent(event);
}

void DraggableWindow::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_dragging = false;
    event->accept();
    return;
  }
  QMainWindow::mouseReleaseEvent(event);
}