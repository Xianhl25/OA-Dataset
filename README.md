# OA-Dataset
This is a scripts repository for OA dataset

## Experiment Protocol

* 

## Sensor type

* Inertial measurement unit (IMU) on trunk, pelvis, thigh, shank and foot
* EMG
* Pressure insole
* Metabolic consumption

## OpenSim

Scripting in Python

🔗：https://opensimconfluence.atlassian.net/wiki/x/ogQqAw



OpenSense - Kinematics with IMU data

🔗：https://opensimconfluence.atlassian.net/wiki/x/KwAqAw

## Data Collection Software

**Some warning**

🔗：[Qt项目拷贝到其他电脑无法构建运行等问题的解决_用qt生成的vbs文件拷贝到桌面运行和自己直接在桌面编写的vbs文件冲突-CSDN博客](https://blog.csdn.net/lym940928/article/details/90208559)

When you open the source code on your PC first time, you shuold:

* Delete the `.user` file in the folder

* Modify the `.pro` file

E.g : 

```
win32:CONFIG(release, debug|release): LIBS += -L'F:/Program Files/msys64/mingw64/lib/'
else:win32:CONFIG(debug, debug|release): LIBS += -L'F:/Program Files/msys64/mingw64/lib/'
INCLUDEPATH += 'F:/Program Files/msys64/mingw64/include'
DEPENDPATH += 'F:/Program Files/msys64/mingw64/include'
```

You need to modify the path of the complier:

```
win32:CONFIG(release, debug|release): LIBS += -L'D:/Qt/Qt5.9.0/Tools/mingw530_32/lib'
else:win32:CONFIG(debug, debug|release): LIBS += -L'D:/Qt/Qt5.9.0/Tools/mingw530_32/lib'
INCLUDEPATH += 'D:/Qt/Qt5.9.0/Tools/mingw530_32/include'
DEPENDPATH += 'D:/Qt/Qt5.9.0/Tools/mingw530_32/include'
```

And some extern lib used in the project, such as freeglut:

```
LIBS += -L"D:/code/LowerLimb/Outdoors-Dataset/Human3D-master/freeglut/lib" -lfreeglut
INCLUDEPATH += 'D:/code/LowerLimb/Outdoors-Dataset/Human3D-master/freeglut/include'
```

OpenGL:

```
LIBS += -lopengl32 -lglu32
```

What's more, the Qt version should be same as the project.
