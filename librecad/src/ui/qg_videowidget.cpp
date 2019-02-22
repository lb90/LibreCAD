#include "qg_videowidget.h"
#include "qg_graphicview.h"
#include "lc_application.h"
#include "gst.h"
#include "videopipelinemoniker.h"
#include "videopipeline.h"

#include <QBoxLayout>
#include <QStackedLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QToolButton>
#include <QGroupBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QMdiSubWindow>

#include <QFileDialog>
#include <QMessageBox>

#include <functional>

static QWidget*
make_widget(QLayout *layout) {
    QWidget *widget = new QWidget();
    widget->setLayout(layout);
    return widget;
}

QG_VideoWidget::QG_VideoWidget(QWidget *parent,
                               const char *name,
                               Qt::WindowFlags f)
 : QWidget(parent, f)
{
    setObjectName(name);

    gst = ((LC_Application*)qApp)->gst();
    if (!gst)
        throw std::runtime_error("NULL gst pointer in QG_VideoWidget");

    QHBoxLayout *camera_source_hbox = new QHBoxLayout();
    camera_source_hbox->addWidget(new QLabel("Camera"));
      camera_combo = new QComboBox();
      camera_combo->setSizePolicy(QSizePolicy::Expanding,
                                  QSizePolicy::Fixed);
    camera_source_hbox->addWidget(camera_combo);
      refresh_button = new QToolButton();
      refresh_button->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    camera_source_hbox->addWidget(refresh_button);

    QHBoxLayout *file_source_hbox = new QHBoxLayout();
    file_source_hbox->addWidget(new QLabel("File"));
      file_edit = new QLineEdit();
      file_edit->setReadOnly(true);
    file_source_hbox->addWidget(file_edit);
      open_button = new QToolButton();
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
      stop_button->setFixedSize(QSize(40,40));
      stop_button->setIconSize(QSize(40,40));
      stop_button->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    control_hbox->addWidget(stop_button);
      pause_button = new QToolButton();
      pause_button->setFixedSize(QSize(40,40));
      pause_button->setIconSize(QSize(40,40));
      pause_button->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    control_hbox->addWidget(pause_button);
      play_button = new QToolButton();
      play_button->setFixedSize(QSize(40,40));
      play_button->setIconSize(QSize(40,40));
      play_button->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    control_hbox->addWidget(play_button);

    QGroupBox *groupbox = new QGroupBox("Posizionamento");
    //groupbox->setChecked(true);
    radio_view_upperleft = new QRadioButton("View UpperLeft");
    radio_view_centered = new QRadioButton("View Centered");
    radio_doc_upperleft = new QRadioButton("Doc UpperLeft");
    radio_doc_centered = new QRadioButton("Doc Centered");
    QVBoxLayout *vbox_pos = new QVBoxLayout();
    vbox_pos->addWidget(radio_view_upperleft);
    vbox_pos->addWidget(radio_view_centered);
    vbox_pos->addWidget(radio_doc_upperleft);
    vbox_pos->addWidget(radio_doc_centered);
    vbox_pos->addStretch(1);
    groupbox->setLayout(vbox_pos);
    connect(radio_view_upperleft, &QAbstractButton::toggled, this, &QG_VideoWidget::on_radio_position_toggled);
    connect(radio_view_centered, &QAbstractButton::toggled, this, &QG_VideoWidget::on_radio_position_toggled);
    connect(radio_doc_upperleft, &QAbstractButton::toggled, this, &QG_VideoWidget::on_radio_position_toggled);
    connect(radio_doc_centered, &QAbstractButton::toggled, this, &QG_VideoWidget::on_radio_position_toggled);

    QHBoxLayout *hbox_off_x = new QHBoxLayout();
    QHBoxLayout *hbox_off_y = new QHBoxLayout();
    QHBoxLayout *hbox_zoom = new QHBoxLayout();
    check_off_x = new QCheckBox("Offset X");
    check_off_y = new QCheckBox("Offset Y");
    check_zoom = new QCheckBox("Zoom");
    slider_off_x = new QSlider(Qt::Horizontal);
    slider_off_y = new QSlider(Qt::Horizontal);
    slider_zoom = new QSlider(Qt::Horizontal);
    spin_off_x = new QSpinBox();
    spin_off_y = new QSpinBox();
    spin_zoom = new QSpinBox();
    hbox_off_x->addWidget(check_off_x);
    hbox_off_x->addWidget(slider_off_x);
    hbox_off_x->addWidget(spin_off_x);
    hbox_off_y->addWidget(check_off_y);
    hbox_off_y->addWidget(slider_off_y);
    hbox_off_y->addWidget(spin_off_y);
    hbox_zoom->addWidget(check_zoom);
    hbox_zoom->addWidget(slider_zoom);
    hbox_zoom->addWidget(spin_zoom);
    QVBoxLayout *vbox_off_zoom = new QVBoxLayout();
    vbox_off_zoom->addWidget(make_widget(hbox_off_x));
    vbox_off_zoom->addWidget(make_widget(hbox_off_y));
    vbox_off_zoom->addWidget(make_widget(hbox_zoom));
    slider_off_x->setRange(-1000,1000);
    slider_off_y->setRange(-1000,1000);
    slider_zoom->setRange(-90,90);
    spin_off_x->setRange(-1000,1000);
    spin_off_y->setRange(-1000,1000);
    spin_zoom->setRange(-90,90);
    connect(check_off_x, &QCheckBox::stateChanged, this, &QG_VideoWidget::on_check_off_x_changed);
    connect(check_off_y, &QCheckBox::stateChanged, this, &QG_VideoWidget::on_check_off_y_changed);
    connect(check_zoom, &QCheckBox::stateChanged, this, &QG_VideoWidget::on_check_zoom_changed);
    connect(slider_off_x, &QAbstractSlider::valueChanged, this, std::bind(&QG_VideoWidget::on_off_x_changed, this, true, std::placeholders::_1));
    connect(slider_off_y, &QAbstractSlider::valueChanged, this, std::bind(&QG_VideoWidget::on_off_y_changed, this, true, std::placeholders::_1));
    connect(slider_zoom, &QAbstractSlider::valueChanged, this, std::bind(&QG_VideoWidget::on_zoom_changed, this, true, std::placeholders::_1));
    connect(spin_off_x, QOverload<int>::of(&QSpinBox::valueChanged), this, std::bind(&QG_VideoWidget::on_off_x_changed, this, false, std::placeholders::_1));
    connect(spin_off_y, QOverload<int>::of(&QSpinBox::valueChanged), this, std::bind(&QG_VideoWidget::on_off_y_changed, this, false, std::placeholders::_1));
    connect(spin_zoom, QOverload<int>::of(&QSpinBox::valueChanged), this, std::bind(&QG_VideoWidget::on_zoom_changed, this, false, std::placeholders::_1));

    QVBoxLayout *main_vbox = new QVBoxLayout();
    source_widget = make_widget(source_stack);
    main_vbox->addWidget(source_widget);
    main_vbox->addWidget(make_widget(page_hbox), 0, Qt::AlignRight);
    main_vbox->addWidget(make_widget(control_hbox), 0, Qt::AlignCenter);
    main_vbox->addWidget(groupbox);
    main_vbox->addWidget(make_widget(vbox_off_zoom));
    main_vbox->addStretch();
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

    /*TODO refresh camera list */
    std::vector<std::string> camera_names = gst->get_camera_names();
    QStringList cameras;
    for (const std::string& camera_name : camera_names)
        cameras.append(QString(camera_name.c_str()));
    camera_combo->addItems(cameras);
    /*TODO always select in combobox?*/

    check_off_x->setChecked(false);
    check_off_y->setChecked(false);
    check_zoom->setChecked(false);
    slider_off_x->setValue(0);
    slider_off_y->setValue(0);
    slider_zoom->setValue(0);
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
    /* useful when you have more than 2 pages in the stackedlayout */
    #error "implement me!"
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
    /* useful when you have more than 2 pages in the stackedlayout */
    #error "implement me!"
#endif
}

void QG_VideoWidget::browse_file() {
    QString fileName = QFileDialog::getOpenFileName(this, "Apri file video", QString());
    if (!fileName.isEmpty()) {
        file_edit->setText(fileName);
        play_button->setEnabled(true);
    }
}

void QG_VideoWidget::stop() {
    if (!view)
        return;
    if (!view->get_video_moniker().active())
        return;

    stop_button->setEnabled(true);
    pause_button->setEnabled(false);
    play_button->setEnabled(false);

    view->get_video_moniker().reset();
}
void QG_VideoWidget::pause() {
    if (!view)
        return;
    if (!view->get_video_moniker().active())
        return;

    stop_button->setEnabled(true);
    pause_button->setEnabled(true);
    play_button->setEnabled(false);

    view->get_video_moniker().pause();
}
void QG_VideoWidget::play() {
    if (!view)
        return;

    if (!view->get_video_moniker().active()) {
        switch (source_stack->currentIndex()) {
            case 0: { /*CAMERA*/
                int index = camera_combo->currentIndex();
                if (index < 0) {
                    QMessageBox msgBox;
                    if (camera_combo->count()) { /* ci sono camera nel sistema */
                        msgBox.setText("Selezionare una camera");
                        msgBox.exec();
                        camera_combo->setFocus();
                    }
                    else {
                        msgBox.setText("Non sono presenti sorgenti camera nel sistema");
                        msgBox.exec();
                    }
                    return;
                }
                view->get_video_moniker().wrap_camera_pipeline(index);
            } break;
            case 1: { /*FILE*/
                if (file_edit->text().isEmpty()) {
                    QMessageBox msgBox;
                    msgBox.setText("Selezionare un file");
                    msgBox.exec();
                    open_button->setFocus();
                    return;
                }
                view->get_video_moniker().wrap_file_pipeline(file_edit->text().toStdString());
            } break;
            default: {
                QMessageBox msgBox;
                msgBox.setText("Errore interno: pagina sorgente fuori intervallo");
                msgBox.exec();
            }
        }
        if (!view->get_video_moniker().active())
            return;

        connect(&view->get_video_moniker(), &VideoPipelineMoniker::StateChanged,
                this, &QG_VideoWidget::PipelineStateChanged);
    }

    view->get_video_moniker().play();
}

void QG_VideoWidget::setGraphicView(QG_GraphicView* graphicView) {
    /* if it is the same as current don't do anything */
    /* protects from spurious invokations */
    if (view == graphicView)
        return;

    /* if there was a view before detach from its pipeline state changed */
    if (view) {
        if (view->get_video_moniker().active()) {
            disconnect(&view->get_video_moniker(), &VideoPipelineMoniker::StateChanged,
                       this, &QG_VideoWidget::PipelineStateChanged);
        }
    }

    view = graphicView;

    if (!view) {
        file_edit->setText(QString());
        radio_doc_centered->setCheckable(false);
        radio_doc_upperleft->setCheckable(false);
        radio_view_centered->setCheckable(false);
        radio_view_upperleft->setCheckable(false);
        check_off_x->setChecked(false);
        check_off_y->setChecked(false);
        check_zoom->setChecked(false);
        slider_off_x->setValue(0);
        slider_off_y->setValue(0);
        slider_zoom->setValue(0);
        setEnabled(false);
        return;
    }

    radio_doc_centered->setCheckable(true);
    radio_doc_upperleft->setCheckable(true);
    radio_view_centered->setCheckable(true);
    radio_view_upperleft->setCheckable(true);
    switch (view->video().position) {
    case QG_GraphicView::Video::Position::doc_centered:
        radio_doc_centered->setChecked(true); break;
    case QG_GraphicView::Video::Position::doc_upperleft:
        radio_doc_upperleft->setChecked(true); break;
    case QG_GraphicView::Video::Position::view_centered:
        radio_view_centered->setChecked(true); break;
    case QG_GraphicView::Video::Position::view_upperleft:
        radio_view_upperleft->setChecked(true); break;
    }
    check_off_x->setChecked(view->video().off_x_set);
    check_off_y->setChecked(view->video().off_y_set);
    check_zoom->setChecked(view->video().zoom_set);
    emit check_off_x->stateChanged(check_off_x->checkState());
    emit check_off_y->stateChanged(check_off_y->checkState());
    emit check_zoom->stateChanged(check_zoom->checkState());
    slider_off_x->setValue(view->video().off_x);
    slider_off_y->setValue(view->video().off_y);
    slider_zoom->setValue(view->video().zoom);

    if (view->get_video_moniker().active()) {
        connect(&view->get_video_moniker(), &VideoPipelineMoniker::StateChanged,
                this, &QG_VideoWidget::PipelineStateChanged);

        /* state changed only notifies playing and paused if it ever got there.
         * we may be betweeen constructed and playing (so we're about-to-play),
         * in this case there is no state changed. */
        set_source_part_enabled(false);
        stop_button->setEnabled(true);
        pause_button->setEnabled(false);
        play_button->setEnabled(false);
        view->get_video_moniker().generate_state_changed();

        VideoPipeline *pipeline = view->get_video_moniker().get_pipeline();
        switch (pipeline->get_source_type()) {
        case VideoPipeline::SourceType::camera: {
            source_stack->setCurrentIndex(0);
            camera_combo->setCurrentIndex(pipeline->camera_index);
            file_edit->setText(QString());
        } break;
        case VideoPipeline::SourceType::file: {
            source_stack->setCurrentIndex(1);
            file_edit->setText(QString(pipeline->file_path.c_str()));
        } break;
        }
    }
    else {
        on_stopped();
    }
}

void QG_VideoWidget::PipelineStateChanged(int arg) {
    switch (arg) {
        case 2:
            on_stopped();
        break;
        case 1:
            on_paused();
        break;
        case 0:
            on_playing();
        break;
    }
}

void QG_VideoWidget::on_stopped() {
    set_source_part_enabled(true);

    stop_button->setEnabled(false);
    pause_button->setEnabled(false);
    play_button->setEnabled(true);
}

void QG_VideoWidget::on_paused() {
    set_source_part_enabled(false);

    stop_button->setEnabled(true);
    pause_button->setEnabled(false);
    play_button->setEnabled(true);
}

void QG_VideoWidget::on_playing() {
    set_source_part_enabled(false);

    stop_button->setEnabled(true);
    pause_button->setEnabled(true);
    play_button->setEnabled(false);
}

void QG_VideoWidget::set_source_part_enabled(bool arg) {
    source_widget->setEnabled(arg);
    if (!arg) {
        source_prev->setEnabled(false);
        source_next->setEnabled(false);
    }
    else {
        if (source_stack->currentIndex() > 0)
            source_prev->setEnabled(true);
        else
            source_prev->setEnabled(false);

        if (source_stack->currentIndex() < source_stack->count()-1)
            source_next->setEnabled(true);
        else
            source_next->setEnabled(false);
    }
}

void QG_VideoWidget::set_playing_part_enabled(bool arg) {

}

void QG_VideoWidget::on_radio_position_toggled(bool checked) {
    if (!view)
        return;
    if (checked) {
        if (radio_doc_centered->isChecked()) {
            view->video().position = QG_GraphicView::Video::Position::doc_centered;
        }
        else if (radio_doc_upperleft->isChecked()) {
            view->video().position = QG_GraphicView::Video::Position::doc_upperleft;
        }
        else if (radio_view_centered->isChecked()) {
            view->video().position = QG_GraphicView::Video::Position::view_centered;
        }
        else if (radio_view_upperleft->isChecked()) {
            view->video().position = QG_GraphicView::Video::Position::view_upperleft;
        }
    }
}

void QG_VideoWidget::on_check_off_x_changed(int) {
    if (!view)
        return;
    if (check_off_x->isChecked()) {
        slider_off_x->setEnabled(true);
        spin_off_x->setEnabled(true);
        view->video().off_x_set = true;
    }
    else {
        slider_off_x->setEnabled(false);
        spin_off_x->setEnabled(false);
        view->video().off_x_set = false;
    }
}

void QG_VideoWidget::on_check_off_y_changed(int) {
    if (!view)
        return;
    if (check_off_y->isChecked()) {
        slider_off_y->setEnabled(true);
        spin_off_y->setEnabled(true);
        view->video().off_y_set = true;
    }
    else {
        slider_off_y->setEnabled(false);
        spin_off_y->setEnabled(false);
        view->video().off_y_set = false;
    }
}

void QG_VideoWidget::on_check_zoom_changed(int) {
    if (!view)
        return;
    if (check_zoom->isChecked()) {
        slider_zoom->setEnabled(true);
        spin_zoom->setEnabled(true);
        view->video().zoom_set = true;
    }
    else {
        slider_zoom->setEnabled(false);
        spin_zoom->setEnabled(false);
        view->video().zoom_set = false;
    }
}

void QG_VideoWidget::on_off_x_changed(bool slider, int val) {
    static bool in_use = false;
    if (in_use)
        return;
    in_use = true;
    if (view)
        view->video().off_x = val;
    if (slider) {
        spin_off_x->setValue(val);
    }
    else
        slider_off_x->setValue(val);
    in_use = false;
}

void QG_VideoWidget::on_off_y_changed(bool slider, int val) {
    static bool in_use = false;
    if (in_use)
        return;
    in_use = true;
    if (view)
        view->video().off_y = val;
    if (slider)
        spin_off_y->setValue(val);
    else
        slider_off_y->setValue(val);
    in_use = false;
}

void QG_VideoWidget::on_zoom_changed(bool slider, int val) {
    static bool in_use = false;
    if (in_use)
        return;
    in_use = true;
    if (view)
        view->video().zoom = val;
    if (slider)
        spin_zoom->setValue(val);
    else
        slider_zoom->setValue(val);
    in_use = false;
}
