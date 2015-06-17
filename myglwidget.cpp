#include "myglwidget.h"

void MyGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(1.0, 0, 0, 1.0);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_TEXTURE_2D);
    initializeCL();
}

void MyGLWidget::initializeCL()
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();

    if (!ctx) {
        qWarning("Attempted CL-GL interop without a current OpenGL context");
        return;
    }

    // Get available platforms
    vector<Platform> platforms;
    vector<Device> devices;
    Platform::get(&platforms);
    // Select the default platform and create a context using this platform and the GPU
    cl_context_properties cps[] = {
        CL_GL_CONTEXT_KHR,
        (cl_context_properties)wglGetCurrentContext(),
        CL_WGL_HDC_KHR,
        (cl_context_properties)wglGetCurrentDC(),
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)(platforms[0])(),
        0
    };
    contextCL = Context(CL_DEVICE_TYPE_GPU, cps);
    // Get a list of devices on this platform
    devices = contextCL.getInfo<CL_CONTEXT_DEVICES>();
    // Create a command queue and use the first device
    queue = CommandQueue(contextCL, devices[0]);
    // Read source file
    ifstream sourceFile("metaballs.cl");
    string sourceCode(
        istreambuf_iterator<char>(sourceFile),
        (istreambuf_iterator<char>()));
    Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length() + 1));
    // Make program of the source code in the context
    program = Program(contextCL, source);
    // Build program for these specific devices
    program.build(devices);
    // Make kernel
    kernel = Kernel(program, "metaballs");
    CLready = true;
    // Fill the sources vector with a grid pattern
    sources.clear();

    for (int i = 0; i <= width; i += 100) {
        for (int j = 0; j <= height; j += 100) {
            sources.push_back(QPoint(i, j));
        }
    }

    // One last point that will be used as the mouse pointer
    sources.push_back(QPoint());
    // Create an OpenGL texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
}

void MyGLWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
}

void MyGLWidget::setCharge(int c)
{
    charge = map(c, 0, 100, 1, 100);
    update();
}

void MyGLWidget::setLowerLimit(int l)
{
    limit = map(l, 0, 100, 0.001, 0.5);
    update();
}

void MyGLWidget::setLimiting(bool l)
{
    applyLimit = l;
    update();
}

MyGLWidget::MyGLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    setFormat(format);
    CLready = false;
    width = 800;
    height = 600;
    setMouseTracking(true);
}

void MyGLWidget::calculateTexture()
{
    int num = sources.size();
    // Create a memory buffer on the GPU
    Buffer bufferSources = Buffer(contextCL, CL_MEM_READ_ONLY, num * sizeof(cl_int2));
    // Create a memory buffer on the CPU
    cl_int2 *sourcePoints = new cl_int2[num];

    // Fill the buffer with the coordinates of the source points
    for (unsigned i = 0; i < num; i++) {
        sourcePoints[i].s[0] = sources.at(i).x();
        sourcePoints[i].s[1] = sources.at(i).y();
    }

    cl_int res;
    // Create an OpenCL handle from an OpenGL texture
    img = clCreateFromGLTexture(contextCL(), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, texture, &res);

    if (res != 0) {
        cout << "Error creating from OpenGL! Error=" << res << endl;
        return;
    }

    // Create an image from the texture
    Image2D output(img);
    // Tell OpenGL to finish so that we can get to its texture
    glFinish();
    vector<Memory> objs;
    objs.push_back(Memory(img));
    // Ask OpenGL to release the object contaning our image
    queue.enqueueAcquireGLObjects(&objs);
    // Copy the buffer with the source points
    res = queue.enqueueWriteBuffer(bufferSources, CL_TRUE, 0, num * sizeof(cl_int2), sourcePoints);
    // Now we can delete the buffer from the CPU side
    delete[] sourcePoints;
    // Set arguments to kernel
    res += kernel.setArg(0, bufferSources);
    res += kernel.setArg(1, cl_int(num));
    res += kernel.setArg(2, cl_float(charge));
    res += kernel.setArg(3, cl_float(limit));
    res += kernel.setArg(4, cl_int(applyLimit));
    res += kernel.setArg(5, output);
    // Run the kernel on specific ND range
    NDRange global(width, height);
    queue.enqueueNDRangeKernel(kernel, NullRange, global, NullRange);
    // Ask OpenCL to release the objects
    queue.enqueueReleaseGLObjects(&objs);
    // Bind the texture so that it will be used in OpenGL
    glBindTexture(GL_TEXTURE_2D, texture);
}

void MyGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    if (CLready) calculateTexture();

    glBegin(GL_QUADS);
    glTexCoord2f(0, 1);
    glVertex2f(0, 0);
    glTexCoord2f(1, 1);
    glVertex2f(1, 0);
    glTexCoord2f(1, 0);
    glVertex2f(1, 1);
    glTexCoord2f(0, 0);
    glVertex2f(0, 1);
    glEnd();
}

float MyGLWidget::map(float x, float xmin, float xmax, float ymin, float ymax)
{
    return ymin + (x - xmin) * (ymax - ymin) / (xmax - xmin);
}

QPoint MyGLWidget::map(QPoint X, QRect input, QRect output)
{
    QPoint res;
    res.setX(map(X.x(), input.left(), input.right(), output.left(), output.right()));
    res.setY(map(X.y(), input.bottom(), input.top(), output.bottom(), output.top()));
    return res;
}

void MyGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    sources.back() = map(event->pos(), QRect(0, 0, size().width(), size().height()), QRect(0, 0, width, height));
    update();
}
