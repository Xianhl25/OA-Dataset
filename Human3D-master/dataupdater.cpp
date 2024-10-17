//压力鞋垫线程
#include "dataupdater.h"
#include <cmath>
#include <QMutexLocker>
#include "mainwindow.h"
#include "globals.h"//变量

int clusterCount = 18;//压力触点数量
float clusterRadius = 2;//压力触点半径
float distance[18];//左鞋垫点距
//足底压力传感位置
float Points[18][2] = {
    {14.3, 39.5}, {16.5, 39.8}, {18.7, 39.8}, {21.3, 38.5},
    {14.8, 33.7}, {17.5, 33.7}, {20.2, 33.7}, {22.9, 33.7},
    {16.8, 28.2}, {19.9, 28.2}, {23, 28.2}, {17.8, 22.7},
    {20.1, 22.7}, {22.6, 22.7}, {18.7, 17.3}, {22, 17.3},
    {18.7, 12.3}, {21.9, 12.3}
};

//脚掌边缘
float minX = 10;
float maxX = 28;
float minY = 7;
float maxY = 47;
//分辨率
int nx = 30, ny = 60; // 分辨率与初始化一致
// DataUpdater 构造函数
DataUpdater::DataUpdater(QCPColorMap *colorMap, const QVector<QPointF> &footprint, float Points[18][2],
                         int nx, int ny, float minX, float maxX, float minY, float maxY, QObject *parent)
    : QThread(parent), // 调用基类 QThread 的构造函数
      colorMap(colorMap), // 初始化 QCPColorMap 对象
      footprint(footprint), // 初始化脚掌边缘的多边形
      points(Points), // 初始化足底压力传感位置的点阵
      nx(nx), ny(ny), // 初始化分辨率
      minX(minX), maxX(maxX), // 初始化 X 轴范围
      minY(minY), maxY(maxY), // 初始化 Y 轴范围
      running(true), // 初始化 running 标志为 true
      updateRequested(false) // 初始化 updateRequested 标志为 false
{
    // 构造函数体，这里没有其他的初始化操作
}

void DataUpdater::run()
{
    while (running)
    {
        QMutexLocker locker(&mutex);
        if (!updateRequested || paused)
        {
            condition.wait(&mutex);
        }
        if (!running)
        {
            break;
        }
        if (!paused)
        {
            updateRequested = false;
            locker.unlock();

            updateHeatmap();
            emit dataUpdated();
        }
    }
}

void DataUpdater::requestUpdate()
{
    QMutexLocker locker(&mutex);
    updateRequested = true;
    condition.wakeOne();
}

void DataUpdater::stop()
{
    QMutexLocker locker(&mutex);
    running = false;
    condition.wakeOne();
}

void DataUpdater::pause()//暂停线程
{
    QMutexLocker locker(&mutex);
    paused = true;
}

void DataUpdater::resume()//恢复线程
{
    QMutexLocker locker(&mutex);
    paused = false;
    condition.wakeOne();
}

float mapPressureToRange(float pressure, float old_min, float old_max, float new_min, float new_max) {
    return new_min + ((pressure - old_min) * (new_max - new_min)) / (old_max - old_min);
}

void DataUpdater::updateHeatmap()
{
    for (int x = 0; x < nx; ++x) {
        for (int y = 0; y < ny; ++y) {
            QPointF point(minX + x * (maxX - minX) / nx, minY + y * (maxY - minY) / ny); // 调整比例以适应实际尺寸
            if (footprint.containsPoint(point, Qt::OddEvenFill)) {
                for (int i = 0; i < clusterCount; ++i) {
                    distance[i] = std::hypot(point.x() - Points[i][0], point.y() - Points[i][1]);
                    if (distance[0] <= clusterRadius) {
                        colorMap->data()->setCell(x, y, mapPressureToRange(dataPoint.pressure_right_1, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance[1] <= clusterRadius) {
                        colorMap->data()->setCell(x, y, mapPressureToRange(dataPoint.pressure_right_2, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance[2] <= clusterRadius) {
                        colorMap->data()->setCell(x, y, mapPressureToRange(dataPoint.pressure_right_3, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance[3] <= clusterRadius) {
                        colorMap->data()->setCell(x, y, mapPressureToRange(dataPoint.pressure_right_4, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance[4] <= clusterRadius) {
                        colorMap->data()->setCell(x, y, mapPressureToRange(dataPoint.pressure_right_5, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance[5] <= clusterRadius) {
                        colorMap->data()->setCell(x, y, mapPressureToRange(dataPoint.pressure_right_6, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance[6] <= clusterRadius) {
                        colorMap->data()->setCell(x, y, mapPressureToRange(dataPoint.pressure_right_7, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance[7] <= clusterRadius) {
                        colorMap->data()->setCell(x, y, mapPressureToRange(dataPoint.pressure_right_8, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance[8] <= clusterRadius) {
                        colorMap->data()->setCell(x, y, mapPressureToRange(dataPoint.pressure_right_9, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance[9] <= clusterRadius) {
                        colorMap->data()->setCell(x, y, mapPressureToRange(dataPoint.pressure_right_10, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance[10] <= clusterRadius) {
                        colorMap->data()->setCell(x, y, mapPressureToRange(dataPoint.pressure_right_11, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance[11] <= clusterRadius) {
                        colorMap->data()->setCell(x, y, mapPressureToRange(dataPoint.pressure_right_12, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance[12] <= clusterRadius) {
                        colorMap->data()->setCell(x, y, mapPressureToRange(dataPoint.pressure_right_13, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance[13] <= clusterRadius) {
                        colorMap->data()->setCell(x, y, mapPressureToRange(dataPoint.pressure_right_14, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance[14] <= clusterRadius) {
                        colorMap->data()->setCell(x, y, mapPressureToRange(dataPoint.pressure_right_15, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance[15] <= clusterRadius) {
                        colorMap->data()->setCell(x, y, mapPressureToRange(dataPoint.pressure_right_16, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance[16] <= clusterRadius) {
                        colorMap->data()->setCell(x, y, mapPressureToRange(dataPoint.pressure_right_17, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                    if (distance[17] <= clusterRadius) {
                        colorMap->data()->setCell(x, y, mapPressureToRange(dataPoint.pressure_right_18, 200, 6665, -60, 40)); // 圆内设置为角度值
                    }
                }
            }

        }
    }

}
