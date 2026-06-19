// here we extend the triangle code to generate a sphere mesh procedurally (UV sphere using stacks and sectors) and draw it with an index buffer
#include <iostream>
#include <cmath>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace std;

void size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// position is already in NDC range, and since the sphere is centered at the origin
// the position vector IS the surface normal (just normalize it) — no extra normal buffer needed
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "out vec3 Normal;\n"
    "void main()\n"
    "{\n"
    "   Normal = normalize(aPos);\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";

// basic Lambert shading: brightness = how much the surface normal faces the light
// this is what actually sells the curvature — flat fill alone is just a colored circle
const char *fragmentShaderSource = "#version 330 core\n"
"in vec3 Normal;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   vec3 lightDir = normalize(vec3(0.3, 0.4, 1.0));\n" // light roughly from the viewer's side
"   float diff = max(dot(Normal, lightDir), 0.0);\n"
"   vec3 baseColor = vec3(1.0, 0.5, 0.2);\n"
"   vec3 result = baseColor * (0.25 + 0.75 * diff);\n" // 0.25 ambient floor so the dark side isn't pure black
"   FragColor = vec4(result, 1.0);\n"
"}\n\0";

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 800, "WindowTitle", NULL, NULL); // square window — NDC x/y map 1:1, so a non-square viewport stretches the sphere into an oval
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


    // a sphere can't be hand-typed like the triangle's 3 verts, so we generate it procedurally as a UV sphere
    // idea: walk stackCount horizontal rings from the top pole to the bottom pole (like latitude lines),
    // and on each ring place sectorCount points around it (like longitude lines)
    // each ring point = (radius*cos(stackAngle)*cos(sectorAngle), radius*cos(stackAngle)*sin(sectorAngle), radius*sin(stackAngle))

    const float radius = 0.5f;
    const unsigned int sectorCount = 36; // longitude divisions
    const unsigned int stackCount = 18;  // latitude divisions
    const float PI = 3.14159265359f;

    vector<float> vertices;
    vector<unsigned int> indices;

    float sectorStep = 2 * PI / sectorCount;
    float stackStep = PI / stackCount;

    for (unsigned int i = 0; i <= stackCount; ++i)
    {
        float stackAngle = PI / 2 - i * stackStep; // from +90deg (top pole) to -90deg (bottom pole)
        float xy = radius * cosf(stackAngle);      // radius of the ring at this stack
        float z = radius * sinf(stackAngle);

        for (unsigned int j = 0; j <= sectorCount; ++j)
        {
            float sectorAngle = j * sectorStep; // from 0 to 2pi

            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    // now stitch the rings together into triangles using indices
    // this is why we need an EBO this time, unlike the triangle which only used a VBO
    for (unsigned int i = 0; i < stackCount; ++i)
    {
        unsigned int k1 = i * (sectorCount + 1); // start index of current ring
        unsigned int k2 = k1 + sectorCount + 1;  // start index of next ring

        for (unsigned int j = 0; j < sectorCount; ++j, ++k1, ++k2)
        {
            // 2 triangles per sector except at the poles, which only need 1
            if (i != 0)
            {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            if (i != (stackCount - 1))
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }


    // VAO and VBO same role as before, plus an EBO to hold the index data
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // same attribute layout as the triangle: location 0, 3 floats per vertex, tightly packed
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
    }


    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);


    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }


    // depth test matters now: triangles on the far side of the sphere must not draw over the near side
    glEnable(GL_DEPTH_TEST);

    // cull the inward-facing triangles of the far hemisphere — pure optimization here since depth
    // test already handles correctness, but it's the right habit once meshes get denser
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // solid fill now that shading exists to convey curvature — wireframe was only a stand-in for that
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear depth buffer this time

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0); // indices instead of glDrawArrays

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    // ── Cleanup ───────────────────────────────────────────
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glfwTerminate();
    return 0;
}