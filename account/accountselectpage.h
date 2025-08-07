#ifndef ACCOUNTSELECTPAGE_H
#define ACCOUNTSELECTPAGE_H

#include <QWidget>

#include <QStringList>
#include <QRandomGenerator>

namespace Ui {
class AccountSelectPage;
}

class AccountSelectPage : public QWidget
{
    Q_OBJECT

public:
    explicit AccountSelectPage(QWidget *parent = nullptr);
    ~AccountSelectPage();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void on_pushButton_addNew_clicked();

signals:
    void accountSelected(const QString &accountName);

private:
    Ui::AccountSelectPage *ui;

    QString btnStyleSheet;

    const int MAX_TOTAL_ACCOUNTS = 5;  // MAX account : 5EA
    int currentAccountCount = 0;
};

#endif // ACCOUNTSELECTPAGE_H
