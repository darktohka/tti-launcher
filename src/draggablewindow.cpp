#include "draggablewindow.h"

#include <QMouseEvent>

DraggableWindow::DraggableWindow(QWidget *parent) : QMainWindow(parent) {}

void DraggableWindow::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
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