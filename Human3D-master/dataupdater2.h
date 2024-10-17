#ifndef DATAUPDATER2_H
#define DATAUPDATER2_H

#include <QObject>
#include <QThread>
#include <QPolygonF>
#include <QMutex>
#include <QWaitCondition>
#include "qcustomplot.h"

class DataUpdater2 : public QThread
{
    Q_OBJECT
public:
    DataUpdater2(QCPColorMap *colorMap2, const QVector<QPointF> &footprint2, float Points2[18][2],
                 int nx2, int ny2, float minX2, float maxX2, float minY2, float maxY2, QObject *parent = nullptr);
    void requestUpdate();
    void stop();
    void pause();
    void resume();

signals:
    void dataUpdated();

protected:
    void run() override;
    void updateHeatmap();

private:
    QCPColorMap *colorMap2;
    QPolygonF footprint2;
    float (*points2)[2];
    int nx2, ny2;
    float minX2, maxX2, minY2, maxY2;
    bool running2;
    bool paused2;
    bool updateRequested2;
    QMutex mutex2;
    QWaitCondition condition2;
};

#endif // DATAUPDATER2_H
