#ifndef MYGLWIDGET_H
#define MYGLWIDGET_H

#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QTimer>
#include <cmath>
#include <fstream>
#define __CL_ENABLE_EXCEPTIONS
#include "CL/cl.hpp"
#include <vector>
#include <QPoint>
#include <QMouseEvent>
#include <iostream>

using namespace std;
using namespace cl;

class MyGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
        Q_OBJECT

        Context contextCL;
        CommandQueue queue;
        Program program;
        Kernel kernel;
        vector<QPoint> sources;
        bool CLready;
        GLuint texture;
        cl_mem img;
        int width;
        int height;
        float limit;
        bool applyLimit;
        float charge;

    public slots:
        void setCharge(int);
        void setLowerLimit(int);
        void setLimiting(bool);

    public:
        MyGLWidget(QWidget *);
        ~MyGLWidget() {}

    protected:
        void initializeGL();
        void initializeCL();
        void resizeGL(int, int);
        void paintGL();
        void mouseMoveEvent(QMouseEvent *event);

    private:
        void calculateTexture();
        float map(float, float, float, float, float);
        QPoint map(QPoint, QRect, QRect);

};

#endif // MYGLWIDGET_H
