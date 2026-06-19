#include <iostream>
#include <cmath> // ADDED: We need cmath for calculating square roots and distances
#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace std;

void size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Vertex shader remains unchanged
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
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
"   float intensity = 1.0 - (dist / 0.5);\n"
"   FragColor = vec4(0.8f, 0.9f, 1.0f, intensity);\n"
"}\n\0";

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
    const float G = 0.001f;     // Gravitational constant
    const float M = 1000.0f;    // Mass of the central star
    
    // Central Star Position (Fixed at center)
    float star_x = 0.0f;
    float star_y = 0.0f;

    // Orbiting Particle Initial State
    float pos_x = 0.6f;         // Start 0.6 units to the right
    float pos_y = 0.0f;
    
    // Calculate initial velocity for a perfect circular orbit: v = sqrt(G*M/r)
    float r_initial = sqrt((pos_x - star_x)*(pos_x - star_x) + (pos_y - star_y)*(pos_y - star_y));
    float v_circular = sqrt((G * M) / r_initial);
    
    float vel_x = 0.0f;         // No horizontal velocity initially
    float vel_y = v_circular;   // Shoot straight UP to begin the orbit
    
    float dt = 0.0002f;          // Time step (how fast the simulation runs per frame)

    // ---------------------------------------------------------

    // CHANGED: We now have 6 floats (2 vertices).
    // The first 3 are the star, the next 3 are the orbiting particle.
    float vertices[] = {
        star_x, star_y, 0.0f,  // Vertex 0: Central Star
        pos_x,  pos_y,  0.0f   // Vertex 1: Orbiting Particle
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // CHANGED: GL_STATIC_DRAW changed to GL_DYNAMIC_DRAW because we will update this every frame!
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
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

    // Enable Blending for the glowing effect
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(20.0f);

    // ---------------------------------------------------------
    // RENDER LOOP
    // ---------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // ADDED: PHYSICS CALCULATION FOR THIS FRAME
        
        // 1. Calculate the distance vector from the particle to the star
        float dx = star_x - pos_x;
        float dy = star_y - pos_y;
        float r = sqrt(dx*dx + dy*dy);
        
        // 2. Calculate gravity acceleration magnitude: a = (G * M) / r^2
        float r_cubed = r * r * r; 
        
        // 3. Apply the direction to the acceleration (dx/r and dy/r)
        // a_x = a * (dx/r)  =>  (G*M/r^2) * (dx/r) => G*M*dx / r^3
        float accel_x = (G * M * dx) / r_cubed;
        float accel_y = (G * M * dy) / r_cubed;

        // 4. Update velocity (v = v + a*dt)
        vel_x += accel_x * dt;
        vel_y += accel_y * dt;

        // 5. Update position (p = p + v*dt) -> This is called Semi-Implicit Euler Integration
        pos_x += vel_x * dt;
        pos_y += vel_y * dt;

        // CHANGED: Update the array with the new particle position
        vertices[3] = pos_x;
        vertices[4] = pos_y;

        // ADDED: Send the updated positions to the GPU
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        // Draw both particles
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        // CHANGED: We now draw 2 points instead of 1
        glDrawArrays(GL_POINTS, 0, 2); 

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ── Cleanup ───────────────────────────────────────────
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glfwTerminate();
    return 0;
}