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
std::vector<size_t> offsets_coin = {0x82C, 0x208};
std::vector<size_t> offsets_golden_coin = {0x82C, 0x20C};
std::vector<size_t> offsets_diamond = {0x82C, 0x210};
std::vector<size_t> offsets_zombies = {0x768, 0x90};


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
    ui->line_1->hide();
    ui->KeepNoCD->hide();
    ui->RefreshCD->hide();
    ui->ToggleLock->hide();
    ui->line_2->hide();
    ui->edit_coin->hide();
    ui->edit_diamond->hide();
    ui->edit_golden_coin->hide();
    ui->Verify_coin->hide();
    ui->Verify_diamond->hide();
    ui->Verify_golden_coin->hide();
    ui->line_3->hide();
    ui->Keep_OneTap->hide();
    this->setWindowTitle("植物大战僵尸杂交版辅助 v1.4");
    ui->lineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]+$")));
    ui->edit_coin->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]+$")));
    ui->edit_golden_coin->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]+$")));
    ui->edit_diamond->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]+$")));

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Widget::_RefreshCD);
    timer_sunlights = new QTimer(this);
    connect(timer_sunlights, &QTimer::timeout, this, &Widget::_verifySunlightData);
    timer_Zombies = new QTimer(this);
    connect(timer_Zombies, &QTimer::timeout, this, &Widget::_DecreaseZombiesHP);
    timer_Defence = new QTimer(this);
    connect(timer_Defence, &QTimer::timeout, this, &Widget::_DecreaseDefenceHP);

}


Widget::~Widget()
{
    delete ui;
}

//获取游戏窗口
HWND Widget::GetGameWindow()
{
    QString className = "MainWindow";
    QString titleSubstring = "植物大战僵尸";
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

    //获取金币等
    uint32_t address_coin;
    int coin;
    address_coin = readMultiLevelPointer(ProcessHandle, BaseAddress, offsets_coin);
    ReadIntFromProcessMemory(ProcessHandle, address_coin, coin);
    ui->edit_coin->setText(QString().number(coin));
    uint32_t address_golden_coin;
    int golden_coin;
    address_golden_coin = readMultiLevelPointer(ProcessHandle, BaseAddress, offsets_golden_coin);
    ReadIntFromProcessMemory(ProcessHandle, address_golden_coin, golden_coin);
    ui->edit_golden_coin->setText(QString().number(golden_coin));
    uint32_t address_diamond;
    int diamond;
    address_diamond = readMultiLevelPointer(ProcessHandle, BaseAddress, offsets_diamond);
    ReadIntFromProcessMemory(ProcessHandle, address_diamond, diamond);
    ui->edit_diamond->setText(QString().number(diamond));
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

void Widget::_DecreaseZombiesHP()
{
    for(int n = 1; n <=500; n++)
    {
        size_t num = 0x204;
        std::vector<size_t> offsets_temp = offsets_zombies;
        offsets_temp.push_back((size_t)(0xc8+num*(n-1)));
        uint32_t address;
        address = readMultiLevelPointer(ProcessHandle, BaseAddress, offsets_temp);
        modifyMemoryData(ProcessHandle, address, 1);
    }
}

void Widget::_DecreaseDefenceHP()
{
    for(int n = 1; n <=500; n++)
    {
        size_t num = 0x204;
        std::vector<size_t> offsets_temp1 = offsets_zombies;
        std::vector<size_t> offsets_temp2 = offsets_zombies;
        offsets_temp1.push_back((size_t)(0xd0+num*(n-1)));
        offsets_temp2.push_back((size_t)(0xdc+num*(n-1)));
        uint32_t address1;
        uint32_t address2;
        address1 = readMultiLevelPointer(ProcessHandle, BaseAddress, offsets_temp1);
        address2 = readMultiLevelPointer(ProcessHandle, BaseAddress, offsets_temp2);
        modifyMemoryData(ProcessHandle, address1, 0);
        modifyMemoryData(ProcessHandle, address2, 0);
    }
}


//前期考虑不周，修改阳光函数不能通用
void Widget::VerifyData(int newData, std::vector<size_t> offsets)
{
    uint32_t address;
    address = readMultiLevelPointer(ProcessHandle, BaseAddress, offsets);
    modifyMemoryData(ProcessHandle, address, newData);
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
    ui->line_1->show();
    ui->KeepNoCD->show();
    ui->RefreshCD->show();
    ui->ToggleLock->show();
    ui->line_2->show();
    ui->edit_coin->show();
    ui->edit_diamond->show();
    ui->edit_golden_coin->show();
    ui->Verify_coin->show();
    ui->Verify_diamond->show();
    ui->Verify_golden_coin->show();
    ui->line_3->show();
    ui->Keep_OneTap->show();
    return;
}

//响应修改阳光按钮，发送输入框值修改阳光
void Widget::on_Verify_clicked()
{
    int newValue = ui->lineEdit->text().toInt();
    verifySunlightData(newValue);
    //qDebug() <<"sended"<<newValue;
}

//响应刷新CD按钮
void Widget::on_RefreshCD_clicked()
{
    RefreshCD();
}

//槽函数：响应无CD定时器信号
void Widget::_RefreshCD()
{
    RefreshCD();
}


//槽函数：响应锁定阳光定时器信号
void Widget::_verifySunlightData()
{
    verifySunlightData(LOCKEDSUNLIGHTS);
}

//无CD复选框状态改变处理
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

//锁定阳光复选框状态改变处理
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

//废物
void Widget::on_pushButton_clicked()
{

}

//响应修改硬币按钮
void Widget::on_Verify_coin_clicked()
{
    int coin = ui->edit_coin->text().toInt();
    VerifyData(coin, offsets_coin);
}

//响应修改金币按钮
void Widget::on_Verify_golden_coin_clicked()
{
    int golden_coin = ui->edit_golden_coin->text().toInt();
    VerifyData(golden_coin, offsets_golden_coin);
}

//响应修改钻石按钮
void Widget::on_Verify_diamond_clicked()
{
    int diamond = ui->edit_diamond->text().toInt();
    VerifyData(diamond, offsets_diamond);
}


void Widget::on_Keep_OneTap_stateChanged(int arg1)
{
    if(arg1 == 2)
    {
        timer_Zombies->start(100);
        timer_Defence->start(100);
    }
    else if(arg1 == 0)
    {
        timer_Zombies->stop();
        timer_Defence->stop();
    }
}

