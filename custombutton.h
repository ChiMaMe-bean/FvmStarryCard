#ifndef CUSTOMBUTTON_H
#define CUSTOMBUTTON_H

#include <QPushButton>

class CustomButton : public QPushButton
{
    Q_OBJECT
public:
    explicit CustomButton(QWidget *parent = nullptr);

signals:
    void leftButtonReleased();

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
};

#endif // CUSTOMBUTTON_H
