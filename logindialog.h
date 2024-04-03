#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT
public:
    enum class Mode
    {
        LOGIN_USER,
        NEW_USER
    };

public:
    explicit LoginDialog(LoginDialog::Mode mode, const QString& title, const QString& user, const QString& password, bool autoLogin, QWidget *parent = nullptr);
    ~LoginDialog();

    const QString& user() const;
    const QString& password() const;
    bool autoLogin() const;

private slots:
    void ok_clicked();
    void cancel_clicked();
    void newUser_clicked();
    void user_textChanged(const QString& text);
    void password_textChanged(const QString& text);

private:
    void okEnabled();

private:
    Ui::LoginDialog *ui;

    LoginDialog::Mode _mode = Mode::LOGIN_USER;

    QString _user = "anonymous";
    QString _password = "anonymous";
    bool _autoLogin = false;
};

#endif // LOGINDIALOG_H
