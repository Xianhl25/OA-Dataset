#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "sensorloader.h"

#include <QMainWindow>
#include <QThread>

#include <QtSerialPort/QSerialPort>//串口
#include <QtSerialPort/QSerialPortInfo>//串口
#include <QDebug>//用于在控制台输出调试信息
#include <QTime>//定时器
#include <QPainter>//坐标系绘图

#include <QComboBox>
#include <QStackedWidget>
#include <QPushButton>

#include <QList>
#include <QFileDialog>
#include <QMessageBox>
#include "xlsxdocument.h" // Include QXlsx library header

#include "qcustomplot.h"
#include <deque>
#include <tuple>
#include "dataupdater.h"
#include "dataupdater2.h"
#include "globals.h" // 全局变量

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow {
  Q_OBJECT
  QThread workerThread;
public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();
  void moveWindowToCenter();

public slots:


signals:
    void stopped();
    void open(const QString &path);

private slots:
    void APP_start();

    void on_btn_search_port_clicked();

    void on_btn_open_port_clicked();

    void AnalyzeData();
    void pressure_UpdateData(float *data, int xIndex, int yIndex, int zIndex,
                                          float *xData, float *yData, float *zData);

    void extractAndUpdateData(float *data, int xIndex, int yIndex, int zIndex,
                                           float *xData, float *yData, float *zData, QCustomPlot *plot,
                                           QLineEdit *xLine, QLineEdit *yLine, QLineEdit *zLine,
                                           std::deque<std::tuple<double, float, float, float>>& recentData);
    void extractAndUpdateData_18channel(float *data, int Inde1, int Inde2, int Inde3,int Index4,int Index5,int Index6,int Index7,int Index8,int Index9,int Index10,int Index11,int Index12,int Index13,int Index14,int Index15,int Index16,int Index17,int Index18,
                                      float *Data1, float *Data2, float *Data3, float *Data4, float *Data5, float *Data6, float *Data7, float *Data8, float *Data9, float *Data10, float *Data11, float *Data12, float *Data13, float *Data14, float *Data15, float *Data16, float *Data17, float *Data18,
                                          QCustomPlot *plot,
                                      std::deque<std::tuple<double, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float>>& recentData);

    void setupPlot();

    void updatePlots();
    void updateAngles();
    void setupGraph(QCustomPlot *plot, const QString &name1, const QString &name2, const QString &name3, double yMin, double yMax);
    void setupGraph_none(QCustomPlot *plot, double yMin, double yMax);
    void setupGraph_none_18channel(QCustomPlot *plot, double yMin, double yMax);
    void on_quit_clicked();
    void on_btn_export_data_clicked();
    void drawHeatmap_init();
    void drawHeatmap2_init();

private:
  Ui::MainWindow *ui;
  QSerialPort *myserial;//声明串口类，myserial是QSerialPort的实例
    bool serial_flag,start_flag;//定义两个标志位
    QByteArray alldata;//接收串口数据
    //绘图函数
    QDateTime mycurrenttime;//系统当前时间
    QDateTime mystarttime;//系统开始时间
    QStackedWidget *stackedWidget;
    QComboBox *channel_choose;
    //云图
    DataUpdater *dataUpdater;
    QCPColorMap *colorMap;  // 添加 colorMap 声明
    QPolygonF footprint;   // 添加 footprint 声明
    
    DataUpdater2 *dataUpdater2;
    QCPColorMap *colorMap2;  // 添加 colorMap 声明
    QPolygonF footprint2;   // 添加 footprint 声明
    // 其他私有成员
    QList<DataPoint> dataPoints;
//    DataPoint dataPoint;
    //缓存区
    std::deque<std::tuple<double, float, float, float>> angle1_recentData;
    std::deque<std::tuple<double, float, float, float>> gyro1_recentData;
    std::deque<std::tuple<double, float, float, float>> acc1_recentData;

    std::deque<std::tuple<double, float, float, float>> angle2_recentData;
    std::deque<std::tuple<double, float, float, float>> gyro2_recentData;
    std::deque<std::tuple<double, float, float, float>> acc2_recentData;

    std::deque<std::tuple<double, float, float, float>> angle3_recentData;
    std::deque<std::tuple<double, float, float, float>> gyro3_recentData;
    std::deque<std::tuple<double, float, float, float>> acc3_recentData;

    std::deque<std::tuple<double, float, float, float>> angle4_recentData;
    std::deque<std::tuple<double, float, float, float>> gyro4_recentData;
    std::deque<std::tuple<double, float, float, float>> acc4_recentData;

    std::deque<std::tuple<double, float, float, float>> angle5_recentData;
    std::deque<std::tuple<double, float, float, float>> gyro5_recentData;
    std::deque<std::tuple<double, float, float, float>> acc5_recentData;

    std::deque<std::tuple<double, float, float, float>> angle6_recentData;
    std::deque<std::tuple<double, float, float, float>> gyro6_recentData;
    std::deque<std::tuple<double, float, float, float>> acc6_recentData;

    std::deque<std::tuple<double, float, float, float>> angleM_recentData;
    std::deque<std::tuple<double, float, float, float>> gyroM_recentData;
    std::deque<std::tuple<double, float, float, float>> accM_recentData;

    std::deque<std::tuple<double, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float>> pressure_left__recentData;
    std::deque<std::tuple<double, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float>> pressure_right__recentData;

    QPolygonF readFootprintFromExcel(const QString &fileName);

};

#endif // MAINWINDOW_H
