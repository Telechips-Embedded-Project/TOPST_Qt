#ifndef ACCOUNTSELECTPAGE_H
#define ACCOUNTSELECTPAGE_H

// new
#include <QPixmap>
#include <QLayout>

#include <QWidget>
#include <QStringList>

namespace Ui { class AccountSelectPage; }

class AccountSelectPage : public QWidget
{
    Q_OBJECT
public:
    explicit AccountSelectPage(QWidget *parent = nullptr);
    ~AccountSelectPage();

signals:
//    void accountSelected(const QString &accountName);
    void accountSelected(int accountId, const QString &accountName);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void on_pushButton_addNew_clicked();

private:
    // new
    void refreshAccountButtons();
    static void clearLayout(QLayout* lay);

    QString pickRandomImagePath() const;
    static QPixmap makeRoundedCover(const QString& path, const QSize& size, int radius);
    QWidget* makeAccountButtonWidget(const QString& name, int id, const QSize& btnSize,
                                     const QString& imagePath);

    bool m_deleteMode = false;     // 삭제모드 on/off
    void enterDeleteMode();
    void exitDeleteMode();
    void confirmDelete(int accountId, const QString& name);

private:
    Ui::AccountSelectPage *ui;

    const int MAX_TOTAL_ACCOUNTS = 5;
    int currentAccountCount = 0;
};

#endif // ACCOUNTSELECTPAGE_H
