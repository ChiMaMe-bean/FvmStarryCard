#include "custombutton.h"
#include <QMouseEvent>

CustomButton::CustomButton(QWidget *parent) : QPushButton(parent)
{
}

void CustomButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit leftButtonReleased();
    }
    QPushButton::mouseReleaseEvent(event);
}
