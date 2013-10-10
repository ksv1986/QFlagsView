#ifndef QFLAGSMODEL_H
#define QFLAGSMODEL_H

#include <QObject>
#include <QStringList>
#include <stdint.h>
#include "qnamedflags.h"

class QFlagsModel : public QObject
{
    Q_OBJECT
public:

    explicit QFlagsModel(QObject *parent = 0);

    uint64_t value() const;
    const QString bitName(uint bit) const;
    bool hasName(uint bit) const;
    void setValue(uint64_t newValue);
    int  fieldLength(uint bit) const;
    void toggleBit(uint bit);
    void setFlags(const QNamedFlags &flags);
    void resetNames();

    bool operator[](uint bit) const;

signals:

public slots:

private:
    uint64_t _value;
    QNamedFlags _bits;
};

#endif // QFLAGSMODEL_H
