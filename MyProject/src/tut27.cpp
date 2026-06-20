#include <iostream>
#include <random>
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

void size_callback(GLFWwindow *window, int width, int height)
{
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

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;
}

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

// ---------------------------------------------------------
// CHANGE FOR COMPUTE: The Compute Shader..
// This runs on the GPU, calculating gravity for N bodies simultaneously.
// ---------------------------------------------------------
const char *computeShaderSource = "#version 430 core\n"
                                  "layout(local_size_x = 256) in;\n"
                                  "\n"
                                  "struct Particle {\n"
                                  "    vec4 posMass;\n"
                                  "    vec4 velocity;\n"
                                  "};\n"
                                  "\n"
                                  "layout(std430, binding = 0) buffer ParticleBuffer {\n"
                                  "    Particle particles[];\n"
                                  "};\n"
                                  "\n"
                                  "uniform float dt;\n"
                                  "uniform int numParticles;\n"
                                  "// ADDED: Uniforms for the analytical potentials\n"
                                  "uniform float G;\n"
                                  "uniform float M_halo;\n"
                                  "uniform float a_halo;\n"
                                  "uniform float M_disk;\n"
                                  "uniform float a_disk;\n"
                                  "uniform float b_disk;\n"
                                  "\n"
                                  "void main() {\n"
                                  "    uint index = gl_GlobalInvocationID.x;\n"
                                  "    if (index >= numParticles) return;\n"
                                  "    \n"
                                  "    vec3 myPos = particles[index].posMass.xyz;\n"
                                  "    \n"
                                  "    // 1. Particle Position\n"
                                  "    float x = myPos.x;\n"
                                  "    float y = myPos.y;\n"
                                  "    float z = myPos.z;\n"
                                  "    float R2 = x*x + y*y;\n"
                                  "    float r = sqrt(R2 + z*z);\n"
                                  "    \n"
                                  "    // Prevent division by zero if a particle is exactly at the origin\n"
                                  "    if (r < 0.0001) r = 0.0001;\n"
                                  "    \n"
                                  "    // 2. Hernquist Halo Acceleration\n"
                                  "    float a_H_mag = (G * M_halo) / ((r + a_halo) * (r + a_halo));\n"
                                  "    vec3 acc_halo = vec3(-x/r, -y/r, -z/r) * a_H_mag;\n"
                                  "    \n"
                                  "    // 3. Miyamoto-Nagai Disk Acceleration\n"
                                  "    float Z = sqrt(z*z + b_disk*b_disk);\n"
                                  "    float A = a_disk + Z;\n"
                                  "    float D = pow(R2 + A*A, 1.5);\n"
                                  "    \n"
                                  "    vec3 acc_disk;\n"
                                  "    acc_disk.x = -(G * M_disk * x) / D;\n"
                                  "    acc_disk.y = -(G * M_disk * y) / D;\n"
                                  "    acc_disk.z = -(G * M_disk * A * z) / (D * Z);\n"
                                  "    \n"
                                  "    // 4. Total Acceleration\n"
                                  "    vec3 accel = acc_halo + acc_disk;\n"
                                  "    \n"
                                  "    // 5. Integration (Semi-Implicit Euler)\n"
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
struct Particle
{
    glm::vec4 posMass;  // x, y, z, mass
    glm::vec4 velocity; // vx, vy, vz, padding
};

const float a = 1.0f;
const float b = 0.1f;
const float R_max = 10 * a;
const float z_max = 5 * b;
const float G = 1.0f;
const float M_total = 1.0f;

float M_disk = 1.0f;
float a_disk = 1.0f;
float b_disk = 0.1f;
float M_halo = 10.0f;
float a_halo = 10.0f;

// Miyamoto Nagai Model for disk and bulge of our galaxy, so first we will calculate the positioning
float prob(float R, float z)
{
    float b = 0.1f;
    float a = 1;
    float M = 1.0f;
    float Rho = ((b * b * M) / (4 * 3.1415)) * ((a * R * R + (a + 3 * sqrt(z * z + b * b)) * (a + sqrt(z * z + b * b)) * (a + sqrt(z * z + b * b))) / ((R * R + (a + sqrt(z * z + b * b)) * (a + sqrt(z * z + b * b))) * (R * R + (a + sqrt(z * z + b * b)) * (a + sqrt(z * z + b * b))) * sqrt(R * R + (a + sqrt(z * z + b * b)) * (a + sqrt(z * z + b * b))) * (z * z + b * b) * sqrt(z * z + b * b)));
    return R * Rho;
}
float calculatePMax(float R_max, float z_max)
{
    float P_max = 0.0f;
    int steps = 1000; // Resolution of our search grid

    // Sweep across the R and z ranges
    for (int i = 0; i <= steps; ++i)
    {
        float R = (R_max * i) / steps;

        // We only need to check positive z because the galaxy is symmetric
        for (int j = 0; j <= steps; ++j)
        {
            float z = (z_max * j) / steps;

            float current_P = prob(R, z);
            if (current_P > P_max)
            {
                P_max = current_P;
            }
        }
    }

    // MULTIPLIER SAFETY NET:
    // Because our grid search might just barely miss the "perfect" microscopic peak,
    // we multiply P_max by 1.05 (a 5% buffer). This guarantees our rejection sampler
    // bounding box is completely above the function curve.
    return P_max * 1.05f;
}
float v_c(float R)
{
    if (R < 0.0001f)
        R = 0.0001f;
    float A = a_disk + b_disk;

    float dPhi_disk = (G * M_disk * R) / pow(R * R + A * A, 1.5f);
    float dPhi_halo = (G * M_halo) / pow(R + a_halo, 2.0f);

    // v_c = sqrt(R * dPhi_total)
    return sqrt(R * (dPhi_disk + dPhi_halo));
}

float kappa(float R)
{
    if (R < 0.0001f)
        R = 0.0001f;

    float A = a_disk + b_disk;

    float dPhi_disk =
        (G * M_disk * R) /
        pow(R * R + A * A, 1.5f);

    float dPhi_halo =
        (G * M_halo) /
        pow(R + a_halo, 2.0f);

    float d2Phi_disk =
        (G * M_disk *
        (A*A - 2.0f*R*R)) /
        pow(R * R + A * A, 2.5f);

    float d2Phi_halo =
        -(2.0f * G * M_halo) /
        pow(R + a_halo, 3.0f);

    float Omega2 =
        (dPhi_disk + dPhi_halo) / R;

    float inside =
        d2Phi_disk +
        d2Phi_halo +
        3.0f * Omega2;

    if (inside < 0.0f)
    {
        std::cout
            << "NEGATIVE KAPPA: "
            << inside
            << " R=" << R
            << std::endl;
    }

    return sqrt(std::max(inside, 0.0f));
}
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // CHANGE FOR COMPUTE: Compute shaders require at least OpenGL 4.3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 800, "Compute Shader N-Body", NULL, NULL);
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
    // CHANGE FOR COMPUTE: Initializing the Particles Array
    // ---------------------------------------------------------
    // INITIALIZING 10,000 PARTICLES (GALAXY MODEL)
    // ---------------------------------------------------------
    const int NUM_PARTICLES = 30000;
    std::vector<Particle> particles(NUM_PARTICLES);

    // Setup C++ random number generators for a natural look
    std::random_device rd;
    std::mt19937 gen(rd());

    // Distribute particles between radius 0.2 and 2.5
    std::uniform_real_distribution<float> dis_radius(0.01f, R_max);
    std::uniform_real_distribution<float> dis_z(-z_max, z_max);
    // 360 degree spread in radians
    std::uniform_real_distribution<float> dis_angle(0.0f, 2.0f * 3.1415926535f);
    std::uniform_real_distribution<float> dis_u(0.0001f, calculatePMax(R_max, z_max));

    float mass = M_total / NUM_PARTICLES;

    for (int i = 0; i < NUM_PARTICLES; ++i)
    {
        const float eps = 1e-3f;
        float r = dis_radius(gen);
        r = std::max(r, eps);
        float kap = kappa(r);

        float vc = v_c(r);
        float angle = dis_angle(gen);
        float z = dis_z(gen);
        float u = dis_u(gen);
        float Omega = vc / std::max(r, eps);
        float sigma_r =
            0.2f * vc *
            std::exp(-r / (2.0f * a_disk));

        // Vertical velocity dispersion
        float sigma_z =
            std::sqrt(
                std::max(
                    0.0f,
                    0.5f * G * b * a *
                        (M_total /
                         std::pow(r * r + a * a, 1.5f))));

        // Protect division by Omega
        float sigma_phi =
            sigma_r *
            (kappa(r) / (2.0f * std::max(std::abs(Omega), eps)));

        // Compute asymmetric drift term safely
        float denom =
            4.0f * vc * vc / (r * r);

        denom = std::max(denom, eps);

        float inside =
            vc * vc +
            sigma_r * sigma_r *
                (1.0f - (kap * kap) / denom - (2.0f * r / a));

        // Prevent NaN from sqrt(negative)
        inside = std::max(inside, 0.0f);

        float v_phi_mean = std::sqrt(inside);
        std::normal_distribution<float> gaussian_phi(0.001f, sigma_r);
        float v_r = gaussian_phi(gen);
        std::normal_distribution<float> gaussian_phi_1(0.001f, sigma_z);
        float v_z = gaussian_phi_1(gen);
        std::normal_distribution<float> gaussian_phi_2(0.001f, sigma_phi);
        float v_phi = gaussian_phi_2(gen) + v_phi_mean;

        if (u < prob(r, z))
        {
            // now if this dorting mechanism works and u<P_target then we keep this

            float x = r * cos(angle);
            float y = r * sin(angle);
            particles[i].posMass = glm::vec4(x, y, z, mass);

            // writing the velocity
            float v_x = v_r * cos(angle) - v_phi * sin(angle);
            float v_y = v_r * sin(angle) + v_phi * cos(angle);
            float vz = gaussian_phi_1(gen);

            if (!std::isfinite(v_phi_mean) ||
                !std::isfinite(sigma_r) ||
                !std::isfinite(sigma_phi) ||
                !std::isfinite(sigma_z))
            {
                std::cout
                    << "BAD VALUES\n"
                    << "r = " << r << "\n"
                    << "vc = " << vc << "\n"
                    << "kappa = " << kap << "\n"
                    << "Omega = " << Omega << "\n"
                    << "inside = " << inside << "\n";
            }

            if (std::isnan(x) || std::isnan(y) || std::isnan(z))
                std::cout << "NaN position\n";

            if (std::isnan(v_x) || std::isnan(v_y) || std::isnan(v_z))
                std::cout << "NaN velocity\n";

            // now defining the velocity
            particles[i].velocity = glm::vec4(v_x, v_y, vz, 0.0f);
        }
        else
        {
            i--;
        }
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
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void *)0);
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
    while (!glfwWindowShouldClose(window))
    {
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
        glUniform1f(glGetUniformLocation(computeProgram, "G"), G);
        glUniform1f(glGetUniformLocation(computeProgram, "M_halo"), 10.0f);
        glUniform1f(glGetUniformLocation(computeProgram, "a_halo"), 10.0f);
        glUniform1f(glGetUniformLocation(computeProgram, "M_disk"), 1.0f);
        glUniform1f(glGetUniformLocation(computeProgram, "a_disk"), 1.0f);
        glUniform1f(glGetUniformLocation(computeProgram, "b_disk"), 0.1f);

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