#include "qnamedflags.h"
#include <QJsonObject>
#include <QJsonArray>

QNamedFlags::QNamedFlags(QObject *parent) :
    QObject(parent)
{
    _names.reserve(BitCount);
    for (int i = 0; i < BitCount; i++)
        _names.append(QString());
    memset(_bits, 0, sizeof(_bits));
}

QNamedFlags::QNamedFlags(const QNamedFlags& other) :
    QNamedFlags(0)
{
    copy(other);
}

bool
QNamedFlags::hasName(uint bit) const
{
    if (invalidBit(bit))
        return false;
    return _bits[bit] != 0;
}

int
QNamedFlags::length(uint bit) const
{
    if (invalidBit(bit))
        return 0;
    return _bits[bit];
}

QString
QNamedFlags::name() const
{
    return _name;
}

QString
QNamedFlags::operator[](uint bit) const
{
    if ((int)bit >= _names.count() || _bits[bit] == 0)
        return QString();
    if (_bits[bit] == 1)
        return _names[bit];
    if (_bits[bit] >  1)
        return QString(_names[bit]).append("[0]");
    if (_bits[bit] <  0)
        return QString(_names[bit + _bits[bit]]).append("[%1]").arg(-(int)_bits[bit]);
    return QString();
}

QNamedFlags&
QNamedFlags::operator=(const QNamedFlags& other)
{
    copy(other);
    return *this;
}

void
QNamedFlags::copy(const QNamedFlags& other)
{
    if (&other != this) {
        _name  = other._name;
        _names = other._names;
        memcpy(_bits, other._bits, sizeof(_bits));
    }
}


void
QNamedFlags::reset(const QString &name)
{
    for (int i = 0; i < BitCount; i++) {
        _bits [i] = 0;
        _names[i].clear();
    }
    _name = name;
}

bool
QNamedFlags::load(const QJsonObject &root)
{
    QJsonArray bits = root.value("bits").toArray();
    if (bits.empty())
        return false;
    QString name = root.value("name").toString();
    if (name.isEmpty())
        return false;

    reset(name);
    bool loaded = false;
    for (int i = 0; i < bits.count(); i++) {
        QJsonObject o = bits.at(i).toObject();
        int bit =  o.value("bit").toDouble(-1.0);
        if (bit < 0 || bit >= BitCount)
            continue;
        name = o.value("name").toString();
        if (name.isEmpty())
            continue;
        int length = o.value("length").toDouble(1.0);
        if (length <= 0 || bit + length > BitCount)
            continue;
        _names[bit] = name;
        _bits [bit] = length;
        for (int b = 1; b < length; b++)
            _bits[bit+b] = -b;
        loaded = true;
    }
    return loaded;
}
