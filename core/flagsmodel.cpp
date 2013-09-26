#include "flagsmodel.h"

#include <string.h>

using namespace flags;

FlagsModel::FlagsModel()
{
    memset(&_model, 0, sizeof(_model));
}

FlagsModel::~FlagsModel()
{
}

uint64_t
FlagsModel::value() const {
    return _model.value;
}

void
FlagsModel::setName(unsigned int bit, const char *name) {
    modelSetBitName(&_model, bit, name);
}

const char*
FlagsModel::bitName(unsigned int bit) const
{
    if (bit >= 8*sizeof(_model.value))
        return NULL;
    return _model.bits[bit];
}

bool
FlagsModel::operator[](unsigned int bit) const
{
    if (bit >= 8*sizeof(_model.value))
        return false;
    return (_model.value & (1ULL << bit));
}

void
FlagsModel::setValue(uint64_t newValue)
{
    _model.value = newValue;
}

void
FlagsModel::setNames(const bitNames_t names)
{
    memcpy(_model.bits, names, sizeof(_model.bits));
    setValue(_model.value);
}

void
FlagsModel::toggleBit(unsigned int bit)
{
    if (bit > 8*sizeof(_model.value))
        return; // throw()
    uint64_t v = 1ULL << bit;
    _model.value ^= v;
}
