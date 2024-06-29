#include "widget.h"
#include "ui_widget.h"
#include <windows.h>
#include <QString>
#include <QMessageBox>
#include <qDebug>
#include <QRegularExpressionValidator>
#include <QTimer>
#include <TlHelp32.h>


std::vector<HWND> foundWindows;
HANDLE ProcessHandle = NULL;
HWND Window = NULL;
DWORD Pid = NULL;
int CARDNUM = 0;
int LOCKEDSUNLIGHTS = 0;
QString WindowName = "植物大战僵尸杂交版v2.1 ";
uint32_t BaseAddress = 0x6A9EC0;
std::vector<size_t> offsets_sunlights = {0x768, 0x5560};
std::vector<size_t> offsets_cardnum = {0x768, 0x144, 0x24};
std::vector<size_t> offsets_card = {0x768, 0x144};


//必须使用全局函数，不能在class中定义
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    char className[256];
    GetClassNameA(hwnd, className, sizeof(className));

    // 获取要查找的窗口类名
    const char* targetClassName = reinterpret_cast<const char*>(lParam);

    // 判断窗口类名是否匹配
    if (strcmp(className, targetClassName) == 0) {
        foundWindows.push_back(hwnd);
    }

    return TRUE;
}

HWND findWindowByClassNameAndTitle(const QString& className, const QString& titleSubstring) {
    // 清空之前的结果
    foundWindows.clear();

    // 使用 EnumWindows 枚举所有顶层窗口
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(className.toStdString().c_str()));

    // 遍历找到的所有符合类名的窗口
    for (HWND hwnd : foundWindows) {
        char windowTitle[256];
        GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));

        QString qTitle = QString::fromLocal8Bit(windowTitle);

        // 判断窗口标题是否包含特定子字符串
        if (qTitle.contains(titleSubstring, Qt::CaseInsensitive)) {
            return hwnd;
        }
    }

    return NULL;
}


Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    ui->NO->hide();
    ui->YES->hide();
    ui->lineEdit->hide();
    ui->Verify->hide();
    ui->line->hide();
    ui->KeepNoCD->hide();
    ui->RefreshCD->hide();
    ui->ToggleLock->hide();
    this->setWindowTitle("植物大战僵尸杂交版辅助");
    ui->lineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]+$")));

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Widget::_RefreshCD);
    timer_sunlights = new QTimer(this);
    connect(timer_sunlights, &QTimer::timeout, this, &Widget::_verifySunlightData);
}

Widget::~Widget()
{
    delete ui;
}

//获取游戏窗口
HWND Widget::GetGameWindow()
{
    QString className = "MainWindow";
    QString titleSubstring = "植物大战僵尸杂交版";
    HWND foundHwnd = findWindowByClassNameAndTitle(className, titleSubstring);
    if (foundHwnd) {
        //qDebug() << "Found window with title containing" << titleSubstring;
        return foundHwnd;
    } else {
        //qDebug() << "No window found with title containing" << titleSubstring;
        return NULL;
    }
}

//获取并设置hwnd、pid、卡槽数量
void Widget::GetGameInfo(HWND w1ndow)
{
    Window = w1ndow;
    GetWindowThreadProcessId(Window, &Pid);
    //qDebug() << Qt::hex <<Pid;
    ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Pid);
    //qDebug() <<"Processhandle:"<<ProcessHandle;

    //获取当前阳光
    uint32_t address1;
    int sunlights = 0;
    address1 = readMultiLevelPointer(ProcessHandle, BaseAddress, offsets_sunlights);
    ReadIntFromProcessMemory(ProcessHandle, address1, sunlights);
    ui->lineEdit->setText(QString::number(sunlights));
    //获取卡槽数量
    uint32_t address;
    address = readMultiLevelPointer(ProcessHandle, BaseAddress, offsets_cardnum);
    ReadIntFromProcessMemory(ProcessHandle, address, CARDNUM);
    //qDebug() << "address:" << Qt::hex << address;
    //qDebug() << "cardnum:" << Qt::dec << CARDNUM;
}

//根据基址和指针读取内存
uint32_t Widget::readMultiLevelPointer(HANDLE processHandle, uint32_t baseAddress, const std::vector<size_t> &offsets11)
{
    uint32_t address = baseAddress;
    for (size_t offset : offsets11) {
        ReadProcessMemory(processHandle, (LPCVOID)address, &address, sizeof(address), NULL);
        if (!address) {
            //qDebug() << "Invalid address.";
            return 0;
        }
        address += offset;
    }
    return address;
}

//修改指定地址内存为int型参数
void Widget::modifyMemoryData(HANDLE processHandle, uint32_t address, int newValue)
{
    if (WriteProcessMemory(processHandle, (LPVOID)(address), &newValue, sizeof(newValue), NULL)) {
        //qDebug() << "Data modified successfully.";
    } else {
        //qDebug() << "Failed to modify data.";
    }
}

//获取内存数据
bool Widget::ReadIntFromProcessMemory(HANDLE processHandle, uint32_t address, int &data)
{
    SIZE_T bytesRead;
    BOOL success = ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), &data, sizeof(int), &bytesRead);

    // 检查读取是否成功以及读取的数据大小是否正确
    return success && (bytesRead == sizeof(int));
}

//通过调用上面两个函数修改阳光值
void Widget::verifySunlightData(int newData)
{
    uint32_t address;
    address = readMultiLevelPointer(ProcessHandle, BaseAddress, offsets_sunlights);
    modifyMemoryData(ProcessHandle, address, newData);
}

//单次刷新植物冷却
void Widget::RefreshCD()
{
    for(int n = 1; n <= CARDNUM; n++)//循环卡槽数次
    {
        size_t third = 0x50;
        std::vector<size_t> offsets_temp = offsets_card;
        offsets_temp.push_back((size_t)(0x70+third*(n-1)));
        uint32_t address;
        address = readMultiLevelPointer(ProcessHandle, BaseAddress, offsets_temp);
        modifyMemoryData(ProcessHandle, address, 1);
    }
}


//响应查找窗口按钮，查找窗口、获取进程信息、设置全局变量等
void Widget::on_GetWindow_clicked()
{
    ui->NO->hide();     //防止错误后二次查找时NO仍显示
    //Window = FindWindow(NULL, (LPCWSTR)WindowName.unicode());

    //-----枚举class为MainWindow的窗口并筛选“植物大战僵尸杂交版”
    HWND w1 = GetGameWindow();
    if(!w1)
    {
        ui->NO->show();
        //qDebug() << "!GetGameWindow()";
        return;
    }
    //qDebug() <<"Window:" << Qt::hex << Window;
    GetGameInfo(w1);

    ui->YES->show();
    ui->lineEdit->show();
    ui->Verify->show();
    ui->GetWindow->hide();
    ui->line->show();
    ui->KeepNoCD->show();
    ui->RefreshCD->show();
    ui->ToggleLock->show();
    return;
}

//响应修改阳光按钮，发送输入框值修改阳光
void Widget::on_Verify_clicked()
{
    int newValue = ui->lineEdit->text().toInt();
    verifySunlightData(newValue);
    //qDebug() <<"sended"<<newValue;
}


void Widget::on_RefreshCD_clicked()
{
    RefreshCD();
}

void Widget::_RefreshCD()
{
    RefreshCD();
}

void Widget::_verifySunlightData()
{
    verifySunlightData(LOCKEDSUNLIGHTS);
}


void Widget::on_KeepNoCD_stateChanged(int arg1)
{
    if(arg1==2)
    {
        timer->start(50);
    }
    else if(arg1==0)
    {
        timer->stop();
        //qDebug()<<"Timer stoped";
    }
}


void Widget::on_ToggleLock_stateChanged(int arg1)
{
    if(arg1==2)
    {
        uint32_t address1;
        address1 = readMultiLevelPointer(ProcessHandle, BaseAddress, offsets_sunlights);
        ReadIntFromProcessMemory(ProcessHandle, address1, LOCKEDSUNLIGHTS);
        timer_sunlights->start(50);
    }
    else if(arg1==0)
    {
        timer_sunlights->stop();
        //qDebug()<<"Timer_sunlights stoped";
    }
}

