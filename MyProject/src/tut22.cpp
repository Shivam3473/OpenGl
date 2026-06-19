#include <iostream>
#include <cmath>  // ADDED: We need cmath for calculating square roots and distances
#include <vector> // ADDED: needed to hold the grid's vertex data
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
using namespace std;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

void size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// some of the stuff there were meant to be pre defined
bool firstMouse = true;
float yaw = -90.0f; // yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float fov = 45.0f;

// timing
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f;

// for the camera to be controlled based on our mouse we can do it very simply based on the glfw function which captures the movement of the cursor and accordingly we set the direction inside the "LOOK AT " matrix, thus

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

// as a little extra to this mouse_callback we also need to add the scroll_callback basically this is the zoom feature which is evry important
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;
} // this also effects the projection martrix largely check it out

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = static_cast<float>(2.5 * deltaTime);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

// Vertex shader remains unchanged
const char *vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"

                                 "uniform mat4 transform;" // this is very important line to work with the transforms
                                 "void main()\n"
                                 "{\n"
                                 "  gl_Position = transform * vec4(aPos, 1.0f);"

                                 "}\0";

// Fragment shader remains unchanged (our glowing star look)
const char *fragmentShaderSource = "#version 330 core\n"
                                   "out vec4 FragColor;\n"
                                   "void main()\n"
                                   "{\n"
                                   "   vec2 coord = gl_PointCoord - vec2(0.5);\n"
                                   "   float dist = length(coord);\n"
                                   "   if (dist > 0.5)\n"
                                   "       discard;\n"
                                   "   float intensity = 20.0 - (dist / 0.5);\n"
                                   "   FragColor = vec4(0.8f, 0.9f, 1.0f, intensity);\n"
                                   "}\n\0";

// ADDED: the grid needs its own fragment shader. The glow trick above relies on
// gl_PointCoord, which only exists for GL_POINTS -- it's meaningless on GL_LINES.
// This one just outputs a flat, slightly-transparent color. It reuses vertexShaderSource,
// since all it needs is aPos + the transform uniform, same as the grid does.
const char *gridFragmentShaderSource = "#version 330 core\n"
                                        "out vec4 FragColor;\n"
                                        "void main()\n"
                                        "{\n"
                                        "   FragColor = vec4(0.3f, 0.5f, 0.9f, 0.35f);\n"
                                        "}\n\0";

// ADDED: lets UpdateGridVertices loop over an arbitrary list of masses
struct Body
{
    float x, y, mass;
};

// ADDED: builds a flat grid of crossing line segments in the X-Y plane (z = 0) --
// the same plane your orbit happens in. "size" = total width/height, "divisions" = cells per side.
vector<float> CreateGridVertices(float size, int divisions)
{
    vector<float> vertices;
    float step = size / divisions;
    float half = size / 2.0f;

    // horizontal lines: sweep x at each fixed y
    for (int yi = 0; yi <= divisions; ++yi)
    {
        float y = -half + yi * step;
        for (int xi = 0; xi < divisions; ++xi)
        {
            float x0 = -half + xi * step;
            float x1 = x0 + step;
            vertices.insert(vertices.end(), {x0, y, 0.0f, x1, y, 0.0f});
        }
    }
    // vertical lines: sweep y at each fixed x
    for (int xi = 0; xi <= divisions; ++xi)
    {
        float x = -half + xi * step;
        for (int yi = 0; yi < divisions; ++yi)
        {
            float y0 = -half + yi * step;
            float y1 = y0 + step;
            vertices.insert(vertices.end(), {x, y0, 0.0f, x, y1, 0.0f});
        }
    }
    return vertices;
}

// ADDED: pushes every grid vertex down in z, proportional to nearby mass.
// This is the "rubber sheet" trick: NOT solving GR, just sum(-strength*mass/distance)
// per vertex, so the sheet dips near heavy bodies and flattens far away.
// Tune strength/softening to taste -- softening stops dz exploding to -inf right under a mass.
vector<float> UpdateGridVertices(vector<float> vertices, const vector<Body> &bodies)
{
    const float strength = 0.015f;
    const float softening = 0.05f;

    for (size_t i = 0; i < vertices.size(); i += 3)
    {
        float vx = vertices[i];
        float vy = vertices[i + 1];
        float dz = 0.0f;
        for (const auto &b : bodies)
        {
            float dx = vx - b.x;
            float dy = vy - b.y;
            float dist = sqrt(dx * dx + dy * dy + softening * softening);
            dz -= strength * b.mass / dist;
        }
        vertices[i + 2] = dz;
    }
    return vertices;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 800, "Orbital Physics", NULL, NULL);
    if (window == NULL)
    {
        cout << "Failed to create window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    glViewport(0, 0, 800, 800);
    glfwSetFramebufferSizeCallback(window, size_callback);

    // ---------------------------------------------------------
    // ADDED: PHYSICS CONSTANTS & INITIAL STATE
    // ---------------------------------------------------------

    // We make up arbitrary values for G and Mass that look good on screen
    const float G = 0.012f;  // Gravitational constant
    const float M = 1000.0f; // Mass of the central star
    const float m = 25.0f;    // mass of the small stars

    // Central Star Position (Fixed at center)
    float star_x = 0.0f;
    float star_y = 0.0f;
    





    // Orbiting Particle Initial State
    float pos_1_x = 10.0f; // Start 0.6 units to the right
    float pos_1_y = -0.0f;

    // orbiting position initial state for particle 2
    float pos_2_x=-0.0f;
    float pos_2_y=10.0f;

    // Calculate initial velocity for a perfect circular orbit: v = sqrt(G*M/r)
    float r_initial = sqrt((pos_1_x - star_x) * (pos_1_x - star_x) + (pos_1_y - star_y) * (pos_1_y - star_y));
    float v_circular = sqrt((G * M) / r_initial);

    // writing velocites initial for both particles
             
    float vel_1_x=0.0f,   vel_1_y=v_circular;
    float vel_2_x=-1*v_circular,   vel_2_y=0.0f;
    float vel_star_x=-0.0f;
    float vel_star_y=-0.0;

    float dt = 0.2f; // Time step (how fast the simulation runs per frame)

    // ---------------------------------------------------------

    // CHANGED: We now have 6 floats (2 vertices).
    // The first 3 are the star, the next 3 are the orbiting particle.
    float vertices[] = {
        star_x, star_y, 0.0f,   // Vertex 0: Central Star
        pos_1_x, pos_1_y, 0.0f, // Vertex 1: Orbiting Particle
        pos_2_x, pos_2_y, 0.0f

    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // CHANGED: GL_STATIC_DRAW changed to GL_DYNAMIC_DRAW because we will update this every frame!
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // ADDED: grid buffer. Built once -- only the z-values get rewritten every frame.
    vector<float> gridVertices = CreateGridVertices(100.0f, 150); // size=2.0 units, 40x40 cells -- tune to taste
    unsigned int gridVAO, gridVBO;
    glGenVertexArrays(1, &gridVAO);
    glBindVertexArray(gridVAO);
    glGenBuffers(1, &gridVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // --- Shader Compilation (Unchanged) ---
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // ADDED: second program for the grid -- same vertex shader, flat-color fragment shader
    unsigned int gridFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(gridFragmentShader, 1, &gridFragmentShaderSource, NULL);
    glCompileShader(gridFragmentShader);

    unsigned int gridShaderProgram = glCreateProgram();
    glAttachShader(gridShaderProgram, vertexShader);
    glAttachShader(gridShaderProgram, gridFragmentShader);
    glLinkProgram(gridShaderProgram);

    // Enable Blending for the glowing effect
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(20.0f);

    // ---------------------------------------------------------
    // RENDER LOOP
    // ---------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        processInput(window);

        // ADDED: PHYSICS CALCULATION FOR THIS FRAME

        // 1. Calculate the distance vector from the particle to the star
        float r_1 = sqrt((pos_1_x - star_x) * (pos_1_x - star_x) + (pos_1_y - star_y) * (pos_1_y - star_y));
        float r_2 = sqrt((pos_2_x - star_x) * (pos_2_x - star_x) + (pos_2_y - star_y) * (pos_2_y - star_y));
        float r_1_2 = sqrt((pos_1_x - pos_2_x) * (pos_1_x - pos_2_x) + (pos_1_y - pos_2_y) * (pos_1_y - pos_2_y));

        // 3. Apply the direction to the acceleration (dx/r and dy/r)
        // a_x = a * (dx/r)  =>  (G*M/r^2) * (dx/r) => G*M*dx / r^3
        float acc_1_x = -1 * (((G * M) / (r_1 * r_1 * r_1)) * (pos_1_x - star_x) + (((G * m) / (r_1_2 * r_1_2 * r_1_2)) * (pos_1_x - pos_2_x)));

        float acc_2_x = (((G * M) / (r_2 * r_2 * r_2)) * (star_x - pos_2_x) + (((G * m) / (r_1_2 * r_1_2 * r_1_2)) * (pos_1_x - pos_2_x)));

        float acc_1_y = (((G * M) / (r_1 * r_1 * r_1)) * (star_y - pos_1_y) - (((G * m) / (r_1_2 * r_1_2 * r_1_2)) * (pos_1_y - pos_2_y)));

        float acc_2_y = (((G * M) / (r_2 * r_2 * r_2)) * (star_y - pos_2_y) + (((G * m) / (r_1_2 * r_1_2 * r_1_2)) * (pos_1_y - pos_2_y)));

        float acc_star_x = (((G * m) / (r_1 * r_1 * r_1)) * (pos_1_x - star_x)) - (((G * m) / (r_2 * r_2 * r_2)) * (star_x - pos_2_x));

        float acc_star_y = -1 * (((G * m) / (r_1 * r_1 * r_1)) * (star_y - pos_1_y)) + (((G * m) / (r_2 * r_2 * r_2)) * (star_y - pos_2_y));

        // 4. Update velocity (v = v + a*dt)
        vel_1_x += acc_1_x * dt;
        vel_1_y += acc_1_y * dt;
        vel_2_x += acc_2_x * dt;
        vel_2_y += acc_2_y * dt;

        vel_star_x += acc_star_x* dt;
        vel_star_y += acc_star_y*dt;

        // 5. Update position (p = p + v*dt) -> This is called Semi-Implicit Euler Integration
        pos_1_x += vel_1_x * dt;
        pos_1_y += vel_1_y * dt;
        pos_2_x += vel_2_x * dt;
        pos_2_y += vel_2_y * dt;
        star_x += vel_star_x * dt;
        star_y += vel_star_y * dt;

        // CHANGED: Update the array with the new particle position
        vertices[3] = pos_1_x;
        vertices[4] = pos_1_y;
        vertices[6] = pos_2_x;
        vertices[7] = pos_2_y;
        vertices[0] = star_x;
        vertices[1] = star_y;

        // ADDED: Send the updated positions to the GPU
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        // ADDED: re-warp the grid around the current body positions/masses, then re-upload it
        vector<Body> bodies = {
            {star_x, star_y, M},
            {pos_1_x, pos_1_y, m},
            {pos_2_x, pos_2_y, m}};
        gridVertices = UpdateGridVertices(gridVertices, bodies);
        glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, gridVertices.size() * sizeof(float), gridVertices.data());

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 view = glm::mat4(1.0f);

        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

        glm::mat4 trans = projection * view * model;

        

        // ADDED: draw the grid first, so the glowing points render on top of it
        glUseProgram(gridShaderProgram);
        unsigned int gridTransformLoc = glGetUniformLocation(gridShaderProgram, "transform");
        glUniformMatrix4fv(gridTransformLoc, 1, GL_FALSE, glm::value_ptr(trans));
        glBindVertexArray(gridVAO);
        glDrawArrays(GL_LINES, 0, gridVertices.size() / 3);

        // Draw both particles
        glUseProgram(shaderProgram);
        unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));
        glBindVertexArray(VAO);
        
        glDrawArrays(GL_POINTS, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ── Cleanup ───────────────────────────────────────────
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    // ADDED: clean up the grid's buffers/program too
    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
    glDeleteProgram(gridShaderProgram);
    glDeleteShader(gridFragmentShader);
    glfwTerminate();
    return 0;
}