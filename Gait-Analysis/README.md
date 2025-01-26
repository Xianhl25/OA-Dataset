# Gait-Analysis
这些脚本用于分析使用 IMU+压力鞋垫 采集的行走步态数据

## 01.my_gait_track_011_multi_people
该脚本整体介绍
具体步骤如下：
0.设置行走步态数据路径 folder_path ，读取该路径下的.csv文件并将数据储存在数组中，按照以下步骤遍历处理数据
1.对所需数据使用低通滤波进行简单去噪处理
2.基于足底压力数据，根据曲线波峰分别进行左腿和右腿的步态周期分割
3.基于小腿Gyro数据，计算所有步态周期的最大值平均值total max mean和最小值平均值total min mean，当某个步态周期的最大值处于total max mean的上下30%区间内、且最小值处于total min mean的上下30%区间内时，判断该步态周期为直行步态周期，否则为非直行步态周期
4.使用matplotlib绘制Roll误差带图、Pitch误差带图并将其储存在 folder_path/csv_name 内

