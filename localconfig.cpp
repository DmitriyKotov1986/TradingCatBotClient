#include "localconfig.h"

using namespace emscripten;

static const QString CONFIG_FILE_NAME = "/offline/TraidingCatBot.ini";

LocalConfig::LocalConfig()
{
    _localStorage = val::global("window")["localStorage"];

    _user = QByteArray::fromBase64(loadValue("user").toUtf8());
    _password = QByteArray::fromBase64(loadValue("password").toUtf8());
    _autoLogin = loadValue("auto_login") == "true";
    _splitterPos = QByteArray::fromBase64(loadValue("splitter_pos").toUtf8());
}

const QString &LocalConfig::user() const
{
    return _user;
}

void LocalConfig::setUser(const QString &user)
{
    _user = user;
    saveValue("user", _user.toUtf8().toBase64());
}

const QString &LocalConfig::password() const
{
    return _password;
}

void LocalConfig::setPassword(const QString &password)
{
    _password = password;
    saveValue("password", _password.toUtf8().toBase64());
}

bool LocalConfig::autoLogin() const
{
    return _autoLogin;
}

void LocalConfig::setAutoLogin(bool autoLogin)
{
    _autoLogin = autoLogin;
    saveValue("auto_login", _autoLogin ? "true" : "false");
}

QByteArray LocalConfig::splitterPos() const
{
    return _splitterPos;
}

void LocalConfig::setSplitterPos(const QByteArray& newPos)
{
    _splitterPos= newPos;
    saveValue("splitter_pos", QString(_splitterPos.toBase64()));
}

void LocalConfig::saveValue(const QString &key, const QString &value)
{
    const std::string keyString = key.toStdString();
    const std::string valueString = value.toStdString();
    _localStorage.call<void>("setItem", keyString, valueString);
}

QString LocalConfig::loadValue(const QString &key)
{
    const std::string keyString = key.toStdString();
    const emscripten::val value = _localStorage.call<val>("getItem", keyString);
    if (value.isNull())
    {
        return "";
    }

    return QString::fromStdString(value.as<std::string>());
}
