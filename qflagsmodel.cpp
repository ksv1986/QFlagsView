#include "qflagsmodel.h"
#include "qnamedflags.h"

QFlagsModel::QFlagsModel(QObject *parent) :
    QObject(parent),
    _value(0)
{
}

uint64_t
QFlagsModel::value() const {
    return _value;
}

void
QFlagsModel::setFlags(const QNamedFlags &flags)
{
    _bits = flags;
}

const QString
QFlagsModel::bitName(uint bit) const
{
    return _bits[bit];
}

bool
QFlagsModel::hasName(uint bit) const
{
    return _bits.hasName(bit);
}

int
QFlagsModel::fieldLength(uint bit) const
{
    return _bits.length(bit);
}

bool
QFlagsModel::operator[](uint bit) const
{
    if (invalidBit(bit))
        return false;
    return (_value & (1ULL << bit));
}

void
QFlagsModel::setValue(uint64_t newValue)
{
    _value = newValue;
}

void
QFlagsModel::resetNames()
{
    _bits.reset();
}

void
QFlagsModel::toggleBit(uint bit)
{
    if (invalidBit(bit))
        return; // throw()
    uint64_t v = 1ULL << bit;
    _value ^= v;
}
