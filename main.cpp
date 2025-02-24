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

struct rgb
{
    float red;
    float green;
    float blue;
};

//Variáveis Globais
std::vector<std::vector<ponto2D>> cloud;
std::vector<std::vector<ponto2D>> aabb;
std::vector<ponto2D> mouseInput;
std::vector<rgb> colors;
std::vector<std::pair<ponto2D, double>> circles;

// Gera um RGB aleatório
rgb randomRGB(){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distrib(0.0f, 1.0f);

    rgb color;
    color.red = distrib(gen);
    color.green = distrib(gen);
    color.blue = distrib(gen);

    return color;
}

// Gera 5 pontos aleatórios na nuvem
void randomPoints(){
    
    std::vector<ponto2D> tmp;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distrib_x(xMin, xMax);
    std::uniform_real_distribution<> distrib_y(yMin, yMax);
    
    for(int i = 0; i < 5; ++i){
        double x = distrib_x(gen);
        double y = distrib_y(gen);
        tmp.emplace_back(ponto2D(x, y));    
    }

    cloud.push_back(tmp);
    colors.push_back(randomRGB()); // Cor i é equivalente à cor do subconjunto i de pontos na nuvem.
}

void calculateAABB(){
    aabb.clear();
    for(const auto& sub : cloud){
        std::vector<ponto2D> tmp;
    
        double min_x = std::numeric_limits<double>::infinity();
        double min_y = std::numeric_limits<double>::infinity();

        double max_x = -std::numeric_limits<double>::infinity();
        double max_y = -std::numeric_limits<double>::infinity();

        for(const auto& p : sub){
            min_x = std::min(min_x, p.x);
            min_y = std::min(min_y, p.y);
    
            max_x = std::max(max_x, p.x);
            max_y = std::max(max_y, p.y);
        }

        tmp.emplace_back(min_x, min_y); // Inferior Esquerdo
        tmp.emplace_back(max_x, min_y); // Inferior Direito

        tmp.emplace_back(min_x, max_y); // Superior Esquerdo
        tmp.emplace_back(max_x, max_y); // Superior Direito

        aabb.push_back(tmp);
    }
}

bool checkBelongsToAABB(const ponto2D& p){

    for(const auto& sub : aabb){
        
        double min_x = sub[0].x;
        double min_y = sub[0].y;

        double max_x = sub[3].x;
        double max_y = sub[3].y;

        if(p.x <= max_x && p.x >= min_x && p.y <= max_y && p.y >= min_y){
            return true;
        }
    }

    return false;
}

ponto2D calculateCentroid(const std::vector<ponto2D>& sub){
    double sum_x = 0.0;
    double sum_y = 0.0;
    for(const auto& p: sub){
        sum_x += p.x;
        sum_y += p.y;
    }

    return ponto2D{(sum_x/sub.size()), (sum_y/sub.size())};
}

void calculateCircle(){
    circles.clear();
    for(const auto& sub : cloud){
        ponto2D centroid = calculateCentroid(sub);

        double raio = -std::numeric_limits<double>::infinity();
        for(const auto& p : sub){
            double distancia = centroid.distance(p);
            if(distancia >= raio){
                raio = distancia;
            }
        }
        circles.emplace_back(std::make_pair(centroid, raio));
    }
}

// Função para gerar os vértices de um círculo --> Circulo é um Poligono com Infinitos Lados
std::vector<float> generateCircleVertices(const ponto2D& center, double raio, int numSegments) {
    std::vector<float> vertices;
    for (int i = 0; i <= numSegments; ++i) {
        float angle = 2.0f * M_PI * float(i) / float(numSegments);
        float x = center.x + raio * cos(angle);
        float y = center.y + raio * sin(angle);
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(0.0f);
    }
    return vertices;
}

void drawCircle(const ponto2D& center, const double& raio, unsigned int shaderProgram, glm::mat4 projection, float red, float green, float blue){
    
    int numSegments = 100;
    std::vector<float> vertices = generateCircleVertices(center, raio, numSegments);

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(glGetUniformLocation(shaderProgram, "color"), red, green, blue);

    glBindVertexArray(VAO);
    glDrawArrays(GL_LINE_LOOP, 0, numSegments + 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
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
        circles.clear();
    }
    if (key == GLFW_KEY_A && action == GLFW_PRESS) {
        calculateAABB();
    }
    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        calculateCircle();
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
    
    if((aabb.empty()) || (aabb[0].size() < 4)){
        std::cout << "Impossivel desenhar 1 Retangulo : Menos que 4 pontos" << std::endl;
        return;
    }

    for(int i = 0; i < aabb.size(); ++i){
        
        auto sub = aabb[i];

        float r = colors[i].red;
        float g = colors[i].green;
        float b = colors[i].blue;

        
        if(sub.size() != 4){
            std::cout << "Sub com menos de 4 Pontos" << std::endl;    
            continue;
        }

        // Aresta Esquerda
        drawSegment(sub[0], sub[2], shaderProgram, projection, r, g, b);
        // Aresta Direita
        drawSegment(sub[1], sub[3], shaderProgram, projection, r, g, b);
        // Aresta Superior
        drawSegment(sub[2], sub[3], shaderProgram, projection, r, g, b);
        // Aresta Inferior
        drawSegment(sub[0], sub[1], shaderProgram, projection, r, g, b);
    }
}

ponto2D FindTheIntersectPoint(const ponto2D&a, const ponto2D&b, const ponto2D&c, const ponto2D&d){
    
    double det = (b.x - a.x) * (d.y - c.y) - (b.y - a.y) * (d.x - c.x);

    double t = ((c.x - a.x) * (d.y - c.y) - (c.y - a.y) * (d.x - c.x)) / det;
    double u = ((c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x)) / det;

    if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
        ponto2D intersectionPoint = { a.x + t * (b.x - a.x), a.y + t * (b.y - a.y) };
        // std::cout << "Ponto de interseção: (" << intersectionPoint.x << ", " << intersectionPoint.y << ")\n";
        return intersectionPoint; 
    }

    std::cout << "Problema no Ponto de interseção" << std::endl;
    return ponto2D{0.0, 0.0};
}

bool checkIntersectSegments(const ponto2D&a, const ponto2D&b, const ponto2D&c, const ponto2D&d){

    // A função crossProduct(p1, p2, p3) calcula o determinante 
    // que indica a posição relativa de p3 em relação ao segmento p1p2.
    auto crossProduct = [](const ponto2D& p1, const ponto2D& p2, const ponto2D& p3) {
        return (p2.x - p1.x) * (p3.y - p1.y) - (p2.y - p1.y) * (p3.x - p1.x);
    };

    double d1 = crossProduct(a, b, c);
    double d2 = crossProduct(a, b, d);
    double d3 = crossProduct(c, d, a);
    double d4 = crossProduct(c, d, b);

    // Se os produtos cruzados tiverem sinais opostos, os segmentos se cruzam
    if ((d1 * d2 < 0) && (d3 * d4 < 0)) {
        return true;
    }

    return false;
}

std::vector<ponto2D> checkIntersectBetweenAABBs(){
    
    std::vector<ponto2D> res;
    
    int i = 0;
    int j = 0;
    for(const auto& sub : aabb){
        // Para cada Sub --> 4 Arestas
        // Para cada aresta eu preciso comparar com as outras 4 arestas de cada outra sub de aabb

        ponto2D inferior_esquerdo = sub[0];
        ponto2D inferior_direito = sub[1];

        ponto2D superior_esquerdo = sub[2];
        ponto2D superior_direito = sub[3];

        /*
            // Aresta Esquerda
            sub[0], sub[2]
            
            // Aresta Direita
            sub[1], sub[3]
            
            // Aresta Superior
            sub[2], sub[3]

            // Aresta Inferior
            sub[0], sub[1]
        */

        for(const auto& element : aabb){
            if(i == j){
                // Não podemos comparar dois subs iguais
                // std::cout << "Debug " << i << " | " << j << std::endl;
                continue;
            }

            // Aresta Esquerda - Aresta Esquerda
            if(checkIntersectSegments(sub[0], sub[2], element[0], element[2])){
                res.push_back(FindTheIntersectPoint(sub[0], sub[2], element[0], element[2]));
            }
            // Aresta Esquerda - Aresta Superior
            if(checkIntersectSegments(sub[0], sub[2], element[2], element[3])){
                res.push_back(FindTheIntersectPoint(sub[0], sub[2], element[2], element[3]));
            }
            // Aresta Esquerda - Aresta Direita
            if(checkIntersectSegments(sub[0], sub[2], element[1], element[3])){
                res.push_back(FindTheIntersectPoint(sub[0], sub[2], element[1], element[3]));
            }
            // Aresta Esquerda - Aresta Inferior
            if(checkIntersectSegments(sub[0], sub[2], element[0], element[1])){
                res.push_back(FindTheIntersectPoint(sub[0], sub[2], element[0], element[1]));
            }

            //------------------------

            // Aresta Superior - Aresta Esquerda
            if(checkIntersectSegments(sub[2], sub[3], element[0], element[2])){
                res.push_back(FindTheIntersectPoint(sub[2], sub[3], element[0], element[2]));
            }
            // Aresta Superior - Aresta Superior
            if(checkIntersectSegments(sub[2], sub[3], element[2], element[3])){
                res.push_back(FindTheIntersectPoint(sub[2], sub[3], element[2], element[3]));
            }
            // Aresta Superior - Aresta Direita
            if(checkIntersectSegments(sub[2], sub[3], element[1], element[3])){
                res.push_back(FindTheIntersectPoint(sub[2], sub[3], element[1], element[3]));
            }
            // Aresta Superior - Aresta Inferior
            if(checkIntersectSegments(sub[2], sub[3], element[0], element[1])){
                res.push_back(FindTheIntersectPoint(sub[2], sub[3], element[0], element[1]));
            }

            //-----------------------

            // Aresta Direita - Aresta Esquerda
            if(checkIntersectSegments(sub[1], sub[3], element[0], element[2])){
                res.push_back(FindTheIntersectPoint(sub[1], sub[3], element[0], element[2]));
            }
            // Aresta Direita - Aresta Superior
            if(checkIntersectSegments(sub[1], sub[3], element[2], element[3])){
                res.push_back(FindTheIntersectPoint(sub[1], sub[3], element[2], element[3]));
            }
            // Aresta Direita - Aresta Direita
            if(checkIntersectSegments(sub[1], sub[3], element[1], element[3])){
                res.push_back(FindTheIntersectPoint(sub[1], sub[3], element[1], element[3]));
            }
            // Aresta Direita - Aresta Inferior
            if(checkIntersectSegments(sub[1], sub[3], element[0], element[1])){
                res.push_back(FindTheIntersectPoint(sub[1], sub[3], element[0], element[1]));
            }

            //----------------------

            // Aresta Inferior - Aresta Esquerda
            if(checkIntersectSegments(sub[0], sub[1], element[0], element[2])){
                res.push_back(FindTheIntersectPoint(sub[0], sub[1], element[0], element[2]));
            }
            // Aresta Inferior - Aresta Superior
            if(checkIntersectSegments(sub[0], sub[1], element[2], element[3])){
                res.push_back(FindTheIntersectPoint(sub[0], sub[1], element[2], element[3]));
            }
            // Aresta Inferior - Aresta Direita
            if(checkIntersectSegments(sub[0], sub[1], element[1], element[3])){
                res.push_back(FindTheIntersectPoint(sub[0], sub[1], element[1], element[3]));
            }
            // Aresta Inferior - Aresta Inferior
            if(checkIntersectSegments(sub[0], sub[1], element[0], element[1])){
                res.push_back(FindTheIntersectPoint(sub[0], sub[1], element[0], element[1]));
            }

            j++;
        }

        i++;
        j = 0;
    }

    return res;
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
            for(int i = 0; i < cloud.size(); ++i){
                auto sub = cloud[i];

                rgb color = colors[i];
                float r = color.red;
                float g = color.green;
                float b = color.blue;

                for(const auto &p : sub){
                    drawPoint(p, shaderProgram, projection, r, g, b);
                }

            }
        }

        if(!aabb.empty()){
            drawRectangle(shaderProgram, projection);
            if(aabb.size() >= 2){ // Temos que ter pelo menos 2 AABB's
                std::vector<ponto2D> intersects = checkIntersectBetweenAABBs();

                for(const auto& p : intersects){
                    drawPoint(p, shaderProgram, projection, 1.0f, 1.0f, 1.0f);
                }
            }
        }

        if(!mouseInput.empty()){
            for(int i = 0; i < mouseInput.size(); ++i){
                bool b = checkBelongsToAABB(mouseInput[i]);
                if(b){
                    drawPoint(mouseInput[i], shaderProgram, projection, 0.0f, 1.0f, 0.0f);
                }else{
                    drawPoint(mouseInput[i], shaderProgram, projection, 1.0f, 0.0f, 0.0f);
                }
            }
        }

        if(!circles.empty()){
            for(int i = 0; i < circles.size(); ++i){
                auto circle = circles[i];

                float r = colors[i].red;
                float g = colors[i].green;
                float b = colors[i].blue;

                drawCircle(circle.first, circle.second, shaderProgram, projection, r, g, b);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}