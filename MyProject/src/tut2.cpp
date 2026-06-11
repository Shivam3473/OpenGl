// here we will learn about making a triable in openGl and we have got the code for the screen with the boilerplate named screen
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
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";


// this code for defining the surce code in glsl for the fragment shader and there we have 4d values for defining the color to be seen

const char *fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
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


    // all the above code foused on producing much of the screen now we will learn separately extra code needed to make a triangle

    // since it contains 3 vertices thus we need 3 set of arrays of 3d coordinates

    // OpenGL only processes 3D coordinates when they're in a specific range between -1.0 and 1.0 on all 3 axes (x, y and z). All coordinates within this so called normalized device coordinates range will end up visible on your screen (and all coordinates outside this region won't).

    // Because we want to render a single triangle we want to specify a total of three vertices with each vertex having a 3D position. We define them in normalized device coordinates (the visible region of OpenGL) in a float array



    // overall steps performed -->
    // doat data array---> vertex shader ---> gemoetry shader(optional) --> primitive assembly (which tells the shapes to be made)  --- > rasterization stage --> clipping ---> fragment shader (A fragment in OpenGL is all the data required for OpenGL to render a single pixel.)----> alpa/blending stage





    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f, 0.5f, 0.0f};

    // here these will be passed to the vertex shaders and then will be returned as screen space coordinates and then transformed to fragments as inputs to ur fragment shader


    // also we need to use vertex buffer object to feed this data of vertex to the graphic card for the intant acess to the vertex shaders

    unsigned int VBO;
    // we also need to make vertex array object , generate it and bind it
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &VBO);  
    

    glBindBuffer(GL_ARRAY_BUFFER, VBO); // we need to bind this buffer to its object type which is GL_ARRAY_BUFFER 
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    //glBufferData is a function specifically targeted to copy user-defined data into the currently bound buffer
    // If, for instance, one would have a buffer with data that is likely to change frequently, a usage type of GL_DYNAMIC_DRAW ensures the graphics card will place the data in memory that allows for faster writes.


    
    // now we will code such that we link the vertex attributes and tell how the graphic card must interpret the data before the rendering
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); 
    // here the isrt is the loc=0 , secong argument is the 3 coordiantes for each vertex which is the vec3, then float type values, then gl_false as we dont need to round off the value to the int for the coordiantes, then comes the stride and then the starting position

    
    // now we will start preparing our vertex shader after we r done with vertex shader object
    
    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);// Next we attach the shader source code to the shader object and compile the shader:




 
    // we have copiled to make the vertex buffer object and the vertex shader however we also need to take care if any error occurs on compilation and thus following setup is useful for a warning on compilation error
    int  success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }




    // now define the fragment shader 
    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);


    // code to link both the shaders
    // A shader program object is the final linked version of multiple shaders combined. To use the recently compiled shaders we have to link them to a shader program object and then activate this shader program when rendering objects. The activated shader program's shaders will be used when we issue render calls
    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if(!success) { // showing the message log if the link fails
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        
    }
    
    
    
    


    
    while (!glfwWindowShouldClose(window))
    {

        // ✅ Add these two lines at the top of the loop
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);   // background color
        glClear(GL_COLOR_BUFFER_BIT);             // wipe the previous frame
        
        glUseProgram(shaderProgram); // activaing the link
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);  // finsally the main code to draw a triangle


        glfwSwapBuffers(window);
        glfwPollEvents();
    }



    // ── Cleanup ───────────────────────────────────────────
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);// once after the shader program we r good to go and delete , all the stuff would be managed by this shader program itself
    glDeleteProgram(shaderProgram);
    glDeleteShader(vertexShader); 
    glDeleteShader(fragmentShader); 
    glfwTerminate();
    return 0;
}