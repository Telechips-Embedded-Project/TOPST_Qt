#ifndef MAINCONTAINER_H
#define MAINCONTAINER_H

#include <QWidget>
#include <QPixmap>

class MainContainer : public QWidget
{
    Q_OBJECT
public:
    explicit MainContainer(QWidget *parent = nullptr);

public slots:
    void setBackground(const QString& path);
    void setBackground(const QPixmap& pm);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap m_background;
};

#endif // MAINCONTAINER_H
