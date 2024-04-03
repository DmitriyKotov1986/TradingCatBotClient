#include "logindialog.h"
#include "ui_logindialog.h"

LoginDialog::LoginDialog(LoginDialog::Mode mode, const QString& title, const QString& user, const QString& password, bool autoLogin, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
    , _autoLogin(autoLogin)
    , _mode(mode)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);

    QObject::connect(ui->okPushButton, SIGNAL(clicked()), SLOT(ok_clicked()));
    QObject::connect(ui->cancelPushButton, SIGNAL(clicked()), SLOT(cancel_clicked()));
    QObject::connect(ui->newUserPushButton, SIGNAL(clicked()), SLOT(newUser_clicked()));

    QObject::connect(ui->userLineEdit, SIGNAL(textChanged(const QString &)), SLOT(user_textChanged(const QString&)));
    QObject::connect(ui->passwordLineEdit, SIGNAL(textChanged(const QString &)), SLOT(password_textChanged(const QString&)));

    ui->titleLabel->setText(title);
    ui->userLineEdit->setText(user.isEmpty() ? "anonymous" : user);
    ui->passwordLineEdit->setText(password.isEmpty() ? "anonymous" : password);
    ui->autoLoginCheckBox->setChecked(_autoLogin);

    switch (_mode)
    {
    case LoginDialog::Mode::LOGIN_USER:
        ui->checkPasswordLineEdit->setVisible(false);
        ui->checkPasswordLabel->setVisible(false);
        break;
    case LoginDialog::Mode::NEW_USER:
        ui->newUserPushButton->setVisible(false);
        break;
    default:
        break;
    }
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

const QString &LoginDialog::user() const
{
    return _user;
}

const QString &LoginDialog::password() const
{
    return _password;
}

bool LoginDialog::autoLogin() const
{
    return _autoLogin;
}

void LoginDialog::ok_clicked()
{
    _user = ui->userLineEdit->text();
    _user = _user.isEmpty() ? "anonymous" : _user;
    _password = ui->passwordLineEdit->text();
    _password = _password.isEmpty() ? "anonymous" : _password;
    _autoLogin = ui->autoLoginCheckBox->isChecked();

    if (_user.isEmpty() || _password.isEmpty())
    {
        return;
    }

    accept();
}

void LoginDialog::cancel_clicked()
{
    reject();
}

void LoginDialog::newUser_clicked()
{
    _mode = Mode::NEW_USER;
    ui->checkPasswordLineEdit->setVisible(true);
    ui->checkPasswordLabel->setVisible(true);
    ui->newUserPushButton->setVisible(false);
    ui->mainLayout->setSizeConstraint(QLayout::SetFixedSize);
}

void LoginDialog::user_textChanged(const QString &text)
{
    okEnabled();
}

void LoginDialog::password_textChanged(const QString &text)
{
    okEnabled();
}

void LoginDialog::okEnabled()
{
  //  ui->okPushButton->setEnabled((!ui->userLineEdit->text().isEmpty()) && (!ui->passwordLineEdit->text().isEmpty()));
    ui->okPushButton->setEnabled(true);
}
