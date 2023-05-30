#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QThread>
#include <QProcess>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QtConcurrent>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public:
    void log(QString str);
    QStringList ReadSequence(QString OrderStr);
    void WriteCsv(QString OrderStr,QString Str);

    void StringToHex(QString str,QByteArray &senddata);
    QString HexToString(QByteArray data);
    char ConvertHexChart(char ch);
    quint8 CrcCheck(const QByteArray &data);

    //指令解析
    void LoopCommand();
    void PreCommand();
    void PostCommand();
    void ParseOrder(QString Ostr);
    void ParseOrder2(QString Ostr,QString PortName,QSerialPort* McuPort);
    int  ParseOrder3(QString Ostr,int result,QString Nextstr,QSerialPort* ChatPort);

    //get device list
    void PortFind();

    //init the port
    void InitChatPort(QSerialPort* ChatPort,QString PortName);
    void InitMcuPort(QSerialPort*  McuPort,QString PortName);

    void starttask2();
    void starttask3();
    void starttask4();
    void starttask5();
    void starttask6();
    void starttask7();
    void starttask8();

    void Looptask1();
    void Looptask2();
    void Looptask3();
    void Looptask4();
    void Looptask5();
    void Looptask6();
    void Looptask7();
    void Looptask8();

    void Posttask1();
    void Posttask2();
    void Posttask3();
    void Posttask4();
    void Posttask5();
    void Posttask6();
    void Posttask7();
    void Posttask8();

public slots:
    void ReadOutput();
    void ThreadInit();
    void starttaskEnd();

    void starttask1();

private slots:
    void on_BeginButton_clicked();
    void on_ClearButton_clicked();
    void on_StopButton_clicked();
    void on_RePortButton_clicked();

private:
    Ui::MainWindow *ui;

    //UI界面上的部件名称列表
    QStringList PNameList;
    QStringList MNameList;

    //get Usb Device list
    QProcess* cmd;
    //USB Device list and Serial Device list
    QStringList UsbList;
    QStringList SerialList;
    QStringList LocateList;
    QMap<QString,QString> UsbHash;

    //Pre/Loop/Post command
    QStringList Prelist;
    QStringList Looplist;
    QStringList Postlist;

    QStringList Prelist1;
    QStringList Prelist2;
    QStringList Prelist3;
    QStringList Prelist4;
    QStringList Prelist5;
    QStringList Prelist6;
    QStringList Prelist7;
    QStringList Prelist8;

    QStringList Looplist1;
    QStringList Looplist2;
    QStringList Looplist3;
    QStringList Looplist4;
    QStringList Looplist5;
    QStringList Looplist6;
    QStringList Looplist7;
    QStringList Looplist8;

    QStringList Postlist1;
    QStringList Postlist2;
    QStringList Postlist3;
    QStringList Postlist4;
    QStringList Postlist5;
    QStringList Postlist6;
    QStringList Postlist7;
    QStringList Postlist8;

    //owner Commandi
    int Orderi1 = 0;
    int Orderi2 = 0;
    int Orderi3 = 0;
    int Orderi4 = 0;
    int Orderi5 = 0;
    int Orderi6 = 0;
    int Orderi7 = 0;
    int Orderi8 = 0;

    //四个线程分别对应四个产品
    QTimer* m_pTimer = new QTimer(this);
    QTimer* m_pTimerEnd = new QTimer(this);


    //Port列表名字集合
    QStringList PortName;
    //Serial Chat Port
    QSerialPort* ChatPort1 = new QSerialPort;
    QSerialPort* ChatPort2 = new QSerialPort;
    QSerialPort* ChatPort3 = new QSerialPort;
    QSerialPort* ChatPort4 = new QSerialPort;
    QSerialPort* ChatPort5 = new QSerialPort;
    QSerialPort* ChatPort6 = new QSerialPort;
    QSerialPort* ChatPort7 = new QSerialPort;
    QSerialPort* ChatPort8 = new QSerialPort;

    //Tboard Port
    QSerialPort* McuPort1 = new QSerialPort;// = new QSerialPort;
    QSerialPort* McuPort2 = new QSerialPort;
    QSerialPort* McuPort3 = new QSerialPort;
    QSerialPort* McuPort4 = new QSerialPort;
    QSerialPort* McuPort5 = new QSerialPort;
    QSerialPort* McuPort6 = new QSerialPort;
    QSerialPort* McuPort7 = new QSerialPort;
    QSerialPort* McuPort8 = new QSerialPort;

    //loop判定值
    bool flag1 = true;
    bool flag2 = true;
    bool flag3 = true;
    bool flag4 = true;
    bool flag5 = true;
    bool flag6 = true;
    bool flag7 = true;
    bool flag8 = true;

    //bool Preflag = true;
    bool Preflag1 = true;
    bool Preflag2 = true;
    bool Preflag3 = true;
    bool Preflag4 = true;
    bool Preflag5 = true;
    bool Preflag6 = true;
    bool Preflag7 = true;
    bool Preflag8 = true;

    bool Loopflag1 = false;
    bool Loopflag2 = false;
    bool Loopflag3 = false;
    bool Loopflag4 = false;
    bool Loopflag5 = false;
    bool Loopflag6 = false;
    bool Loopflag7 = false;
    bool Loopflag8 = false;

    //bool Postflag = false;
    bool Postflag1 = true;
    bool Postflag2 = true;
    bool Postflag3 = true;
    bool Postflag4 = true;
    bool Postflag5 = true;
    bool Postflag6 = true;
    bool Postflag7 = true;
    bool Postflag8 = true;

    int Loop1 = 0;
    int Loop2 = 0;
    int Loop3 = 0;
    int Loop4 = 0;
    int Loop5 = 0;
    int Loop6 = 0;
    int Loop7 = 0;
    int Loop8 = 0;

    int ThreadTime = 2000;

    QFuture<void> Threadflag1;
    QFuture<void> Threadflag2;
    QFuture<void> Threadflag3;
    QFuture<void> Threadflag4;
    QFuture<void> Threadflag5;
    QFuture<void> Threadflag6;
    QFuture<void> Threadflag7;
    QFuture<void> Threadflag8;
};
#endif // MAINWINDOW_H
