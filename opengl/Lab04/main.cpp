#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <cmath>

// GLM 数学库 (需要提前安装)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// --------------- 全局变量区域 --------------- //

// 窗口大小
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// 机械臂参数
float link1Length = 2.0f;  // 上臂长度
float link2Length = 1.5f;  // 下臂长度

// 目标 (末端执行器希望到达的位置)，初始在(2.5, 1.0) 附近
float targetX = 2.5f;
float targetY = 1.0f;

// 经过 IK 计算后的关节角度(弧度值)
float jointAngle1 = 0.0f;  // 上臂与“躯干”连接处关节角
float jointAngle2 = 0.0f;  // 下臂与上臂连接处关节角

// 摄像机 / 投影相关
glm::mat4 projMat;    // 投影矩阵
glm::mat4 viewMat;    // 视图矩阵

// 用于演示对目标位置的简单动画
float timeElapsed = 0.0f;

// --------------- 函数声明 --------------- //
void initGL();
void resize(int w, int h);
void display();
void idle();
void calculateIK(float tx, float ty);
void drawCoordinateAxes();
void drawCube(float size);
void drawArmSystem();

// ---------------------------------------- //
//  主函数
// ---------------------------------------- //
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow("Inverse Kinematics Demo");
	// 初始化 GLEW
	GLenum glewInitResult = glewInit();
	if (glewInitResult != GLEW_OK)
	{
		std::cerr << "Error initializing GLEW: "
			<< glewGetErrorString(glewInitResult) << std::endl;
		return -1;
	}

	initGL();

	glutReshapeFunc(resize);
	glutDisplayFunc(display);
	glutIdleFunc(idle);

	glutMainLoop();
	return 0;
}

// ---------------------------------------- //
//  OpenGL 设置
// ---------------------------------------- //
void initGL()
{
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	// 简单设置相机
	viewMat = glm::lookAt(glm::vec3(0.0f, 5.0f, 8.0f), // 摄像机位置
		glm::vec3(0.0f, 0.0f, 0.0f), // 观察目标
		glm::vec3(0.0f, 1.0f, 0.0f)); // 上方向
}

void resize(int w, int h)
{
	glViewport(0, 0, w, h);
	float aspect = static_cast<float>(w) / static_cast<float>(h);
	// 简单使用透视投影
	projMat = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
}

// ---------------------------------------- //
//  主循环：绘制场景
// ---------------------------------------- //
void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// 准备模型-视图-投影矩阵
	glm::mat4 modelMat = glm::mat4(1.0f); // 单位矩阵
	glm::mat4 mvMat = viewMat * modelMat;
	glm::mat4 mvpMat = projMat * mvMat;

	// (本示例使用固定管线方式简单绘制，若使用自定义着色器，请在这里传 uniform 等。)

	// 1) 画一下坐标轴，方便观察(可注释掉)
	drawCoordinateAxes();

	// 2) 绘制机械臂系统(包括“躯干”、“上臂”、“下臂”、“末端执行器”)
	drawArmSystem();

	glutSwapBuffers();
}

// ---------------------------------------- //
//  idle 回调：让目标位置简单地随时间运动
// ---------------------------------------- //
void idle()
{
	timeElapsed += 0.01f; // 时间步，可根据系统刷新率或定时器自行调整
	// 让 targetX, targetY 做一个小范围的动态演示：
// 例如在 (2.5, 1.0) 附近画圆，半径 0.5
	float radius = 0.5f;
	targetX = 2.5f + radius * std::cos(timeElapsed);
	targetY = 1.0f + radius * std::sin(timeElapsed);

	// 计算 IK，更新关节角
	calculateIK(targetX, targetY);

	// 再次请求绘制
	glutPostRedisplay();
}

// ---------------------------------------- //
//  逆运动学解析解 (2 段连杆, 2D 平面简化)
//  输入: 目标 (tx, ty)
//  输出: 更新 jointAngle1, jointAngle2
// ---------------------------------------- //
void calculateIK(float tx, float ty)
{
	// 1) 计算到目标的距离 d
	float d = std::sqrt(tx * tx + ty * ty);
	// 避免目标过近或过远导致无解，这里做下简单限制
	float eps = 0.0001f;
	if (d < eps) d = eps; // 避免除以 0
	if (d > link1Length + link2Length - eps)
		d = link1Length + link2Length - eps;

	// 2) 根据几何关系计算夹角 (参考两连杆 IK 典型公式)
	//   cos(theta2) = (d^2 - L1^2 - L2^2) / (2 * L1 * L2)
	float cosAngle2 = (d * d - link1Length * link1Length - link2Length * link2Length)
		/ (2.0f * link1Length * link2Length);
	// 由于浮点误差，需限制范围 [-1, 1]
	if (cosAngle2 > 1.0f) cosAngle2 = 1.0f;
	if (cosAngle2 < -1.0f) cosAngle2 = -1.0f;

	float theta2 = std::acos(cosAngle2);

	// 3) 计算 theta1
	//   theta1 = atan2(ty, tx) - atan2(L2 * sin(theta2), L1 + L2 * cos(theta2))
	float sinAngle2 = std::sqrt(1.0f - cosAngle2 * cosAngle2);
	float phi = std::atan2(link2Length * sinAngle2,
		link1Length + link2Length * cosAngle2);
	float alpha = std::atan2(ty, tx);
	float theta1 = alpha - phi;

	// 更新全局变量
	jointAngle1 = theta1;  // 上臂关节角
	jointAngle2 = theta2;  // 下臂关节角
}

// ---------------------------------------- //
//  绘制坐标轴 (仅调试用)
// ---------------------------------------- //
void drawCoordinateAxes()
{
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glm::value_ptr(projMat));
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(glm::value_ptr(viewMat));

	glBegin(GL_LINES);
	// X 轴 红色
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(5.0f, 0.0f, 0.0f);

	// Y 轴 绿色
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 5.0f, 0.0f);

	// Z 轴 蓝色
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 5.0f);
	glEnd();
}

// ---------------------------------------- //
//  绘制一个方块 (size 指定边长)
// ---------------------------------------- //
void drawCube(float size)
{
	float half = size * 0.5f;
	glBegin(GL_QUADS);
	// 前面
	glColor3f(0.7f, 0.7f, 0.7f);
	glVertex3f(-half, -half, half);
	glVertex3f(half, -half, half);
	glVertex3f(half, half, half);
	glVertex3f(-half, half, half);
	// 后面
	glVertex3f(-half, -half, -half);
	glVertex3f(-half, half, -half);
	glVertex3f(half, half, -half);
	glVertex3f(half, -half, -half);

	// 左面
	glVertex3f(-half, -half, -half);
	glVertex3f(-half, -half, half);
	glVertex3f(-half, half, half);
	glVertex3f(-half, half, -half);

	// 右面
	glVertex3f(half, -half, -half);
	glVertex3f(half, half, -half);
	glVertex3f(half, half, half);
	glVertex3f(half, -half, half);

	// 上面
	glVertex3f(-half, half, half);
	glVertex3f(half, half, half);
	glVertex3f(half, half, -half);
	glVertex3f(-half, half, -half);

	// 下面
	glVertex3f(-half, -half, -half);
	glVertex3f(half, -half, -half);
	glVertex3f(half, -half, half);
	glVertex3f(-half, -half, half);
	glEnd();
}

// ---------------------------------------- //
//  绘制机械臂系统
//  包括 “躯干 + 上臂 + 下臂 + 末端执行器”
// ---------------------------------------- //
void drawArmSystem()
{
	// 切换矩阵模式并设置好投影/视图
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glm::value_ptr(projMat));
	glMatrixMode(GL_MODELVIEW);
	// 基于 viewMat 做后续模型变换
	glm::mat4 modelMat = viewMat;

	// 1) 绘制躯干 (此处用一个竖直放置的方块表示)
	{
		glm::mat4 torsoMat = glm::translate(modelMat, glm::vec3(0.0f, 0.0f, 0.0f));
		torsoMat = glm::scale(torsoMat, glm::vec3(1.0f, 2.0f, 0.5f)); // 高一点，作为“躯干”
		glLoadMatrixf(glm::value_ptr(torsoMat));
		drawCube(1.0f);
	}

	// 2) 绘制上臂: 基点(0,0,0)，此处简化假设关节在躯干顶部附近
	{
		glm::mat4 upperArmMat = glm::translate(modelMat, glm::vec3(0.0f, 1.0f, 0.0f));
		// 绕 Z 轴 或 Y 轴旋转都视需求而定，这里假设关节旋转轴垂直屏幕(简单 2D 平面)
		upperArmMat = glm::rotate(upperArmMat, jointAngle1, glm::vec3(0.0f, 0.0f, 1.0f));
		// 再把方块拉伸成“上臂”
		upperArmMat = glm::scale(upperArmMat, glm::vec3(link1Length, 0.3f, 0.3f));

		glLoadMatrixf(glm::value_ptr(upperArmMat));
		drawCube(1.0f);
	}

	// 3) 绘制下臂: 基于上臂末端
	{
		// 先平移到上臂末端 (上臂长度 link1Length)
		glm::mat4 lowerArmMat = glm::translate(modelMat, glm::vec3(0.0f, 1.0f, 0.0f));
		lowerArmMat = glm::rotate(lowerArmMat, jointAngle1, glm::vec3(0.0f, 0.0f, 1.0f));
		lowerArmMat = glm::translate(lowerArmMat, glm::vec3(link1Length, 0.0f, 0.0f));
		// 再对下臂进行自身旋转
		lowerArmMat = glm::rotate(lowerArmMat, jointAngle2, glm::vec3(0.0f, 0.0f, 1.0f));
		// 拉伸成“下臂”
		lowerArmMat = glm::scale(lowerArmMat, glm::vec3(link2Length, 0.2f, 0.2f));

		glLoadMatrixf(glm::value_ptr(lowerArmMat));
		drawCube(1.0f);
	}

	// 4) 绘制末端执行器(小球或小方块)
	{
		// 计算末端在下臂末端的位置
		glm::mat4 endEffectorMat = glm::translate(modelMat, glm::vec3(0.0f, 1.0f, 0.0f));
		endEffectorMat = glm::rotate(endEffectorMat, jointAngle1, glm::vec3(0.0f, 0.0f, 1.0f));
		endEffectorMat = glm::translate(endEffectorMat, glm::vec3(link1Length, 0.0f, 0.0f));
		endEffectorMat = glm::rotate(endEffectorMat, jointAngle2, glm::vec3(0.0f, 0.0f, 1.0f));
		endEffectorMat = glm::translate(endEffectorMat, glm::vec3(link2Length, 0.0f, 0.0f));
		// 末端稍微放大一点点
		endEffectorMat = glm::scale(endEffectorMat, glm::vec3(0.2f));

		glLoadMatrixf(glm::value_ptr(endEffectorMat));
		// 用方块代替小球
		glColor3f(1.0f, 0.0f, 0.0f);
		drawCube(1.0f);
	}
}