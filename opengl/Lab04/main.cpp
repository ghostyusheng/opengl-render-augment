
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cmath>
#include <vector>

float shoulderX = 0.0f, shoulderY = 0.0f, shoulderZ = 0.0f; // 肩膀位置
float shoulderAngle = 45.0f; // 肩部旋转角度
float elbowAngle = 30.0f;    // 肘部旋转角度

float fingerBaseAngle[5] = { 10, 15, 10, 15, 10 }; // 手指第一关节旋转角
float fingerMidAngle[5] = { 10, 15, 10, 15, 10 };  // 手指第二关节旋转角
float fingerTipAngle[5] = { 5, 10, 5, 10, 5 };     // 手指第三关节旋转角


// 摄像机参数
float cameraAngleX = 45.0f; // 水平方向旋转角度
float cameraAngleY = 30.0f; // 垂直方向旋转角度
float cameraDistance = 10.0f; // 摄像机距离中心点的距离

// ---------------- 全局变量 ---------------- //
float upperArmLength = 2.0f; // 上臂长度
float lowerArmLength = 2.0f; // 下臂长度
float fingerLength = 0.5f;   // 每节手指长度


float fingerAngles[5][3] = { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} }; // 每根手指的三个关节角度

float targetX = 3.0f; // 目标点 X 坐标
float targetY = 3.0f; // 目标点 Y 坐标
float targetZ = 0.0f; // 目标点 Z 坐标

int windowWidth = 800; // 窗口宽度
int windowHeight = 600; // 窗口高度

bool isGrabbing = false; // 是否正在抓取

// ---------------- 函数声明 ---------------- //
void initGL();
void display();
void resize(int w, int h);
void idle();
void drawArm();
void drawHand();
void drawFinger(float baseAngle, float midAngle, float tipAngle);
void calculateIK(float tx, float ty, float tz);
void mouseMotion(int x, int y);
void mouseClick(int button, int state, int x, int y);

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 'w': // 向上旋转
        cameraAngleY += 5.0f;
        if (cameraAngleY > 89.0f) cameraAngleY = 89.0f; // 限制垂直角度范围
        break;
    case 's': // 向下旋转
        cameraAngleY -= 5.0f;
        if (cameraAngleY < -89.0f) cameraAngleY = -89.0f;
        break;
    case 'a': // 向左旋转
        cameraAngleX -= 5.0f;
        break;
    case 'd': // 向右旋转
        cameraAngleX += 5.0f;
        break;
    case '+': // 缩近
        cameraDistance -= 0.5f;
        if (cameraDistance < 2.0f) cameraDistance = 2.0f; // 限制最近距离
        break;
    case '-': // 拉远
        cameraDistance += 0.5f;
        break;
    case 27: // ESC 键退出程序
        exit(0);
        break;
    }
    glutPostRedisplay(); // 触发重绘
}


// ---------------- 主函数 ---------------- //
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("3D Arm with Grabbing Fingers");

    GLenum glewInitResult = glewInit();
    if (glewInitResult != GLEW_OK)
    {
        return -1;
    }

    initGL();
    glutDisplayFunc(display);
    glutReshapeFunc(resize);
    glutIdleFunc(idle);
    glutPassiveMotionFunc(mouseMotion); // 捕获鼠标移动
    glutMouseFunc(mouseClick);          // 捕获鼠标点击
    glutKeyboardFunc(keyboard); // 注册键盘回调

    glutMainLoop();
    return 0;
}

// ---------------- OpenGL 初始化 ---------------- //
void initGL()
{
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
}

// ---------------- 窗口大小变化 ---------------- //
void resize(int w, int h)
{
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

// ---------------- 绘制机械臂 ---------------- //
// ---------------- 绘制机械臂 ---------------- //
void drawArm() {
    GLUquadric* quad = gluNewQuadric();

    // **肩部**
    glPushMatrix();
    glRotatef(shoulderAngle, 0.0f, 0.0f, 1.0f); // 肩部旋转

    // **上臂**
    glPushMatrix();
    glRotatef(90, 0.0f, 1.0f, 0.0f);
    gluCylinder(quad, 0.2f, 0.2f, upperArmLength, 16, 16);
    glPopMatrix();

    // **肘部**
    glTranslatef(upperArmLength, 0.0f, 0.0f);
    glRotatef(elbowAngle, 0.0f, 0.0f, 1.0f);

    // **前臂**
    glPushMatrix();
    glRotatef(90, 0.0f, 1.0f, 0.0f);
    gluCylinder(quad, 0.15f, 0.15f, lowerArmLength, 16, 16);
    glPopMatrix();

    // **手掌**
    glTranslatef(lowerArmLength, 0.0f, 0.0f);
    glPushMatrix();
    glColor3f(0.8f, 0.8f, 0.3f);
    glutSolidCube(0.4f);
    glPopMatrix();

    // **绘制爪子（线条方式）**
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);
    for (int i = 0; i < 5; i++) {
        float x = 0.1f * (i - 2);
        float y = 0.2f;

        // 第一节
        glVertex3f(x, y, 0.0f);
        glVertex3f(x + 0.2f * cos(fingerBaseAngle[i] * 3.14 / 180.0f),
            y + 0.2f * sin(fingerBaseAngle[i] * 3.14 / 180.0f), 0.0f);

        // 第二节
        float x2 = x + 0.2f * cos(fingerBaseAngle[i] * 3.14 / 180.0f);
        float y2 = y + 0.2f * sin(fingerBaseAngle[i] * 3.14 / 180.0f);
        glVertex3f(x2, y2, 0.0f);
        glVertex3f(x2 + 0.15f * cos(fingerBaseAngle[i] * 3.14 / 180.0f),
            y2 + 0.15f * sin(fingerBaseAngle[i] * 3.14 / 180.0f), 0.0f);
    }
    glEnd();

    glPopMatrix();
}

// ---------------- 绘制手掌和手指 ---------------- //
void drawHand()
{
    glColor3f(0.3f, 0.3f, 0.8f);
    glutSolidSphere(0.2f, 16, 16); // 手掌为一个小球

    // 绘制 5 根手指
    for (int i = 0; i < 5; ++i)
    {
        glPushMatrix();
        float angleOffset = -30.0f + i * 15.0f; // 每根手指的初始角度偏移
        glRotatef(angleOffset, 0.0f, 1.0f, 0.0f);
        drawFinger(fingerAngles[i][0], fingerAngles[i][1], fingerAngles[i][2]);
        glPopMatrix();
    }
}

// ---------------- 绘制单根手指 ---------------- //
void drawFinger(float baseAngle, float midAngle, float tipAngle)
{
    glColor3f(0.8f, 0.8f, 0.3f);

    // 绘制第一节手指
    glPushMatrix();
    glRotatef(baseAngle, 0.0f, 0.0f, 1.0f);
    glColor3f(0.8f, 0.8f, 0.3f);
    glutSolidCube(fingerLength);
    glTranslatef(fingerLength * 2.6f, 0.0f, 0.0f); // 适当调整连接位置

    // 绘制第二节手指
    glRotatef(midAngle, 0.0f, 0.0f, 1.0f);
    glutSolidCube(fingerLength * 0.8f);
    glTranslatef(fingerLength * 2.5f, 0.0f, 0.0f);

    // 绘制第三节手指
    glRotatef(tipAngle, 0.0f, 0.0f, 1.0f);
    glutSolidCube(fingerLength * 2.6f);
    glPopMatrix();
}

// ---------------- 逆运动学计算 ---------------- //
void calculateIK(float tx, float ty, float tz)
{
    // 计算平面距离
    float targetDistance = sqrt(tx * tx + ty * ty);
    float totalArmLength = upperArmLength + lowerArmLength;

    // 限制目标点在机械臂的可达范围内
    if (targetDistance > totalArmLength)
        targetDistance = totalArmLength;

    // 计算肘部角度
    float cosAngle = (upperArmLength * upperArmLength + lowerArmLength * lowerArmLength - targetDistance * targetDistance) /
        (2.0f * upperArmLength * lowerArmLength);
    elbowAngle = acos(cosAngle) * 180.0f / M_PI;

    // 计算肩部角度
    float angleToTarget = atan2(ty, tx) * 180.0f / M_PI;
    float offsetAngle = acos((upperArmLength * upperArmLength + targetDistance * targetDistance - lowerArmLength * lowerArmLength) /
        (2.0f * upperArmLength * targetDistance)) *
        180.0f / M_PI;
    shoulderAngle = angleToTarget - offsetAngle;
}



// ---------------- 鼠标移动回调 ---------------- //
void mouseMotion(int x, int y)
{
    // 将鼠标坐标转换为 OpenGL 世界坐标
    float normalizedX = (float)x / windowWidth * 2.0f - 1.0f; // 转换到 [-1, 1]
    float normalizedY = 1.0f - (float)y / windowHeight * 2.0f; // 转换到 [-1, 1]

    // 假设视图在 XY 平面上，Z 坐标固定为 0
    targetX = normalizedX * 5.0f; // 放大到机械臂的工作范围
    targetY = normalizedY * 5.0f; // 放大到机械臂的工作范围
    targetZ = 0.0f;

    // 更新逆运动学
    calculateIK(targetX, targetY, targetZ);
}

// ---------------- 鼠标点击回调 ---------------- //
void mouseClick(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        isGrabbing = !isGrabbing; // 切换抓取状态

        // 更新手指角度
        for (int i = 0; i < 5; ++i)
        {
            if (isGrabbing)
            {
                fingerAngles[i][0] = 45.0f; // 根关节弯曲角度
                fingerAngles[i][1] = 45.0f; // 中关节弯曲角度
                fingerAngles[i][2] = 30.0f; // 末端关节弯曲角度
            }
            else
            {
                fingerAngles[i][0] = 0.0f;
                fingerAngles[i][1] = 0.0f;
                fingerAngles[i][2] = 0.0f;
            }
        }
    }
}

// ---------------- 空闲回调 ---------------- //
void idle()
{
    // 触发重绘
    glutPostRedisplay();
}

// ---------------- 主绘制函数 ---------------- //
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

    // 计算摄像机位置
    float camX = cameraDistance * cos(cameraAngleY * M_PI / 180.0f) * sin(cameraAngleX * M_PI / 180.0f);
    float camY = cameraDistance * sin(cameraAngleY * M_PI / 180.0f);
    float camZ = cameraDistance * cos(cameraAngleY * M_PI / 180.0f) * cos(cameraAngleX * M_PI / 180.0f);

    gluLookAt(camX, camY, camZ, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    drawArm();

    glutSwapBuffers();
}

