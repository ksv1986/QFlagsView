#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QByteArray>
#include <QFont>
#include "qflagsmodel.h"

namespace Ui {
class MainWindow;
}

class QFile;
class QHBoxLayout;
class QSignalMapper;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    static QList<QNamedFlags> models;

    Ui::MainWindow *ui;
    QFont digitFont;
    QFlagsModel model;
    QSignalMapper * signalMapper;
    uint valueWidth;
    static const QChar kFillChar;
    static const uint  kBitCount;

    void modelInit(uint64_t initialValue=0);
    void uiInit();

    void setValueWidth(uint width);

    QWidget * createBit (uint i, int value=0);
    QWidget * createName(uint i, int value=0);
    QString   getName(uint i) const;
    QString   getNameTip(uint i) const;
    QString   hexToString(uint64_t v) const;
    void toggleBit(uint i, int value=0);
    void updateValueEdit();
    void updateBits();
    void updateBitNames();
    void updateModels();
    void updateSummary();
    QHBoxLayout * getBitLayout(uint i);
    QWidget * getBitWidget(uint i);

    bool loadModel(QFile &file);
private slots:
    void bitClicked(int);
    void valueChanged(QString);
    void namesChanged(int);
    void valueWidthChanged(int);
    void pasteValue();
protected:
    void keyPressEvent(QKeyEvent *event);
};

#endif // MAINWINDOW_H
