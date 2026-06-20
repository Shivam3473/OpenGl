#include <iostream>
#include <algorithm>
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
// ---------------------------------------------------------
// COMPUTE SHADER: BARNES-HUT TREE TRAVERSAL
// ---------------------------------------------------------
const char *computeShaderSource = "#version 430 core\n"
                                  "layout(local_size_x = 256) in;\n"
                                  "\n"
                                  "struct Particle {\n"
                                  "    vec4 posMass;\n"
                                  "    vec4 velocity;\n"
                                  "};\n"
                                  "\n"
                                  "// Must perfectly match the C++ memory layout (96 bytes)\n"
                                  "struct OctreeNode {\n"
                                  "    vec4 centerOfMassMass;\n"
                                  "    vec4 boundsMin;\n"
                                  "    vec4 boundsMax;\n"
                                  "    int children[8];\n"
                                  "    int isLeaf;\n"
                                  "    int particleIndex;\n"
                                  "    int pad[2];\n"
                                  "};\n"
                                  "\n"
                                  "layout(std430, binding = 0) buffer ParticleBuffer {\n"
                                  "    Particle particles[];\n"
                                  "};\n"
                                  "\n"
                                  "layout(std430, binding = 1) buffer TreeBuffer {\n"
                                  "    OctreeNode tree[];\n"
                                  "};\n"
                                  "\n"
                                  "uniform float dt;\n"
                                  "uniform int numParticles;\n"
                                  "uniform float G;\n"
                                  "uniform float M_halo;\n"
                                  "uniform float a_halo;\n"
                                  "uniform float M_disk;\n"
                                  "uniform float a_disk;\n"
                                  "uniform float b_disk;\n"
                                  "uniform float theta; // Barnes-Hut Opening Angle\n"
                                  "\n"
                                  "void main() {\n"
                                  "    uint index = gl_GlobalInvocationID.x;\n"
                                  "    if (index >= numParticles) return;\n"
                                  "    \n"
                                  "    vec3 myPos = particles[index].posMass.xyz;\n"
                                  "    float x = myPos.x;\n"
                                  "    float y = myPos.y;\n"
                                  "    float z = myPos.z;\n"
                                  "    float R2 = x*x + y*y;\n"
                                  "    float r = sqrt(R2 + z*z);\n"
                                  "    if (r < 0.0001) r = 0.0001;\n"
                                  "    \n"
                                  "    vec3 accel_nbody = vec3(0.0);\n"
                                  "    const float softeningSq = 0.13 * 0.13;\n"
                                  "    \n"
                                  "    // ---------------------------------------------------------\n"
                                  "    // BARNES-HUT STACK TRAVERSAL\n"
                                  "    // ---------------------------------------------------------\n"
                                  "    int stack[64]; \n"
                                  "    int stackPtr = 0;\n"
                                  "    \n"
                                  "    // Push root node (index 0)\n"
                                  "    stack[stackPtr++] = 0;\n"
                                  "    \n"
                                  "    while (stackPtr > 0) {\n"
                                  "        // Pop the top node off the stack\n"
                                  "        int nodeIdx = stack[--stackPtr];\n"
                                  "        OctreeNode node = tree[nodeIdx];\n"
                                  "        \n"
                                  "        vec3 com = node.centerOfMassMass.xyz;\n"
                                  "        float mass = node.centerOfMassMass.w;\n"
                                  "        \n"
                                  "        if (mass <= 0.0) continue; // Skip empty nodes\n"
                                  "        \n"
                                  "        vec3 distVec = com - myPos;\n"
                                  "        float distSq = dot(distVec, distVec) + softeningSq;\n"
                                  "        float dist = sqrt(distSq);\n"
                                  "        \n"
                                  "        if (node.isLeaf == 1) {\n"
                                  "            // If it's a leaf, interact directly (as long as it's not ourselves!)\n"
                                  "            if (node.particleIndex != int(index) && node.particleIndex != -1) {\n"
                                  "                accel_nbody += (G * mass / (distSq * dist)) * distVec;\n"
                                  "            }\n"
                                  "        } else {\n"
                                  "            // It's an internal node. Check the Barnes-Hut MAC (Multipole Acceptance Criterion)\n"
                                  "            vec3 boundsSize = node.boundsMax.xyz - node.boundsMin.xyz;\n"
                                  "            // Get the maximum width of the node cell\n"
                                  "            float width = max(max(boundsSize.x, boundsSize.y), boundsSize.z);\n"
                                  "            \n"
                                  "            if ((width / dist) < theta) {\n"
                                  "                // The node is far enough away! Treat the whole cluster as one giant particle.\n"
                                  "                accel_nbody += (G * mass / (distSq * dist)) * distVec;\n"
                                  "            } else {\n"
                                  "                // The node is too close. We need to open it and push its children onto the stack.\n"
                                  "                for (int i = 0; i < 8; i++) {\n"
                                  "                    if (node.children[i] != -1) {\n"
                                  "                        if (stackPtr < 64) {\n"
                                  "                            stack[stackPtr++] = node.children[i];\n"
                                  "                        }\n"
                                  "                    }\n"
                                  "                }\n"
                                  "            }\n"
                                  "        }\n"
                                  "    }\n"
                                  "    \n"
                                  "    // Total Acceleration \n"
                                  "    vec3 accel = accel_nbody;\n"
                                  "    \n"
                                  "    // Integration (Semi-Implicit Euler)\n"
                                  "    particles[index].velocity.xyz += accel * dt;\n"
                                  "    particles[index].posMass.xyz += particles[index].velocity.xyz * dt;\n"
                                  "}\n\0";
const char *vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec4 aPosMass;\n"
                                 "uniform mat4 transform;\n"
                                 "uniform int u_numDiskParticles;\n"
                                 "out vec4 particleColor;\n"
                                 "void main() {\n"
                                 "   gl_Position = transform * vec4(aPosMass.xyz, 1.0f);\n"
                                 "   if (gl_VertexID < u_numDiskParticles) {\n"
                                 "       particleColor = vec4(1.0f, 0.9f, 0.7f, 1.0f); // Bright Disk\n"
                                 "   } else {\n"
                                 "       particleColor = vec4(0.0f, 0.0f, 0.0f, 0.0f); // Completely Invisible Halo\n"
                                 "   }\n"
                                 "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
                                   "in vec4 particleColor;\n" // Receive color
                                   "out vec4 FragColor;\n"
                                   "void main() {\n"
                                   "   vec2 coord = gl_PointCoord - vec2(0.5);\n"
                                   "   float dist = length(coord);\n"
                                   "   if (dist > 0.5) discard;\n"
                                   "   float intensity = 0.5 - (dist / 0.5);\n"
                                   // Multiply the base color by our intensity curve
                                   "   FragColor = vec4(particleColor.rgb, particleColor.a * intensity * 2.0f);\n"
                                   "}\n\0";

// CHANGE FOR COMPUTE: C++ Struct aligned with the GLSL struct
struct Particle
{
    glm::vec4 posMass;  // x, y, z, mass
    glm::vec4 velocity; // vx, vy, vz, padding
};

const float a = 1.0f;
const float b = 0.1f;
const float R_max = 5.0f * a;
const float z_max = 5.0f * b;
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
    float b_disk = 0.02f;
    float a_disk = 1;
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
         (A * A - 2.0f * R * R)) /
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

// ---------------------------------------------------------
// BARNES-HUT OCTREE DATA STRUCTURES
// ---------------------------------------------------------
// Padded to exactly 96 bytes to align with GLSL std430 layout
struct alignas(16) OctreeNode
{
    glm::vec4 centerOfMassMass; // x, y, z = Center of Mass, w = Total Mass
    glm::vec4 boundsMin;        // x, y, z = Min Bounding Box
    glm::vec4 boundsMax;        // x, y, z = Max Bounding Box
    int children[8];            // Indices of 8 children. -1 means no child.
    int isLeaf;                 // 1 if leaf node, 0 if internal node
    int particleIndex;          // Index of particle if leaf, -1 otherwise
    int pad[2];                 // Padding to guarantee 16-byte alignment
};

class BarnesHutTree
{
public:
    std::vector<OctreeNode> nodes;

    void build(const std::vector<Particle> &particles)
    {
        nodes.clear();
        // Pre-allocate memory to prevent slow reallocations during the frame
        nodes.reserve(particles.size() * 8);

        // 1. Find the master bounding box containing all particles
        glm::vec3 minB(1e9f), maxB(-1e9f);
        for (const auto &p : particles)
        {
            minB = glm::min(minB, glm::vec3(p.posMass));
            maxB = glm::max(maxB, glm::vec3(p.posMass));
        }

        // Add a microscopic buffer to the edges to prevent boundary precision errors
        minB -= glm::vec3(0.01f);
        maxB += glm::vec3(0.01f);

        // 2. Create the Root Node (Index 0)
        createNode(minB, maxB);

        // 3. Insert all particles one by one
        for (int i = 0; i < particles.size(); ++i)
        {
            insert(0, i, particles);
        }
    }

private:
    int createNode(glm::vec3 minB, glm::vec3 maxB)
    {
        OctreeNode n;
        n.centerOfMassMass = glm::vec4(0.0f);
        n.boundsMin = glm::vec4(minB, 0.0f);
        n.boundsMax = glm::vec4(maxB, 0.0f);
        for (int i = 0; i < 8; ++i)
            n.children[i] = -1;
        n.isLeaf = 1;
        n.particleIndex = -1;
        nodes.push_back(n);
        return nodes.size() - 1;
    }

    int getOctant(glm::vec3 center, glm::vec3 pos)
    {
        int oct = 0;
        if (pos.x >= center.x)
            oct |= 1;
        if (pos.y >= center.y)
            oct |= 2;
        if (pos.z >= center.z)
            oct |= 4;
        return oct;
    }

    void getChildBounds(glm::vec3 minB, glm::vec3 maxB, glm::vec3 center, int octant, glm::vec3 &cMin, glm::vec3 &cMax)
    {
        cMin.x = (octant & 1) ? center.x : minB.x;
        cMax.x = (octant & 1) ? maxB.x : center.x;
        cMin.y = (octant & 2) ? center.y : minB.y;
        cMax.y = (octant & 2) ? maxB.y : center.y;
        cMin.z = (octant & 4) ? center.z : minB.z;
        cMax.z = (octant & 4) ? maxB.z : center.z;
    }

    void insert(int nodeIdx, int pIdx, const std::vector<Particle> &particles)
    {
        glm::vec3 pPos = particles[pIdx].posMass;
        float pMass = particles[pIdx].posMass.w;

        int currNode = nodeIdx;

        // Iterative traversal to avoid Deep Recursion stack overflows in C++
        while (true)
        {
            // Update Center of Mass and Total Mass for the current node
            glm::vec4 currentCoM = nodes[currNode].centerOfMassMass;
            float currentMass = currentCoM.w;
            glm::vec3 newCoM = (glm::vec3(currentCoM) * currentMass + pPos * pMass) / (currentMass + pMass);
            nodes[currNode].centerOfMassMass = glm::vec4(newCoM, currentMass + pMass);

            if (nodes[currNode].isLeaf)
            {
                if (nodes[currNode].particleIndex == -1)
                {
                    // Scenario A: Empty leaf. Just drop the particle here.
                    nodes[currNode].particleIndex = pIdx;
                    break;
                }
                else
                {
                    // Scenario B: Collision! Another particle is already in this leaf.
                    int existingPIdx = nodes[currNode].particleIndex;
                    glm::vec3 existingPos = particles[existingPIdx].posMass;

                    // Safety net: If two particles are at the exact same coordinate,
                    // merge them to prevent infinite subdivision loops.
                    if (glm::distance(existingPos, pPos) < 1e-5f)
                        break;

                    // Convert this leaf into an internal node
                    nodes[currNode].isLeaf = 0;
                    nodes[currNode].particleIndex = -1;

                    glm::vec3 minB = nodes[currNode].boundsMin;
                    glm::vec3 maxB = nodes[currNode].boundsMax;
                    glm::vec3 center = (minB + maxB) * 0.5f;

                    // Push the EXISTING particle down into a new child node
                    int oct1 = getOctant(center, existingPos);
                    glm::vec3 cMin, cMax;
                    getChildBounds(minB, maxB, center, oct1, cMin, cMax);
                    int childIdx = createNode(cMin, cMax);
                    nodes[currNode].children[oct1] = childIdx;

                    nodes[childIdx].centerOfMassMass = particles[existingPIdx].posMass;
                    nodes[childIdx].particleIndex = existingPIdx;

                    // Allow the while loop to continue so it pushes the NEW particle down too
                    int oct2 = getOctant(center, pPos);
                    if (nodes[currNode].children[oct2] == -1)
                    {
                        getChildBounds(minB, maxB, center, oct2, cMin, cMax);
                        nodes[currNode].children[oct2] = createNode(cMin, cMax);
                    }
                    currNode = nodes[currNode].children[oct2];
                }
            }
            else
            {
                // Scenario C: Internal node. Push the new particle down to the correct octant.
                glm::vec3 minB = nodes[currNode].boundsMin;
                glm::vec3 maxB = nodes[currNode].boundsMax;
                glm::vec3 center = (minB + maxB) * 0.5f;

                int oct = getOctant(center, pPos);
                if (nodes[currNode].children[oct] == -1)
                {
                    glm::vec3 cMin, cMax;
                    getChildBounds(minB, maxB, center, oct, cMin, cMax);
                    nodes[currNode].children[oct] = createNode(cMin, cMax);
                }
                currNode = nodes[currNode].children[oct];
            }
        }
    }
};

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
    const int NUM_PARTICLES = 80000;
    const int NUM_DISK_PARTICLES = 30000;
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

    float mass = (M_disk / NUM_DISK_PARTICLES);

    for (int i = 0; i < NUM_DISK_PARTICLES; ++i)
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
        float Q = 1.2f; // thesis Fig 6.4 value
        float Sigma0 = M_disk / (2.0f * 3.1415926535f * a_disk * a_disk);
        float Sigma_R = Sigma0 * std::exp(-r / a_disk);

        float sigma_r = Q * (3.36f * G * Sigma_R) / std::max(kap, eps);

        // eq 6.11: v_z^2 = pi*G*Sigma(R)*z0, z0 = disk scale height
        float sigma_z = std::sqrt(3.1415926535f * G * Sigma_R * b_disk);

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
        std::normal_distribution<float> gaussian_phi(0.0f, sigma_r);
        float v_r = gaussian_phi(gen);
        std::normal_distribution<float> gaussian_phi_1(0.0f, sigma_z);
        float v_z = gaussian_phi_1(gen);
        std::normal_distribution<float> gaussian_phi_2(0.0f, sigma_phi);
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

            // now defining the velocity
            particles[i].velocity = glm::vec4(v_x, v_y, vz, 0.0f);
        }
        else
        {
            i--;
        }
    }
    // ---------------------------------------------------------
    // INITIALIZING THE HERNQUIST HALO (30,000 Particles)
    // ---------------------------------------------------------

    const int NUM_HALO_PARTICLES = 50000;

    float halo_mass_per_particle = M_halo / NUM_HALO_PARTICLES;

    // We need uniform random numbers between 0 and 1
    std::uniform_real_distribution<float> dis_uniform(0.0f, 1.0f);

    for (int i = NUM_DISK_PARTICLES; i < NUM_PARTICLES; ++i)
    {
        float u1 = dis_uniform(gen);
        float u2 = dis_uniform(gen);
        float u3 = dis_uniform(gen);

        if (u1 >= 0.9999f)
            u1 = 0.9999f;

        float sqrt_u1 = std::sqrt(u1);
        float r = a_halo * (sqrt_u1 / (1.0f - sqrt_u1));
        if (r > 50.0f)
        {
            i--;
            continue;
        }

        if (r < 0.001f)
            r = 0.001f;

        float z = r * (1.0f - 2.0f * u2);
        float xy_radius = std::sqrt(r * r - z * z);
        float phi = 2.0f * 3.1415926535f * u3;

        float x = xy_radius * std::cos(phi);
        float y = xy_radius * std::sin(phi);

        // Assign Position and Halo Mass
        particles[i].posMass = glm::vec4(x, y, z, halo_mass_per_particle);

        float A = a_disk + b_disk;

        float Phi_halo =
            -(G * M_halo) / (r + a_halo);

        float Phi_disk =
            -(G * M_disk) /
            sqrt(r * r + A * A);

        float Phi_r =
            Phi_halo + Phi_disk;

        float v_esc = sqrt(2.0f * abs(Phi_r));
        float v_g = std::sqrt((G * M_halo) / a_halo);

        float v_mag = 0.0f;
        bool accepted = false;

        // ---------------------------------------------------------
        // NEW: DYNAMIC f_max SEARCH
        // Find the actual peak of the probability curve for this specific radius
        // ---------------------------------------------------------
        float f_max = 0.0f;
        for (int step = 1; step <= 500; step++)
        {
            float test_v = (v_esc * step) / 50.0f;
            float test_E = 0.5f * test_v * test_v + Phi_r;
            if (test_E >= 0.0f)
                continue;

            float test_q = std::sqrt(-(a_halo * test_E) / (G * M_halo));
            float test_q2 = test_q * test_q;

            // Protect against sqrt of negative due to floating point imprecision
            float q_safe = std::max(1.0f - test_q2, 0.0f);

            float t1 = 3.0f * std::asin(test_q);
            float t2 = test_q * std::sqrt(q_safe) * (1.0f - 2.0f * test_q2) * (8.0f * test_q2 * test_q2 - 8.0f * test_q2 - 3.0f);

            float denom = 8.0f * std::sqrt(2.0f) * std::pow(3.14159265f, 3.0f) * (a_halo * a_halo * a_halo) * (v_g * v_g * v_g) * std::pow(q_safe, 2.5f);
            float p_test = (test_v * test_v) * (M_halo / denom) * (t1 + t2);

            if (p_test > f_max)
                f_max = p_test;
        }

        // Add a 10% safety margin. If curve is flat/zero, set a tiny baseline.
        f_max *= 1.1f;
        if (f_max < 1e-10f)
            f_max = 1e-10f;
        // ---------------------------------------------------------

        // 2. The Acceptance-Rejection Loop for Speed
        int counter = 0;
        while (!accepted)
        {
            counter++;
            if (counter > 50000)
            {
                // Failsafe fallback: assign random bound velocity if strictly stuck
                v_mag = dis_uniform(gen) * v_esc * 0.5f;
                break;
            }

            float v_guess = dis_uniform(gen) * v_esc;
            float u_prob = dis_uniform(gen) * f_max;

            float E = 0.5f * v_guess * v_guess + Phi_r;
            if (E >= 0.0f)
                continue;

            float q = std::sqrt(-(a_halo * E) / (G * M_halo));
            float q2 = q * q;
            if (q < 0.0f)
                q = 0.0f;
            if (q > 0.9999f)
                q = 0.9999f;

            float term1 = 3.0f * std::asin(q);
            float term2 = q * std::sqrt(1.0f - q2) * (1.0f - 2.0f * q2) * (8.0f * q2 * q2 - 8.0f * q2 - 3.0f);

            float denominator = 8.0f * std::sqrt(2.0f) * std::pow(3.14159265f, 3.0f) * (a_halo * a_halo * a_halo) * (v_g * v_g * v_g) * std::pow(1.0f - q2, 2.5f);
            float f_E = (M_halo / denominator) * (term1 + term2);
            float P_target = (v_guess * v_guess) * f_E;

            if (u_prob < P_target)
            {
                v_mag = v_guess;
                accepted = true;
            }
        }

        // Random Isotropic Direction for Halo Thermal Velocity
        float cos_theta_v = 1.0f - 2.0f * dis_uniform(gen);
        float sin_theta_v = std::sqrt(std::max(1.0f - cos_theta_v * cos_theta_v, 0.0f));
        float phi_v = 2.0f * 3.1415926535f * dis_uniform(gen);

        float vx = v_mag * sin_theta_v * std::cos(phi_v);
        float vy = v_mag * sin_theta_v * std::sin(phi_v);
        float vz = v_mag * cos_theta_v;

        // Thesis Correction: Add Disk Circular Velocity
        float R_cyl = std::sqrt(x * x + y * y);
        float A_disk = a_disk + b_disk;
        float v_c_disk = std::sqrt((G * M_disk * R_cyl * R_cyl) / std::pow(R_cyl * R_cyl + A_disk * A_disk, 1.5f));

        float R_safe = std::max(R_cyl, 1e-5f);

        float v_cx = -v_c_disk * y / R_safe;
        float v_cy = v_c_disk * x / R_safe;
        float v_cz = 0.0f;

        // Final Composite Velocity
        particles[i].velocity = glm::vec4(vx + v_cx, vy + v_cy, vz + v_cz, 0.0f);
    };

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

    // here now we will need to add the BARNES HUT ALGO, like the buffer for the treeecode
    // Bind Particle Buffer (Binding = 0)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, SSBO);

    // ---------------------------------------------------------
    // NEW: Setup the Tree SSBO (Binding = 1)
    // ---------------------------------------------------------
    unsigned int TreeSSBO;
    glGenBuffers(1, &TreeSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, TreeSSBO);
    // We don't allocate size yet because the tree size changes every frame
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, TreeSSBO);

    // Instantiate our tree object
    BarnesHutTree tree;

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
    glPointSize(5.0f);

    float dt = 0.005f;

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

        // ---------------------------------------------------------
        // BARNES-HUT: CPU-GPU DATA SYNCHRONIZATION
        // ---------------------------------------------------------

        // 1. Read updated particle positions FROM the GPU back to the CPU
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
        Particle *ptr = (Particle *)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, particles.size() * sizeof(Particle), GL_MAP_READ_BIT);

        if (ptr)
        {
            // Quickly copy the data into our vector
            std::copy(ptr, ptr + particles.size(), particles.begin());
            // Instantly unmap so the GPU can use it again
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        }

        // 2. Build the Octree on the CPU
        tree.build(particles);

        // 3. Upload the new Tree structure TO the GPU
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, TreeSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, tree.nodes.size() * sizeof(OctreeNode), tree.nodes.data(), GL_DYNAMIC_DRAW);

        glUniform1f(glGetUniformLocation(computeProgram, "dt"), dt);
        glUniform1i(glGetUniformLocation(computeProgram, "numParticles"), NUM_PARTICLES);
        glUniform1f(glGetUniformLocation(computeProgram, "G"), G);
        glUniform1f(glGetUniformLocation(computeProgram, "M_halo"), 10.0f);
        glUniform1f(glGetUniformLocation(computeProgram, "a_halo"), 10.0f);
        glUniform1f(glGetUniformLocation(computeProgram, "M_disk"), 1.0f);
        glUniform1f(glGetUniformLocation(computeProgram, "a_disk"), 1.0f);
        glUniform1f(glGetUniformLocation(computeProgram, "b_disk"), 0.1f);

        glUniform1f(glGetUniformLocation(computeProgram, "theta"), 0.5f); // Barnes-Hut Opening Angle

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

        glUniform1i(glGetUniformLocation(renderProgram, "u_numDiskParticles"), NUM_DISK_PARTICLES);

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