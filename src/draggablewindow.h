#pragma once

#include <QMainWindow>

class QMouseEvent;

// Frameless, draggable window base class.
// Mirrors the "DraggableWindow" class of the original launcher.
class DraggableWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DraggableWindow(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_dragging = false;
    QPoint m_dragOffset;
};