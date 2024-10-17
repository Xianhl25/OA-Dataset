#ifndef GLOBALS_H
#define GLOBALS_H
struct DataPoint {
    double timestamp;
    float angle1_x;
    float angle1_y;
    float angle1_z;
    float gyro1_x;
    float gyro1_y;
    float gyro1_z;
    float acc1_x;
    float acc1_y;
    float acc1_z;

    float angle2_x;
    float angle2_y;
    float angle2_z;
    float gyro2_x;
    float gyro2_y;
    float gyro2_z;
    float acc2_x;
    float acc2_y;
    float acc2_z;

    float angle3_x;
    float angle3_y;
    float angle3_z;
    float gyro3_x;
    float gyro3_y;
    float gyro3_z;
    float acc3_x;
    float acc3_y;
    float acc3_z;

    float angle4_x;
    float angle4_y;
    float angle4_z;
    float gyro4_x;
    float gyro4_y;
    float gyro4_z;
    float acc4_x;
    float acc4_y;
    float acc4_z;

    float pressure_left_1;
    float pressure_left_2;
    float pressure_left_3;
    float pressure_left_4;
    float pressure_left_5;
    float pressure_left_6;
    float pressure_left_7;
    float pressure_left_8;
    float pressure_left_9;
    float pressure_left_10;
    float pressure_left_11;
    float pressure_left_12;
    float pressure_left_13;
    float pressure_left_14;
    float pressure_left_15;
    float pressure_left_16;
    float pressure_left_17;
    float pressure_left_18;

    float pressure_right_1;
    float pressure_right_2;
    float pressure_right_3;
    float pressure_right_4;
    float pressure_right_5;
    float pressure_right_6;
    float pressure_right_7;
    float pressure_right_8;
    float pressure_right_9;
    float pressure_right_10;
    float pressure_right_11;
    float pressure_right_12;
    float pressure_right_13;
    float pressure_right_14;
    float pressure_right_15;
    float pressure_right_16;
    float pressure_right_17;
    float pressure_right_18;

    float angle5_x;
    float angle5_y;
    float angle5_z;
    float gyro5_x;
    float gyro5_y;
    float gyro5_z;
    float acc5_x;
    float acc5_y;
    float acc5_z;

    float angle6_x;
    float angle6_y;
    float angle6_z;
    float gyro6_x;
    float gyro6_y;
    float gyro6_z;
    float acc6_x;
    float acc6_y;
    float acc6_z;

    float angleM_x;
    float angleM_y;
    float angleM_z;
    float gyroM_x;
    float gyroM_y;
    float gyroM_z;
    float accM_x;
    float accM_y;
    float accM_z;
};

extern DataPoint dataPoint; // 声明全局变量
extern float Points[18][2];
extern float Points2[18][2];
extern float minX;
extern float maxX;
extern float minY;
extern float maxY;
extern int nx, ny; // 分辨率与初始化一致
extern float minX2;
extern float maxX2;
extern float minY2;
extern float maxY2;
extern int nx2, ny2; // 分辨率与初始化一致
#endif // GLOBALS_H
