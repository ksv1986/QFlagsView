#ifndef QNAMEDFLAGS_H
#define QNAMEDFLAGS_H

#include <QObject>
#include <QStringList>

class QJsonObject;

class QNamedFlags : public QObject
{
    Q_OBJECT
public:
    static const int BitCount = 64;

    explicit QNamedFlags(QObject *parent = 0);
    explicit QNamedFlags(const QNamedFlags& other);

    bool hasName(uint bit) const;
    int  length(uint bit) const;
    QString name() const;

    QString operator[](uint bit) const;
    QNamedFlags& operator=(const QNamedFlags& other);

    bool load(const QJsonObject &o);
    void reset(const QString &name = QString());

private:
    void copy(const QNamedFlags& other);

    QString     _name;
    QStringList _names;
    //  0 - unnamed bit
    //  1 - named flag
    // >1 - most significant digit of multibit field
    // <0 - relative index to most significant digit of multibit field
    // f.e: [4, -1, -2, -3] - 4-bit field
    int _bits[QNamedFlags::BitCount];
};

static inline bool
invalidBit(uint bit) {
    return bit >= QNamedFlags::BitCount;
}

#endif // QNAMEDFLAGS_H
