#include "qg_videowidget.h"

#include <QBoxLayout>
#include <QStackedLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QToolButton>
#include <QMdiSubWindow>

#include <QFileDialog>

static QWidget*
make_widget(QLayout *layout) {
    QWidget *widget = new QWidget();
    widget->setLayout(layout);
    return widget;
}
/*
static QWidget*
make_widget_expanding(QLayout *layout, QSizePolicy::Policy expanding) {
    QWidget *widget = make_widget(layout);
    widget->setSizePolicy(expanding, QSizePolicy::Fixed);
    return widget;
}
*/

QG_VideoWidget::QG_VideoWidget(QWidget *parent,
                               const char *name,
                               Qt::WindowFlags f)
 : QWidget(parent, f)
{
    setObjectName(name);

    QHBoxLayout *camera_source_hbox = new QHBoxLayout();
    camera_source_hbox->addWidget(new QLabel("Camera"));
      camera_combo = new QComboBox();
      camera_combo->setSizePolicy(QSizePolicy::Expanding,
                                  QSizePolicy::Fixed);
    camera_source_hbox->addWidget(camera_combo);
      QToolButton *refresh_button = new QToolButton();
      refresh_button->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    camera_source_hbox->addWidget(refresh_button);

    QHBoxLayout *file_source_hbox = new QHBoxLayout();
    file_source_hbox->addWidget(new QLabel("File"));
      file_edit = new QLineEdit();
      file_edit->setReadOnly(true);
    file_source_hbox->addWidget(file_edit);
      QToolButton *open_button = new QToolButton();
      open_button->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    file_source_hbox->addWidget(open_button);

    source_stack = new QStackedLayout();
    source_stack->addWidget(make_widget(camera_source_hbox));
    source_stack->addWidget(make_widget(file_source_hbox));
    source_stack->setCurrentIndex(0);

    QHBoxLayout *page_hbox = new QHBoxLayout();
      source_prev = new QToolButton();
      source_prev->setIcon(style()->standardIcon(QStyle::SP_ArrowLeft));
    page_hbox->addWidget(source_prev);
      source_next = new QToolButton();
      source_next->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
    page_hbox->addWidget(source_next);

    QHBoxLayout *control_hbox = new QHBoxLayout();
      stop_button = new QToolButton();
      stop_button->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    control_hbox->addWidget(stop_button);
      pause_button = new QToolButton();
      pause_button->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    control_hbox->addWidget(pause_button);
      play_button = new QToolButton();
      play_button->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    control_hbox->addWidget(play_button);

    QVBoxLayout *main_vbox = new QVBoxLayout();
    main_vbox->addWidget(make_widget(source_stack));
    main_vbox->addWidget(make_widget(page_hbox), 0, Qt::AlignRight);
    main_vbox->addWidget(make_widget(control_hbox), 0, Qt::AlignCenter);
    setLayout(main_vbox);

    source_prev->setEnabled(false);
    stop_button->setEnabled(false);
    pause_button->setEnabled(false);
    play_button->setEnabled(false);

    connect(source_prev, &QAbstractButton::clicked, this, &QG_VideoWidget::source_page_prev);
    connect(source_next, &QAbstractButton::clicked, this, &QG_VideoWidget::source_page_next);

    connect(open_button, &QAbstractButton::clicked, this, &QG_VideoWidget::browse_file);

    connect(stop_button, &QAbstractButton::clicked, this, &QG_VideoWidget::stop);
    connect(pause_button, &QAbstractButton::clicked, this, &QG_VideoWidget::pause);
    connect(play_button, &QAbstractButton::clicked, this, &QG_VideoWidget::play);
    /* refresh camera list */
}

void QG_VideoWidget::source_page_prev() {
#if 1
    /* useful when you only have 2 pages in the stackedlayout */
    auto new_index = source_stack->currentIndex() - 1;
    if (new_index >= 0) {
        source_stack->setCurrentIndex(new_index);
        source_next->setEnabled(true);
    }
    if (source_stack->currentIndex() <= 0)
        source_prev->setEnabled(false);
#else
    /* useful when you only have >2 pages in the stackedlayout */
#endif
}
void QG_VideoWidget::source_page_next() {
#if 1
    /* useful when you only have 2 pages in the stackedlayout */
    auto new_index = source_stack->currentIndex() + 1;
    if (new_index < source_stack->count()) {
        source_stack->setCurrentIndex(new_index);
        source_prev->setEnabled(true);
    }
    if (source_stack->currentIndex() == source_stack->count() - 1)
        source_next->setEnabled(false);
#else
    /* useful when you only have >2 pages in the stackedlayout */
#endif
}

void QG_VideoWidget::browse_file() {
    QString fileName = QFileDialog::getOpenFileName(this, "Apri file video", QString(),
                                                    "File video (*.mpeg *.avi *.mpg);;"
                                                    "Tutti i file (*.*)");
    if (!fileName.isEmpty()) {
        file_edit->setText(fileName);
        play_button->setEnabled(true);
    }
}

void QG_VideoWidget::stop() {

    stop_button->setEnabled(false);
    pause_button->setEnabled(false);
    play_button->setEnabled(true);
}
void QG_VideoWidget::pause() {

    stop_button->setEnabled(true);
    pause_button->setEnabled(false);
    play_button->setEnabled(true);
}
void QG_VideoWidget::play() {

    stop_button->setEnabled(true);
    pause_button->setEnabled(true);
    play_button->setEnabled(false);
}

void QG_VideoWidget::subWindowChanged(QMdiSubWindow*) {

}
