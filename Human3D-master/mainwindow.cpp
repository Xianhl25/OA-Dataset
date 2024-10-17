#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "stlfileloader.h"
#include "sensorloader.h"
#include <QDebug>
#include <QDesktopWidget>
#include <QVBoxLayout>
#include <QStyleFactory>
#include <QFileDialog>

#include <QTimer>
#include <QElapsedTimer>
#include <algorithm>
#include <tuple>
#include <deque>
//热力云图
#include "HotPlot.h"
#include <QFile>
#include <QTextStream>
#include <QVector>
#include <QPolygonF>
#include "globals.h"//变量
#include <QCoreApplication>
#include <QString>
DataPoint dataPoint;
QDateTime mystarttime;
int numb,numb1;

//初始化
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setWindowTitle("Outdoor Bioinformatics Collection System");
    moveWindowToCenter();

    SensorLoader *worker = new SensorLoader;
    worker->moveToThread(&workerThread);
    connect(this, SIGNAL(open(const QString)), worker, SLOT(parser(const QString)));
    connect(this, SIGNAL(stopped()), worker, SLOT(stopWork()));
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(worker, SIGNAL(resultReady(QVariant)), this, SLOT(slot_handleResult(QVariant)));
    APP_start();
    //增加IMU接收
    myserial = new QSerialPort();
    myserial = new QSerialPort();
    serial_flag = true;
    start_flag = true;
    setupPlot();//IMU图表初始化
    // 设置定时器用于更新图表
    QTimer *plotTimer = new QTimer(this);
    connect(plotTimer, &QTimer::timeout, this, &MainWindow::updatePlots);
    plotTimer->start(20); // 每20毫秒更新一次图表

    // 设置定时器用于更新角度
    QTimer *angleTimer = new QTimer(this);
    connect(angleTimer, &QTimer::timeout, this, &MainWindow::updateAngles);
    angleTimer->start(10);

    //多线程
    drawHeatmap_init();
    drawHeatmap2_init();
    // 多线程
    dataUpdater = new DataUpdater(colorMap, footprint, Points, nx, ny, minX, maxX, minY, maxY, this);
    connect(dataUpdater, &DataUpdater::dataUpdated, this, [=]() {
        ui->customPlot->replot(); // 收到数据更新信号后重绘图表
    });
    dataUpdater2 = new DataUpdater2(colorMap2, footprint2, Points2, nx2, ny2, minX2, maxX2, minY2, maxY2, this);
    connect(dataUpdater2, &DataUpdater2::dataUpdated, this, [=]() {
        ui->customPlot_2->replot(); // 收到数据更新信号后重绘图表
    });

    // 使用定时器请求数据更新
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, dataUpdater, &DataUpdater::requestUpdate);
    timer->start(25); // 每16ms请求一次更新，相当于每秒60帧
    // 使用定时器请求数据更新
    QTimer *timer2 = new QTimer(this);
    connect(timer2, &QTimer::timeout, dataUpdater2, &DataUpdater2::requestUpdate);
    timer2->start(25); // 每16ms请求一次更新，相当于每秒60帧

    dataUpdater->start();
    dataUpdater->resume();//线程开启
    dataUpdater2->start();
    dataUpdater2->resume();//线程开启
}

MainWindow::~MainWindow() {
//    workerThread.wait();
//    workerThread.quit();
    dataUpdater->stop();
    dataUpdater->wait(); // 等待线程完全停止
    dataUpdater2->stop();
    dataUpdater2->wait(); // 等待线程完全停止
    delete ui;
}

void MainWindow::moveWindowToCenter() {
  //屏幕居中
  this->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter,
                                        this->size(),
                                        qApp->desktop()->availableGeometry()));
}

void MainWindow::APP_start()
{
   ui->humanGLWidget->setLeftArm1Angle(0,75,0);
   ui->humanGLWidget->setRightArm1Angle(0,-75,0);
   ui->humanGLWidget->setLeftLegAngle(0,0,0);
   ui->humanGLWidget->setRightLegAngle(0,0,0);
   emit stopped();
}
//搜索串口
void MainWindow::on_btn_search_port_clicked()
{
    ui->comboBox->clear(); // Clear the combo box to avoid duplicate entries

    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) // Read serial port information
    {
        myserial->setPort(info); // Set the serial port info
        if (myserial->open(QIODevice::ReadWrite))
        {
            if (ui->comboBox->findText(myserial->portName()) == -1) // Check if the port is already in the combo box
            {
                ui->comboBox->addItem(myserial->portName()); // Add the serial port name to the combo box
            }
            myserial->close(); // Close the serial port and wait for user to open it manually
        }
    }

}
//打开串口
void MainWindow::on_btn_open_port_clicked()
{
    if (serial_flag)
    {
        ui->comboBox->setDisabled(true); // Disable combo box to prevent port changes
        myserial->setPortName(ui->comboBox->currentText()); // Set serial port name
//        myserial->setBaudRate(921600); // Set custom baud rate(有线)73Hz
        myserial->setBaudRate(115200); // Set custom baud rate(无线)40Hz
        myserial->setDataBits(QSerialPort::Data8); // Set data bits
        myserial->setParity(QSerialPort::NoParity); // Set parity
        myserial->setStopBits(QSerialPort::OneStop); // Set stop bits
        myserial->setFlowControl(QSerialPort::NoFlowControl); // Set flow control

        if (myserial->open(QIODevice::ReadWrite))
        {
            connect(myserial, &QSerialPort::readyRead, this, &MainWindow::AnalyzeData);
            if (!mystarttime.isValid()) // Only set start time if not already set
            {
                mystarttime = QDateTime::currentDateTime(); // Set initial reference time
            }
            qDebug() << "串口打开成功";
        }
        else
        {
            qDebug() << "串口打开失败";
        }
        ui->btn_open_port->setText("关闭串口");
        serial_flag = false; // Set serial flag to false
    }
    else
    {
        ui->comboBox->setEnabled(true); // Enable combo box
        myserial->close(); // Close serial port
        ui->btn_open_port->setText("打开串口"); // Update button text
        serial_flag = true; // Set serial flag to true
    }
}
void MainWindow::AnalyzeData()
{
    static QByteArray buffer;
    buffer.append(myserial->readAll()); // Read all data from the serial port

    while (buffer.size() >= static_cast<int>(sizeof(float) * 100 + 4)) {// Check if we have enough data for one complete frame
        // Check for frame tail
        if (buffer[sizeof(float) * 100] == static_cast<char>(0x00) && buffer[sizeof(float) * 100 + 1] == static_cast<char>(0x00) &&
                    buffer[sizeof(float) * 100 + 2] == static_cast<char>(0x80) && buffer[sizeof(float) * 100 + 3] == static_cast<char>(0x7f)) {

            float *data = reinterpret_cast<float*>(buffer.data());
            // 获取当前时间戳
            QDateTime currentTime = QDateTime::currentDateTime();
            double xValue = mystarttime.msecsTo(currentTime) / 1000.0;
            dataPoint.timestamp = xValue;
            // 提取数据并更新图表
            extractAndUpdateData(data, 99, 1, 2, &dataPoint.angle1_x, &dataPoint.angle1_y, &dataPoint.angle1_z, ui->widget_plot_angle1, ui->line_angle1_x, ui->line_angle1_y, ui->line_angle1_z, angle1_recentData);
            extractAndUpdateData(data, 3, 4, 5, &dataPoint.gyro1_x, &dataPoint.gyro1_y, &dataPoint.gyro1_z, ui->widget_plot_gyro1, ui->line_gyro1_x, ui->line_gyro1_y, ui->line_gyro1_z, gyro1_recentData);
            extractAndUpdateData(data, 6, 7, 8, &dataPoint.acc1_x, &dataPoint.acc1_y, &dataPoint.acc1_z, ui->widget_plot_acc1, ui->line_acc1_x, ui->line_acc1_y, ui->line_acc1_z, acc1_recentData);

            extractAndUpdateData(data, 9, 10, 11, &dataPoint.angle2_x, &dataPoint.angle2_y, &dataPoint.angle2_z, ui->widget_plot_angle2, ui->line_angle2_x, ui->line_angle2_y, ui->line_angle2_z, angle2_recentData);
            extractAndUpdateData(data, 12, 13, 14, &dataPoint.gyro2_x, &dataPoint.gyro2_y, &dataPoint.gyro2_z, ui->widget_plot_gyro2, ui->line_gyro2_x, ui->line_gyro2_y, ui->line_gyro2_z, gyro2_recentData);
            extractAndUpdateData(data, 15, 16, 17, &dataPoint.acc2_x, &dataPoint.acc2_y, &dataPoint.acc2_z, ui->widget_plot_acc2, ui->line_acc2_x, ui->line_acc2_y, ui->line_acc2_z, acc2_recentData);

            extractAndUpdateData(data, 18, 19, 20, &dataPoint.angle3_x, &dataPoint.angle3_y, &dataPoint.angle3_z, ui->widget_plot_angle3, ui->line_angle3_x, ui->line_angle3_y, ui->line_angle3_z, angle3_recentData);
            extractAndUpdateData(data, 21, 22, 23, &dataPoint.gyro3_x, &dataPoint.gyro3_y, &dataPoint.gyro3_z, ui->widget_plot_gyro3, ui->line_gyro3_x, ui->line_gyro3_y, ui->line_gyro3_z, gyro3_recentData);
            extractAndUpdateData(data, 24, 25, 26, &dataPoint.acc3_x, &dataPoint.acc3_y, &dataPoint.acc3_z, ui->widget_plot_acc3, ui->line_acc3_x, ui->line_acc3_y, ui->line_acc3_z, acc3_recentData);

            extractAndUpdateData(data, 27, 28, 29, &dataPoint.angle4_x, &dataPoint.angle4_y, &dataPoint.angle4_z, ui->widget_plot_angle4, ui->line_angle4_x, ui->line_angle4_y, ui->line_angle4_z, angle4_recentData);
            extractAndUpdateData(data, 30, 31, 32, &dataPoint.gyro4_x, &dataPoint.gyro4_y, &dataPoint.gyro4_z, ui->widget_plot_gyro4, ui->line_gyro4_x, ui->line_gyro4_y, ui->line_gyro4_z, gyro4_recentData);
            extractAndUpdateData(data, 33, 34, 35, &dataPoint.acc4_x, &dataPoint.acc4_y, &dataPoint.acc4_z, ui->widget_plot_acc4, ui->line_acc4_x, ui->line_acc4_y, ui->line_acc4_z, acc4_recentData);

            extractAndUpdateData_18channel(data,36, 37, 38, 39, 40, 41,42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53,
                                           &dataPoint.pressure_left_1, &dataPoint.pressure_left_2, &dataPoint.pressure_left_3,
                                           &dataPoint.pressure_left_4, &dataPoint.pressure_left_5, &dataPoint.pressure_left_6,
                                           &dataPoint.pressure_left_7, &dataPoint.pressure_left_8, &dataPoint.pressure_left_9,
                                           &dataPoint.pressure_left_10, &dataPoint.pressure_left_11, &dataPoint.pressure_left_12,
                                           &dataPoint.pressure_left_13, &dataPoint.pressure_left_14, &dataPoint.pressure_left_15,
                                           &dataPoint.pressure_left_16, &dataPoint.pressure_left_17, &dataPoint.pressure_left_18,
                                           ui->pressure_left,pressure_left__recentData);


            extractAndUpdateData(data, 54, 55, 56, &dataPoint.angle5_x, &dataPoint.angle5_y, &dataPoint.angle5_z, ui->widget_plot_angle5, ui->line_angle5_x, ui->line_angle5_y, ui->line_angle5_z, angle5_recentData);
            extractAndUpdateData(data, 57, 58, 59, &dataPoint.gyro5_x, &dataPoint.gyro5_y, &dataPoint.gyro5_z, ui->widget_plot_gyro5, ui->line_gyro5_x, ui->line_gyro5_y, ui->line_gyro5_z, gyro5_recentData);
            extractAndUpdateData(data, 60, 61, 62, &dataPoint.acc5_x, &dataPoint.acc5_y, &dataPoint.acc5_z, ui->widget_plot_acc5, ui->line_acc5_x, ui->line_acc5_y, ui->line_acc5_z, acc5_recentData);

            extractAndUpdateData_18channel(data,63, 64, 65, 66, 67, 68,69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
                                           &dataPoint.pressure_right_1, &dataPoint.pressure_right_2, &dataPoint.pressure_right_3,
                                           &dataPoint.pressure_right_4, &dataPoint.pressure_right_5, &dataPoint.pressure_right_6,
                                           &dataPoint.pressure_right_7, &dataPoint.pressure_right_8, &dataPoint.pressure_right_9,
                                           &dataPoint.pressure_right_10, &dataPoint.pressure_right_11, &dataPoint.pressure_right_12,
                                           &dataPoint.pressure_right_13, &dataPoint.pressure_right_14, &dataPoint.pressure_right_15,
                                           &dataPoint.pressure_right_16, &dataPoint.pressure_right_17, &dataPoint.pressure_right_18,
                                           ui->pressure_right,pressure_right__recentData);


            extractAndUpdateData(data, 81, 82, 83, &dataPoint.angle6_x, &dataPoint.angle6_y, &dataPoint.angle6_z, ui->widget_plot_angle6, ui->line_angle6_x, ui->line_angle6_y, ui->line_angle6_z, angle6_recentData);
            extractAndUpdateData(data, 84, 85, 86, &dataPoint.gyro6_x, &dataPoint.gyro6_y, &dataPoint.gyro6_z, ui->widget_plot_gyro6, ui->line_gyro6_x, ui->line_gyro6_y, ui->line_gyro6_z, gyro6_recentData);
            extractAndUpdateData(data, 87, 88, 89, &dataPoint.acc6_x, &dataPoint.acc6_y, &dataPoint.acc6_z, ui->widget_plot_acc6, ui->line_acc6_x, ui->line_acc6_y, ui->line_acc6_z, acc6_recentData);


            extractAndUpdateData(data, 90, 91, 92, &dataPoint.angleM_x, &dataPoint.angleM_y, &dataPoint.angleM_z, ui->widget_plot_angleM, ui->line_angleM_x, ui->line_angleM_y, ui->line_angleM_z, angleM_recentData);
            extractAndUpdateData(data, 93, 94, 95, &dataPoint.gyroM_x, &dataPoint.gyroM_y, &dataPoint.gyroM_z, ui->widget_plot_gyroM, ui->line_gyroM_x, ui->line_gyroM_y, ui->line_gyroM_z, gyroM_recentData);
            extractAndUpdateData(data, 96, 97, 98, &dataPoint.accM_x, &dataPoint.accM_y, &dataPoint.accM_z, ui->widget_plot_accM, ui->line_accM_x, ui->line_accM_y, ui->line_accM_z, accM_recentData);

            dataPoints.append(dataPoint);

            buffer.remove(0, sizeof(float) * 100 + 4); // Remove processed frame
        } else {
            buffer.remove(0, 1); // Remove the first byte and check again
        }
    }


}
//不画图数据获取
void MainWindow::pressure_UpdateData(float *data, int xIndex, int yIndex, int zIndex,
                                      float *xData, float *yData, float *zData)
{
    float data_x = data[xIndex];
    float data_y = data[yIndex];
    float data_z = data[zIndex];
    // 滤波，去除异常值
    if (data_x > 10000 || data_x < -500) data_x = *xData;
    if (data_y > 10000 || data_y < -500) data_y = *yData;
    if (data_z > 10000 || data_z < -500) data_z = *zData;
//    xLine->setText(QString::number(data_x, 'f', 3));
//    yLine->setText(QString::number(data_y, 'f', 3));
//    zLine->setText(QString::number(data_z, 'f', 3));

    *xData = data_x;
    *yData = data_y;
    *zData = data_z;

}

//IMU波形数据获取
void MainWindow::extractAndUpdateData(float *data, int xIndex, int yIndex, int zIndex,
                                  float *xData, float *yData, float *zData, QCustomPlot *plot,
                                  QLineEdit *xLine, QLineEdit *yLine, QLineEdit *zLine,
                                  std::deque<std::tuple<double, float, float, float>>& recentData)
{
    float data_x = data[xIndex];
    float data_y = data[yIndex];
    float data_z = data[zIndex];

    // 滤波，去除异常值
    if (data_x > 500 || data_x < -500) data_x = *xData;
    if (data_y > 500 || data_y < -500) data_y = *yData;
    if (data_z > 500 || data_z < -500) data_z = *zData;

    xLine->setText(QString::number(data_x, 'f', 3));
    yLine->setText(QString::number(data_y, 'f', 3));
    zLine->setText(QString::number(data_z, 'f', 3));

    QDateTime currentTime = QDateTime::currentDateTime();
    double xValue = mystarttime.msecsTo(currentTime) / 1000.0;

    plot->graph(0)->addData(xValue, data_x);
    plot->graph(1)->addData(xValue, data_y);
    plot->graph(2)->addData(xValue, data_z);

    *xData = data_x;
    *yData = data_y;
    *zData = data_z;

    recentData.emplace_back(xValue, data_x, data_y, data_z);

    // Remove data older than 5 seconds
    while (!recentData.empty() && std::get<0>(recentData.front()) < xValue - 5) {
        recentData.pop_front();
    }

    // Calculate min and max for recent data
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    for (const auto& entry : recentData) {
        minY = std::min(minY, std::min({std::get<1>(entry), std::get<2>(entry), std::get<3>(entry)}));
        maxY = std::max(maxY, std::max({std::get<1>(entry), std::get<2>(entry), std::get<3>(entry)}));
    }

    if (xValue > 5) {
        plot->xAxis->setRange(xValue - 5, xValue);
    } else {
        plot->xAxis->setRange(0, 5);
    }
    //上下边距设置
    if(xIndex==72||xIndex==9||xIndex==18||xIndex==27||xIndex==54||xIndex==81||xIndex==90)plot->yAxis->setRange(minY - 10, maxY + 10);
    if(xIndex==3||xIndex==12||xIndex==21||xIndex==30||xIndex==57||xIndex==84||xIndex==93)plot->yAxis->setRange(minY - 30, maxY + 30);
    if(xIndex==6||xIndex==15||xIndex==24||xIndex==33||xIndex==60||xIndex==87||xIndex==96)plot->yAxis->setRange(minY - 0.5, maxY + 0.5);

    plot->graph(0)->rescaleValueAxis(true);
    plot->graph(1)->rescaleValueAxis(true);
    plot->graph(2)->rescaleValueAxis(true);
}

//18通道足底压力数据获取
void MainWindow::extractAndUpdateData_18channel(float *data, int Index1, int Index2, int Index3,int Index4,int Index5,int Index6,int Index7,int Index8,int Index9,int Index10,int Index11,int Index12,int Index13,int Index14,int Index15,int Index16,int Index17,int Index18,
                                  float *Data1, float *Data2, float *Data3, float *Data4, float *Data5, float *Data6, float *Data7, float *Data8, float *Data9, float *Data10, float *Data11, float *Data12, float *Data13, float *Data14, float *Data15, float *Data16, float *Data17, float *Data18,
                                      QCustomPlot *plot,
                                  std::deque<std::tuple<double, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float>>& recentData)
{
    float data1 = data[Index1];
    float data2 = data[Index2];
    float data3 = data[Index3];
    float data4 = data[Index4];
    float data5 = data[Index5];
    float data6 = data[Index6];
    float data7 = data[Index7];
    float data8 = data[Index8];
    float data9 = data[Index9];
    float data10 = data[Index10];
    float data11 = data[Index11];
    float data12 = data[Index12];
    float data13 = data[Index13];
    float data14 = data[Index14];
    float data15 = data[Index15];
    float data16 = data[Index16];
    float data17 = data[Index17];
    float data18 = data[Index18];

    // 滤波，去除异常值
    if (data1 > 10000 || data1 < -500) data1 = *Data1;
    if (data2 > 10000 || data2 < -500) data2 = *Data2;
    if (data3 > 10000 || data3 < -500) data3 = *Data3;
    if (data4 > 10000 || data4 < -500) data4 = *Data4;
    if (data5 > 10000 || data5 < -500) data5 = *Data5;
    if (data6 > 10000 || data6 < -500) data6 = *Data6;
    if (data7 > 10000 || data7 < -500) data7 = *Data7;
    if (data8 > 10000 || data8 < -500) data8 = *Data8;
    if (data9 > 10000 || data9 < -500) data9 = *Data9;
    if (data10 > 10000 || data10 < -500) data10 = *Data10;
    if (data11 > 10000 || data11 < -500) data11 = *Data11;
    if (data12 > 10000 || data12 < -500) data12 = *Data12;
    if (data13 > 10000 || data13 < -500) data13 = *Data13;
    if (data14 > 10000 || data14 < -500) data14 = *Data14;
    if (data15 > 10000 || data15 < -500) data15 = *Data15;
    if (data16 > 10000 || data16 < -500) data16 = *Data16;
    if (data17 > 10000 || data17 < -500) data17 = *Data17;
    if (data18 > 10000 || data18 < -500) data18 = *Data18;

    QDateTime currentTime = QDateTime::currentDateTime();
    double xValue = mystarttime.msecsTo(currentTime) / 1000.0;

    plot->graph(0)->addData(xValue, data1);
    plot->graph(1)->addData(xValue, data2);
    plot->graph(2)->addData(xValue, data3);
    plot->graph(3)->addData(xValue, data4);
    plot->graph(4)->addData(xValue, data5);
    plot->graph(5)->addData(xValue, data6);
    plot->graph(6)->addData(xValue, data7);
    plot->graph(7)->addData(xValue, data8);
    plot->graph(8)->addData(xValue, data9);
    plot->graph(9)->addData(xValue, data10);
    plot->graph(10)->addData(xValue, data11);
    plot->graph(11)->addData(xValue, data12);
    plot->graph(12)->addData(xValue, data13);
    plot->graph(13)->addData(xValue, data14);
    plot->graph(14)->addData(xValue, data15);
    plot->graph(15)->addData(xValue, data16);
    plot->graph(16)->addData(xValue, data17);
    plot->graph(17)->addData(xValue, data18);

    *Data1 = data1;
    *Data2 = data2;
    *Data3 = data3;
    *Data4 = data4;
    *Data5 = data5;
    *Data6 = data6;
    *Data7 = data7;
    *Data8 = data8;
    *Data9 = data9;
    *Data10 = data10;
    *Data11 = data11;
    *Data12 = data12;
    *Data13 = data13;
    *Data14 = data14;
    *Data15 = data15;
    *Data16 = data16;
    *Data17 = data17;
    *Data18 = data18;

    recentData.emplace_back(xValue, data1, data2, data3, data4, data5, data6, data7, data8, data9, data10, data11, data12, data13, data14, data15, data16, data17, data18);


    // Remove data older than 5 seconds
    while (!recentData.empty() && std::get<0>(recentData.front()) < xValue - 5) {
        recentData.pop_front();
    }

    // Calculate min and max for recent data
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    for (const auto& entry : recentData) {
        minY = std::min({minY, std::get<1>(entry), std::get<2>(entry), std::get<3>(entry), std::get<4>(entry), std::get<5>(entry), std::get<6>(entry), std::get<7>(entry), std::get<8>(entry), std::get<9>(entry), std::get<10>(entry), std::get<11>(entry), std::get<12>(entry), std::get<13>(entry), std::get<14>(entry), std::get<15>(entry), std::get<16>(entry), std::get<17>(entry), std::get<18>(entry)});
        maxY = std::max({maxY, std::get<1>(entry), std::get<2>(entry), std::get<3>(entry), std::get<4>(entry), std::get<5>(entry), std::get<6>(entry), std::get<7>(entry), std::get<8>(entry), std::get<9>(entry), std::get<10>(entry), std::get<11>(entry), std::get<12>(entry), std::get<13>(entry), std::get<14>(entry), std::get<15>(entry), std::get<16>(entry), std::get<17>(entry), std::get<18>(entry)});
    }

    if (xValue > 5) {
        plot->xAxis->setRange(xValue - 5, xValue);
    } else {
        plot->xAxis->setRange(0, 5);
    }
    //上下边距设置
    plot->yAxis->setRange(minY - 10, maxY + 10);

    for (int i = 3; i < 18; ++i) {
        plot->graph(i)->rescaleValueAxis(true);
    }
}

void MainWindow::setupPlot()
{
    setupGraph_none(ui->widget_plot_angle1, 0, 30);
    setupGraph_none(ui->widget_plot_gyro1, -5, 5);
    setupGraph_none(ui->widget_plot_acc1, -1, 1);

    setupGraph_none(ui->widget_plot_angle2, 0, 30);
    setupGraph_none(ui->widget_plot_gyro2, -5, 5);
    setupGraph_none(ui->widget_plot_acc2, -1, 1);

    setupGraph_none(ui->widget_plot_angle3, 0, 30);
    setupGraph_none(ui->widget_plot_gyro3, -5, 5);
    setupGraph_none(ui->widget_plot_acc3, -1, 1);

    setupGraph_none(ui->widget_plot_angle4, 0, 30);
    setupGraph_none(ui->widget_plot_gyro4, -5, 5);
    setupGraph_none(ui->widget_plot_acc4, -1, 1);

    setupGraph_none(ui->widget_plot_angle5, 0, 30);
    setupGraph_none(ui->widget_plot_gyro5, -5, 5);
    setupGraph_none(ui->widget_plot_acc5, -1, 1);

    setupGraph_none(ui->widget_plot_angle6, 0, 30);
    setupGraph_none(ui->widget_plot_gyro6, -5, 5);
    setupGraph_none(ui->widget_plot_acc6, -1, 1);

    setupGraph_none(ui->widget_plot_angleM, 0, 30);
    setupGraph_none(ui->widget_plot_gyroM, -5, 5);
    setupGraph_none(ui->widget_plot_accM, -1, 1);

    setupGraph_none_18channel(ui->pressure_left, -5, 500);
    setupGraph_none_18channel(ui->pressure_right, -5, 500);

}

void MainWindow::updatePlots()
{

    switch (ui->channel_choose->currentIndex()) {
        case 1:
//            dataUpdater->pause();
            ui->widget_plot_angleM->replot();
            ui->widget_plot_gyroM->replot();
            ui->widget_plot_accM->replot();
            break;
        case 2:
//            dataUpdater->pause();
            ui->widget_plot_angle1->replot();
            ui->widget_plot_gyro1->replot();
            ui->widget_plot_acc1->replot();
            break;
        case 3:
//            dataUpdater->pause();
            ui->widget_plot_angle2->replot();
            ui->widget_plot_gyro2->replot();
            ui->widget_plot_acc2->replot();
            break;
        case 4:
//            dataUpdater->pause();
            ui->widget_plot_angle3->replot();
            ui->widget_plot_gyro3->replot();
            ui->widget_plot_acc3->replot();
            break;
        case 5:
//            dataUpdater->pause();
            ui->widget_plot_angle4->replot();
            ui->widget_plot_gyro4->replot();
            ui->widget_plot_acc4->replot();
            break;
        case 6:
//            dataUpdater->pause();
            ui->widget_plot_angle5->replot();
            ui->widget_plot_gyro5->replot();
            ui->widget_plot_acc5->replot();
            break;
        case 7:
//            dataUpdater->pause();//线程关闭
            ui->widget_plot_angle6->replot();
            ui->widget_plot_gyro6->replot();
            ui->widget_plot_acc6->replot();
            break;
        case 8:
//            dataUpdater->resume();//线程开启
            ui->pressure_left->replot();
            ui->pressure_right->replot();
            break;
        case 0:
//            dataUpdater->resume();//足底压力线程开启
            if(numb>7)numb=0;
            switch (numb) {
            case 0:
                ui->widget_plot_angleM->replot();
                ui->widget_plot_gyroM->replot();
                ui->widget_plot_accM->replot();
                break;
            case 1:
                ui->widget_plot_angle1->replot();
                ui->widget_plot_gyro1->replot();
                ui->widget_plot_acc1->replot();
                break;
            case 2:
                ui->widget_plot_angle2->replot();
                ui->widget_plot_gyro2->replot();
                ui->widget_plot_acc2->replot();
                break;
            case 3:
                ui->widget_plot_angle3->replot();
                ui->widget_plot_gyro3->replot();
                ui->widget_plot_acc3->replot();
                break;
            case 4:
                ui->widget_plot_angle4->replot();
                ui->widget_plot_gyro4->replot();
                ui->widget_plot_acc4->replot();
                break;
            case 5:
                ui->widget_plot_angle5->replot();
                ui->widget_plot_gyro5->replot();
                ui->widget_plot_acc5->replot();
                break;
            case 6:
                ui->widget_plot_angle6->replot();
                ui->widget_plot_gyro6->replot();
                ui->widget_plot_acc6->replot();
                break;
            case 7:
                ui->pressure_left->replot();
                ui->pressure_right->replot();
                break;
            default:
                break;
            }
            numb++;
        default:
            break;
    }
}

void MainWindow::updateAngles()
{
    if(numb1>4)numb1=0;
    switch (numb1) {
    case 0:
//        ui->humanGLWidget->setLeftThighAngle(dataPoint.angle1_x, dataPoint.angle1_y, dataPoint.angle1_z);
        ui->humanGLWidget->setLeftThighAngle(dataPoint.angle1_x, 0, 0);
        break;
    case 1:
//        ui->humanGLWidget->setLeftLegAngle(dataPoint.angle2_x, dataPoint.angle2_y, dataPoint.angle2_z);
        ui->humanGLWidget->setLeftLegAngle(dataPoint.angle2_x, 0, 0);
        break;
    case 2:
//        ui->humanGLWidget->setRightThighAngle(dataPoint.angle3_x, dataPoint.angle3_y, dataPoint.angle3_z);
        ui->humanGLWidget->setRightThighAngle(dataPoint.angle3_x, 0, 0);
        break;
    case 3:
//        ui->humanGLWidget->setRightLegAngle(dataPoint.angle4_x, dataPoint.angle4_y, dataPoint.angle4_z);
        ui->humanGLWidget->setRightLegAngle(dataPoint.angle4_x, 0, 0);
        break;
    case 4:
//        ui->humanGLWidget->setWholeAngle(dataPoint.angleM_x, dataPoint.angleM_y, dataPoint.angleM_z);
        ui->humanGLWidget->setWholeAngle(dataPoint.angleM_x, 0, 0);
        break;
    default:
        break;
    }
    numb1++;

}



//带图例初始化
void MainWindow::setupGraph(QCustomPlot *plot, const QString &name1, const QString &name2, const QString &name3, double yMin, double yMax)
{
    plot->addGraph();
    QPen pen;
    pen.setWidth(1);
    pen.setColor(Qt::blue);
    plot->graph(0)->setPen(pen);
    plot->graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20)));
    plot->graph(0)->setName(name1);
    plot->graph(0)->setAntialiasedFill(false);
    plot->graph(0)->setLineStyle((QCPGraph::LineStyle)1);
    plot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone, 5));

    plot->addGraph();
    pen.setColor(Qt::red);
    plot->graph(1)->setPen(pen);
    plot->graph(1)->setBrush(QBrush(QColor(0, 0, 255, 20)));
    plot->graph(1)->setName(name2);
    plot->graph(1)->setAntialiasedFill(false);
    plot->graph(1)->setLineStyle((QCPGraph::LineStyle)1);
    plot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone, 5));

    plot->addGraph();
    pen.setColor(Qt::green);
    plot->graph(2)->setPen(pen);
    plot->graph(2)->setBrush(QBrush(QColor(0, 0, 255, 20)));
    plot->graph(2)->setName(name3);
    plot->graph(2)->setAntialiasedFill(false);
    plot->graph(2)->setLineStyle((QCPGraph::LineStyle)1);
    plot->graph(2)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone, 5));

    plot->xAxis->setLabelColor(QColor(20, 20, 20));
    plot->xAxis->setRange(0, 5);
    plot->yAxis->setLabelColor(QColor(20, 20, 20));
    plot->yAxis->setRange(yMin, yMax);

    plot->axisRect()->setupFullAxesBox(true);
    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes);
    plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignLeft);
    plot->legend->setVisible(true);
}

//无图例初始化
void MainWindow::setupGraph_none(QCustomPlot *plot, double yMin, double yMax)
{
    plot->addGraph();
    QPen pen;
    pen.setWidth(1);
    pen.setColor(Qt::blue);
    plot->graph(0)->setPen(pen);
    plot->graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20)));
//    plot->graph(0)->setName(name1);
    plot->graph(0)->setAntialiasedFill(false);
    plot->graph(0)->setLineStyle((QCPGraph::LineStyle)1);
    plot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone, 5));

    plot->addGraph();
    pen.setColor(Qt::red);
    plot->graph(1)->setPen(pen);
    plot->graph(1)->setBrush(QBrush(QColor(0, 0, 255, 20)));
//    plot->graph(1)->setName(name2);
    plot->graph(1)->setAntialiasedFill(false);
    plot->graph(1)->setLineStyle((QCPGraph::LineStyle)1);
    plot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone, 5));

    plot->addGraph();
    pen.setColor(Qt::green);
    plot->graph(2)->setPen(pen);
    plot->graph(2)->setBrush(QBrush(QColor(0, 0, 255, 20)));
//    plot->graph(2)->setName(name3);
    plot->graph(2)->setAntialiasedFill(false);
    plot->graph(2)->setLineStyle((QCPGraph::LineStyle)1);
    plot->graph(2)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone, 5));

    plot->xAxis->setLabelColor(QColor(20, 20, 20));
    plot->xAxis->setRange(0, 5);
    plot->yAxis->setLabelColor(QColor(20, 20, 20));
    plot->yAxis->setRange(yMin, yMax);

    plot->axisRect()->setupFullAxesBox(true);
    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes);
    plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignLeft);
//    plot->legend->setVisible(true);
}
//18通道初始化
void MainWindow::setupGraph_none_18channel(QCustomPlot *plot, double yMin, double yMax)
{
    QPen pen;
    QVector<QColor> colors = {Qt::blue, Qt::red, Qt::green, Qt::cyan, Qt::magenta, Qt::yellow, Qt::gray, Qt::darkRed, Qt::darkGreen, Qt::darkBlue, Qt::darkCyan, Qt::darkMagenta, Qt::darkYellow, Qt::darkGray, Qt::black, Qt::darkRed, Qt::darkGreen, Qt::darkBlue};

    for (int i = 0; i < 18; ++i) {
        plot->addGraph();
        pen.setWidth(1);
        pen.setColor(colors[i % colors.size()]);
        plot->graph(i)->setPen(pen);
        plot->graph(i)->setBrush(QBrush(QColor(0, 0, 255, 20)));
        plot->graph(i)->setAntialiasedFill(false);
        plot->graph(i)->setLineStyle((QCPGraph::LineStyle)1);
        plot->graph(i)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone, 5));
    }

    plot->xAxis->setLabelColor(QColor(20, 20, 20));
    plot->xAxis->setRange(0, 5);
    plot->yAxis->setLabelColor(QColor(20, 20, 20));
    plot->yAxis->setRange(yMin, yMax);

    plot->axisRect()->setupFullAxesBox(true);
    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes);
    plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignLeft);
//    plot->legend->setVisible(true);
}

void MainWindow::on_quit_clicked()
{
    QCoreApplication::quit();
}
//导出excel
void MainWindow::on_btn_export_data_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Data"), "", tr("Excel Files (*.xlsx)"));
    if (fileName.isEmpty())
        return;

    QXlsx::Document xlsx;
    xlsx.write("A1", "Timestamp");
    xlsx.write("B1", "angle1_x");
    xlsx.write("C1", "angle1_y");
    xlsx.write("D1", "angle1_z");
    xlsx.write("E1", "gyro1_x");
    xlsx.write("F1", "gyro1_y");
    xlsx.write("G1", "gyro1_z");
    xlsx.write("H1", "acc1_x");
    xlsx.write("I1", "acc1_y");
    xlsx.write("J1", "acc1_z");

    xlsx.write("K1", "angle2_x");
    xlsx.write("L1", "angle2_y");
    xlsx.write("M1", "angle2_z");
    xlsx.write("N1", "gyro2_x");
    xlsx.write("O1", "gyro2_y");
    xlsx.write("P1", "gyro2_z");
    xlsx.write("Q1", "acc2_x");
    xlsx.write("R1", "acc2_y");
    xlsx.write("S1", "acc2_z");

    xlsx.write("T1", "angle3_x");
    xlsx.write("U1", "angle3_y");
    xlsx.write("V1", "angle3_z");
    xlsx.write("W1", "gyro3_x");
    xlsx.write("X1", "gyro3_y");
    xlsx.write("Y1", "gyro3_z");
    xlsx.write("Z1", "acc3_x");
    xlsx.write("AA1", "acc3_y");
    xlsx.write("AB1", "acc3_z");

    xlsx.write("AC1", "angle4_x");
    xlsx.write("AD1", "angle4_y");
    xlsx.write("AE1", "angle4_z");
    xlsx.write("AF1", "gyro4_x");
    xlsx.write("AG1", "gyro4_y");
    xlsx.write("AH1", "gyro4_z");
    xlsx.write("AI1", "acc4_x");
    xlsx.write("AJ1", "acc4_y");
    xlsx.write("AK1", "acc4_z");

    xlsx.write("AL1", "angle5_x");
    xlsx.write("AM1", "angle5_y");
    xlsx.write("AN1", "angle5_z");
    xlsx.write("AO1", "gyro5_x");
    xlsx.write("AP1", "gyro5_y");
    xlsx.write("AQ1", "gyro5_z");
    xlsx.write("AR1", "acc5_x");
    xlsx.write("AS1", "acc5_y");
    xlsx.write("AT1", "acc5_z");

    xlsx.write("AU1", "angle6_x");
    xlsx.write("AV1", "angle6_y");
    xlsx.write("AW1", "angle6_z");
    xlsx.write("AX1", "gyro6_x");
    xlsx.write("AY1", "gyro6_y");
    xlsx.write("AZ1", "gyro6_z");
    xlsx.write("BA1", "acc6_x");
    xlsx.write("BB1", "acc6_y");
    xlsx.write("BC1", "acc6_z");

    xlsx.write("BD1", "angleM_x");
    xlsx.write("BE1", "angleM_y");
    xlsx.write("BF1", "angleM_z");
    xlsx.write("BG1", "gyroM_x");
    xlsx.write("BH1", "gyroM_y");
    xlsx.write("BI1", "gyroM_z");
    xlsx.write("BJ1", "accM_x");
    xlsx.write("BK1", "accM_y");
    xlsx.write("BL1", "accM_z");

    xlsx.write("BM1", "press_L1");
    xlsx.write("BN1", "press_L2");
    xlsx.write("BO1", "press_L3");
    xlsx.write("BP1", "press_L4");
    xlsx.write("BQ1", "press_L5");
    xlsx.write("BR1", "press_L6");
    xlsx.write("BS1", "press_L7");
    xlsx.write("BT1", "press_L8");
    xlsx.write("BU1", "press_L9");
    xlsx.write("BV1", "press_L10");
    xlsx.write("BW1", "press_L11");
    xlsx.write("BX1", "press_L12");
    xlsx.write("BY1", "press_L13");
    xlsx.write("BZ1", "press_L14");
    xlsx.write("CA1", "press_L15");
    xlsx.write("CB1", "press_L16");
    xlsx.write("CC1", "press_L17");
    xlsx.write("CD1", "press_L18");

    xlsx.write("CE1", "press_R1");
    xlsx.write("CF1", "press_R2");
    xlsx.write("CG1", "press_R3");
    xlsx.write("CH1", "press_R4");
    xlsx.write("CI1", "press_R5");
    xlsx.write("CJ1", "press_R6");
    xlsx.write("CK1", "press_R7");
    xlsx.write("CL1", "press_R8");
    xlsx.write("CM1", "press_R9");
    xlsx.write("CN1", "press_R10");
    xlsx.write("CO1", "press_R11");
    xlsx.write("CP1", "press_R12");
    xlsx.write("CQ1", "press_R13");
    xlsx.write("CR1", "press_R14");
    xlsx.write("CS1", "press_R15");
    xlsx.write("CT1", "press_R16");
    xlsx.write("CU1", "press_R17");
    xlsx.write("CV1", "press_R18");
    for (int i = 0; i < dataPoints.size(); ++i) {
        const DataPoint &data = dataPoints[i];
        xlsx.write(i + 2, 1, data.timestamp);
        xlsx.write(i + 2, 2, data.angle1_x);
        xlsx.write(i + 2, 3, data.angle1_y);
        xlsx.write(i + 2, 4, data.angle1_z);
        xlsx.write(i + 2, 5, data.gyro1_x);
        xlsx.write(i + 2, 6, data.gyro1_y);
        xlsx.write(i + 2, 7, data.gyro1_z);
        xlsx.write(i + 2, 8, data.acc1_x);
        xlsx.write(i + 2, 9, data.acc1_y);
        xlsx.write(i + 2, 10, data.acc1_z);

        xlsx.write(i + 2, 11, data.angle2_x);
        xlsx.write(i + 2, 12, data.angle2_y);
        xlsx.write(i + 2, 13, data.angle2_z);
        xlsx.write(i + 2, 14, data.gyro2_x);
        xlsx.write(i + 2, 15, data.gyro2_y);
        xlsx.write(i + 2, 16, data.gyro2_z);
        xlsx.write(i + 2, 17, data.acc2_x);
        xlsx.write(i + 2, 18, data.acc2_y);
        xlsx.write(i + 2, 19, data.acc2_z);

        xlsx.write(i + 2, 20, data.angle3_x);
        xlsx.write(i + 2, 21, data.angle3_y);
        xlsx.write(i + 2, 22, data.angle3_z);
        xlsx.write(i + 2, 23, data.gyro3_x);
        xlsx.write(i + 2, 24, data.gyro3_y);
        xlsx.write(i + 2, 25, data.gyro3_z);
        xlsx.write(i + 2, 26, data.acc3_x);
        xlsx.write(i + 2, 27, data.acc3_y);
        xlsx.write(i + 2, 28, data.acc3_z);

        xlsx.write(i + 2, 29, data.angle4_x);
        xlsx.write(i + 2, 30, data.angle4_y);
        xlsx.write(i + 2, 31, data.angle4_z);
        xlsx.write(i + 2, 32, data.gyro4_x);
        xlsx.write(i + 2, 33, data.gyro4_y);
        xlsx.write(i + 2, 34, data.gyro4_z);
        xlsx.write(i + 2, 35, data.acc4_x);
        xlsx.write(i + 2, 36, data.acc4_y);
        xlsx.write(i + 2, 37, data.acc4_z);

        xlsx.write(i + 2, 38, data.angle5_x);
        xlsx.write(i + 2, 39, data.angle5_y);
        xlsx.write(i + 2, 40, data.angle5_z);
        xlsx.write(i + 2, 41, data.gyro5_x);
        xlsx.write(i + 2, 42, data.gyro5_y);
        xlsx.write(i + 2, 43, data.gyro5_z);
        xlsx.write(i + 2, 44, data.acc5_x);
        xlsx.write(i + 2, 45, data.acc5_y);
        xlsx.write(i + 2, 46, data.acc5_z);

        xlsx.write(i + 2, 47, data.angle6_x);
        xlsx.write(i + 2, 48, data.angle6_y);
        xlsx.write(i + 2, 49, data.angle6_z);
        xlsx.write(i + 2, 50, data.gyro6_x);
        xlsx.write(i + 2, 51, data.gyro6_y);
        xlsx.write(i + 2, 52, data.gyro6_z);
        xlsx.write(i + 2, 53, data.acc6_x);
        xlsx.write(i + 2, 54, data.acc6_y);
        xlsx.write(i + 2, 55, data.acc6_z);

        xlsx.write(i + 2, 56, data.angleM_x);
        xlsx.write(i + 2, 57, data.angleM_y);
        xlsx.write(i + 2, 58, data.angleM_z);
        xlsx.write(i + 2, 59, data.gyroM_x);
        xlsx.write(i + 2, 60, data.gyroM_y);
        xlsx.write(i + 2, 61, data.gyroM_z);
        xlsx.write(i + 2, 62, data.accM_x);
        xlsx.write(i + 2, 63, data.accM_y);
        xlsx.write(i + 2, 64, data.accM_z);

        xlsx.write(i + 2, 65, data.pressure_left_1);
        xlsx.write(i + 2, 66, data.pressure_left_2);
        xlsx.write(i + 2, 67, data.pressure_left_3);
        xlsx.write(i + 2, 68, data.pressure_left_4);
        xlsx.write(i + 2, 69, data.pressure_left_5);
        xlsx.write(i + 2, 70, data.pressure_left_6);
        xlsx.write(i + 2, 71, data.pressure_left_7);
        xlsx.write(i + 2, 72, data.pressure_left_8);
        xlsx.write(i + 2, 73, data.pressure_left_9);
        xlsx.write(i + 2, 74, data.pressure_left_10);
        xlsx.write(i + 2, 75, data.pressure_left_11);
        xlsx.write(i + 2, 76, data.pressure_left_12);
        xlsx.write(i + 2, 77, data.pressure_left_13);
        xlsx.write(i + 2, 78, data.pressure_left_14);
        xlsx.write(i + 2, 79, data.pressure_left_15);
        xlsx.write(i + 2, 80, data.pressure_left_16);
        xlsx.write(i + 2, 81, data.pressure_left_17);
        xlsx.write(i + 2, 82, data.pressure_left_18);

        xlsx.write(i + 2, 83, data.pressure_right_1);
        xlsx.write(i + 2, 84, data.pressure_right_2);
        xlsx.write(i + 2, 85, data.pressure_right_3);
        xlsx.write(i + 2, 86, data.pressure_right_4);
        xlsx.write(i + 2, 87, data.pressure_right_5);
        xlsx.write(i + 2, 88, data.pressure_right_6);
        xlsx.write(i + 2, 89, data.pressure_right_7);
        xlsx.write(i + 2, 90, data.pressure_right_8);
        xlsx.write(i + 2, 91, data.pressure_right_9);
        xlsx.write(i + 2, 92, data.pressure_right_10);
        xlsx.write(i + 2, 93, data.pressure_right_11);
        xlsx.write(i + 2, 94, data.pressure_right_12);
        xlsx.write(i + 2, 95, data.pressure_right_13);
        xlsx.write(i + 2, 96, data.pressure_right_14);
        xlsx.write(i + 2, 97, data.pressure_right_15);
        xlsx.write(i + 2, 98, data.pressure_right_16);
        xlsx.write(i + 2, 99, data.pressure_right_17);
        xlsx.write(i + 2, 100, data.pressure_right_18);
    }

    if (xlsx.saveAs(fileName)) {
        QMessageBox::information(this, tr("Success"), tr("Data exported successfully."));
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to export data."));
    }
}

QPolygonF MainWindow::readFootprintFromExcel(const QString &fileName)
{
    QPolygonF polygon;
    QXlsx::Document xlsx(fileName);

    int row = 2; // 从第二行开始读取，假设第一行是标题
    while (true) {
        QXlsx::Cell *cellX = xlsx.cellAt(row, 1);
        QXlsx::Cell *cellY = xlsx.cellAt(row, 2);

        if (!cellX || !cellY) {
            break;
        }

        bool xOk, yOk;
        double x = cellX->value().toDouble(&xOk);
        double y = cellY->value().toDouble(&yOk);

        if (xOk && yOk)polygon << QPointF(x, y);
        row++;
    }
    return polygon;
}

void MainWindow::drawHeatmap_init()
{
    QCustomPlot *customPlot = ui->customPlot;
    colorMap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);
    colorMap->data()->setSize(nx, ny);
    colorMap->data()->setRange(QCPRange(minX, maxX), QCPRange(minY, maxY));
    colorMap->setGradient(QCPColorGradient::gpJet);
    colorMap->setDataRange(QCPRange(-60, 60));
    customPlot->rescaleAxes();
    customPlot->xAxis->setVisible(false);
    customPlot->yAxis->setVisible(false);

//    footprint = readFootprintFromExcel("H:\\program\\QT\\IMU_3D_2\\鞋垫.xlsx");
    QString footprintPath = QCoreApplication::applicationDirPath() + "/鞋垫.xlsx";
    footprint = readFootprintFromExcel(footprintPath);
        for (int x = 0; x < nx; ++x) {
            for (int y = 0; y < ny; ++y) {
                QPointF point(minX + x * (maxX - minX) / nx, minY + y * (maxY - minY) / ny);
                if (footprint.containsPoint(point, Qt::OddEvenFill)) {
                    colorMap->data()->setCell(x, y, -60); // 轮廓内设置为深蓝色
                } else {
                    colorMap->data()->setAlpha(x, y, 0); // 轮廓外设置为透明
                }
            }
        }

        for (int i = 0; i < footprint.size(); ++i) {
            QCPItemLine *line = new QCPItemLine(customPlot);
            QPen pen(Qt::black);
            pen.setWidth(5);
            line->setPen(pen);
            line->setAntialiased(true);
            line->start->setCoords(footprint[i].x()+0.3, footprint[i].y());
            line->end->setCoords(footprint[(i + 1) % footprint.size()].x()+0.3, footprint[(i + 1) % footprint.size()].y());
        }

        customPlot->replot();
}

void MainWindow::drawHeatmap2_init()
{
    QCustomPlot *customPlot2 = ui->customPlot_2;
    colorMap2 = new QCPColorMap(customPlot2->xAxis, customPlot2->yAxis);
    colorMap2->data()->setSize(nx, ny);
    colorMap2->data()->setRange(QCPRange(minX, maxX), QCPRange(minY, maxY));
    colorMap2->setGradient(QCPColorGradient::gpJet);
    colorMap2->setDataRange(QCPRange(-60, 60));
    customPlot2->rescaleAxes();
    customPlot2->xAxis->setVisible(false);
    customPlot2->yAxis->setVisible(false);
    // 计算翻转中心
//    footprint2 = readFootprintFromExcel("H:\\program\\QT\\IMU_3D_2\\左鞋垫.xlsx");
    QString footprintPath = QCoreApplication::applicationDirPath() + "/左鞋垫.xlsx";
    footprint2 = readFootprintFromExcel(footprintPath);
        for (int x = 0; x < nx; ++x) {
            for (int y = 0; y < ny; ++y) {
//                QPointF point(2 * centerX - (minX + x * (maxX - minX) / nx), minY + y * (maxY - minY) / ny);
                QPointF point(minX + x * (maxX - minX) / nx+2.5, minY + y * (maxY - minY) / ny);

                if (footprint2.containsPoint(point, Qt::OddEvenFill)) {
                    colorMap2->data()->setCell(x, y, -60); // 轮廓内设置为深蓝色
                } else {
                    colorMap2->data()->setAlpha(x, y, 0); // 轮廓外设置为透明
                }
            }
        }

        for (int i = 0; i < footprint2.size(); ++i) {
            QCPItemLine *line = new QCPItemLine(customPlot2);
            QPen pen(Qt::black);
            pen.setWidth(5);
            line->setPen(pen);
            line->setAntialiased(true);
            line->start->setCoords(footprint2[i].x()-2.3, footprint2[i].y());
            line->end->setCoords(footprint2[(i + 1) % footprint2.size()].x()-2.3, footprint2[(i + 1) % footprint2.size()].y());
        }
        customPlot2->replot();
}
