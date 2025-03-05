/* 3D 机械臂逆运动学演示代码 */
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <cmath>

// GLM 数学库
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ---------------- 全局变量 ---------------- //
// 窗口大小
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// 机械臂参数（连杆长度）
float link1Length = 2.0f;  // 上臂长度
float link2Length = 1.5f;  // 下臂长度

// 目标点（三维）：初始设置在 (2.5, 1.0, 2.5) 附近
float targetX = 2.5f;
float targetY = 1.0f;
float targetZ = 2.5f;

// IK 计算后的关节角（弧度）
// baseAngle：基座绕 Y 轴旋转角
// jointAngle1：肩关节在机械臂平面内（绕 Z 轴旋转）的角度
// jointAngle2：肘关节角（平面内）
float baseAngle = 0.0f;
float jointAngle1 = 0.0f;
float jointAngle2 = 0.0f;

// 摄像机和投影相关
glm::mat4 projMat;    // 投影矩阵
glm::mat4 viewMat;    // 视图矩阵

// 时间变量，用于目标动画
float timeElapsed = 0.0f;

// ---------------- 函数声明 ---------------- //
void initGL();
void resize(int w, int h);
void display();
void idle();
void calculateIK3D(float tx, float ty, float tz);
void drawCoordinateAxes();
void drawCube(float size);
void drawArmSystem();

// ---------------- 主函数 ---------------- //
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow("3D Inverse Kinematics Demo");
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

// ---------------- OpenGL 设置 ---------------- //
void initGL()
{
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	// 设置摄像机：摄像机放在 (0,5,8) 观察原点
	viewMat = glm::lookAt(glm::vec3(0.0f, 5.0f, 8.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));
}

void resize(int w, int h)
{
	glViewport(0, 0, w, h);
	float aspect = static_cast<float>(w) / static_cast<float>(h);
	projMat = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
}

// ---------------- 主绘制函数 ---------------- //
void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// 重新加载视图和投影矩阵（使用固定管线）
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glm::value_ptr(projMat));
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(glm::value_ptr(viewMat));

	// 画坐标轴辅助调试
	drawCoordinateAxes();

	// 绘制机械臂系统
	drawArmSystem();

	glutSwapBuffers();
}

// ---------------- idle 回调 ---------------- //
void idle()
{
	timeElapsed += 0.01f; // 时间步长，可以根据需要调整
	// 动画：让目标在水平面上转动，同时 Y 方向有小幅波动
	float radius = 0.5f;
	targetX = 2.5f + radius * std::cos(timeElapsed);
	targetZ = 2.5f + radius * std::sin(timeElapsed);
	targetY = 1.0f + 0.3f * std::sin(timeElapsed * 0.5f);
	// 更新 IK，计算新的关节角
	calculateIK3D(targetX, targetY, targetZ);

	glutPostRedisplay();
}

// ---------------- 3D 逆运动学解析函数 ---------------- //
// 针对两连杆：第一步先求水平距离，再在垂直平面内处理
void calculateIK3D(float tx, float ty, float tz)
{
	// 求水平投影距离
	float r = std::sqrt(tx * tx + tz * tz);
	// 求目标到肩关节（原点于水平面上偏移后）的距离 R
	float R = std::sqrt(r * r + ty * ty);
	// 为了防止超过机械臂可达极限，进行简单限制（略去 eps 细节）
	if (R > link1Length + link2Length)
		R = link1Length + link2Length - 0.0001f;

	// 基座角：使机械臂转向目标水平位置（绕 Y 轴旋转）
	baseAngle = std::atan2(tz, tx);

	// 利用两连杆平面 IK 求解关节角：
	// cos(theta2) = (R^2 - L1^2 - L2^2) / (2 * L1 * L2)
	float cosTheta2 = (R * R - link1Length * link1Length - link2Length * link2Length) /
		(2.0f * link1Length * link2Length);
	if (cosTheta2 > 1.0f) cosTheta2 = 1.0f;
	if (cosTheta2 < -1.0f) cosTheta2 = -1.0f;
	jointAngle2 = std::acos(cosTheta2);  // 肘关节角

	// 求解肩关节角：先计算两连杆夹角中的一个辅助量
	float sinTheta2 = std::sqrt(1.0f - cosTheta2 * cosTheta2);
	// atan2(L2*sin(theta2), L1+L2*cos(theta2)) ：辅助角 phi
	float phi = std::atan2(link2Length * sinTheta2, link1Length + link2Length * cosTheta2);
	// 目标在平面内的方向角 α = atan2(ty, r)
	float alpha = std::atan2(ty, r);
	// 设定肩关节角：调整为“抬起”角度（可以选择“elbow up”还是“elbow down”解）
	jointAngle1 = alpha - phi;
}

// ---------------- 辅助函数：绘制坐标轴 ---------------- //
void drawCoordinateAxes()
{
	glBegin(GL_LINES);
	// X 轴：红色
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(5.0f, 0.0f, 0.0f);
	// Y 轴：绿色
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 5.0f, 0.0f);

	// Z 轴：蓝色
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 5.0f);
	glEnd();
}

// ---------------- 辅助函数：绘制方块 ---------------- //
// size 为方块的边长
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

// ---------------- 绘制机械臂系统 ---------------- //
void drawArmSystem()
{
	// 为了避免多次调用 glLoadMatrixf，可以利用固定管线的堆栈操作
	// 1) 绘制躯干（用一个竖直的方块表示）
	{
		glPushMatrix();
		// 躯干放置于原点（这里人为设置）并拉伸变换
		glm::mat4 torsoMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		torsoMat = glm::scale(torsoMat, glm::vec3(1.0f, 2.0f, 0.5f));
		glLoadMatrixf(glm::value_ptr(viewMat * torsoMat));
		drawCube(1.0f);
		glPopMatrix();
	}
	// 2) 绘制机械臂（以上臂、下臂和末端执行器构成）
	{
		glPushMatrix();
		// 把肩关节设在躯干顶部，假定其位置为 (0, 1, 0)
		glTranslatef(0.0f, 1.0f, 0.0f);
		// 先绕 Y 轴旋转，调整机械臂在水平面面向目标
		glRotatef(glm::degrees(baseAngle), 0.0f, 1.0f, 0.0f);
		// 然后在平面内（X-Y 平面）绕 Z 轴旋转，即为肩关节抬起角度
		glRotatef(glm::degrees(jointAngle1), 0.0f, 0.0f, 1.0f);

		// 绘制上臂：从 (0,0,0) 沿 X 轴方向拉伸
		glPushMatrix();
		glScalef(link1Length, 0.3f, 0.3f);
		drawCube(1.0f);
		glPopMatrix();

		// 绘制下臂
		glPushMatrix();
		// 先平移到上臂末端（沿 X 轴正方向平移 link1Length）
		glTranslatef(link1Length, 0.0f, 0.0f);
		// 肘关节旋转：同样绕 Z 轴旋转
		glRotatef(glm::degrees(jointAngle2), 0.0f, 0.0f, 1.0f);
		// 绘制下臂，拉伸形成连杆
		glScalef(link2Length, 0.2f, 0.2f);
		drawCube(1.0f);
		glPopMatrix();

		// 绘制末端执行器（例如一个小红色方块），位于下臂末端
		glPushMatrix();
		// 平移到下臂末端
		glTranslatef(link1Length + link2Length, 0.0f, 0.0f);
		glScalef(0.2f, 0.2f, 0.2f);
		glColor3f(1.0f, 0.0f, 0.0f);
		drawCube(1.0f);
		glPopMatrix();

		glPopMatrix();
	}
}