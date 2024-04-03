#ifndef USERMESSAGE_H
#define USERMESSAGE_H

#include <QDialog>

namespace Ui {
class UserMessage;
}

class UserMessage
    : public QDialog
{
    Q_OBJECT
public:
    enum class MessageType
    {
        INFORMATION,
        ERROR,
        QUESTION
    };
public:
    explicit UserMessage(MessageType type, const QString& title, const QString& message, QWidget *parent = nullptr);
    ~UserMessage();

private slots:
    void ok_clicked();
    void cancel_clicked();

private:
    Ui::UserMessage *ui;

    MessageType _type  = MessageType::INFORMATION;
};

#endif // USERMESSAGE_H
