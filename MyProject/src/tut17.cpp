// Geometry Shader : Intro
#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

using namespace std;

void size_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
}

// CHANGED: vertex shader now takes vec2 aPos (z=0 implied) + vec3 aColor per-point
// and forwards color to geometry shader via interface block VS_OUT
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec3 aColor;\n"
    "out VS_OUT {\n"
    "    vec3 color;\n"
    "} vs_out;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
    "   vs_out.color = aColor;\n"
    "}\0";

// CHANGED: fragment shader now reads fColor passed down from geometry shader
// instead of a hardcoded orange value
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 fColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(fColor, 1.0f);\n"
    "}\n\0";

// NEW: geometry shader — sits between vertex and fragment shader
// takes each GL_POINT and builds a house shape (triangle_strip, 5 vertices)
// passes per-house color from VS_OUT, and overrides the roof vertex to white (snow)
const char *geometryShaderSource = "#version 330 core\n"
    "layout (points) in;\n"  // taking the point in from the vertex shader
    "layout (triangle_strip, max_vertices = 5) out;\n" // this means it would deal in the triangle strips and max vertexes =5
    "\n"
    "in VS_OUT {\n"
    "    vec3 color;\n"
    "} gs_in[];\n"          // array because GS receives all vertices of a primitive, basically we receive it from the vertex shader and stores it in the array
    "\n"
    "out vec3 fColor;\n" // this is further passed inside the fragment shader 
    "\n"
    "void build_house(vec4 position)\n"
    "{\n"
    "    fColor = gs_in[0].color;\n"           // set house body color from input point, so and yes then passed to the fragment shader 
    "    gl_Position = position + vec4(-0.2, -0.2, 0.0, 0.0);\n"  // 1: bottom-left
    "    EmitVertex();\n" // basically we had a location and we shift it to some extend the with emit function creates it
    "    gl_Position = position + vec4( 0.2, -0.2, 0.0, 0.0);\n"  // 2: bottom-right
    "    EmitVertex();\n"
    "    gl_Position = position + vec4(-0.2,  0.2, 0.0, 0.0);\n"  // 3: top-left
    "    EmitVertex();\n"
    "    gl_Position = position + vec4( 0.2,  0.2, 0.0, 0.0);\n"  // 4: top-right
    "    EmitVertex();\n"
    "    gl_Position = position + vec4( 0.0,  0.4, 0.0, 0.0);\n"  // 5: roof peak
    "    fColor = vec3(1.0, 1.0, 1.0);\n"     // override roof to white (snow)
    "    EmitVertex();\n"
    "    EndPrimitive();\n"
    "}\n"
    "\n"
    "void main()\n"
    "{\n"
    "    build_house(gl_in[0].gl_Position);\n" // finally the function is called and position is passed in as the parameter which we received from the vertex shader
    "}\0";

int main(){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "WindowTitle", NULL, NULL);
    if(window == NULL){
        cout << "Failed to create window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, size_callback);

    // -------------GEOMETRY SHADER HOUSES PRACTICE-----------------------------------

    // CHANGED: each vertex is now vec2 position + vec3 color (5 floats total)
    // 4 points placed at corners of NDC space, each with a distinct color
    float points[] = {
        -0.5f,  0.5f, 1.0f, 0.0f, 0.0f,  // top-left     | red
         0.5f,  0.5f, 0.0f, 1.0f, 0.0f,  // top-right    | green
         0.5f, -0.5f, 0.0f, 0.0f, 1.0f,  // bottom-right | blue
        -0.5f, -0.5f, 1.0f, 1.0f, 0.0f   // bottom-left  | yellow
    };

 

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

    // CHANGED: attrib 0 is now vec2 (position only), stride is 5 floats
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // NEW: attrib 1 carries the RGB color, offset by 2 floats past position
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // NEW: compile the geometry shader, same pattern as vertex/fragment
    unsigned int geometryShader;
    geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geometryShader, 1, &geometryShaderSource, NULL);
    glCompileShader(geometryShader);

    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, geometryShader);  // NEW: attach geometry shader between VS and FS
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    while(!glfwWindowShouldClose(window)){
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        // CHANGED: draw GL_POINTS (not GL_TRIANGLES); geometry shader expands each point into a house
        glDrawArrays(GL_POINTS, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ── Cleanup ───────────────────────────────────────────
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(geometryShader);  // NEW: delete geometry shader too
    glDeleteShader(fragmentShader);
    glfwTerminate();
    return 0;
}