#include "mainwindow.h"
#include "ui_mainwindow.h"

#include<QFile>
#include<QDateTime>
#include<QJsonArray>
#include<QJsonParseError>
#include<QJsonObject>
#include<QTimer>
#include<QTime>
#include<QMetaObject>

typedef unsigned char u8;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qDebug() << "Main Thread=" << QThread::currentThread();
    qDebug() << "Main Thread=" << QThread::currentThread();
    //ui->Picturelabel->setPixmap(QPixmap("/Users/RELTEST/1.jpg"));
    //ui->Picturelabel->setScaledContents(true);
    QThreadPool::globalInstance()->setMaxThreadCount(1000);
    ui->MesDisplay->append("最大线程数=" + QString::number(QThreadPool::globalInstance()->maxThreadCount()));

    //ReadSequence("/Users/RELTEST/Command/Pre Table.csv");
    PreCommand();
    LoopCommand();
    PostCommand();

    //终端命令读取接收
    cmd = new QProcess();
    connect(cmd,SIGNAL(readyRead()),this,SLOT(ReadOutput()));
    cmd->start("bash");
    cmd->waitForStarted();
    cmd->write("system_profiler SPUSBDataType\n");
}

MainWindow::~MainWindow()
{
    QThreadPool::globalInstance()->clear();
    QThreadPool::globalInstance()->waitForDone(-1);

    /*
    McuPort1->close();
    McuPort2->close();
    McuPort3->close();
    McuPort4->close();
    McuPort5->close();
    McuPort6->close();
    McuPort7->close();
    McuPort8->close();


    ChatPort1->close();
    ChatPort2->close();
    ChatPort3->close();
    ChatPort4->close();
    ChatPort5->close();
    ChatPort6->close();
    ChatPort7->close();
    ChatPort8->close();
    */

    /*
    ChatPort1->deleteLater();
    ChatPort2->deleteLater();
    ChatPort3->deleteLater();
    ChatPort4->deleteLater();
    ChatPort5->deleteLater();
    ChatPort6->deleteLater();
    ChatPort7->deleteLater();
    ChatPort8->deleteLater();

    McuPort1->deleteLater();
    McuPort2->deleteLater();
    McuPort3->deleteLater();
    McuPort4->deleteLater();
    McuPort5->deleteLater();
    McuPort6->deleteLater();
    McuPort7->deleteLater();
    McuPort8->deleteLater();
    */

    delete ui;
}

//log 字符串写入
void MainWindow::log(QString str){
    QDateTime current_date_time = QDateTime::currentDateTime();
    QString current_data = current_date_time.toString("yyyy.MM.dd hh:mm:ss");

    QString logfile = "/Users/RELTEST/log.json";
    QFile file(logfile);

//    if(!file.open(QIODevice::ReadOnly)){
//        return;
//    }

//    QByteArray ArrayData = file.readAll();
//    file.close();

//    QJsonParseError jsonError;
//    QJsonDocument jsonDoc(QJsonDocument::fromJson(ArrayData,&jsonError));

//    if(jsonError.error != QJsonParseError::NoError){
//        //ui->textEdit->append(jsonError.errorString());
//    }

    QJsonObject rootObj;// = jsonDoc.object();
    rootObj.insert(current_data,str);

    QJsonDocument doc;
    doc.setObject(rootObj);

    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
        return;
    }

    file.write(doc.toJson());
    file.close();
}

//读取选好的csv命令
QStringList MainWindow::ReadSequence(QString OrderStr)
{
    //QString OrderStr = "/Users/RELTEST/" + ui->SeqCombox->currentText();
    QStringList tempOption;
    QFile file(OrderStr);
    if (!file.open(QIODevice::ReadOnly))
    {
        //log("Error: cannot open XMLfile");
        return tempOption;
    }

    QTextStream *out = new QTextStream(&file);//文本流
    tempOption = out->readAll().split("\r\n");//每一行以\n区分
    file.close();

    return tempOption;
}

void MainWindow::WriteCsv(QString OrderStr,QString Str)
{
    QDateTime current_date_time = QDateTime::currentDateTime();
    QString current_data = current_date_time.toString("yyyy.MM.dd hh:mm:ss.zzz");

    QFile file(OrderStr);

    if(!file.exists())
    {
        file.open(QIODevice::WriteOnly);
        file.close();
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        //log("Error: cannot open XMLfile");
        return;
    }

    QTextStream out(&file);//文本流

    out << current_data + "\n" + Str  + "\n";

    file.close();
}

//16进止转换
void MainWindow::StringToHex(QString str, QByteArray &senddata)
{
    int hexdata,lowhexdata;
    int hexdatalen = 0;
    int len = str.length();
    senddata.resize(len/2);
    char lstr,hstr;
    for(int i=0; i<len; )
    {
        hstr=str[i].toLatin1();
        if(hstr == ' ')
        {
            i++;
            continue;
        }
        i++;
        if(i >= len)
            break;
        lstr = str[i].toLatin1();
        hexdata = ConvertHexChart(hstr);
        lowhexdata = ConvertHexChart(lstr);
        if((hexdata == 16) || (lowhexdata == 16))
            break;
        else
            hexdata = hexdata*16+lowhexdata;
        i++;
        senddata[hexdatalen] = (char)hexdata;
        hexdatalen++;
    }
    senddata.resize(hexdatalen);
}

char MainWindow::ConvertHexChart(char ch)
{
    if((ch >= '0') && (ch <= '9'))
        return ch-0x30;  // 0x30 对应 ‘0’
    else if((ch >= 'A') && (ch <= 'F'))
        return ch-'A'+10;
    else if((ch >= 'a') && (ch <= 'f'))
        return ch-'a'+10;
    else
        return ch-ch;//不在0-f范围内的会发送成0
}

//16进制转换为QString
QString MainWindow::HexToString(QByteArray data)
{
    //QByteArray data = Port1mcu->readAll();
    QString savestr;
    QDataStream out(&data,QIODevice::ReadOnly);
    while(!out.atEnd())
    {
        qint8 outChar = 0;
        out >> outChar;
        QString str = QString("%1").arg(outChar&0xFF,2,16,QLatin1Char('0'));
        savestr.append(str.toUpper());
        savestr.append(" ");
    }

    return savestr;
}

//CRC校验算法
static const u8 caCrc8Data[256] = {
    0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
    0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65, 0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
    0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5, 0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85, 0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
    0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2, 0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
    0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2, 0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32, 0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
    0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42, 0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
    0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C, 0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC, 0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
    0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C, 0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
    0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C, 0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B, 0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
    0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B, 0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
    0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB, 0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
    0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB, 0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};

quint8 MainWindow::CrcCheck(const QByteArray &data)
{
    quint8 buf;
    quint8 crc8 = 0x00; /* 该处修改 */

    for(auto i = 0;i<data.size();i++)
    {
        buf = data.at(i) ^crc8;
        crc8 = caCrc8Data[buf];
        //qDebug()<<crc8;
    }
    return crc8;
}

//Command List CommandList 为命令集合，CombackList 为返回命令集合
void MainWindow::PreCommand()
{
    QString CommandPath = "/Users/RELTEST/Command/pre test.csv";

    QStringList SumList = ReadSequence(CommandPath);
    if(SumList.count() > 4)
    {
        SumList.remove(0,4);
        SumList.removeOne("");
    }

    for(int i = 0;i < SumList.count();i++)
    {
        QString tendstr = SumList[i].split(",")[2];
        Prelist.append(tendstr);
    }

    //CombackList合并操作
    for(int i = 0;i < Prelist.count();i++)
    {
        Prelist[i] = Prelist[i] + "\n";
    }
    //qDebug() << "CommandList len=" << CommandList.count();


    Prelist1 = Prelist;
    Prelist2 = Prelist;
    Prelist3 = Prelist;
    Prelist4 = Prelist;
    Prelist5 = Prelist;
    Prelist6 = Prelist;
    Prelist7 = Prelist;
    Prelist8 = Prelist;
    /*
    Prelist9 = Prelist;
    PrelistA = Prelist;
    PrelistB = Prelist;
    PrelistC = Prelist;
    PrelistD = Prelist;
    PrelistE = Prelist;
    PrelistF = Prelist;
    PrelistG = Prelist;
    */

    /*
    PrelistH = Prelist;
    PrelistI = Prelist;
    PrelistJ = Prelist;
    PrelistK = Prelist;
    PrelistL = Prelist;
    PrelistM = Prelist;
    PrelistN = Prelist;
    PrelistO = Prelist;
    PrelistP = Prelist;
    PrelistQ = Prelist;
    PrelistR = Prelist;
    PrelistS = Prelist;
    PrelistT = Prelist;
    PrelistU = Prelist;
    PrelistV = Prelist;
    PrelistW = Prelist;
    */
}

void MainWindow::LoopCommand()
{
    QString CommandPath = "/Users/RELTEST/Command/loop test.csv";

    QStringList SumList = ReadSequence(CommandPath);
    if(SumList.count() > 4)
    {
        SumList.remove(0,4);
        SumList.removeOne("");
    }

    for(int i = 0;i < SumList.count();i++)
    {
        QString tendstr = SumList[i].split(",")[2];
        Looplist.append(tendstr);
    }

    //CombackList合并操作
    for(int i = 0;i < Looplist.count();i++)
    {
        Looplist[i] = Looplist[i] + "\n";
    }
    //qDebug() << "CommandList len=" << Looplist.count();

    //for(int i=0;i < Looplist.count();i++)
    //{
    //    qDebug() << "command" << Looplist[i];
    //}


    Looplist1 = Looplist;
    Looplist2 = Looplist;
    Looplist3 = Looplist;
    Looplist4 = Looplist;
    Looplist5 = Looplist;
    Looplist6 = Looplist;
    Looplist7 = Looplist;
    Looplist8 = Looplist;
    /*
    Looplist9 = Looplist;
    LooplistA = Looplist;
    LooplistB = Looplist;
    LooplistC = Looplist;
    LooplistD = Looplist;
    LooplistE = Looplist;
    LooplistF = Looplist;
    LooplistG = Looplist;
    */

    /*
    LooplistH = Looplist;
    LooplistI = Looplist;
    LooplistJ = Looplist;
    LooplistK = Looplist;
    LooplistL = Looplist;
    LooplistM = Looplist;
    LooplistN = Looplist;
    LooplistO = Looplist;
    LooplistP = Looplist;
    LooplistQ = Looplist;
    LooplistR = Looplist;
    LooplistS = Looplist;
    LooplistT = Looplist;
    LooplistU = Looplist;
    LooplistV = Looplist;
    LooplistW = Looplist;
    */
}

void MainWindow::PostCommand()
{
    QString CommandPath = "/Users/RELTEST/Command/Post test.csv";

    QStringList SumList = ReadSequence(CommandPath);

    if(SumList.count() > 4)
    {
        SumList.remove(0,4);
        SumList.removeOne("");
    }

    for(int i = 0;i < SumList.count();i++)
    {
        QString tendstr = SumList[i].split(",")[2];
        Postlist.append(tendstr);
    }

    //CombackList合并操作
    for(int i = 0;i < Postlist.count();i++)
    {
        Postlist[i] = Postlist[i] + "\n";
    }
    //qDebug() << "CommandList len=" << CommandList.count();


    Postlist1 = Postlist;
    Postlist2 = Postlist;
    Postlist3 = Postlist;
    Postlist4 = Postlist;
    Postlist5 = Postlist;
    Postlist6 = Postlist;
    Postlist7 = Postlist;
    Postlist8 = Postlist;
    /*
    Postlist9 = Postlist;
    PostlistA = Postlist;
    PostlistB = Postlist;
    PostlistC = Postlist;
    PostlistD = Postlist;
    PostlistE = Postlist;
    PostlistF = Postlist;
    PostlistG = Postlist;
    */

    /*
    PostlistH = Postlist;
    PostlistI = Postlist;
    PostlistJ = Postlist;
    PostlistK = Postlist;
    PostlistL = Postlist;
    PostlistM = Postlist;
    PostlistN = Postlist;
    PostlistO = Postlist;
    PostlistP = Postlist;
    PostlistQ = Postlist;
    PostlistR = Postlist;
    PostlistS = Postlist;
    PostlistT = Postlist;
    PostlistU = Postlist;
    PostlistV = Postlist;
    PostlistW = Postlist;
    */
}

//Process 处理函数
void MainWindow::ReadOutput()
{
    QString str = cmd->readAll();
    //qDebug() << str;
    //WriteCsv("/Users/RELTEST/log.csv",str+"\n");
    QString Usbtend;
    QString Serialtend;
    QString Locatetend;
    //qDebug() << str;
    UsbList.clear();
    LocateList.clear();
    SerialList.clear();
    UsbHash.clear();
    PNameList.clear();
    MNameList.clear();

    for(int i = 0;i < str.length();i++)
    {

        if(i+5 < str.length() && str[i] == '\xa' && str[i+3] == ' ' && str[i+4] == ' ' && str[i+5] != ' ')
        {
            i = i+5;
        }
        else if(i+9 < str.length() && str[i] == '\xa' && str[i+7] == ' ' && str[i+8] == ' ' && str[i+9] != ' ')
        {
            i = i+9;
            Usbtend.append("  ");
        }
        else if(i+13 < str.length() && str[i] == '\xa' && str[i+11] == ' ' && str[i+12] == ' ' && str[i+13] != ' ')
        {
            i = i+13;
            Usbtend.append("    ");
        }
        else if(i+17 < str.length() && str[i] == '\xa' && str[i+15] == ' ' && str[i+16] == ' ' && str[i+17] != ' ')
        {
            i = i+17;
            Usbtend.append("      ");
        }
        else if(i+21 < str.length() && str[i] == '\xa' && str[i+19] == ' ' && str[i+20] == ' ' && str[i+21] != ' ')
        {
            i = i+21;
            Usbtend.append("        ");
        }
        else
        {
            if(str[i] == 'S' && str.mid(i,13) == "Serial Number" && str[i+25] == '\xa')
            {
                i = i + 15;

                while(str[i] != '\xa')
                {
                    Serialtend.append(str[i]);
                    i++;
                }

                //Serialtend.append(UsbList.last());

                SerialList << Serialtend;
                Serialtend.clear();

                while(str.mid(i,11) != "Location ID")
                {   
                    i++;
                }

                i = i + 13;

                while(str[i] != '\xa')
                {
                    Locatetend.append(str[i]);
                    i++;
                }



                //Serialtend.append(UsbList.last());
                LocateList << Locatetend;
                Locatetend.clear();
            }



            continue;
        }

        while(str[i] != '\xa')
        {
            Usbtend.append(str[i]);
            i++;
        }

        UsbList << Usbtend;
        Usbtend.clear();
    }

    //WriteCsv("/Users/RELTEST/PortMap.csv","Llen="+QString::number(LocateList.count())+"SLen="+QString::number(SerialList.count())+"\n");

    for(int i = 0;i < SerialList.count();i++)
    {
        UsbHash.insert(LocateList[i],SerialList[i]);
    }

    QMapIterator<QString,QString> iter(UsbHash);
    QDateTime current_date_time = QDateTime::currentDateTime();
    QString current_data = current_date_time.toString("yyyy.MM.dd");

    //qDebug() << LocateList.count() << SerialList.count();
    while(iter.hasNext())
    {
        iter.next();
        WriteCsv("/Users/RELTEST/" + current_data + "PortMap.csv",iter.key() + "," + iter.value() + "\n");
        //qDebug() << iter.key() << ":" << iter.value();
    }
    //WriteCsv("/Users/RELTEST/" + current_data + "PortMap.csv","1234");
    //qDebug() << iter.key() << ":" << iter.value();

    PortFind();
    QStringList SerialTend;
    SerialTend = PortName;
    //qDebug() << SerialTend.count();

    /*
    for(int i = 0;i < SerialTend.count();i++)
    {
        if(SerialTend[i].contains("H36_H37"))
        {
            qDebug() << SerialTend[i];
        }

    }
    */

    //UI填充
    QList<QString> FindMap = UsbHash.values();

    QList<QComboBox*> Comboxlist;
    int QBoxk = 0;
    Comboxlist << ui->Port1 << ui->Port2 << ui->Port3
               << ui->Port4 << ui->Port5 << ui->Port6
               << ui->Port7 << ui->Port8 << ui->Port9
               << ui->PortA << ui->PortB << ui->PortC
               << ui->PortD << ui->PortE << ui->PortF
               << ui->PortG << ui->PortH << ui->PortI
               << ui->PortJ << ui->PortK << ui->PortL
               << ui->PortM << ui->PortN << ui->PortO
               << ui->PortP << ui->PortQ << ui->PortR
               << ui->PortS << ui->PortT << ui->PortU
               << ui->PortV << ui->PortW;

    QList<QComboBox*> Tboxlist;
    int Tboxk = 0;
    Tboxlist << ui->Tboard1 << ui->Tboard2 << ui->Tboard3
             << ui->Tboard4 << ui->Tboard5 << ui->Tboard6
             << ui->Tboard7 << ui->Tboard8;

    for(int k = 0;k < FindMap.count();k++)
    {
        //qDebug() << "FindMap=" << FindMap[k];
        for(int j = 0;j < SerialTend.count();j++)
        {

            if(FindMap[k].contains("H36_H37") && SerialTend[j].contains(FindMap[k]) && SerialTend[j].contains("cu"))
            {
                while((QBoxk)%4 != 0)
                {
                    QBoxk++;
                }
                //qDebug() << "SerialTend=" << SerialTend[j];
                Tboxlist[Tboxk]->addItem(SerialTend[j]);
                Tboxk++;

                break;
            }
            else if(SerialTend[j].contains(FindMap[k]) && SerialTend[j].contains("cu"))
            {
                //qDebug() << "SerialTend=" << SerialTend[j];
                Comboxlist[QBoxk]->addItem(SerialTend[j]);
                QBoxk++;

                break;
            }

        }
    }


    //UI参数录入
    PNameList << "PortName" << ui->Port1->currentText() << ui->Port2->currentText() << ui->Port3->currentText()
             << ui->Port4->currentText() << ui->Port5->currentText() << ui->Port6->currentText()
             << ui->Port7->currentText() << ui->Port8->currentText() << ui->Port9->currentText()
             << ui->PortA->currentText() << ui->PortB->currentText() << ui->PortC->currentText()
             << ui->PortD->currentText() << ui->PortE->currentText() << ui->PortF->currentText()
             << ui->PortG->currentText() << ui->PortH->currentText() << ui->PortI->currentText()
             << ui->PortJ->currentText() << ui->PortK->currentText() << ui->PortL->currentText()
             << ui->PortM->currentText() << ui->PortN->currentText() << ui->PortO->currentText()
             << ui->PortP->currentText() << ui->PortQ->currentText() << ui->PortR->currentText()
             << ui->PortS->currentText() << ui->PortT->currentText() << ui->PortU->currentText()
             << ui->PortV->currentText() << ui->PortW->currentText();

    MNameList << "TboardName" << ui->Tboard1->currentText() << ui->Tboard1->currentText() << ui->Tboard1->currentText() << ui->Tboard1->currentText()
             << ui->Tboard2->currentText() << ui->Tboard2->currentText() << ui->Tboard2->currentText() << ui->Tboard2->currentText()
             << ui->Tboard3->currentText() << ui->Tboard3->currentText() << ui->Tboard3->currentText() << ui->Tboard3->currentText()
             << ui->Tboard4->currentText() << ui->Tboard4->currentText() << ui->Tboard4->currentText() << ui->Tboard4->currentText()
             << ui->Tboard5->currentText() << ui->Tboard5->currentText() << ui->Tboard5->currentText() << ui->Tboard5->currentText()
             << ui->Tboard6->currentText() << ui->Tboard6->currentText() << ui->Tboard6->currentText() << ui->Tboard6->currentText()
             << ui->Tboard7->currentText() << ui->Tboard7->currentText() << ui->Tboard7->currentText() << ui->Tboard7->currentText()
             << ui->Tboard8->currentText() << ui->Tboard8->currentText() << ui->Tboard8->currentText() << ui->Tboard8->currentText();

    /*
    for(int j = 0;j < SerialTend.count();j++)
    {
        if(SerialTend[j].contains(FindMap[0]) && SerialTend[j].contains("cu"))
        {
            ui->Port1->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[1]) && SerialTend[j].contains("cu"))
        {
            ui->Port2->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[2]) && SerialTend[j].contains("cu"))
        {
            ui->Port3->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[3]) && SerialTend[j].contains("cu"))
        {
            ui->Port4->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[4]) && SerialTend[j].contains("cu"))
        {
            ui->Tboard1->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[5]) && SerialTend[j].contains("cu"))
        {
            ui->Port5->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[6]) && SerialTend[j].contains("cu"))
        {
            ui->Port6->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[7]) && SerialTend[j].contains("cu"))
        {
            ui->Port7->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[8]) && SerialTend[j].contains("cu"))
        {
            ui->Port8->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[9]) && SerialTend[j].contains("cu"))
        {
            ui->Tboard2->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[10]) && SerialTend[j].contains("cu"))
        {
            ui->Port9->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[11]) && SerialTend[j].contains("cu"))
        {
            ui->PortA->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[12]) && SerialTend[j].contains("cu"))
        {
            ui->PortB->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[13]) && SerialTend[j].contains("cu"))
        {
            ui->PortC->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[14]) && SerialTend[j].contains("cu"))
        {
            ui->Tboard3->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[15]) && SerialTend[j].contains("cu"))
        {
            ui->PortD->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[16]) && SerialTend[j].contains("cu"))
        {
            ui->PortE->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[17]) && SerialTend[j].contains("cu"))
        {
            ui->PortF->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[18]) && SerialTend[j].contains("cu"))
        {
            ui->PortG->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[19]) && SerialTend[j].contains("cu"))
        {
            ui->Tboard4->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[20]) && SerialTend[j].contains("cu"))
        {
            ui->PortH->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[21]) && SerialTend[j].contains("cu"))
        {
            ui->PortI->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[22]) && SerialTend[j].contains("cu"))
        {
            ui->PortJ->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[23]) && SerialTend[j].contains("cu"))
        {
            ui->PortK->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[24]) && SerialTend[j].contains("cu"))
        {
            ui->Tboard5->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[25]) && SerialTend[j].contains("cu"))
        {
            ui->PortL->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[26]) && SerialTend[j].contains("cu"))
        {
            ui->PortM->addItem(SerialTend[j]);
        }
        else if(SerialTend[j].contains(FindMap[27]) && SerialTend[j].contains("cu"))
        {
            ui->PortA->addItem(SerialTend[j]);
        }
    }
    */
    //PortInit();
    connect(m_pTimer,&QTimer::timeout,this,&MainWindow::ThreadInit);
    connect(m_pTimerEnd,&QTimer::timeout,this,&MainWindow::starttaskEnd);
}

//串口数据与Process匹配
void MainWindow::PortFind()
{
    PortName.clear();

    QString description;
    QString manufacturer;
    QString serialNumber;
    //QStringList PortName;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        description = info.description();
        manufacturer = info.manufacturer();
        serialNumber = info.serialNumber();
        PortName << info.portName();

        //qDebug() << info.portName();
        //        << (!description.isEmpty() ? description : "")
        //        << (!manufacturer.isEmpty() ? manufacturer : "")
        //        << (!serialNumber.isEmpty() ? serialNumber : "")
        //        << info.systemLocation()
        //        << (info.vendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : "")
        //        << (info.productIdentifier() ? QString::number(info.productIdentifier(), 16) : "");
    }

    //const QSerialPortInfo endinfo = infos.at(infos.count()-1);
    //qDebug() << "endinfoPortname=" <<endinfo.portName();
}

//初始化串口数据
void MainWindow::InitChatPort(QSerialPort* ChatPort,QString PortName)
{
    if(PortName == "")
    {
        return;
    }

    ChatPort->setPortName(PortName);
    ChatPort->setBaudRate(203400);
    ChatPort->setDataBits(QSerialPort::Data8);
    ChatPort->setStopBits(QSerialPort::OneStop);
    ChatPort->setParity(QSerialPort::NoParity);
    ChatPort->setFlowControl(QSerialPort::NoFlowControl);
    //connect(ChatPort,&QSerialPort::readyRead,this,&MainWindow::BoxairRead);
}

void MainWindow::InitMcuPort(QSerialPort* McuPort,QString PortName)
{
    if(PortName == "")
    {
        return;
    }

    //McuPort = new QSerialPort;

    McuPort->setPortName(PortName);
    McuPort->setBaudRate(115200);
    McuPort->setDataBits(QSerialPort::Data8);
    McuPort->setStopBits(QSerialPort::OneStop);
    McuPort->setParity(QSerialPort::NoParity);
    McuPort->setFlowControl(QSerialPort::NoFlowControl);
    //connect(McuPort,&QSerialPort::readyRead,this,&MainWindow::BoxmcuRead);
}

void MainWindow::ParseOrder(QString Ostr)
{
    QString timedelay = Ostr.mid(10,Ostr.indexOf(']')-10);
    int timetend = timedelay.toDouble()*1000;

    QEventLoop loop;
    QTimer::singleShot(timetend,&loop,SLOT(quit()));
    loop.exec();

    /*
    else if(str == "case charger control")
    {
        ChatPort->write("batman data\n");

        QEventLoop loop;
        QTimer::singleShot(2000,&loop,SLOT(quit()));
        loop.exec();
    }
    else if(str == "bud charger control")
    {
        ChatPort->write("bud state\n");

        QEventLoop loop;
        QTimer::singleShot(2000,&loop,SLOT(quit()));
        loop.exec();
    }
    */
}

void MainWindow::ParseOrder2(QString Ostr,QString PortName,QSerialPort* McuPort)
{
    if(McuPort->isOpen() || McuPort->open(QIODevice::ReadWrite))
    {
        int Pindex = PNameList.indexOf(PortName);
        int Cindex = Pindex % 4;

        if(McuPort->isWritable())
        {
            if(Ostr == "[usb vbus off]\n")
            {
                if(Cindex == 1)
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 04 20 04 00 04",Hextend);
                    McuPort->write(Hextend);
                }
                else if(Cindex == 2)
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 04 20 03 00 6F",Hextend);
                    McuPort->write(Hextend);
                }
                else if(Cindex == 3)
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 04 20 02 00 7A",Hextend);
                    McuPort->write(Hextend);
                }
                else
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 04 20 01 00 45",Hextend);
                    McuPort->write(Hextend);
                }


            }
            else if(Ostr == "[usb vbus on]\n")
            {

                if(Cindex == 1)
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 04 20 04 01 03",Hextend);
                    McuPort->write(Hextend);
                }
                else if(Cindex == 2)
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 04 20 03 01 68",Hextend);
                    McuPort->write(Hextend);
                }
                else if(Cindex == 3)
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 04 20 02 01 7D",Hextend);
                    McuPort->write(Hextend);
                }
                else
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 04 20 01 01 42",Hextend);
                    McuPort->write(Hextend);
                }
            }
            else if(Ostr == "[wireless charger power on]\n")
            {
                if(Cindex == 1)
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 04 22 04 00 D2",Hextend);
                    McuPort->write(Hextend);
                }
                else if(Cindex == 2)
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 04 22 03 00 B9",Hextend);
                    McuPort->write(Hextend);
                }
                else if(Cindex == 3)
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 04 22 02 00 AC",Hextend);
                    McuPort->write(Hextend);
                }
                else
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 04 22 01 00 93",Hextend);
                    McuPort->write(Hextend);
                }
            }
            else if(Ostr == "[wireless charger power off]\n")
            {
                if(Cindex == 1)
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 04 22 04 01 D5",Hextend);
                    McuPort->write(Hextend);
                }
                else if(Cindex == 2)
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 04 22 03 01 BE",Hextend);
                    McuPort->write(Hextend);
                }
                else if(Cindex == 3)
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 04 22 02 01 AB",Hextend);
                    McuPort->write(Hextend);
                }
                else
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 04 22 01 01 94",Hextend);
                    McuPort->write(Hextend);
                }
            }
            else if(Ostr == "[cb get vbus info]\n")
            {
                if(Cindex == 1)
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 03 01 04 C0",Hextend);
                    McuPort->write(Hextend);
                }
                else if(Cindex == 2)
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 03 01 03 D5",Hextend);
                    McuPort->write(Hextend);
                }
                else if(Cindex == 3)
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 03 01 02 D2",Hextend);
                    McuPort->write(Hextend);
                }
                else
                {
                    QByteArray Hextend;
                    StringToHex("55 AA 03 01 01 DB",Hextend);
                    McuPort->write(Hextend);
                }
            }

            while(McuPort->waitForReadyRead(200))
            {
                QByteArray data = McuPort->readAll();
                QString strdata = HexToString(data);
                //strdata = McuPort->portName() + "\n" + strdata;
                //ui->MesDisplay->append(strdata);
                if(Ostr == "[cb get vbus info]\n")
                {
                    QString tendstr = strdata.mid(9,5);
                    int Currentnum = 0;

                    if(tendstr[0] <= '9' && tendstr[0] >= '0')
                    {
                        Currentnum += (tendstr[0].toLatin1() - '0' )*16;
                    }
                    else
                    {
                        Currentnum += (tendstr[0].toLatin1() - 'A' + 10)*16;
                    }

                    if(tendstr[1] <= '9' && tendstr[1] >= '0')
                    {
                        Currentnum += (tendstr[1].toLatin1()-'0');
                    }
                    else
                    {
                        Currentnum += (tendstr[1].toLatin1()-'A'+10);
                    }

                    if(tendstr[3] <= '9' && tendstr[3] >= '0')
                    {
                        Currentnum += (tendstr[3].toLatin1()-'0')*16*16*16;
                    }
                    else
                    {
                        Currentnum += (tendstr[3].toLatin1()-'A'+10)*16*16*16;
                    }

                    if(tendstr[4] <= '9' && tendstr[4] >= '0')
                    {
                        Currentnum += (tendstr[4].toLatin1()-'0')*16*16;
                    }
                    else
                    {
                        Currentnum += (tendstr[4].toLatin1()-'A'+10)*16*16;
                    }

                    tendstr = QString::number(Currentnum) + "mA";
                    WriteCsv("/Users/RELTEST/"+ PortName + "-savedata.csv",Ostr + "\n电流为：" + tendstr + "\n");
                }
                else if(Ostr == "[usb vbus off]\n" || Ostr == "[usb vbus on]\n")
                {
                    QString tendstr = strdata.mid(9,2);
                    if(tendstr == "00")
                    {
                        WriteCsv("/Users/RELTEST/"+ PortName + "-savedata.csv",Ostr + "\nvbus关闭\n");
                    }
                    else
                    {
                        WriteCsv("/Users/RELTEST/"+ PortName + "-savedata.csv",Ostr + "\nvbus打开\n");
                    }
                }
                else
                {
                    WriteCsv("/Users/RELTEST/"+ PortName + "-savedata.csv",Ostr + "\n命令处理代码还没编写好\n");
                }

                //qDebug() << "Port1box1back=" << strdata;
                McuPort->clear();
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/" + PortName + "-savedata.csv","Control box in Queue");
        }

    }
}

int MainWindow::ParseOrder3(QString Ostr,int result,QString Nextstr,QSerialPort* ChatPort)
{
    if(ChatPort->isOpen() || ChatPort->open(QIODevice::ReadWrite))
    {
        if(ChatPort->isWritable())
        {
            ChatPort->write(Ostr.toLocal8Bit());

            int timedelay = 200;
            if(Ostr == "batman data\n")
            {
                timedelay = 500;
            }
            else if(Ostr == "bud status\n")
            {
                timedelay = 500;
            }

            while(ChatPort->waitForReadyRead(timedelay))
            {

                QByteArray data = ChatPort->readAll();
                QString strdata = data;
                //ui->MesDisplay->append(ChatPort1->portName() + "\n" + strdata);
                if(strdata != "")
                {
                    WriteCsv("/Users/RELTEST/" + ChatPort->portName() + "-savedata.csv",Ostr + "\n" + strdata);
                }
                else
                {
                    WriteCsv("/Users/RELTEST/" + ChatPort->portName() + "-savedata.csv",Ostr + "\n" + "返回为空");
                }


                if(Ostr == "batman data\n")
                {
                    if(strdata.contains("UISOC"))
                    {
                        int index = strdata.indexOf("UISOC");

                        int beginindex = index + 6;
                        int endindex = index + 6;
                        while(strdata[endindex] != '%')
                        {
                            endindex++;
                        }

                        QString tendValue = strdata.mid(beginindex,endindex - beginindex);

                        if(tendValue.toInt() >= 90)
                        {
                            result = result + 1;
                        }
                        else if(tendValue.toInt() <= 10)
                        {
                            result = result + 12;
                        }
                        else{
                            result = result + 22;
                        }
                        //VolValue 判定-》controlbox命令
                    }
                }
                else if(Ostr == "bud status\n")
                {
                    QStringList strdatalist = strdata.split("Left Bud Status");
                    QString tendValue;
                    QString tendValue1;
                    QString tendValue2;

                    if(strdatalist[0].contains("SoC"))//右耳
                    {
                        int index = strdatalist[0].indexOf("SoC");

                        int beginindex = index + 4;
                        int endindex = index + 4;
                        while(strdatalist[0][endindex] != ',')
                        {
                            endindex++;
                        }

                        tendValue1 = strdatalist[0].mid(beginindex,endindex - beginindex);


                    }

                    if(strdatalist[1].contains("SoC"))//左耳
                    {
                        int index = strdatalist[1].indexOf("SoC");
                        int beginindex = index + 4;
                        int endindex = index + 4;
                        while(strdatalist[1][endindex] != ',')
                        {
                            endindex++;
                        }

                        tendValue2 = strdatalist[1].mid(beginindex,endindex - beginindex);
                    }

                    if(Nextstr.contains("L Bud"))
                    {
                        tendValue = tendValue2;
                    }
                    else{
                        tendValue = tendValue1;
                    }

                    if(tendValue.toInt() >= 70)//fully charger
                    {
                        result = result + 1;
                    }
                    else if(tendValue.toInt() <= 30)
                    {
                        result = result + 12;
                    }
                    else{
                        result = result + 22;
                    }

                }

                ChatPort->clear();
                //ChatPort->close();
                //qDebug() << "Port1box1back=" << strdata;
            }

            //ChatPort->deleteLater();
        }
        else
        {
            WriteCsv("/Users/RELTEST/" + ChatPort->portName() + "-savedata.csv","Product in Queue");
        }
    }

    return result;
}

//数据处理
//按键控制
void MainWindow::on_ClearButton_clicked()
{
    ui->MesDisplay->clear();
}

void MainWindow::on_BeginButton_clicked()
{
    UsbList.clear();
    LocateList.clear();
    SerialList.clear();
    UsbHash.clear();
    m_pTimerEnd->stop();

    //Predeal();
    ui->MesDisplay->append("测试开始，请勿关闭程序");
    ui->BeginButton->setEnabled(false);
    ui->ClearButton->setEnabled(false);
    ui->RePortButton->setEnabled(false);


    /*
    QFuture<void> Threadflag9;
    QFuture<void> ThreadflagA;
    QFuture<void> ThreadflagB;
    QFuture<void> ThreadflagC;
    QFuture<void> ThreadflagD;
    QFuture<void> ThreadflagE;
    QFuture<void> ThreadflagF;
    QFuture<void> ThreadflagG;
    */

    //QMetaObject::invokeMethod(this,"starttask1",Qt::QueuedConnection);
    if(PNameList[1] != "")
    {
       //QMetaObject::invokeMethod(this,"starttask1",Qt::QueuedConnection);
       Threadflag1 = QtConcurrent::run(&MainWindow::starttask1,this);
    }

    if(PNameList[2] != "")
    {
        Threadflag2 = QtConcurrent::run(&MainWindow::starttask2,this);
    }

    if(PNameList[3] != "")
    {
        Threadflag3 = QtConcurrent::run(&MainWindow::starttask3,this);
    }

    if(PNameList[4] != "")
    {
        Threadflag4 = QtConcurrent::run(&MainWindow::starttask4,this);
    }

    if(PNameList[5] != "")
    {
       Threadflag5 = QtConcurrent::run(&MainWindow::starttask5,this);
    }

    if(PNameList[6] != "")
    {
        Threadflag6 = QtConcurrent::run(&MainWindow::starttask6,this);
    }

    if(PNameList[7] != "")
    {
        Threadflag7 = QtConcurrent::run(&MainWindow::starttask7,this);
    }

    if(PNameList[8] != "")
    {
        Threadflag8 = QtConcurrent::run(&MainWindow::starttask8,this);
    }

    while(!Threadflag1.isFinished())
    {
        QApplication::processEvents(QEventLoop::AllEvents,100);
    }
    while(!Threadflag2.isFinished())
    {
        QApplication::processEvents(QEventLoop::AllEvents,100);
    }
    while(!Threadflag3.isFinished())
    {
        QApplication::processEvents(QEventLoop::AllEvents,100);
    }
    while(!Threadflag4.isFinished())
    {
        QApplication::processEvents(QEventLoop::AllEvents,100);
    }
    while(!Threadflag5.isFinished())
    {
        QApplication::processEvents(QEventLoop::AllEvents,100);
    }
    while(!Threadflag6.isFinished())
    {
        QApplication::processEvents(QEventLoop::AllEvents,100);
    }
    while(!Threadflag7.isFinished())
    {
        QApplication::processEvents(QEventLoop::AllEvents,100);
    }
    while(!Threadflag8.isFinished())
    {
        QApplication::processEvents(QEventLoop::AllEvents,100);
    }

    Threadflag1.cancel();
    Threadflag2.cancel();
    Threadflag3.cancel();
    Threadflag4.cancel();
    Threadflag5.cancel();
    Threadflag6.cancel();
    Threadflag7.cancel();
    Threadflag8.cancel();


    //QThreadPool::globalInstance()->waitForDone(-1);
    ui->MesDisplay->append("目前线程数=" + QString::number(QThreadPool::globalInstance()->activeThreadCount()));

    ChatPort1 = new QSerialPort;
    InitChatPort(ChatPort1,PNameList[1]);
    McuPort1 = new QSerialPort;
    InitChatPort(McuPort1,MNameList[1]);
    //m_Tpool->setMaxThreadCount(16)
    //m_pTimer->start(ThreadTime);
}

void MainWindow::on_StopButton_clicked()
{
    m_pTimer->stop();

    //QThreadPool::globalInstance()->waitForDone(-1);
    ui->MesDisplay->append("目前线程数=" + QString::number(QThreadPool::globalInstance()->activeThreadCount()));

    Threadflag1 = QtConcurrent::run(&MainWindow::Posttask1,this);
    Threadflag2 = QtConcurrent::run(&MainWindow::Posttask2,this);
    Threadflag3 = QtConcurrent::run(&MainWindow::Posttask3,this);
    Threadflag4 = QtConcurrent::run(&MainWindow::Posttask4,this);
    Threadflag5 = QtConcurrent::run(&MainWindow::Posttask5,this);
    Threadflag6 = QtConcurrent::run(&MainWindow::Posttask6,this);
    Threadflag7 = QtConcurrent::run(&MainWindow::Posttask7,this);
    Threadflag8 = QtConcurrent::run(&MainWindow::Posttask8,this);

    while(!Threadflag1.isFinished())
    {
        QApplication::processEvents(QEventLoop::AllEvents,100);
    }
    while(!Threadflag2.isFinished())
    {
        QApplication::processEvents(QEventLoop::AllEvents,100);
    }
    while(!Threadflag3.isFinished())
    {
        QApplication::processEvents(QEventLoop::AllEvents,100);
    }
    while(!Threadflag4.isFinished())
    {
        QApplication::processEvents(QEventLoop::AllEvents,100);
    }
    while(!Threadflag5.isFinished())
    {
        QApplication::processEvents(QEventLoop::AllEvents,100);
    }
    while(!Threadflag6.isFinished())
    {
        QApplication::processEvents(QEventLoop::AllEvents,100);
    }
    while(!Threadflag7.isFinished())
    {
        QApplication::processEvents(QEventLoop::AllEvents,100);
    }
    while(!Threadflag8.isFinished())
    {
        QApplication::processEvents(QEventLoop::AllEvents,100);
    }

    Threadflag1.cancel();
    Threadflag2.cancel();
    Threadflag3.cancel();
    Threadflag4.cancel();
    Threadflag5.cancel();
    Threadflag6.cancel();
    Threadflag7.cancel();
    Threadflag8.cancel();

    //QThreadPool::globalInstance()->waitForDone(-1);
    ui->MesDisplay->append("目前线程数=" + QString::number(QThreadPool::globalInstance()->activeThreadCount()));

    ui->BeginButton->setEnabled(true);
    ui->ClearButton->setEnabled(true);
    ui->RePortButton->setEnabled(true);
    m_pTimerEnd->start(ThreadTime);
}

//刷新现有串口配置
void MainWindow::on_RePortButton_clicked()
{
    ui->Port1->clear();
    ui->Port2->clear();
    ui->Port3->clear();
    ui->Port4->clear();
    ui->Port5->clear();
    ui->Port6->clear();
    ui->Port7->clear();
    ui->Port8->clear();
    ui->Port9->clear();
    ui->PortA->clear();
    ui->PortB->clear();
    ui->PortC->clear();
    ui->PortD->clear();
    ui->PortE->clear();
    ui->PortF->clear();
    ui->PortG->clear();
    ui->PortH->clear();
    ui->PortI->clear();
    ui->PortJ->clear();
    ui->PortK->clear();
    ui->PortL->clear();
    ui->PortM->clear();
    ui->PortN->clear();
    ui->PortO->clear();
    ui->PortP->clear();
    ui->PortQ->clear();
    ui->PortR->clear();
    ui->PortS->clear();
    ui->PortT->clear();
    ui->PortU->clear();
    ui->PortV->clear();
    ui->PortW->clear();

    ui->Tboard1->clear();
    ui->Tboard2->clear();
    ui->Tboard3->clear();
    ui->Tboard4->clear();
    ui->Tboard5->clear();
    ui->Tboard6->clear();
    ui->Tboard7->clear();
    ui->Tboard8->clear();

    //重新填充串口
    cmd->close();

    cmd->start("bash");
    cmd->waitForStarted();
    cmd->write("system_profiler SPUSBDataType\n");
}

//Thread deal math
//1-16
void MainWindow::starttask1()
{
    if(flag1)
    {
        flag1 = false;

        ChatPort1 = new QSerialPort;
        InitChatPort(ChatPort1,PNameList[1]);
        McuPort1 = new QSerialPort;
        InitChatPort(McuPort1,MNameList[1]);


        qDebug() << QThread::currentThread();

        if((ChatPort1->isOpen() || ChatPort1->open(QIODevice::ReadWrite)) && ChatPort1->isWritable())
        {
            if(Preflag1)
            {
                Preflag1 = false;
                WriteCsv("/Users/RELTEST/" + ChatPort1->portName() + "-savedata.csv","DUT in Pre");

                for(int i = 0;i < Prelist1.count();i++)//CommandList.count()
                {
                    QString tend = Prelist1[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort1->portName(),McuPort1);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Prelist1[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort1);
                    }
                }

                Loopflag1 = true;
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort1->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort1串口未打开");
        }


        McuPort1->close();
        McuPort1->deleteLater();
        ChatPort1->close();
        ChatPort1->deleteLater();


        flag1 = true;
    }
}

void MainWindow::starttask2()
{
    if(flag2)
    {
        flag2 = false;

        ChatPort2 = new QSerialPort;
        InitChatPort(ChatPort2,PNameList[2]);
        McuPort2 = new QSerialPort;
        InitChatPort(McuPort2,MNameList[2]);

        if((ChatPort2->isOpen() || ChatPort2->open(QIODevice::ReadWrite)) && ChatPort2->isWritable())
        {
            if(Preflag2)
            {
                Preflag2 = false;
                WriteCsv("/Users/RELTEST/" + ChatPort2->portName() + "-savedata.csv","DUT in Pre");

                for(int i = 0;i < Prelist2.count();i++)//CommandList.count()
                {
                    QString tend = Prelist2[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort2->portName(),McuPort2);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Prelist2[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort2);
                    }
                }

                Loopflag2 = true;
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort2->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort2串口未打开");
        }

        McuPort2->close();
        McuPort2->deleteLater();
        ChatPort2->close();
        ChatPort2->deleteLater();

        flag2 = true;
    }
}

void MainWindow::starttask3()
{
    if(flag3)
    {
        flag3 = false;

        ChatPort3 = new QSerialPort;
        InitChatPort(ChatPort3,PNameList[3]);
        McuPort3 = new QSerialPort;
        InitChatPort(McuPort3,MNameList[3]);

        if((ChatPort3->isOpen() || ChatPort3->open(QIODevice::ReadWrite)) && ChatPort3->isWritable())
        {
            if(Preflag3)
            {
                Preflag3 = false;
                WriteCsv("/Users/RELTEST/" + ChatPort3->portName() + "-savedata.csv","DUT in Pre");

                for(int i = 0;i < Prelist3.count();i++)//CommandList.count()
                {
                    QString tend = Prelist3[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort3->portName(),McuPort3);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Prelist3[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort3);
                    }
                }

                Loopflag3 = true;
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort3->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort3串口未打开");
        }

        McuPort3->close();
        McuPort3->deleteLater();
        ChatPort3->close();
        ChatPort3->deleteLater();

        flag3 = true;
    }
}

void MainWindow::starttask4()
{
    if(flag4)
    {
        flag4 = false;

        ChatPort4 = new QSerialPort;
        InitChatPort(ChatPort4,PNameList[4]);
        McuPort4 = new QSerialPort;
        InitChatPort(McuPort4,MNameList[4]);

        if((ChatPort4->isOpen() || ChatPort4->open(QIODevice::ReadWrite)) && ChatPort4->isWritable())
        {
            if(Preflag4)
            {
                Preflag4 = false;
                WriteCsv("/Users/RELTEST/" + ChatPort4->portName() + "-savedata.csv","DUT in Pre");

                for(int i = 0;i < Prelist4.count();i++)//CommandList.count()
                {
                    QString tend = Prelist4[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort4->portName(),McuPort4);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Prelist4[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort4);
                    }
                }

                Loopflag4 = true;
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort4->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort4串口未打开");
        }

        McuPort4->close();
        McuPort4->deleteLater();
        ChatPort4->close();
        ChatPort4->deleteLater();

        flag4 = true;
    }
}

void MainWindow::starttask5()
{
    if(flag5)
    {
        flag5 = false;

        ChatPort5 = new QSerialPort;
        InitChatPort(ChatPort5,PNameList[5]);
        McuPort5 = new QSerialPort;
        InitChatPort(McuPort5,MNameList[5]);

        if((ChatPort5->isOpen() || ChatPort5->open(QIODevice::ReadWrite)) && ChatPort5->isWritable())
        {
            if(Preflag5)
            {
                Preflag5 = false;
                WriteCsv("/Users/RELTEST/" + ChatPort5->portName() + "-savedata.csv","DUT in Pre");

                for(int i = 0;i < Prelist5.count();i++)//CommandList.count()
                {
                    QString tend = Prelist5[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort5->portName(),McuPort5);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Prelist5[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort5);
                    }
                }

                Loopflag5 = true;
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort5->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort5串口未打开");
        }

        McuPort5->close();
        McuPort5->deleteLater();
        ChatPort5->close();
        ChatPort5->deleteLater();

        flag5 = true;
    }
}

void MainWindow::starttask6()
{
    if(flag6)
    {
        flag6 = false;

        ChatPort6 = new QSerialPort;
        InitChatPort(ChatPort6,PNameList[6]);
        McuPort6 = new QSerialPort;
        InitChatPort(McuPort6,MNameList[6]);

        if((ChatPort6->isOpen() || ChatPort6->open(QIODevice::ReadWrite)) && ChatPort6->isWritable())
        {
            if(Preflag6)
            {
                Preflag6 = false;
                WriteCsv("/Users/RELTEST/" + ChatPort6->portName() + "-savedata.csv","DUT in Pre");

                for(int i = 0;i < Prelist6.count();i++)//CommandList.count()
                {
                    QString tend = Prelist6[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort6->portName(),McuPort6);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Prelist6[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort6);
                    }
                }

                Loopflag6 = true;
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort6->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort6串口未打开");
        }

        McuPort6->close();
        McuPort6->deleteLater();
        ChatPort6->close();
        ChatPort6->deleteLater();

        flag6 = true;
    }
}

void MainWindow::starttask7()
{
    if(flag7)
    {
        flag7 = false;

        ChatPort7 = new QSerialPort;
        InitChatPort(ChatPort7,PNameList[7]);
        McuPort7 = new QSerialPort;
        InitChatPort(McuPort7,MNameList[7]);

        if((ChatPort7->isOpen() || ChatPort7->open(QIODevice::ReadWrite)) && ChatPort7->isWritable())
        {
            if(Preflag7)
            {
                Preflag7 = false;
                WriteCsv("/Users/RELTEST/" + ChatPort7->portName() + "-savedata.csv","DUT in Pre");

                for(int i = 0;i < Prelist7.count();i++)//CommandList.count()
                {
                    QString tend = Prelist7[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort7->portName(),McuPort7);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Prelist7[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort7);
                    }
                }

                Loopflag7 = true;
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort7->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort7串口未打开");
        }

        McuPort7->close();
        McuPort7->deleteLater();
        ChatPort7->close();
        ChatPort7->deleteLater();

        flag7 = true;
    }
}

void MainWindow::starttask8()
{
    if(flag8)
    {
        flag8 = false;

        ChatPort8 = new QSerialPort;
        InitChatPort(ChatPort8,PNameList[8]);
        McuPort8 = new QSerialPort;
        InitChatPort(McuPort8,MNameList[8]);

        if((ChatPort8->isOpen() || ChatPort8->open(QIODevice::ReadWrite)) && ChatPort8->isWritable())
        {
            if(Preflag8)
            {
                Preflag8 = false;
                WriteCsv("/Users/RELTEST/" + ChatPort8->portName() + "-savedata.csv","DUT in Pre");

                for(int i = 0;i < Prelist8.count();i++)//CommandList.count()
                {
                    QString tend = Prelist8[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort8->portName(),McuPort8);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Prelist8[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort8);
                    }
                }

                Loopflag8 = true;
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort8->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort8串口未打开");
        }

        McuPort8->close();
        McuPort8->deleteLater();
        ChatPort8->close();
        ChatPort8->deleteLater();

        flag8 = true;
    }
}

//loop
void MainWindow::Looptask1()
{
    if(flag1)
    {
        flag1 = false;

        ChatPort1 = new QSerialPort;
        InitChatPort(ChatPort1,PNameList[1]);
        McuPort1 = new QSerialPort;
        InitChatPort(McuPort1,MNameList[1]);


        if((ChatPort1->isOpen() || ChatPort1->open(QIODevice::ReadWrite)) && ChatPort1->isWritable())
        {
            if(Loopflag1)
            {
                Loopflag1 = false;
                Loop1++;
                WriteCsv("/Users/RELTEST/" + ChatPort1->portName() + "-savedata.csv","DUT in Loop=" + QString::number(Loop1));



                for(int i = 0;i < Looplist1.count();i++)//CommandList.count()
                {
                    QString tend = Looplist1[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort1->portName(),McuPort1);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Looplist1[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort1);
                    }
                }

                Loopflag1 = true;
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort1->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort1串口未打开");
        }


        McuPort1->close();
        McuPort1->deleteLater();
        ChatPort1->close();
        ChatPort1->deleteLater();


        flag1 = true;
    }
}

void MainWindow::Looptask2()
{
    if(flag2)
    {
        flag2 = false;

        ChatPort2 = new QSerialPort;
        InitChatPort(ChatPort2,PNameList[2]);
        McuPort2 = new QSerialPort;
        InitChatPort(McuPort2,MNameList[2]);

        if((ChatPort2->isOpen() || ChatPort2->open(QIODevice::ReadWrite)) && ChatPort2->isWritable())
        {
            if(Loopflag2)
            {
                Loopflag2 = false;
                Loop2++;
                WriteCsv("/Users/RELTEST/" + ChatPort2->portName() + "-savedata.csv","DUT in Loop=" + QString::number(Loop2));


                for(int i = 0;i < Looplist2.count();i++)//CommandList.count()
                {
                    QString tend = Looplist2[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort2->portName(),McuPort2);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Looplist2[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort2);
                    }
                }

                Loopflag2 = true;
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort2->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort2串口未打开");
        }

        McuPort2->close();
        McuPort2->deleteLater();
        ChatPort2->close();
        ChatPort2->deleteLater();

        flag2 = true;
    }
}

void MainWindow::Looptask3()
{
    if(flag3)
    {
        flag3 = false;

        ChatPort3 = new QSerialPort;
        InitChatPort(ChatPort3,PNameList[3]);
        McuPort3 = new QSerialPort;
        InitChatPort(McuPort3,MNameList[3]);

        if((ChatPort3->isOpen() || ChatPort3->open(QIODevice::ReadWrite)) && ChatPort3->isWritable())
        {
            if(Loopflag3)
            {
                Loopflag3 = false;
                Loop3++;
                WriteCsv("/Users/RELTEST/" + ChatPort3->portName() + "-savedata.csv","DUT in Loop=" + QString::number(Loop3));


                for(int i = 0;i < Looplist3.count();i++)//CommandList.count()
                {
                    QString tend = Looplist3[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort3->portName(),McuPort3);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Looplist3[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort3);
                    }
                }

                Loopflag3 = true;
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort3->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort3串口未打开");
        }

        McuPort3->close();
        McuPort3->deleteLater();
        ChatPort3->close();
        ChatPort3->deleteLater();

        flag3 = true;
    }
}

void MainWindow::Looptask4()
{
    if(flag4)
    {
        flag4 = false;

        ChatPort4 = new QSerialPort;
        InitChatPort(ChatPort4,PNameList[4]);
        McuPort4 = new QSerialPort;
        InitChatPort(McuPort4,MNameList[4]);

        if((ChatPort4->isOpen() || ChatPort4->open(QIODevice::ReadWrite)) && ChatPort4->isWritable())
        {
            if(Loopflag4)
            {
                Loopflag4 = false;
                Loop4++;
                WriteCsv("/Users/RELTEST/" + ChatPort4->portName() + "-savedata.csv","DUT in Loop=" + QString::number(Loop4));


                for(int i = 0;i < Looplist4.count();i++)//CommandList.count()
                {
                    QString tend = Looplist4[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort4->portName(),McuPort4);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Looplist4[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort4);
                    }
                }

                Loopflag4 = true;
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort4->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort4串口未打开");
        }

        McuPort4->close();
        McuPort4->deleteLater();
        ChatPort4->close();
        ChatPort4->deleteLater();

        flag4 = true;
    }
}

void MainWindow::Looptask5()
{
    if(flag5)
    {
        flag5 = false;

        ChatPort5 = new QSerialPort;
        InitChatPort(ChatPort5,PNameList[5]);
        McuPort5 = new QSerialPort;
        InitChatPort(McuPort5,MNameList[5]);

        if((ChatPort5->isOpen() || ChatPort5->open(QIODevice::ReadWrite)) && ChatPort5->isWritable())
        {
            if(Loopflag5)
            {
                Loopflag5 = false;
                Loop5++;
                WriteCsv("/Users/RELTEST/" + ChatPort5->portName() + "-savedata.csv","DUT in Loop=" + QString::number(Loop5));

                for(int i = 0;i < Looplist5.count();i++)//CommandList.count()
                {
                    QString tend = Looplist5[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort5->portName(),McuPort5);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Looplist5[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort5);
                    }
                }

                Loopflag5 = true;
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort5->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort5串口未打开");
        }

        McuPort5->close();
        McuPort5->deleteLater();
        ChatPort5->close();
        ChatPort5->deleteLater();

        flag5 = true;
    }
}

void MainWindow::Looptask6()
{
    if(flag6)
    {
        flag6 = false;

        ChatPort6 = new QSerialPort;
        InitChatPort(ChatPort6,PNameList[6]);
        McuPort6 = new QSerialPort;
        InitChatPort(McuPort6,MNameList[6]);

        if((ChatPort6->isOpen() || ChatPort6->open(QIODevice::ReadWrite)) && ChatPort6->isWritable())
        {
            if(Loopflag6)
            {
                Loopflag6 = false;
                Loop6++;
                WriteCsv("/Users/RELTEST/" + ChatPort6->portName() + "-savedata.csv","DUT in Loop=" + QString::number(Loop6));


                for(int i = 0;i < Looplist6.count();i++)//CommandList.count()
                {
                    QString tend = Looplist6[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort6->portName(),McuPort6);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Looplist6[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort6);
                    }
                }

                Loopflag6 = true;
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort6->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort6串口未打开");
        }

        McuPort6->close();
        McuPort6->deleteLater();
        ChatPort6->close();
        ChatPort6->deleteLater();

        flag6 = true;
    }
}

void MainWindow::Looptask7()
{
    if(flag7)
    {
        flag7 = false;

        ChatPort7 = new QSerialPort;
        InitChatPort(ChatPort7,PNameList[7]);
        McuPort7 = new QSerialPort;
        InitChatPort(McuPort7,MNameList[7]);

        if((ChatPort7->isOpen() || ChatPort7->open(QIODevice::ReadWrite)) && ChatPort7->isWritable())
        {
            if(Loopflag7)
            {
                Loopflag7 = false;
                Loop7++;
                WriteCsv("/Users/RELTEST/" + ChatPort7->portName() + "-savedata.csv","DUT in Loop=" + QString::number(Loop7));


                for(int i = 0;i < Looplist7.count();i++)//CommandList.count()
                {
                    QString tend = Looplist7[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort7->portName(),McuPort7);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Looplist7[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort7);
                    }
                }

                Loopflag7 = true;
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort7->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort7串口未打开");
        }

        McuPort7->close();
        McuPort7->deleteLater();
        ChatPort7->close();
        ChatPort7->deleteLater();

        flag7 = true;
    }
}

void MainWindow::Looptask8()
{
    if(flag8)
    {
        flag8 = false;

        ChatPort8 = new QSerialPort;
        InitChatPort(ChatPort8,PNameList[8]);
        McuPort8 = new QSerialPort;
        InitChatPort(McuPort8,MNameList[8]);

        if((ChatPort8->isOpen() || ChatPort8->open(QIODevice::ReadWrite)) && ChatPort8->isWritable())
        {
            if(Loopflag8)
            {
                Loopflag8 = false;
                Loop8++;
                WriteCsv("/Users/RELTEST/" + ChatPort8->portName() + "-savedata.csv","DUT in Loop=" + QString::number(Loop8));


                for(int i = 0;i < Looplist8.count();i++)//CommandList.count()
                {
                    QString tend = Looplist8[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort8->portName(),McuPort8);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Looplist8[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort8);
                    }
                }

                Loopflag8 = true;
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort8->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort8串口未打开");
        }

        McuPort8->close();
        McuPort8->deleteLater();
        ChatPort8->close();
        ChatPort8->deleteLater();

        flag8 = true;
    }
}

//Post
void MainWindow::Posttask1()
{
    if(flag1)
    {
        flag1 = false;


        ChatPort1 = new QSerialPort;
        InitChatPort(ChatPort1,PNameList[1]);
        McuPort1 = new QSerialPort;
        InitChatPort(McuPort1,MNameList[1]);


        if((ChatPort1->isOpen() || ChatPort1->open(QIODevice::ReadWrite)) && ChatPort1->isWritable())
        {
            if(Postflag1)
            {
                Postflag1 = false;
                Loopflag1 = false;
                WriteCsv("/Users/RELTEST/" + ChatPort1->portName() + "-savedata.csv","DUT in Post");

                for(int i = 0;i < Postlist1.count();i++)//CommandList.count()
                {
                    QString tend = Postlist1[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort1->portName(),McuPort1);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Postlist1[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort1);
                    }
                }


            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort1->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort1串口未打开");
        }


        McuPort1->close();
        McuPort1->deleteLater();
        ChatPort1->close();
        ChatPort1->deleteLater();


        flag1 = true;
    }
}

void MainWindow::Posttask2()
{
    if(flag2)
    {
        flag2 = false;

        ChatPort2 = new QSerialPort;
        InitChatPort(ChatPort2,PNameList[2]);
        McuPort2 = new QSerialPort;
        InitChatPort(McuPort2,MNameList[2]);

        if((ChatPort2->isOpen() || ChatPort2->open(QIODevice::ReadWrite)) && ChatPort2->isWritable())
        {
            if(Postflag2)
            {
                Postflag2 = false;
                Loopflag2 = false;
                WriteCsv("/Users/RELTEST/" + ChatPort2->portName() + "-savedata.csv","DUT in Post");

                for(int i = 0;i < Postlist2.count();i++)//CommandList.count()
                {
                    QString tend = Postlist2[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort2->portName(),McuPort2);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Postlist2[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort2);
                    }
                }


            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort2->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort2串口未打开");
        }

        McuPort2->close();
        McuPort2->deleteLater();
        ChatPort2->close();
        ChatPort2->deleteLater();

        flag2 = true;
    }
}

void MainWindow::Posttask3()
{
    if(flag3)
    {
        flag3 = false;

        ChatPort3 = new QSerialPort;
        InitChatPort(ChatPort3,PNameList[3]);
        McuPort3 = new QSerialPort;
        InitChatPort(McuPort3,MNameList[3]);

        if((ChatPort3->isOpen() || ChatPort3->open(QIODevice::ReadWrite)) && ChatPort3->isWritable())
        {
            if(Postflag3)
            {
                Postflag3 = false;
                Loopflag3 = false;
                WriteCsv("/Users/RELTEST/" + ChatPort3->portName() + "-savedata.csv","DUT in Post");

                for(int i = 0;i < Postlist3.count();i++)//CommandList.count()
                {
                    QString tend = Postlist3[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort3->portName(),McuPort3);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Postlist3[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort3);
                    }
                }
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort3->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort3串口未打开");
        }

        McuPort3->close();
        McuPort3->deleteLater();
        ChatPort3->close();
        ChatPort3->deleteLater();

        flag3 = true;
    }
}

void MainWindow::Posttask4()
{
    if(flag4)
    {
        flag4 = false;

        ChatPort4 = new QSerialPort;
        InitChatPort(ChatPort4,PNameList[4]);
        McuPort4 = new QSerialPort;
        InitChatPort(McuPort4,MNameList[4]);

        if((ChatPort4->isOpen() || ChatPort4->open(QIODevice::ReadWrite)) && ChatPort4->isWritable())
        {
            if(Postflag4)
            {
                Postflag4 = false;
                Loopflag4 = false;
                WriteCsv("/Users/RELTEST/" + ChatPort4->portName() + "-savedata.csv","DUT in Post");

                for(int i = 0;i < Postlist4.count();i++)//CommandList.count()
                {
                    QString tend = Postlist4[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort4->portName(),McuPort4);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Postlist4[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort4);
                    }
                }
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort4->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort4串口未打开");
        }

        McuPort4->close();
        McuPort4->deleteLater();
        ChatPort4->close();
        ChatPort4->deleteLater();

        flag4 = true;
    }
}

void MainWindow::Posttask5()
{
    if(flag5)
    {
        flag5 = false;

        ChatPort5 = new QSerialPort;
        InitChatPort(ChatPort5,PNameList[5]);
        McuPort5 = new QSerialPort;
        InitChatPort(McuPort5,MNameList[5]);

        if((ChatPort5->isOpen() || ChatPort5->open(QIODevice::ReadWrite)) && ChatPort5->isWritable())
        {
            if(Postflag5)
            {
                Postflag5 = false;
                Loopflag5 = false;
                WriteCsv("/Users/RELTEST/" + ChatPort5->portName() + "-savedata.csv","DUT in Post");

                for(int i = 0;i < Postlist5.count();i++)//CommandList.count()
                {
                    QString tend = Postlist5[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort5->portName(),McuPort5);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Postlist5[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort5);
                    }
                }
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort5->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort5串口未打开");
        }

        McuPort5->close();
        McuPort5->deleteLater();
        ChatPort5->close();
        ChatPort5->deleteLater();

        flag5 = true;
    }
}

void MainWindow::Posttask6()
{
    if(flag6)
    {
        flag6 = false;

        ChatPort6 = new QSerialPort;
        InitChatPort(ChatPort6,PNameList[6]);
        McuPort6 = new QSerialPort;
        InitChatPort(McuPort6,MNameList[6]);

        if((ChatPort6->isOpen() || ChatPort6->open(QIODevice::ReadWrite)) && ChatPort6->isWritable())
        {
            if(Postflag6)
            {
                Postflag6 = false;
                Loopflag6 = false;
                WriteCsv("/Users/RELTEST/" + ChatPort6->portName() + "-savedata.csv","DUT in Post");

                for(int i = 0;i < Postlist6.count();i++)//CommandList.count()
                {
                    QString tend = Postlist6[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort6->portName(),McuPort6);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Postlist6[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort6);
                    }
                }
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort6->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort6串口未打开");
        }

        McuPort6->close();
        McuPort6->deleteLater();
        ChatPort6->close();
        ChatPort6->deleteLater();

        flag6 = true;
    }
}

void MainWindow::Posttask7()
{
    if(flag7)
    {
        flag7 = false;

        ChatPort7 = new QSerialPort;
        InitChatPort(ChatPort7,PNameList[7]);
        McuPort7 = new QSerialPort;
        InitChatPort(McuPort7,MNameList[7]);

        if((ChatPort7->isOpen() || ChatPort7->open(QIODevice::ReadWrite)) && ChatPort7->isWritable())
        {
            if(Postflag7)
            {
                Postflag7 = false;
                Loopflag7 = false;
                WriteCsv("/Users/RELTEST/" + ChatPort7->portName() + "-savedata.csv","DUT in Post");

                for(int i = 0;i < Postlist7.count();i++)//CommandList.count()
                {
                    QString tend = Postlist7[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort7->portName(),McuPort7);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Postlist7[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort7);
                    }
                }
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort7->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort7串口未打开");
        }

        McuPort7->close();
        McuPort7->deleteLater();
        ChatPort7->close();
        ChatPort7->deleteLater();
    }
}

void MainWindow::Posttask8()
{
    if(flag8)
    {
        flag8 = false;

        ChatPort8 = new QSerialPort;
        InitChatPort(ChatPort8,PNameList[8]);
        McuPort8 = new QSerialPort;
        InitChatPort(McuPort8,MNameList[8]);

        if((ChatPort8->isOpen() || ChatPort8->open(QIODevice::ReadWrite)) && ChatPort8->isWritable())
        {
            if(Postflag8)
            {
                Postflag8 = false;
                Loopflag8 = false;
                WriteCsv("/Users/RELTEST/" + ChatPort8->portName() + "-savedata.csv","DUT in Post");

                for(int i = 0;i < Postlist8.count();i++)//CommandList.count()
                {
                    QString tend = Postlist8[i];

                    if(tend.contains(">=") || tend.contains("<=")){ i = i + 10;}
                    else if(tend == "\n")
                    {

                    }
                    else if(tend.contains("sw delay"))
                    {
                        ParseOrder(tend);
                    }
                    else if(tend == "[usb vbus off]\n" || tend == "[usb vbus on]\n" || tend == "[wireless charger power on]\n" || tend == "[wireless charger power off]\n" || tend == "[cb get vbus info]\n")
                    {
                        ParseOrder2(tend,ChatPort8->portName(),McuPort8);
                    }
                    else
                    {
                        QString dstr = "";
                        if(tend == "bud status\n")
                        {
                            dstr = Postlist8[i+1];
                        }
                        i = ParseOrder3(tend,i,dstr,ChatPort8);
                    }
                }
            }
        }
        else
        {
            WriteCsv("/Users/RELTEST/ERROR/" + ChatPort8->portName() + "-error.csv","串口未打开");
            //ui->MesDisplay->append("ChatPort8串口未打开");
        }

        McuPort8->close();
        McuPort8->deleteLater();
        ChatPort8->close();
        ChatPort8->deleteLater();
    }
}


//Thread Init
void MainWindow::ThreadInit()
{
    if(QThreadPool::globalInstance()->activeThreadCount() == 0)
    {
        Threadflag1 = QtConcurrent::run(&MainWindow::Looptask1,this);
        Threadflag2 = QtConcurrent::run(&MainWindow::Looptask2,this);
        Threadflag3 = QtConcurrent::run(&MainWindow::Looptask3,this);
        Threadflag4 = QtConcurrent::run(&MainWindow::Looptask4,this);
        Threadflag5 = QtConcurrent::run(&MainWindow::Looptask5,this);
        Threadflag6 = QtConcurrent::run(&MainWindow::Looptask6,this);
        Threadflag7 = QtConcurrent::run(&MainWindow::Looptask7,this);
        Threadflag8 = QtConcurrent::run(&MainWindow::Looptask8,this);

        while(!Threadflag1.isFinished())
        {
            QApplication::processEvents(QEventLoop::AllEvents,100);
        }
        while(!Threadflag2.isFinished())
        {
            QApplication::processEvents(QEventLoop::AllEvents,100);
        }
        while(!Threadflag3.isFinished())
        {
            QApplication::processEvents(QEventLoop::AllEvents,100);
        }
        while(!Threadflag4.isFinished())
        {
            QApplication::processEvents(QEventLoop::AllEvents,100);
        }
        while(!Threadflag5.isFinished())
        {
            QApplication::processEvents(QEventLoop::AllEvents,100);
        }
        while(!Threadflag6.isFinished())
        {
            QApplication::processEvents(QEventLoop::AllEvents,100);
        }
        while(!Threadflag7.isFinished())
        {
            QApplication::processEvents(QEventLoop::AllEvents,100);
        }
        while(!Threadflag8.isFinished())
        {
            QApplication::processEvents(QEventLoop::AllEvents,100);
        }

        Threadflag1.cancel();
        Threadflag2.cancel();
        Threadflag3.cancel();
        Threadflag4.cancel();
        Threadflag5.cancel();
        Threadflag6.cancel();
        Threadflag7.cancel();
        Threadflag8.cancel();
    }

    QThreadPool::globalInstance()->waitForDone(-1);
    ui->MesDisplay->append("目前线程数=" + QString::number(QThreadPool::globalInstance()->activeThreadCount()));
    /*
    if(Threadflag9.isFinished())
    {
        Threadflag9 = QtConcurrent::run(&MainWindow::starttask9,this);
    }

    if(ThreadflagA.isFinished())
    {
        ThreadflagA = QtConcurrent::run(&MainWindow::starttaskA,this);
    }

    if(ThreadflagB.isFinished())
    {
        ThreadflagB = QtConcurrent::run(&MainWindow::starttaskB,this);
    }

    if(ThreadflagC.isFinished())
    {
        ThreadflagC = QtConcurrent::run(&MainWindow::starttaskC,this);
    }

    if(ThreadflagD.isFinished())
    {
        ThreadflagD = QtConcurrent::run(&MainWindow::starttaskD,this);
    }

    if(ThreadflagE.isFinished())
    {
        ThreadflagE = QtConcurrent::run(&MainWindow::starttaskE,this);
    }

    if(ThreadflagF.isFinished())
    {
        ThreadflagF = QtConcurrent::run(&MainWindow::starttaskF,this);
    }

    if(ThreadflagF.isFinished())
    {
        ThreadflagG = QtConcurrent::run(&MainWindow::starttaskG,this);
    }
    */
}

void MainWindow::starttaskEnd()
{

}
