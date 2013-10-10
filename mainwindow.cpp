#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QObject>
#include <QString>
#include <QPushButton>
#include <QSignalMapper>
#include <QKeyEvent>
#include <QClipboard>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

const QChar MainWindow::kFillChar('0');
const uint  MainWindow::kBitCount = 32;
QList<QNamedFlags> MainWindow::models;

static inline int
hexLength(uint bits)
{
    switch (bits) {
    case  8: return 2;
    case 16: return 4;
    case 32: return 8;
    };
    return 16;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    signalMapper(new QSignalMapper(this)),
    valueWidth(kBitCount)
{
    modelInit();
    uiInit();

    setValueWidth(kBitCount);
    namesChanged(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void
MainWindow::modelInit(uint64_t initialValue)
{
    model.setValue(initialValue);
}

void
MainWindow::uiInit()
{
    ui->setupUi(this);

    digitFont.setStyleHint(QFont::Monospace);
#ifdef _WIN32
    digitFont.setFamily(QStringLiteral("Courier New"));
    digitFont.setPointSize(11);
#elif __APPLE__
    digitFont.setFamily("Menlo-Regular");
#endif
    ui->valueEdit->setFont(digitFont);
    ui->nameSummary->setFont(digitFont);

    uint64_t bit = 1ULL;
    for (uint i = 0; i < kBitCount; ++i) {
        int value = !!(model.value() & bit);
        QWidget *w;

        w = createBit(i, value);
        getBitLayout(i)->insertWidget(0, w);

        w = createName(i, value);
        ui->gridLayout->addWidget(w, i / 4, i % 4);

        bit <<= 1;
    }
    for (uint b = 8; b <= kBitCount; b <<= 1) {
        ui->bitComboBox->addItem(QString::number(b));
    }
    ui->bitComboBox->setCurrentIndex(ui->bitComboBox->count()-1);

    updateModels();

    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(bitClicked(int)));
    connect(ui->valueEdit, SIGNAL(textEdited(QString)), this, SLOT(valueChanged(QString)));
    connect(ui->bitComboBox, SIGNAL(activated(int)), this, SLOT(valueWidthChanged(int)));
    connect(ui->comboBox, SIGNAL(activated(int)), this, SLOT(namesChanged(int)));
    connect(ui->pasteButton, SIGNAL(clicked()), this, SLOT(pasteValue()));
}

void
MainWindow::setValueWidth(uint width)
{
    switch (width) {
    case 8: case 16: case 32: case 64:
        break;
    default:
        return;
    }
    for (uint i = valueWidth; i < width; i++) {
        ui->gridLayout->itemAt(i)->widget()->show();
    }
    for (uint i = width; i < kBitCount; i++) {
        ui->gridLayout->itemAt(i)->widget()->hide();
    }
    if (width > 16)
        ui->highWord->show();
    else
        ui->highWord->hide();
    if (width > 8) {
        for (int i = 0; i < ui->ahBits->count(); i++) {
            ui->ahBits->itemAt(i)->widget()->show();
        }
    } else {
        for (int i = 0; i < ui->ahBits->count(); i++) {
            ui->ahBits->itemAt(i)->widget()->hide();
        }
    }
    valueWidth = width;
    ui->valueEdit->setMaxLength(hexLength(valueWidth));
    ui->valueEdit->setInputMask(QString(hexLength(valueWidth), QChar('H')));
    updateValueEdit();
}

QHBoxLayout *
MainWindow::getBitLayout(uint i){
    if (i >= 24)
        return ui->ehBits;
    if (i >= 16)
        return ui->elBits;
    if (i >=  8)
        return ui->ahBits;
    return ui->alBits;
}

QWidget *
MainWindow::getBitWidget(uint i)
{
    if (i >= 32)
        return 0;
    if (i >= 24)
        return ui->ehBits->itemAt(32-1-i)->widget();
    if (i >= 16)
        return ui->elBits->itemAt(24-1-i)->widget();
    if (i >=  8)
        return ui->ahBits->itemAt(16-1-i)->widget();
    return ui->alBits->itemAt(8-1-i)->widget();
}

QWidget *
MainWindow::createBit(uint i, int value)
{
    QPushButton *pushButton = new QPushButton(this);
    pushButton->setObjectName(QString("bitBtn%1").arg(i, 2, 10, kFillChar));
    pushButton->setFont(digitFont);
    pushButton->setFlat(true);

    pushButton->setMinimumSize(QSize(20, 26));
    pushButton->setMaximumSize(QSize(20, 26));

    pushButton->setText(QString::number(value));
    pushButton->setToolTip(QString::number(i));

    signalMapper->setMapping(pushButton, i);
    connect(pushButton, SIGNAL(clicked()), signalMapper, SLOT(map()));

    return pushButton;
}

void
MainWindow::toggleBit(uint i, int value)
{
    QPushButton * b;
    b = qobject_cast<QPushButton*>(getBitWidget(i));
    b->setText(QString::number(value));

    b = qobject_cast<QPushButton*>(ui->gridLayout->itemAt(i)->widget());
    b->setChecked(value);
}

QWidget *
MainWindow::createName(uint i, int value)
{
    QPushButton *b = new QPushButton(getName(i));
    b->setObjectName(QString("nameBtn%1").arg(i, 2, 10, kFillChar));
    b->setCheckable(true);
    b->setFlat(true);
    b->setChecked(value);
    b->setToolTip(getNameTip(i));

    signalMapper->setMapping(b, i);
    connect(b, SIGNAL(clicked()), signalMapper, SLOT(map()));

    return b;
}

QString
MainWindow::getName(uint i) const
{
    return model.hasName(i) ? model.bitName(i) : QString::number(i);
}

QString
MainWindow::getNameTip(uint i) const
{
    return hexToString(1ULL << i);
}

QString
MainWindow::hexToString(uint64_t v) const
{
    return QString("0x%1").arg(v, hexLength(valueWidth), 16, kFillChar);
}

void
MainWindow::updateBitNames()
{
    for (uint i = 0; i < kBitCount; i++) {
        QPushButton *b = qobject_cast<QPushButton*>(ui->gridLayout->itemAt(i)->widget());
        b->setText(getName(i));
    }
}

bool
MainWindow::loadModel(QFile &inputFile)
{
    QJsonDocument json = QJsonDocument::fromJson(inputFile.readAll());
    if (json.isNull())
        return false;
    QNamedFlags flags;
    if (!flags.load(json.object()))
        return false;
    models.append(flags);
    return true;
}

void
MainWindow::updateModels()
{
    ui->comboBox->clear();
    models.clear();

    QDir modelsDir("models");
    QStringList modelFiles = modelsDir.entryList(QDir::Files,QDir::SortFlag::Name);
    for (int i = 0; i < modelFiles.count(); i++) {
        QFile inputFile(modelsDir.filePath(modelFiles[i]));
        if (!inputFile.open(QIODevice::ReadOnly))
            continue;
        loadModel(inputFile);
        inputFile.close();
    }
    for (int i = 0; i < models.count(); i++)
        ui->comboBox->addItem(models[i].name());
}

static const uint64_t
mask_table[QNamedFlags::BitCount] =
{
    0x0000000000000001, 0x0000000000000003, 0x0000000000000007, 0x000000000000000f,
    0x000000000000001f, 0x000000000000003f, 0x000000000000007f, 0x00000000000000ff,
    0x00000000000001ff, 0x00000000000003ff, 0x00000000000007ff, 0x0000000000000fff,
    0x0000000000001fff, 0x0000000000003fff, 0x0000000000007fff, 0x000000000000ffff,
    0x000000000001ffff, 0x000000000003ffff, 0x000000000007ffff, 0x00000000000fffff,
    0x00000000001fffff, 0x00000000003fffff, 0x00000000007fffff, 0x0000000000ffffff,
    0x0000000001ffffff, 0x0000000003ffffff, 0x0000000007ffffff, 0x000000000fffffff,
    0x000000001fffffff, 0x000000003fffffff, 0x000000007fffffff, 0x00000000ffffffff,
    0x00000001ffffffff, 0x00000003ffffffff, 0x00000007ffffffff, 0x0000000fffffffff,
    0x0000001fffffffff, 0x0000003fffffffff, 0x0000007fffffffff, 0x000000ffffffffff,
    0x000001ffffffffff, 0x000003ffffffffff, 0x000007ffffffffff, 0x00000fffffffffff,
    0x00001fffffffffff, 0x00003fffffffffff, 0x00007fffffffffff, 0x0000ffffffffffff,
    0x0001ffffffffffff, 0x0003ffffffffffff, 0x0007ffffffffffff, 0x000fffffffffffff,
    0x001fffffffffffff, 0x003fffffffffffff, 0x007fffffffffffff, 0x00ffffffffffffff,
    0x01ffffffffffffff, 0x03ffffffffffffff, 0x07ffffffffffffff, 0x0fffffffffffffff,
    0x1fffffffffffffff, 0x3fffffffffffffff, 0x7fffffffffffffff, 0xffffffffffffffff,
};

void
MainWindow::updateSummary()
{
    QString summary;
    uint64_t rest = 0, bit = 1ULL;
    for (uint i = 0; i < valueWidth; i++) {
        if (model[i]) {
            int length = model.fieldLength(i);
            if (length == 0)
                rest |= bit;
            else if (length == 1)
                summary.append('|').append(model.bitName(i));
            else {
                int index  = i;
                if (length < 0) {
                    index  = i + length;
                    length = model.fieldLength(index);
                }
                if (length > 1 && length <= QNamedFlags::BitCount) {
                    uint64_t field = (model.value() >> index) & mask_table[length-1];
                    summary.append('|').append(QString("%1<<%2").arg(field).arg(index));
                    bit <<= length - (i-index);
                    i = index + length - 1;
                    continue;
                }
            }
        }
        bit <<= 1;
    }
    summary.prepend(hexToString(rest));
    ui->nameSummary->setText(summary);
}

void
MainWindow::bitClicked(int b)
{
    model.toggleBit(b);
    updateValueEdit();
    toggleBit(b, model[b]);
}

void
MainWindow::updateValueEdit()
{
    int pos = ui->valueEdit->cursorPosition();
    ui->valueEdit->setText(QString("%1").arg(model.value(), hexLength(valueWidth), 16, kFillChar));
    if (pos < 0 || pos >= hexLength(valueWidth))
        pos = 0;
    ui->valueEdit->setCursorPosition(pos);
    updateSummary();
}

void
MainWindow::updateBits()
{
    uint64_t bit = 1UL;
    for (uint i = 0; i < valueWidth; i++) {
        toggleBit(i, !!(model.value() & bit));
        bit <<= 1;
    }
}

void
MainWindow::valueChanged(QString)
{
    if (ui->valueEdit->cursorPosition() >= hexLength(valueWidth))
        ui->valueEdit->setCursorPosition(hexLength(valueWidth)-1);

    bool ok = true;
    uint64_t v = ui->valueEdit->text().toULongLong(&ok, 16);
    if (!ok)
        return;
    model.setValue(v);
    updateBits();
    updateSummary();
}

void
MainWindow::namesChanged(int index)
{
    if (index >= models.count())
        return;
    model.setFlags(models.at(index));
    updateBitNames();
    updateSummary();
}

void
MainWindow::valueWidthChanged(int index)
{
    if (index < 0)
        return;
    setValueWidth(8 << index);
}

void
MainWindow::pasteValue()
{
    QClipboard *clipboard = QApplication::clipboard();
    QString originalText = clipboard->text();
    bool ok = false;
    uint64_t v = originalText.toULongLong(&ok, 16);
    if (!ok)
        v = originalText.toULongLong(&ok, 10);

    if (ok) {
        model.setValue(v);
        updateValueEdit();
        updateBits();
    }
}

void
MainWindow::keyPressEvent(QKeyEvent *event)
{
   if (event->matches(QKeySequence::Paste)) {
       event->ignore();
       pasteValue();
   }
   else return QMainWindow::keyPressEvent(event);
}
