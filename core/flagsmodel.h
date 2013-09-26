#ifndef FLAGSMODEL_H
#define FLAGSMODEL_H

#include <stdint.h>
#include <stddef.h>

typedef const char * bitNames_t [8*sizeof(uint64_t)];

typedef struct PlainFlagsModel_s {
    uint64_t   value;
    bitNames_t bits;
} PlainFlagsModel;

static inline int modelSetBitName(
        PlainFlagsModel * model,
        unsigned int      bit,
        const char      * name)
{
    if (bit >= 8*sizeof(model->value))
        return -1;
    model->bits[bit] = name;
    return 0;
}

#ifdef __cplusplus

namespace flags {

class FlagsModel
{
public:
    FlagsModel();
    virtual ~FlagsModel();

    uint64_t value() const;
    const char* bitName(unsigned int bit) const;
    void setValue(uint64_t newValue);
    void toggleBit(unsigned int bit);
    void setName(unsigned int bit, const char *name);
    void setNames(const bitNames_t names);

    bool operator[](unsigned int bit) const;

private:
    PlainFlagsModel _model;
};

} // namespace flags

#endif // __cplusplus

#endif // FLAGSMODEL_H
