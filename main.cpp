#include "Libraries/vectors.h"
#include "Libraries/point.h"
#include "glad/include/glad/glad.h"
#include <GLFW/glfw3.h>
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <vector>
#include <array>
#include <random>
#include <utility>
#include <cstdlib>
#include <ctime>
#include <limits>

// Janela 800x800
const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 800;

// Limites do Plano Cartesiano 2D
float xMin = -100.0f;
float xMax = 100.0f;
float yMin = -100.0f;
float yMax = 100.0f;

//Variáveis Globais
std::vector<ponto2D> cloud;
std::vector<ponto2D> aabb;
std::vector<ponto2D> mouseInput;

// Gera 10 pontos aleatórios na nuvem
void randomPoints(){
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distrib_x(xMin, xMax);
    std::uniform_real_distribution<> distrib_y(yMin, yMax);
    
    for(int i = 0; i < 10; ++i){
        double x = distrib_x(gen);
        double y = distrib_y(gen);
        cloud.emplace_back(ponto2D(x, y));    
    }

}

void calculateAABB(){
    
    double min_x = std::numeric_limits<double>::infinity();
    double min_y = std::numeric_limits<double>::infinity();

    double max_x = -std::numeric_limits<double>::infinity();
    double max_y = -std::numeric_limits<double>::infinity();

    for(const auto& p : cloud){
        min_x = std::min(min_x, p.x);
        min_y = std::min(min_y, p.y);

        max_x = std::max(max_x, p.x);
        max_y = std::max(max_y, p.y);
    }

    aabb.emplace_back(min_x, min_y); // Inferior Esquerdo
    aabb.emplace_back(max_x, min_y); // Inferior Direito

    aabb.emplace_back(min_x, max_y); // Superior Esquerdo
    aabb.emplace_back(max_x, max_y); // Superior Direito
}

std::vector<bool> checkBelongsToAABB(){
    std::vector<bool> res;

    double min_x = aabb[0].x;
    double min_y = aabb[0].y;

    double max_x = aabb[3].x;
    double max_y = aabb[3].y;

    for(const auto& p : mouseInput){
        if(p.x <= max_x && p.x >= min_x && p.y <= max_y && p.y >= min_y){
            res.push_back(true);
        }
        else{
            res.push_back(false);
        }
    }

    return res;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        // Mouse --> Coordenadas de Mundo.
        double x = static_cast<double>((xpos / WIDTH) * (xMax - xMin) + xMin);
        double y = static_cast<double>(((HEIGHT - ypos) / HEIGHT) * (yMax - yMin) + yMin);
        mouseInput.emplace_back(ponto2D{x, y});
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        randomPoints();
    }
    if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        cloud.clear();
        aabb.clear();
        mouseInput.clear();
    }
    if (key == GLFW_KEY_A && action == GLFW_PRESS) {
        calculateAABB();
    }
}

unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Erro ao compilar shader: " << infoLog << std::endl;
    }

    return shader;
}

unsigned int setupCartesianPlane(float xMin, float xMax, float yMin, float yMax) {
    std::array<float, 12> planeVertices = {
        xMin, 0.0f, 0.0f,   xMax, 0.0f, 0.0f,  // Eixo X
        0.0f, yMin, 0.0f,   0.0f, yMax, 0.0f   // Eixo Y
    };

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return vao;
}

void drawPoint(const ponto2D& p, unsigned int shaderProgram, glm::mat4 projection, float red, float green, float blue){
    float pointVertices[] = {
        p.x, p.y, 0.0f
    };
    
    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pointVertices), pointVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(glGetUniformLocation(shaderProgram, "color"), red, green, blue);
    
    glPointSize(7.0f);
    glDrawArrays(GL_POINTS, 0, 1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void drawSegment(const ponto2D& p1, const ponto2D& p2, unsigned int shaderProgram, glm::mat4 projection, float red, float green, float blue) {
    float lineVertices[] = {
        p1.x, p1.y, 0.0f,
        p2.x, p2.y, 0.0f
    };
    
    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(glGetUniformLocation(shaderProgram, "color"), red, green, blue);
    
    glDrawArrays(GL_LINES, 0, 2);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void drawRectangle(unsigned int shaderProgram, glm::mat4 projection){
    // Permite apenas exatamente 1 Bounding Box
    if(aabb.size() != 4){
        std::cout << "Problema no DrawRectangle" << std::endl;
        return;
    }

    // Aresta Esquerda
    drawSegment(aabb[0], aabb[2], shaderProgram, projection, 0.0f, 0.0f, 1.0f);
    // Aresta Direita
    drawSegment(aabb[1], aabb[3], shaderProgram, projection, 0.0f, 0.0f, 1.0f);
    // Aresta Superior
    drawSegment(aabb[2], aabb[3], shaderProgram, projection, 0.0f, 0.0f, 1.0f);
    // Aresta Inferior
    drawSegment(aabb[0], aabb[1], shaderProgram, projection, 0.0f, 0.0f, 1.0f);
}

int main(){
    if (!glfwInit()) {
        std::cerr << "Erro ao inicializar GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Bounding Volume", nullptr, nullptr);
    if (!window) {
        std::cerr << "Erro ao criar janela GLFW." << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Erro ao carregar GLAD." << std::endl;
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);

    const char* vertexShaderSource = R"(
        #version 430 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * vec4(aPos, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 430 core
        out vec4 FragColor;
        uniform vec3 color;
        void main() {
            FragColor = vec4(color, 1.0);
        }
    )";
        
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Erro ao vincular shaders: " << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    unsigned int cartesianVAO = setupCartesianPlane(xMin, xMax, yMin, yMax);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::ortho(xMin, xMax, yMin, yMax);

        glBindVertexArray(cartesianVAO);
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.0f, 1.0f, 0.0f);
        glDrawArrays(GL_LINES, 0, 4);

        if(!cloud.empty()){
            for(const auto &p : cloud){
                drawPoint(p, shaderProgram, projection, 1.0f, 0.0f, 0.0f);
            }
        }

        if(!aabb.empty()){
            drawRectangle(shaderProgram, projection);
        }

        if(!mouseInput.empty()){
            std::vector<bool> b = checkBelongsToAABB();
            for(int i = 0; i < mouseInput.size(); ++i){
                if(b[i]){
                    drawPoint(mouseInput[i], shaderProgram, projection, 0.0f, 1.0f, 0.0f);
                }else{
                    drawPoint(mouseInput[i], shaderProgram, projection, 1.0f, 0.0f, 0.0f);
                }
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}