#ifndef QG_VIDEOWIDGET_H
#define QG_VIDEOWIDGET_H

#include <QWidget>
#include "gst.h"

QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QHBoxLayout;
class QStackedLayout;
class QLabel;
class QComboBox;
class QLineEdit;
class QToolButton;
class QMdiSubWindow;
QT_END_NAMESPACE

class QG_GraphicView;

class QG_VideoWidget
 : public QWidget
{
public:
    QG_VideoWidget(QWidget *parent,
                   const char *name = NULL,
                   Qt::WindowFlags f = 0);
    QG_VideoWidget(const QG_VideoWidget&) = delete;
    void operator=(const QG_VideoWidget&) = delete;

    void setGraphicView(QG_GraphicView* view);

signals:
    void escape();

private:
    QWidget        *source_widget;
      QStackedLayout *source_stack;
        QComboBox      *camera_combo;
        QToolButton    *refresh_button;
        QLineEdit      *file_edit;
        QToolButton    *open_button;
    QToolButton    *source_prev;
    QToolButton    *source_next;
    QToolButton    *stop_button;
    QToolButton    *pause_button;
    QToolButton    *play_button;

    QG_GraphicView *view {nullptr};

private:
    void set_source_part_enabled(bool);

    void on_stopped();
    void on_paused();
    void on_playing();
/*slots:*/
    void source_page_prev();
    void source_page_next();
    void browse_file();
    void stop();
    void pause();
    void play();

    void subWindowActivated(QMdiSubWindow*);
    void PipelineStateChanged(int);
private:
    Gst *gst {nullptr};
};

#endif
