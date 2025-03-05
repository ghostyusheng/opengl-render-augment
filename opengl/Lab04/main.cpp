#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cmath>
#include <vector>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


// ---------------- 全局变量 ---------------- //
float upperArmLength = 2.0f; // 上臂长度
float lowerArmLength = 2.0f; // 下臂长度
float fingerLength = 0.5f;   // 每节手指长度

float shoulderAngle = 45.0f; // 肩关节角度
float elbowAngle = 45.0f;    // 肘关节角度

float fingerAngles[5][2] = { {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f} }; // 每根手指的两个关节角度

float targetX = 3.0f; // 目标点 X 坐标
float targetY = 3.0f; // 目标点 Y 坐标
float targetZ = 0.0f; // 目标点 Z 坐标

int windowWidth = 800; // 窗口宽度
int windowHeight = 600; // 窗口高度

// ---------------- 函数声明 ---------------- //
void initGL();
void display();
void resize(int w, int h);
void idle();
void drawArm();
void drawHand();
void drawFinger(float baseAngle, float midAngle);
void calculateIK(float tx, float ty, float tz);
void mouseMotion(int x, int y);

// ---------------- 主函数 ---------------- //
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("3D Arm Following Mouse");

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
void drawArm()
{
    glPushMatrix();

    // 绘制上臂
    glColor3f(0.8f, 0.3f, 0.3f);
    glRotatef(shoulderAngle, 0.0f, 0.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(upperArmLength, 0.0f, 0.0f);
    glEnd();
    glTranslatef(upperArmLength, 0.0f, 0.0f);

    // 绘制下臂
    glColor3f(0.3f, 0.8f, 0.3f);
    glRotatef(elbowAngle, 0.0f, 0.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(lowerArmLength, 0.0f, 0.0f);
    glEnd();
    glTranslatef(lowerArmLength, 0.0f, 0.0f);

    // 绘制手掌
    drawHand();

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
        drawFinger(fingerAngles[i][0], fingerAngles[i][1]);
        glPopMatrix();
    }
}

// ---------------- 绘制单根手指 ---------------- //
void drawFinger(float baseAngle, float midAngle)
{
    glColor3f(0.8f, 0.8f, 0.3f);

    // 绘制第一节
    glRotatef(baseAngle, 0.0f, 0.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(fingerLength, 0.0f, 0.0f);
    glEnd();
    glTranslatef(fingerLength, 0.0f, 0.0f);

    // 绘制第二节
    glRotatef(midAngle, 0.0f, 0.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(fingerLength, 0.0f, 0.0f);
    glEnd();
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
    gluLookAt(6.0, 6.0, 10.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    drawArm();

    glutSwapBuffers();
}
