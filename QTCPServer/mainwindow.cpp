#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_server = new QTcpServer();

    if(m_server->listen(QHostAddress::Any, 8080))
    {
       connect(this, &MainWindow::newMessage, this, &MainWindow::displayMessage);
       connect(m_server, &QTcpServer::newConnection, this, &MainWindow::newConnection);
       //ui->statusBar->showMessage("Server is listening...");

    }
    else
    {
        QMessageBox::critical(this,"QTCPServer",QString("Unable to start the server: %1.").arg(m_server->errorString()));
        exit(EXIT_FAILURE);
    }
    menu();
}

MainWindow::~MainWindow()
{
    foreach (QTcpSocket* socket, connection_set)
    {
        socket->close();
        socket->deleteLater();
    }

    m_server->close();
    m_server->deleteLater();

    delete ui;
}

void MainWindow::newConnection()
{
    while (m_server->hasPendingConnections())
        appendToSocketList(m_server->nextPendingConnection());
}

void MainWindow::appendToSocketList(QTcpSocket* socket)
{
    connection_set.insert(socket);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readSocket);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::discardSocket);
    connect(socket, &QAbstractSocket::errorOccurred, this, &MainWindow::displayError);
    ui->lineEdit->setText(QString::number(socket->socketDescriptor()));
    displayMessage(QString("INFO :: Client with sockd:%1 has just entered the room").arg(socket->socketDescriptor()));
}

void MainWindow::login(QString user, QString pass)
{
    QString message;
    QString ID, userName, name, surname, password;
    QFile file("C:/Users/ilker/Desktop/database.txt");
   if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
       return;
   QTextStream in(&file);

   while (!in.atEnd())
   {
       //QByteArray text = file.readAll();
       in >> ID >> userName >> name >> surname >> password;
       if (userName == user && password == pass)
       {
           message = "Giriş başarılı";
           qDebug() << message;
       }
       else
       {
           message = "giriş başarısız";
           qDebug() << message;
       }
   }
   foreach (QTcpSocket* socket, connection_set)
   {
       sendMessage(socket,message);
   }
   file.close();
}


static int moneyValue;
void MainWindow::deposit(QString Name, int value)
{
    QString message;
    QString ID, userName, name, surname, password, bankName, bankAccount;
    QFile file("C:/Users/ilker/Desktop/database.txt");
   if (!file.open(QIODevice::ReadOnly | QIODevice::Text |QIODevice::WriteOnly))
       return;
   QTextStream in(&file);
   int newValue = 0;
   while (!in.atEnd())
   {
       //QByteArray text = file.readAll();
       in >> ID >> userName >> name >> surname >> password >> bankName >> bankAccount >> moneyValue;

       if (userName == Name)
       {
           qDebug() << "ok";
           newValue = moneyValue + value;
           message = "gönderme tamam";
       }
   }
   file.close();
   if(file.open(QFile::WriteOnly)) //| QFile::Truncate))
   {
       in << ID << " " << userName << " " << name << " " << surname << " " << password << " " << bankName << " " << bankAccount << " " << newValue;
   }
   foreach (QTcpSocket* socket, connection_set)
   {
       sendMessage(socket,message);
   }
   file.close();
}
void MainWindow::readInfo()
{
       QString fileContent;
       QFile file("C:/Users/ilker/Desktop/database.txt");
       if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
           return;
       QTextStream in(&file);
       while (!in.atEnd())
       {
           fileContent.append(in.readAll());
       }

       foreach (QTcpSocket* socket, connection_set)
       {
           sendMessage(socket,fileContent);
       }
       file.close();

}


void MainWindow::readSocket()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());

    QByteArray buffer;

    QDataStream socketStream(socket);
    socketStream.setVersion(QDataStream::Qt_5_15);

    socketStream.startTransaction();
    socketStream >> buffer;

    if(!socketStream.commitTransaction())
    {
        QString message = QString("%1 :: Waiting for more data to come..").arg(socket->socketDescriptor());
        emit newMessage(message);
        return;
    }


    buffer = buffer.mid(128);

    QString message = QString::fromStdString(buffer.toStdString());// QString("%1 :: %2").arg(socket->socketDescriptor()).arg(QString::fromStdString(buffer.toStdString()));

    emit newMessage(message);
    if(message == "1")
    {
        readInfo();
    }
    else if (message == "2")
    {
        qDebug() << "heee";
        deposit("itopal",500);
    }
}

void MainWindow::discardSocket()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());
    QSet<QTcpSocket*>::iterator it = connection_set.find(socket);
    if (it != connection_set.end()){
        displayMessage(QString("INFO :: A client has just left the room").arg(socket->socketDescriptor()));
        connection_set.remove(*it);
    }
    refreshComboBox();

    socket->deleteLater();
}

void MainWindow::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
        case QAbstractSocket::RemoteHostClosedError:
        break;
        case QAbstractSocket::HostNotFoundError:
            QMessageBox::information(this, "QTCPServer", "The host was not found. Please check the host name and port settings.");
        break;
        case QAbstractSocket::ConnectionRefusedError:
            QMessageBox::information(this, "QTCPServer", "The connection was refused by the peer. Make sure QTCPServer is running, and check that the host name and port settings are correct.");
        break;
        default:
            QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
            QMessageBox::information(this, "QTCPServer", QString("The following error occurred: %1.").arg(socket->errorString()));
        break;
    }
}

void MainWindow::on_pushButton_sendMessage_clicked()
{
    QString receiver = ui->lineEdit->text();
    QString data = ui->lineEdit_message->text();
        foreach (QTcpSocket* socket,connection_set)
        {
            if(socket->socketDescriptor() == receiver.toLongLong())
            {
                sendMessage(socket, data);
                break;
            }
        }

    ui->lineEdit_message->clear();
}



void MainWindow::sendMessage(QTcpSocket* socket, QString str)
{
    if(socket)
    {
        if(socket->isOpen())
        {

            QDataStream socketStream(socket);
            socketStream.setVersion(QDataStream::Qt_5_15);

            QByteArray header;
            header.prepend(QString("fileType:message,fileName:null,fileSize:%1;").arg(str.size()).toUtf8());
            header.resize(128);

            QByteArray byteArray = str.toUtf8();
            byteArray.prepend(header);

            socketStream.setVersion(QDataStream::Qt_5_15);
            socketStream << byteArray;
            //socket->write(byteArray);
        }
        else
            QMessageBox::critical(this,"QTCPServer","Socket doesn't seem to be opened");
    }
    else
        QMessageBox::critical(this,"QTCPServer","Not connected");
}



void MainWindow::displayMessage(const QString& str)
{
    ui->textBrowser_receivedMessages->append(str);
}

void MainWindow::refreshComboBox(){
    ui->lineEdit->clear();
    ui->lineEdit->setText("Broadcast");
    foreach(QTcpSocket* socket, connection_set)
        ui->lineEdit->setText(QString::number(socket->socketDescriptor()));
}
