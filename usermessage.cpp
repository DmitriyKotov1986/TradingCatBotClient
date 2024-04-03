#include "usermessage.h"
#include "ui_usermessage.h"

UserMessage::UserMessage(MessageType type, const QString &title, const QString &message, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::UserMessage)
    , _type(type)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);

    ui->titleLabel->setText(title);
    ui->textLabel->setText(message);

    switch (type)
    {
    case MessageType::INFORMATION:
    {
        break;
    }
    case MessageType::ERROR:
    {
        break;
    }
    case MessageType::QUESTION:
    {
        break;
    }
    default:
        Q_ASSERT(false);
    }

    QObject::connect(ui->okPushButton, SIGNAL(clicked()), SLOT(ok_clicked()));
    QObject::connect(ui->cancelPushButton, SIGNAL(clicked()), SLOT(cancel_clicked()));
}

UserMessage::~UserMessage()
{
    delete ui;
}

void UserMessage::ok_clicked()
{

}

void UserMessage::cancel_clicked()
{

}
