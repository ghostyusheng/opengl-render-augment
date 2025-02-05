#include <GL/glew.h>
#include <GL/freeglut.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 数据结构
struct ModelData {
    std::vector<float> vertices;
    std::vector<float> normals;
    GLuint textureID = 0;       // 纹理 ID
    glm::vec3 position = { 0.0f, 0.0f, 0.0f }; // 空间位置
    size_t pointCount = 0;
};

GLuint vao[9], vboVertices[9], vboNormals[9], shaderProgram, cubeMapTexture;
ModelData modelData[9];

// 控制变量
float cameraDistance = 10.0f;  // 视角距离
float cameraAngleX = 0.0f;     // 视角绕 X 轴旋转角度
float cameraAngleY = 0.0f;     // 视角绕 Y 轴旋转角度
float modelRotationY = 0.0f;   // 模型绕 Y 轴旋转角度


// 加载模型函数
ModelData loadModel(const char* fileName, const char* textureFile = nullptr, glm::vec3 position = { 0.0f, 0.0f, 0.0f }) {
    ModelData data;
    const aiScene* scene = aiImportFile(fileName, aiProcess_Triangulate | aiProcess_GenNormals);
    if (!scene) {
        std::cerr << "Error loading model: " << fileName << std::endl;
        return data;
    }

    const aiMesh* mesh = scene->mMeshes[0];
    data.pointCount = mesh->mNumVertices;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        const aiVector3D* pos = &mesh->mVertices[i];
        const aiVector3D* norm = &mesh->mNormals[i];
        data.vertices.push_back(pos->x);
        data.vertices.push_back(pos->y);
        data.vertices.push_back(pos->z);
        data.normals.push_back(norm->x);
        data.normals.push_back(norm->y);
        data.normals.push_back(norm->z);
    }

    // 加载纹理（如果提供）
    if (textureFile) {
        glGenTextures(1, &data.textureID);
        glBindTexture(GL_TEXTURE_2D, data.textureID);

        int width, height, nrChannels;
        unsigned char* dataTexture = stbi_load(textureFile, &width, &height, &nrChannels, 0);
        if (dataTexture) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, dataTexture);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(dataTexture);
            std::cout << "Success to load texture: " << textureFile << std::endl;
        }
        else {
            std::cerr << "Failed to load texture: " << textureFile << std::endl;
            stbi_image_free(dataTexture);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    // 设置模型空间位置
    data.position = position;

    aiReleaseImport(scene);
    return data;
}



// 顶点着色器源码
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in vec2 vertex_texcoord; // 添加纹理坐标输入

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexcoord; // 传递纹理坐标

void main() {
    fragPosition = vec3(model * vec4(vertex_position, 1.0));
    fragNormal = mat3(transpose(inverse(model))) * vertex_normal;
    fragTexcoord = vertex_texcoord; // 传递纹理坐标
    gl_Position = projection * view * vec4(fragPosition, 1.0);
}

)";

// 片段着色器源码
const char* fragmentShaderSource = R"(
#version 330 core
in vec3 fragPosition;
in vec3 fragNormal;

uniform sampler2D textureSampler; // 2D 纹理采样器
uniform vec3 viewPosition;

out vec4 fragColor;

void main() {
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(viewPosition - fragPosition);

    // 简单的漫反射光照
    vec3 lightDir = normalize(vec3(0.0, 1.0, 1.0)); // 光源方向
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 textureColor = texture(textureSampler, fragPosition.xy).rgb; // 使用 2D 纹理
    vec3 finalColor = diff * textureColor;

    fragColor = vec4(finalColor, 1.0);
}

)";

// 编译单个着色器
GLuint compileShader(GLenum shaderType, const char* shaderSource) {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSource, nullptr);
    glCompileShader(shader);

    // 检查编译错误
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compilation error: " << log << std::endl;
    }

    return shader;
}

// 初始化着色器程序
void initShaders() {
    // 编译顶点着色器
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);

    // 编译片段着色器
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    // 链接着色器程序
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // 检查链接错误
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, log);
        std::cerr << "Shader program linking error: " << log << std::endl;
    }

    // 删除着色器对象
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

// 设置视图矩阵和投影矩阵
glm::mat4 getViewMatrix() {
    // 根据球面坐标计算相机位置
    float cameraX = cameraDistance * cos(cameraAngleX) * sin(cameraAngleY);
    float cameraY = cameraDistance * sin(cameraAngleX);
    float cameraZ = cameraDistance * cos(cameraAngleX) * cos(cameraAngleY);

    glm::vec3 cameraPos = glm::vec3(cameraX, cameraY, cameraZ);
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f); // 目标点
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);     // 世界坐标系的上方向

    return glm::lookAt(cameraPos, target, up);
}


glm::mat4 getProjectionMatrix() {
    return glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
}

// 初始化缓冲区
void initBuffers() {
    glGenVertexArrays(9, vao);
    glGenBuffers(9, vboVertices);
    glGenBuffers(9, vboNormals);

    for (int i = 0; i < 9; i++) {
        glBindVertexArray(vao[i]);

        glBindBuffer(GL_ARRAY_BUFFER, vboVertices[i]);
        glBufferData(GL_ARRAY_BUFFER, modelData[i].vertices.size() * sizeof(float), modelData[i].vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, vboNormals[i]);
        glBufferData(GL_ARRAY_BUFFER, modelData[i].normals.size() * sizeof(float), modelData[i].normals.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }
}

// 键盘控制
void keypress(unsigned char key, int x, int y) {
    switch (key) {
    case 'w': cameraDistance -= 2.5f; break; // 拉近视角
    case 's': cameraDistance += 2.5f; break; // 拉远视角
    case 'a': cameraAngleY -= 0.05f; break;  // 左旋视角
    case 'd': cameraAngleY += 0.05f; break;  // 右旋视角
    case 'i': cameraAngleX += 0.05f; break;  // 上旋视角
    case 'k': cameraAngleX -= 0.05f; break;  // 下旋视角
    case 'q': modelRotationY -= 0.1f; break; // 模型左旋
    case 'e': modelRotationY += 0.1f; break; // 模型右旋
    }

    // 限制 cameraAngleX 的值在 -89 到 89 度之间
    if (cameraAngleX > glm::radians(89.0f)) cameraAngleX = glm::radians(89.0f);
    if (cameraAngleX < glm::radians(-89.0f)) cameraAngleX = glm::radians(-89.0f);

    glutPostRedisplay();
}


// 渲染函数
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);

    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPosition");

    glm::mat4 view = getViewMatrix();
    glm::mat4 projection = getProjectionMatrix();

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(viewPosLoc, 0.0f, 0.0f, cameraDistance);

    for (int i = 0; i < 9; i++) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), modelData[i].position); // 使用模型位置
        model = glm::rotate(model, modelRotationY, glm::vec3(0.0f, 1.0f, 0.0f)); // Y 轴旋转

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        // 绑定该模型的纹理（如果存在）
        if (modelData[i].textureID) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, modelData[i].textureID);
        }

        glBindVertexArray(vao[i]);
        glDrawArrays(GL_TRIANGLES, 0, modelData[i].pointCount);
    }

    glBindVertexArray(0);
    glutSwapBuffers();
}



// 初始化 OpenGL
void initOpenGL() {
    glewInit();
    glEnable(GL_DEPTH_TEST);
    initShaders();

    // 加载模型及其纹理
    modelData[0] = loadModel("monkey.dae", "diffuse.jpg", { 0.0f, 0.0f, 0.0f });
    modelData[1] = loadModel("cube.dae", "asset/facade0.jpg", { 2.0f, 0.0f, 0.0f });
    // 加载其他模型...

    initBuffers();
}



int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Lab 1 - Phong/Toon/Cook-Torrance lighting model");
    initOpenGL();
    glutDisplayFunc(display);
    glutKeyboardFunc(keypress);
    glutMainLoop();
    return 0;
}