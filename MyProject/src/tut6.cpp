// Shaders : Changing color with the locatiion inside the figure
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace std;

void size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}
// this has to be written for vertex shaders
const char *vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 1) in vec3 aColor;" // the color variable has attribute position 1 is done for var color input as descibed in the vertices array made
                                 "layout (location = 0) in vec3 aPos;\n"
                                 "out vec3 ourColor;" // output a color to the fragment shader
                                 "void main()\n"
                                 "{\n"
                                 "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);"
                                 "ourColor = aColor;" // set ourColor to the input color we got from the vertex data
                                 "}\0";

// this code for defining the surce code in glsl for the fragment shader and there we have 4d values for defining the color to be seen

const char *fragmentShaderSource = "#version 330 core\n"
                                   "out vec4 FragColor;\n"
                                   "in vec3 ourColor;" // thus used that with "in" functionality
                                   "void main()\n"
                                   "{\n"
                                   "   FragColor = vec4(ourColor, 1.0f);\n"
                                   "}\n\0";

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 600, "WindowTitle", NULL, NULL);
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

    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, size_callback);

    // here we will look how we can form a triangle with the colors changing with the location , for that in the vertices we dont only give the three parameters fr each vertex but also the color codes in rgf belonging from 0->1, now to make it work , we have to set the location=1 for the acolor variable made in vertex shader source code glsl, then pass it to the fragment shader with in and out functionality, so change in vertex shader source code, change in verteces array and a change in vertex attribute pointer 

    float vertices[] = {
        // positions         // colors
        0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // bottom left
        0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f    // top
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);



    // position attribute , so all the change is seen next on the vertex attribute pointer, look very important as abov we used in the source code, the location to be 0 or 1 thus here it has been used as the first argument and others anre manipulated logically
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);



    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    while (!glfwWindowShouldClose(window))
    {
        // ✅ Add these two lines at the top of the loop
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // background color
        glClear(GL_COLOR_BUFFER_BIT);         // wipe the previous frame

        glUseProgram(shaderProgram); // activaing the link
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3); // finsally the main code to draw a triangle

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ── Cleanup ───────────────────────────────────────────
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO); // once after the shader program we r good to go and delete , all the stuff would be managed by this shader program itself
    glDeleteProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glfwTerminate();
    return 0;
}