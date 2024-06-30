#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <windows.h>
#include <TlHelp32.h>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    HWND GetGameWindow();
    void GetGameInfo(HWND w1ndow);
    uint32_t readMultiLevelPointer(HANDLE processHandle, uint32_t baseAddress, const std::vector<size_t> &offsets11);
    void modifyMemoryData(HANDLE processHandle, uint32_t address, int newValue);
    bool ReadIntFromProcessMemory(HANDLE processHandle, uint32_t address, int& data);
    void verifySunlightData(int newData);
    void RefreshCD();
    void VerifyData(int newData, std::vector<size_t> offsets);

signals:
    void timeout();

private slots:
    void on_GetWindow_clicked();

    void on_Verify_clicked();

    void on_RefreshCD_clicked();

    void _RefreshCD();

    void _verifySunlightData();

    void on_KeepNoCD_stateChanged(int arg1);

    void on_ToggleLock_stateChanged(int arg1);

    void on_pushButton_clicked();

    void on_Verify_coin_clicked();

    void on_Verify_golden_coin_clicked();

    void on_Verify_diamond_clicked();

private:
    Ui::Widget *ui;
    QTimer *timer;
    QTimer *timer_sunlights;
};
#endif // WIDGET_H
