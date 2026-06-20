#include <iostream>
#include<random>
#include <cmath> 
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

void size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool firstMouse = true;
float yaw = -90.0f; 
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float fov = 45.0f;

float deltaTime = 0.0f; 
float lastFrame = 0.0f;

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos; lastY = ypos; firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset; pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    fov -= (float)yoffset;
    if (fov < 1.0f) fov = 1.0f;
    if (fov > 45.0f) fov = 45.0f;
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = static_cast<float>(2.5 * deltaTime);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

// ---------------------------------------------------------
// CHANGE FOR COMPUTE: The Compute Shader..
// This runs on the GPU, calculating gravity for N bodies simultaneously.
// ---------------------------------------------------------
const char* computeShaderSource = "#version 430 core\n"
"layout(local_size_x = 256) in;\n" // Process 256 particles per workgroup
"\n"
"struct Particle {\n"
"    vec4 posMass; // x, y, z, and w is Mass\n"
"    vec4 velocity; // x, y, z, and padding\n"
"};\n"
"\n"
"layout(std430, binding = 0) buffer ParticleBuffer {\n"
"    Particle particles[];\n" // this helps us in associating those 10000 particles each to the core of threads of gpu by making a buffer object
"};\n"
"\n"
"uniform float dt;\n" // they r passed fom c++ and will not change
"uniform int numParticles;\n"
"\n"
"void main() {\n"
"    uint index = gl_GlobalInvocationID.x;\n" // here this acesses the nth thread which is being run, we know index =75 if 57th thread runs and obviously it looks after the 57th particle
"    if (index >= numParticles) return;\n"
"    \n"
"    vec3 myPos = particles[index].posMass.xyz;\n" // we simply extract the particle position on which our thread is currently , remember all these threads which in 10000's run simultaneosly
"    vec3 accel = vec3(0.0);\n" // for that thread particle we assign 0 acc initially 
"    const float G = 0.001;\n"
"    const float softening = 0.05; // Prevents infinite forces on collision\n"
"    \n" // here is just a preentive measure that if collision occurs the force doesnot then to infinity
"    // N-Body Gravity Calculation\n"  // the most exciting calc are here
"    for (int i = 0; i < numParticles; i++) {\n"
"        if (i == index) continue;\n"
"        \n"
"        vec3 otherPos = particles[i].posMass.xyz;\n"
"        float otherMass = particles[i].posMass.w;\n"
"        \n"
"        vec3 distVec = otherPos - myPos;\n"
"        float distSq = dot(distVec, distVec) + (softening * softening);\n"
"        float dist = sqrt(distSq);\n"
"        \n"
"        accel += (G * otherMass / (distSq * dist)) * distVec;\n"
"    }\n"
"    \n"
"    // Semi-Implicit Euler Integration\n" 
"    particles[index].velocity.xyz += accel * dt;\n"
"    particles[index].posMass.xyz += particles[index].velocity.xyz * dt;\n"
"}\n\0";

const char *vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec4 aPosMass;\n" // CHANGE FOR COMPUTE: Input is now a vec4
"uniform mat4 transform;\n"
"void main() {\n"
"   gl_Position = transform * vec4(aPosMass.xyz, 1.0f);\n" // Extract just the xyz
"}\0";

const char *fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main() {\n"
"   vec2 coord = gl_PointCoord - vec2(0.5);\n"
"   float dist = length(coord);\n"
"   if (dist > 0.5) discard;\n"
"   float intensity = 0.5 - (dist / 0.5);\n"
"   FragColor = vec4(0.8f, 0.9f, 1.0f, intensity);\n"
"}\n\0";

// CHANGE FOR COMPUTE: C++ Struct aligned with the GLSL struct
struct Particle {
    glm::vec4 posMass;  // x, y, z, mass
    glm::vec4 velocity; // vx, vy, vz, padding
};

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // CHANGE FOR COMPUTE: Compute shaders require at least OpenGL 4.3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 800, "Compute Shader N-Body", NULL, NULL);
    if (window == NULL) {
        cout << "Failed to create window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    glViewport(0, 0, 800, 800);
    glfwSetFramebufferSizeCallback(window, size_callback);

    // ---------------------------------------------------------
    // CHANGE FOR COMPUTE: Initializing the Particles Array
    // ---------------------------------------------------------
    // INITIALIZING 10,000 PARTICLES (GALAXY MODEL)
    // ---------------------------------------------------------
    const int NUM_PARTICLES = 60000;
    std::vector<Particle> particles(NUM_PARTICLES);

    // Setup C++ random number generators for a natural look
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Distribute particles between radius 0.2 and 2.5
    std::uniform_real_distribution<float> dis_radius(0.5f, 4.0f);
    // 360 degree spread in radians
    std::uniform_real_distribution<float> dis_angle(0.0f, 2.0f * 3.1415926535f);
    // Slight variation on the Z-axis to give the disk some 3D thickness
    std::uniform_real_distribution<float> dis_z(-0.1f, 0.1f); 

    // Distribute particles between radius 0.2 and 2.5
    std::uniform_real_distribution<float> dis_radius_1(1.0f, 4.0f);
    
    const float G = 0.001f;
    const float CENTRAL_MASS = 100000.0f; // Supermassive center

    // 1. Place the supermassive central body at index 0
    particles[0].posMass = glm::vec4(0.0f, 0.0f, 0.0f, CENTRAL_MASS);
    particles[0].velocity = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

    particles[1].posMass = glm::vec4(20.0f, 0.0f, -2.0f, 20*CENTRAL_MASS);
    particles[1].velocity = glm::vec4(0.0f, 0.0f, 3.0f, 0.0f);


    // 2. Loop through the remaining 9,999 particles
    for (int i = 2; i < NUM_PARTICLES/2; ++i) {
        float r = dis_radius(gen);
        float angle = dis_angle(gen);

        // Convert polar coordinates to Cartesian (XY plane)
        float x = r * cos(angle);
        float y = r * sin(angle);
        float z = dis_z(gen);
        float mass = 2.0f; // Much smaller stellar mass

        particles[i].posMass = glm::vec4(x, y, z, mass);

        // 3. Calculate Orbital Velocity
        // v = sqrt(G * M / r)
        float v = sqrt((G * CENTRAL_MASS) / r);

        // 4. Find the tangential vector
        // If a point is at (x, y), its tangent direction is (-y, x)
        glm::vec2 dir = glm::normalize(glm::vec2(-y, x));

        // Assign the velocity
        particles[i].velocity = glm::vec4(dir.x * v, dir.y * v, 0.0f, 0.0f);
    }

    for (int i =  (NUM_PARTICLES/2)+1; i < NUM_PARTICLES; ++i) {
        float r = dis_radius_1(gen);
        float angle = dis_angle(gen);

        // Convert polar coordinates to Cartesian (XY plane)
        float x = 20.0f+ r * cos(angle);
        float y = r * sin(angle);
        float z = dis_z(gen);
        float mass = 2.0f; // Much smaller stellar mass

        particles[i].posMass = glm::vec4(x, y, z, mass);

        // 3. Calculate Orbital Velocity
        // v = sqrt(G * M / r)
        float v = sqrt((G * 20*CENTRAL_MASS) / r);

        // 4. Find the tangential vector
        // If a point is at (x, y), its tangent direction is (-y, x)
        glm::vec2 dir = glm::normalize(glm::vec2(-y, x-20.0f));

        // Assign the velocity
        particles[i].velocity = glm::vec4(dir.x * v, dir.y * v, 0.0f, 0.0f);
    }

    // ---------------------------------------------------------
    // CHANGE FOR COMPUTE: Setup SSBO (Shader Storage Buffer Object)
    // ---------------------------------------------------------
    unsigned int SSBO, VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    glGenBuffers(1, &SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, particles.size() * sizeof(Particle), particles.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, SSBO);

    // Bind the same buffer as a VBO so the Vertex Shader can draw directly from it!
    glBindBuffer(GL_ARRAY_BUFFER, SSBO);
    
    // Define how the Vertex Shader reads the struct: Stride is sizeof(Particle)
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)0);
    glEnableVertexAttribArray(0);

    // --- Compile Compute Shader ---
    unsigned int computeShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(computeShader, 1, &computeShaderSource, NULL);
    glCompileShader(computeShader);
    
    unsigned int computeProgram = glCreateProgram();
    glAttachShader(computeProgram, computeShader);
    glLinkProgram(computeProgram);

    // --- Compile Render Shaders ---
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int renderProgram = glCreateProgram();
    glAttachShader(renderProgram, vertexShader);
    glAttachShader(renderProgram, fragmentShader);
    glLinkProgram(renderProgram);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(20.0f);

    float dt = 0.0002f;

    // ---------------------------------------------------------
    // RENDER LOOP
    // ---------------------------------------------------------
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        processInput(window);

        // ---------------------------------------------------------
        // CHANGE FOR COMPUTE: 1. Run the Physics on the GPU
        // ---------------------------------------------------------
        glUseProgram(computeProgram);
        glUniform1f(glGetUniformLocation(computeProgram, "dt"), dt);
        glUniform1i(glGetUniformLocation(computeProgram, "numParticles"), NUM_PARTICLES);
        
        // Dispatch compute groups. We use ceil(NUM_PARTICLES / 256.0) to cover all particles.
        int numGroups = (NUM_PARTICLES + 255) / 256;
        glDispatchCompute(numGroups, 1, 1); // this is the final launch
        
        // Ensure all physics math is finished before we try to draw the results
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        // ---------------------------------------------------------
        // CHANGE FOR COMPUTE: 2. Draw the updated memory directly
        // ---------------------------------------------------------
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(-75.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(fov), 800.0f / 800.0f, 0.1f, 100.0f);
        glm::mat4 trans = projection * view * model;

        glUseProgram(renderProgram);
        glUniformMatrix4fv(glGetUniformLocation(renderProgram, "transform"), 1, GL_FALSE, glm::value_ptr(trans));
        
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &SSBO);
    glDeleteProgram(renderProgram);
    glDeleteProgram(computeProgram);
    glfwTerminate();
    return 0;
}