#ifndef DATAUPDATER_H
#define DATAUPDATER_H

#include <QObject>
#include <QThread>
#include <QPolygonF>
#include <QMutex>
#include <QWaitCondition>
#include "qcustomplot.h"

// 初始化聚类坐标点

class DataUpdater : public QThread
{
    Q_OBJECT
public:
    DataUpdater(QCPColorMap *colorMap, const QVector<QPointF> &footprint, float Points[18][2], int nx, int ny, float minX, float maxX, float minY, float maxY, QObject *parent = nullptr);
    void requestUpdate();
    void stop();
    void pause();
    void resume();

signals:
    void dataUpdated();

protected:
    void run() override;

private:
    void updateHeatmap();

    QCPColorMap *colorMap;
    QPolygonF footprint;
    const float (*points)[2];
    int nx, ny;
    float minX, maxX, minY, maxY;
    bool running;
    bool updateRequested;
    bool paused; // 添加 paused 变量声明
    QMutex mutex;
    QWaitCondition condition;

};

#endif // DATAUPDATER_H
