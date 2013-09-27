#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "core/models.h"
#include <QDebug>
#include <QObject>
#include <QString>
#include <QPushButton>
#include <QSignalMapper>
#include <QKeyEvent>
#include <QClipboard>

struct named_model {
    const char *name;
    const bitNames_t *model;
};
const struct named_model models[] = {
    { "eflags", &x86_eflags, },
    { "cr0",    &x86_cr0, },
    { "cr4",    &x86_cr4, },
};

const QChar MainWindow::kFillChar('0');
const uint  MainWindow::kBitCount = 32;

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

    for (size_t i = 0; i < sizeof(models)/sizeof(models[0]); i++) {
        ui->comboBox->addItem(QString(models[i].name));
    }

    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(bitClicked(int)));
    connect(ui->valueEdit, SIGNAL(textEdited(QString)), this, SLOT(valueChanged(QString)));
    connect(ui->bitComboBox, SIGNAL(activated(int)), this, SLOT(valueWidthChanged(int)));
    connect(ui->comboBox, SIGNAL(activated(int)), this, SLOT(namesChanged(int)));
}

void
MainWindow::setNames(const bitNames_t names)
{
    model.setNames(names);
    updateBitNames();
    updateSummary();
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
    return model.bitName(i) ?
                QString(model.bitName(i)) :
                QString::number(i);
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

void
MainWindow::updateSummary()
{
    QString summary;
    uint64_t rest = 0, bit = 1ULL;
    for (uint i = 0; i < valueWidth; i++) {
        if (model[i]) {
            if (model.bitName(i)) {
                summary.append('|').append(model.bitName(i));
            } else
                rest |= bit;
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
    if (index >= 0 && index < (int)(sizeof(models)/sizeof(models[0])))
        setNames(*models[index].model);
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
MainWindow::keyPressEvent(QKeyEvent *event)
{
   if (event->matches(QKeySequence::Paste)) {
       event->ignore();
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
   else return QMainWindow::keyPressEvent(event);
}
