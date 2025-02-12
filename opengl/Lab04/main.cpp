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

// 新增全局变量
GLuint skyboxShader;  // 天空盒着色器程序
GLuint skyboxVAO, skyboxVBO;  // 天空盒VAO/VBO
GLuint cubeMapTexture;

// 用于控制不同旋转的角度
float pitchAngle = 0.0f;  // 俯仰
float rollAngle = 0.0f;   // 横滚
float yawAngle = 0.0f;    // 偏航
float propellerAngle = 0.0f;  // 螺旋桨的旋转角度
bool bumpMappingEnabled = false;




// ---------- 新增全局变量 ----------
enum EffectMode { REFLECTION, REFRACTION };
EffectMode currentMode = REFLECTION;  // 当前效果模式
bool chromaticAberration = false;     // 是否开启色散
float FresnelRatio = 0.5f;            // 菲涅耳混合系数（0-1）

// 数据结构
struct ModelData {
    std::vector<float> vertices;   // 顶点数据
    std::vector<float> normals;    // 法线数据
    std::vector<float> texCoords;  // 纹理坐标数据
    GLuint textureID = 0;          // 纹理 ID
    GLuint normalMapTexture = 0;
    glm::vec3 position = { 0.0f, 0.0f, 0.0f }; // 空间位置
    size_t pointCount = 0;         // 顶点数量
    glm::mat4 rotationMatrix = glm::mat4(1.0f); // 旋转矩阵，默认是单位矩阵
};

// 立方体顶点数据（NDC坐标，已移除Z分量）
void initSkybox() {
    // 1. 创建立方体顶点数据
    // 立方体顶点数据（NDC坐标，已移除Z分量）
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };


    // 2. 创建VAO/VBO
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // 3. 加载立方体贴图纹理
    std::vector<std::string> faces = {
        "right.jpg", "left.jpg",
        "top.jpg", "bottom.jpg",
        "front.jpg", "back.jpg"
    };

    glGenTextures(1, &cubeMapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);

    // 加载6个面的图片
    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }

    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}


// 顶点着色器
const char* skyboxVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 view;
uniform mat4 projection;

void main() {
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // 使用最大深度值
}

)";

// 片段着色器
const char* skyboxFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main() {
    FragColor = texture(skybox, TexCoords);
}

)";

GLuint vao[9], vboVertices[9], vboNormals[9], vboTexCoords[9], shaderProgram;
ModelData modelData[9];

// 控制变量
float cameraDistance = 10.0f;  // 视角距离
float cameraAngleX = 0.0f;     // 视角绕 X 轴旋转角度
float cameraAngleY = 0.0f;     // 视角绕 Y 轴旋转角度
float modelRotationY = 0.0f;   // 模型绕 Y 轴旋转角度



ModelData loadHeightmap(const char* heightmapFile, glm::vec3 position, float scaleX, float scaleY, float scaleZ) {
    ModelData data;

    // 载入高度图
    int width, height, nrChannels;
    unsigned char* heightmapData = stbi_load(heightmapFile, &width, &height, &nrChannels, 0);
    if (!heightmapData) {
        std::cerr << "Error loading heightmap: " << heightmapFile << std::endl;
        return data;
    }

    // 生成地形的顶点数据
    for (int z = 0; z < height; z++) {
        for (int x = 0; x < width; x++) {
            // 获取每个像素的灰度值并映射到高度
            float pixelHeight = (float)heightmapData[(z * width + x) * nrChannels] / 255.0f * scaleY;  // 归一化后乘以scaleY

            // 生成顶点数据
            data.vertices.push_back((float)x * scaleX);  // x 坐标
            data.vertices.push_back(pixelHeight);        // y 坐标（高度）
            data.vertices.push_back((float)z * scaleZ);  // z 坐标

            // 对应法线（暂时用单位法线，稍后可能需要计算）
            data.normals.push_back(0.0f);
            data.normals.push_back(1.0f);
            data.normals.push_back(0.0f);

            // 默认的纹理坐标（这里可以根据需要进行调整）
            data.texCoords.push_back((float)x / width);
            data.texCoords.push_back((float)z / height);
        }
    }

    // 计算顶点数量
    data.pointCount = data.vertices.size() / 3;

    // 设置地形的位置
    data.position = position;

    // 释放高度图数据
    stbi_image_free(heightmapData);

    std::cout << "Heightmap loaded: " << heightmapFile << std::endl;
    std::cout << "Terrain size: " << width << "x" << height << std::endl;
    std::cout << "Number of vertices: " << data.pointCount << std::endl;

    return data;
}

// 加载模型函数
// 加载模型函数
// 加载模型函数
ModelData loadModel(const char* fileName, const char* textureFile = nullptr, const char* normalMapFile = nullptr, glm::vec3 position = { 0.0f, 0.0f, 0.0f }, float rotateX = 0.0f, float rotateY = 0.0f, float rotateZ = 0.0f) {
    ModelData data;
    data.position = position;
    const aiScene* scene = aiImportFile(fileName, aiProcess_Triangulate | aiProcess_GenNormals);
    if (!scene) {
        std::cerr << "Error loading model: " << fileName << std::endl;
        return data;
    }

    const aiMesh* mesh = scene->mMeshes[0];
    data.pointCount = mesh->mNumVertices;

    std::cout << "Model: " << fileName << " - Number of vertices: " << data.pointCount << std::endl;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        const aiVector3D* pos = &mesh->mVertices[i];
        const aiVector3D* norm = &mesh->mNormals[i];
        const aiVector3D* texCoord = mesh->mTextureCoords[0] ? &mesh->mTextureCoords[0][i] : nullptr;

        data.vertices.push_back(pos->x);
        data.vertices.push_back(pos->y);
        data.vertices.push_back(pos->z);
        data.normals.push_back(norm->x);
        data.normals.push_back(norm->y);
        data.normals.push_back(norm->z);
        if (texCoord) {
            data.texCoords.push_back(texCoord->x);
            data.texCoords.push_back(texCoord->y);
        }
        else {
            data.texCoords.push_back(0.0f); // 默认纹理坐标
            data.texCoords.push_back(0.0f);
        }
    }

    // 加载漫反射纹理
    if (textureFile) {
        glGenTextures(1, &data.textureID);
        glBindTexture(GL_TEXTURE_2D, data.textureID);

        int textureWidth, textureHeight, nrChannels;
        unsigned char* textureData = stbi_load(textureFile, &textureWidth, &textureHeight, &nrChannels, 0);
        if (textureData) {
            // 上传纹理数据到 GPU
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);

            // **生成 MIP Maps**
            glGenerateMipmap(GL_TEXTURE_2D);

            // 设置纹理参数（包括 MIP Mapping）
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // 启用 MIP Mapping
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
            stbi_image_free(textureData);
        }
        else {
            std::cerr << "Failed to load texture: " << textureFile << std::endl;
            stbi_image_free(textureData);
        }
    }

    // 加载法线贴图（如果提供了）
    if (normalMapFile) {
        glGenTextures(1, &data.normalMapTexture);
        glBindTexture(GL_TEXTURE_2D, data.normalMapTexture);

        int normalMapWidth, normalMapHeight, nrChannels;
        unsigned char* normalMapData = stbi_load(normalMapFile, &normalMapWidth, &normalMapHeight, &nrChannels, 0);
        if (normalMapData) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, normalMapWidth, normalMapHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, normalMapData);
            glGenerateMipmap(GL_TEXTURE_2D); // **生成 MIP Maps**
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // 启用 MIP Mapping
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            stbi_image_free(normalMapData);
        }
        else {
            std::cerr << "Failed to load normal map: " << normalMapFile << std::endl;
            stbi_image_free(normalMapData);
        }
    }

    aiReleaseImport(scene);

    glm::mat4 rotation = glm::mat4(1.0f);
    rotation = glm::rotate(rotation, glm::radians(rotateX), glm::vec3(1.0f, 0.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(rotateY), glm::vec3(0.0f, 1.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(rotateZ), glm::vec3(0.0f, 0.0f, 1.0f));
    data.rotationMatrix = rotation;

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
// 修改片段着色器
const char* fragmentShaderSource = R"(
#version 330 core
in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexcoord;

uniform sampler2D textureSampler;     // 漫反射纹理
uniform sampler2D normalMap;          // 法线贴图
uniform vec3 viewPosition;
uniform int useTexture;
uniform vec3 defaultColor;
uniform bool chromaticAberration;
uniform float FresnelRatio;
uniform int currentMode; // 0: 反射, 1: 折射

out vec4 fragColor;

void main() {
    // 初始的法线，若不使用法线贴图，则使用原始的法线
    vec3 normal = normalize(fragNormal);

    // 如果启用了法线贴图，则从法线贴图中采样并计算新的法线
    if (useTexture == 1) {
        // 从法线贴图中采样
        vec3 sampledNormal = texture(normalMap, fragTexcoord).rgb;
        
        // 将法线贴图的颜色值从 [0, 1] 转换到 [-1, 1]
        sampledNormal = normalize(sampledNormal * 2.0 - 1.0);
        
        // 使用法线贴图中的法线
        normal = sampledNormal;
    }

    // 计算视线方向和光源方向
    vec3 viewDir = normalize(viewPosition - fragPosition);
    vec3 lightDir = normalize(vec3(0.0, 1.0, 1.0));

    // 漫反射
    float diff = max(dot(normal, lightDir), 0.0);
    
    // 高光反射
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    vec3 specular = vec3(0.5) * spec; // 50% 的高光
    
    // 环境光
    vec3 ambient = vec3(0.2, 0.2, 0.2); // 20% 的环境光
    
    // 读取漫反射材质颜色
    vec3 textureColor = useTexture == 1 ? texture(textureSampler, fragTexcoord).rgb : defaultColor;
    
    // 反射与折射计算
    vec3 reflectDir = reflect(-viewDir, normal);
    vec3 refractDirR = refract(-viewDir, normal, 1.0 / 1.1);
    vec3 refractDirG = refract(-viewDir, normal, 1.0 / 1.2);
    vec3 refractDirB = refract(-viewDir, normal, 1.0 / 1.3);
    
    float fresnel = pow(1.0 - dot(viewDir, normal), 5.0) * (1.0 - FresnelRatio) + FresnelRatio;
    
    vec3 finalColor;
    if (currentMode == 0) { // 反射
        finalColor = reflectDir * fresnel;
    } else { // 折射
        if (chromaticAberration) {
            finalColor = vec3(refractDirR.r, refractDirG.g, refractDirB.b);
        } else {
            finalColor = mix(textureColor, vec3(refractDirG), 0.5);
        }
    }
    
    // 组合光照（漫反射、环境光、高光）
    finalColor = finalColor * diff + specular + ambient;
    
    // 结合材质颜色与光照效果
    fragColor = vec4(textureColor * 0.7 + finalColor * 0.3, 1.0);
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
    glGenBuffers(9, vboTexCoords); // 为纹理坐标生成缓冲区

    for (int i = 0; i < 9; i++) {
        glBindVertexArray(vao[i]);

        // 顶点位置
        glBindBuffer(GL_ARRAY_BUFFER, vboVertices[i]);
        glBufferData(GL_ARRAY_BUFFER, modelData[i].vertices.size() * sizeof(float), modelData[i].vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);

        // 法线
        glBindBuffer(GL_ARRAY_BUFFER, vboNormals[i]);
        glBufferData(GL_ARRAY_BUFFER, modelData[i].normals.size() * sizeof(float), modelData[i].normals.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(1);

        // 纹理坐标
        glBindBuffer(GL_ARRAY_BUFFER, vboTexCoords[i]); // 绑定纹理坐标缓冲区
        glBufferData(GL_ARRAY_BUFFER, modelData[i].texCoords.size() * sizeof(float), modelData[i].texCoords.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr); // 注意：纹理坐标是 2D
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }
}


// 键盘控制
void keypress(unsigned char key, int x, int y) {
    switch (key) {
    case 'W': cameraDistance -= 2.5f; break; // 拉近视角
    case 'S': cameraDistance += 2.5f; break; // 拉远视角
    case 'A': cameraAngleY -= 0.05f; break;  // 左旋视角
    case 'D': cameraAngleY += 0.05f; break;  // 右旋视角
    case 'i': cameraAngleX += 0.05f; break;  // 上旋视角
    case 'k': cameraAngleX -= 0.05f; break;  // 下旋视角
    case 'Q': modelRotationY -= 0.1f; break; // 模型左旋
    case 'E': modelRotationY += 0.1f; break; // 模型右旋
    case 'R':
        currentMode = (currentMode == REFLECTION) ? REFRACTION : REFLECTION;
        std::cout << "Mode switched to: " << (currentMode == REFLECTION ? "Reflection" : "Refraction") << std::endl;
        break;
    case 'C':
        chromaticAberration = !chromaticAberration;
        std::cout << "Chromatic Aberration: " << (chromaticAberration ? "Enabled" : "Disabled") << std::endl;
        break;
    case 'F':
        FresnelRatio = glm::clamp(FresnelRatio + 0.4f, 0.0f, 4.0f);
        std::cout << "Fresnel Ratio increased: " << FresnelRatio << std::endl;
        break;
    case 'V':
        FresnelRatio = glm::clamp(FresnelRatio - 0.4f, 0.0f, 4.0f);
        std::cout << "Fresnel Ratio decreased: " << FresnelRatio << std::endl;
        break;

        case 'w': pitchAngle += 5.0f; break;  // 俯仰向上
        case 's': pitchAngle -= 5.0f; break;  // 俯仰向下
        case 'a': rollAngle += 5.0f; break;   // 横滚向左
        case 'd': rollAngle -= 5.0f; break;   // 横滚向右
        case 'q': yawAngle += 5.0f; break;    // 偏航向左
        case 'e': yawAngle -= 5.0f; break;    // 偏航向右

        case 'b':
            bumpMappingEnabled = !bumpMappingEnabled;
            std::cout << "Bump Mapping " << (bumpMappingEnabled ? "Enabled" : "Disabled") << std::endl;
            break;

    }

    // 限制 cameraAngleX 的值在 -89 到 89 度之间
    if (cameraAngleX > glm::radians(89.0f)) cameraAngleX = glm::radians(89.0f);

    glutPostRedisplay();
}


// 渲染函数
void display() {
    // 清除颜色缓冲区和深度缓冲区
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(shaderProgram);

    GLuint bumpMappingLoc = glGetUniformLocation(shaderProgram, "bumpMappingEnabled");
    glUniform1i(bumpMappingLoc, bumpMappingEnabled);

    propellerAngle += 1.0f;

    // 获取 Shader 中的 Uniform 变量位置
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPosition");
    GLuint useTextureLoc = glGetUniformLocation(shaderProgram, "useTexture");
    GLuint defaultColorLoc = glGetUniformLocation(shaderProgram, "defaultColor");
    GLuint textureSamplerLoc = glGetUniformLocation(shaderProgram, "textureSampler");

    // 设置视图矩阵和投影矩阵
    glm::mat4 view = getViewMatrix();
    glm::mat4 projection = getProjectionMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(viewPosLoc, 0.0f, 0.0f, cameraDistance);

    // 遍历所有模型并渲染
    for (int i = 0; i < 9; i++) {
        // 如果启用 bump mapping，则绑定法线贴图
        if (bumpMappingEnabled) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, modelData[i].normalMapTexture);
            glUniform1i(glGetUniformLocation(shaderProgram, "normalMap"), 1); // 将法线贴图绑定到纹理单元 1
        }

        // 设置模型矩阵
        glm::mat4 model = glm::translate(glm::mat4(1.0f), modelData[i].position); // 设置模型位置
        model = model * modelData[i].rotationMatrix; // 应用模型的旋转矩阵
        model = glm::rotate(model, modelRotationY, glm::vec3(0.0f, 1.0f, 0.0f)); // 应用 Y 轴旋转

        // 对螺旋桨模型应用旋转（假设螺旋桨是 modelData[0]）
        if (i == 0) {  // 假设螺旋桨在 modelData[0]
            glm::mat4 propellerRotation = glm::rotate(glm::mat4(1.0f), glm::radians(propellerAngle), glm::vec3(0.0f, 1.0f, 0.0f));
            model = model * propellerRotation;  // 将旋转矩阵应用到螺旋桨模型
        }

        // 俯仰、横滚和偏航旋转
        model = glm::rotate(model, glm::radians(pitchAngle), glm::vec3(1.0f, 0.0f, 0.0f));  // 俯仰旋转
        model = glm::rotate(model, glm::radians(rollAngle), glm::vec3(0.0f, 0.0f, 1.0f));   // 横滚旋转
        model = glm::rotate(model, glm::radians(yawAngle), glm::vec3(0.0f, 1.0f, 0.0f));    // 偏航旋转

        // 传递给着色器
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        // 判断是否有纹理
        if (modelData[i].textureID) {
            glUniform1i(useTextureLoc, 1); // 通知 Shader 使用纹理
            glActiveTexture(GL_TEXTURE0);  // 激活纹理单元 0
            glBindTexture(GL_TEXTURE_2D, modelData[i].textureID); // 绑定纹理
            glUniform1i(textureSamplerLoc, 0); // 将纹理采样器绑定到纹理单元 0
        }
        else {
            glUniform1i(useTextureLoc, 0); // 通知 Shader 不使用纹理
            glUniform3f(defaultColorLoc, 0.8f, 0.8f, 0.8f); // 设置默认颜色为灰色
        }

        // 绑定 VAO 并绘制
        glBindVertexArray(vao[i]);
        glDrawArrays(GL_TRIANGLES, 0, modelData[i].pointCount);
    }

    // 渲染天空盒
    glDepthFunc(GL_LEQUAL);  // 修改深度测试比较方式
    glUseProgram(skyboxShader);

    // 创建无位移的视图矩阵（关键修复点）
    glm::mat4 skyboxView = glm::mat4(glm::mat3(view)); // 移除位移分量

    // 获取uniform位置（建议提前缓存这些位置）
    GLint skyboxViewLoc = glGetUniformLocation(skyboxShader, "view");
    GLint skyboxProjLoc = glGetUniformLocation(skyboxShader, "projection");

    // 绑定立方体贴图
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);

    // 传递矩阵（使用处理后的视图矩阵）
    glUniformMatrix4fv(skyboxViewLoc, 1, GL_FALSE, glm::value_ptr(skyboxView));
    glUniformMatrix4fv(skyboxProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // 渲染天空盒
    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    // 恢复深度设置
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE); // 如果前面修改过需要恢复

    // 解绑 VAO
    glBindVertexArray(0);

    // 交换缓冲区
    glutSwapBuffers();
}






// 初始化 OpenGL
void initOpenGL() {
    glewInit();
    glEnable(GL_DEPTH_TEST);
    initShaders();
    initSkybox();

    // 创建天空盒着色器程序（替换原有错误代码）
    skyboxShader = glCreateProgram();
    GLuint vs = compileShader(GL_VERTEX_SHADER, skyboxVertexShader);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, skyboxFragmentShader);
    glAttachShader(skyboxShader, vs);
    glAttachShader(skyboxShader, fs);
    glLinkProgram(skyboxShader);

    // 检查链接错误
    GLint success;
    glGetProgramiv(skyboxShader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(skyboxShader, 512, NULL, infoLog);
        std::cerr << "SKYBOX SHADER LINK ERROR: " << infoLog << std::endl;
    }

    // 清理着色器对象
    glDeleteShader(vs);
    glDeleteShader(fs);



    // 加载模型及其纹理
    //modelData[0] = loadModel("luoxuanjiang3.dae", "diffuse.jpg", nullptr, { 0.5f, -3.2f, 10.0f }, 180, 180, -90);
    //modelData[1] = loadModel("plane2.obj", "plane3.jpg", "metal_normal.jpg", {0.0f, 2.5f, 0.0f}, 180, 180, 0);
    modelData[2] = loadModel("pink_cube.dae", "diffuse.jpg", nullptr, { 0.0f, 0.0f, 0.0f }, 0, 0, 0);
    modelData[3] = loadModel("pink_cube.dae", "diffuse.jpg", nullptr, { 5.0f, 0.0f, 0.0f }, 0, 0, 0);

     // 加载地形数据
    //modelData[8] = loadHeightmap("hmap.png", { 0.0f, 0.0f, 0.0f }, 180.0f, 10.0f, 1.0f);  // Scale Y 用于控制高度

    initBuffers();
}



int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Lab - plane");
    initOpenGL();
    glutDisplayFunc(display);
    glutIdleFunc(display);
    glutKeyboardFunc(keypress);
    glutMainLoop();
    return 0;
}