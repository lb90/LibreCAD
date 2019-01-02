#ifndef QG_VIDEOWIDGET_H
#define QG_VIDEOWIDGET_H

#include <QWidget>

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

class QG_VideoWidget
 : public QWidget
{
public:
    QG_VideoWidget(QWidget *parent,
                   const char *name = NULL,
                   Qt::WindowFlags f = 0);
    QG_VideoWidget(const QG_VideoWidget&) = delete;
    void operator=(const QG_VideoWidget&) = delete;

signals:
    void escape();
private:
    QStackedLayout *source_stack;
    QComboBox      *camera_combo;
    QLineEdit      *file_edit;
    QToolButton    *source_prev;
    QToolButton    *source_next;
    QToolButton    *stop_button;
    QToolButton    *pause_button;
    QToolButton    *play_button;

/*slots:*/
    void source_page_prev();
    void source_page_next();
    void browse_file();
    void stop();
    void pause();
    void play();

    void subWindowChanged(QMdiSubWindow*);
};

#endif
