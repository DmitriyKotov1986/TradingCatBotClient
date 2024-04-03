#ifndef LOCALCONFIG_H
#define LOCALCONFIG_H

#include <QString>
#include <QByteArray>

#include <emscripten.h>
#include <emscripten/val.h>

class LocalConfig
{
public:
    LocalConfig();

    const QString& user() const;
    void setUser(const QString& user);
    const QString& password() const ;
    void setPassword(const QString& password);
    bool autoLogin() const;
    void setAutoLogin(bool autoLogin);
    QByteArray splitterPos() const;
    void setSplitterPos(const QByteArray& newPos);

private:
    void saveValue(const QString& key, const QString& value);
    QString loadValue(const QString& key);

private:
    QString _user;
    QString _password;
    bool _autoLogin = false;
    QByteArray _splitterPos;

    emscripten::val _localStorage;
};

#endif // LOCALCONFIG_H
