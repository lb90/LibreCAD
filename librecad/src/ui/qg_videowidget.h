#ifndef QG_VIDEOWIDGET_H
#define QG_VIDEOWIDGET_H

#include <QWidget>

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

};

#endif
