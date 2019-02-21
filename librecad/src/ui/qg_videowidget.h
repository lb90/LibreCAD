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
class QRadioButton;
class QSlider;
class QSpinBox;
class QCheckBox;
class QMdiSubWindow;
QT_END_NAMESPACE

class QG_GraphicView;
class Gst;

class QG_VideoWidget
 : public QWidget
{
Q_OBJECT
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
    QRadioButton   *radio_view_upperleft;
    QRadioButton   *radio_view_centered;
    QRadioButton   *radio_doc_upperleft;
    QRadioButton   *radio_doc_centered;
    QCheckBox      *check_off_x;
    QSlider        *slider_off_x;
    QSpinBox       *spin_off_x;
    QCheckBox      *check_off_y;
    QSlider        *slider_off_y;
    QSpinBox       *spin_off_y;
    QCheckBox      *check_zoom;
    QSlider        *slider_zoom;
    QSpinBox       *spin_zoom;

    QG_GraphicView *view {nullptr};

private:
    void set_source_part_enabled(bool);

    void on_stopped();
    void on_paused();
    void on_playing();

    void source_page_prev();
    void source_page_next();
    void browse_file();
    void stop();
    void pause();
    void play();

private:
    void on_check_off_x_changed(int);
    void on_check_off_y_changed(int);
    void on_check_zoom_changed(int);
    void on_off_x_changed(bool, int);
    void on_off_y_changed(bool, int);
    void on_zoom_changed(bool, int);

private slots:
    void PipelineStateChanged(int);

private:
    Gst *gst {nullptr};
};

#endif
