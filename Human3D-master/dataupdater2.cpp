#include "dataupdater2.h"
#include <cmath>
#include <QMutexLocker>
#include "mainwindow.h"
#include "globals.h"//变量

int clusterCount2 = 18;//压力触点数量
float clusterRadius2 = 2;//压力触点半径
float distance2[18];//右鞋垫点距
//右足底压力传感位置
float Points2[18][2];

//脚掌边缘
float minX2 = 10;
float maxX2 = 28;
float minY2 = 7;
float maxY2 = 47;
//分辨率
int nx2 = 30, ny2 = 60; // 分辨率与初始化一致
float centerX = (minX + maxX) / 2.0;
// DataUpdater2 构造函数
DataUpdater2::DataUpdater2(QCPColorMap *colorMap2, const QVector<QPointF> &footprint2, float Points2[18][2],
                           int nx2, int ny2, float minX2, float maxX2, float minY2, float maxY2, QObject *parent)
    : QThread(parent), // 调用基类 QThread 的构造函数
      colorMap2(colorMap2), // 初始化 QCPColorMap 对象
      footprint2(footprint2), // 初始化脚掌边缘的多边形
      points2(Points2), // 初始化足底压力传感位置的点阵
      nx2(nx2), ny2(ny2), // 初始化分辨率
      minX2(minX2), maxX2(maxX2), // 初始化 X 轴范围
      minY2(minY2), maxY2(maxY2), // 初始化 Y 轴范围
      running2(true), // 初始化 running2 标志为 true
      paused2(false), // 初始化 paused2 标志为 false
      updateRequested2(false) // 初始化 updateRequested2 标志为 false
{
    // 初始化 Points2 的镜像坐标
    for (int i = 0; i < 18; ++i) {
        Points2[i][0] = centerX - (Points[i][0] - centerX)+1.3;
//        Points2[i][0] = Points[i][0];
        Points2[i][1] = Points[i][1];
    }

}

void DataUpdater2::run()
{
    while (running2)
    {
        QMutexLocker locker(&mutex2);
        if (!updateRequested2 || paused2)
        {
            condition2.wait(&mutex2);
        }
        if (!running2)
        {
            break;
        }
        if (!paused2)
        {
            updateRequested2 = false;
            locker.unlock();

            updateHeatmap();
            emit dataUpdated();
        }
    }
}

void DataUpdater2::requestUpdate()
{
    QMutexLocker locker(&mutex2);
    updateRequested2 = true;
    condition2.wakeOne();
}

void DataUpdater2::stop()
{
    QMutexLocker locker(&mutex2);
    running2 = false;
    condition2.wakeOne();
}

void DataUpdater2::pause()//暂停线程
{
    QMutexLocker locker(&mutex2);
    paused2 = true;
}

void DataUpdater2::resume()//恢复线程
{
    QMutexLocker locker(&mutex2);
    paused2 = false;
    condition2.wakeOne();
}

float mapPressureToRange2(float pressure, float old_min, float old_max, float new_min, float new_max) {
    return new_min + ((pressure - old_min) * (new_max - new_min)) / (old_max - old_min);
}

void DataUpdater2::updateHeatmap()
{
    for (int x = 0; x < nx2; ++x) {
        for (int y = 0; y < ny2; ++y) {
            QPointF point((minX2 + x * (maxX2 - minX2)/ nx2)+2.1, minY2 + y * (maxY2 - minY2) / ny2); // 调整比例以适应实际尺寸
            if (footprint2.containsPoint(point, Qt::OddEvenFill)) {
                for (int i = 0; i < clusterCount2; ++i) {
                    distance2[i] = std::hypot(point.x() - Points2[i][0], point.y() - Points2[i][1]);
                    if (distance2[0] <= clusterRadius2) {
                        colorMap2->data()->setCell(x, y, mapPressureToRange2(dataPoint.pressure_left_1, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance2[1] <= clusterRadius2) {
                        colorMap2->data()->setCell(x, y, mapPressureToRange2(dataPoint.pressure_left_2, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance2[2] <= clusterRadius2) {
                        colorMap2->data()->setCell(x, y, mapPressureToRange2(dataPoint.pressure_left_3, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance2[3] <= clusterRadius2) {
                        colorMap2->data()->setCell(x, y, mapPressureToRange2(dataPoint.pressure_left_4, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance2[4] <= clusterRadius2) {
                        colorMap2->data()->setCell(x, y, mapPressureToRange2(dataPoint.pressure_left_5, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance2[5] <= clusterRadius2) {
                        colorMap2->data()->setCell(x, y, mapPressureToRange2(dataPoint.pressure_left_6, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance2[6] <= clusterRadius2) {
                        colorMap2->data()->setCell(x, y, mapPressureToRange2(dataPoint.pressure_left_7, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance2[7] <= clusterRadius2) {
                        colorMap2->data()->setCell(x, y, mapPressureToRange2(dataPoint.pressure_left_8, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance2[8] <= clusterRadius2) {
                        colorMap2->data()->setCell(x, y, mapPressureToRange2(dataPoint.pressure_left_9, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance2[9] <= clusterRadius2) {
                        colorMap2->data()->setCell(x, y, mapPressureToRange2(dataPoint.pressure_left_10, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance2[10] <= clusterRadius2) {
                        colorMap2->data()->setCell(x, y, mapPressureToRange2(dataPoint.pressure_left_11, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance2[11] <= clusterRadius2) {
                        colorMap2->data()->setCell(x, y, mapPressureToRange2(dataPoint.pressure_left_12, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance2[12] <= clusterRadius2) {
                        colorMap2->data()->setCell(x, y, mapPressureToRange2(dataPoint.pressure_left_13, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance2[13] <= clusterRadius2) {
                        colorMap2->data()->setCell(x, y, mapPressureToRange2(dataPoint.pressure_left_14, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance2[14] <= clusterRadius2) {
                        colorMap2->data()->setCell(x, y, mapPressureToRange2(dataPoint.pressure_left_15, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance2[15] <= clusterRadius2) {
                        colorMap2->data()->setCell(x, y, mapPressureToRange2(dataPoint.pressure_left_16, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance2[16] <= clusterRadius2) {
                        colorMap2->data()->setCell(x, y, mapPressureToRange2(dataPoint.pressure_left_17, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance2[17] <= clusterRadius2) {
                        colorMap2->data()->setCell(x, y, mapPressureToRange2(dataPoint.pressure_left_18, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                }
            }

        }
    }
}
