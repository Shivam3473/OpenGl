//Shaders: Vectors/in/out
#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

// Shaders are written in the C-like language GLSL. GLSL is tailored for use with graphics and contains useful features specifically targeted at vector and matrix manipulation.
// Shaders always begin with a version declaration, followed by a list of input and output variables, uniforms and its main function. Each shader's entry point is at its main function where we process any input variables and output the results in its output variables. Don't worry if you don't know what uniforms are, we'll get to those shortly.
//So if we want to send data from one shader to the other we'd have to declare an output in the sending shader and a similar input in the receiving shader. When the types and the names are equal on both sides OpenGL will link those variables together and then it is possible to send data between shaders (this is done when linking a program object). 




using namespace std;

void size_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
}
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "out vec4 vertexColor;"  // here we have use the out so that this variable is available to be used in fragment shader as well
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "vertexColor = vec4(0.5, 0.0, 0.0, 0.5); "
    "}\0";


// this code for defining the surce code in glsl for the fragment shader and there we have 4d values for defining the color to be seen

const char *fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec4 vertexColor;" // thus we have "in" for the vertexcolor which was defined above in the vertex shader glsl
"void main()\n"
"{\n"
"   FragColor = vertexColor;\n"
"}\n\0";
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

    // -------------RECTANGLE PRACTICE-----------------------------------

   
    // a better solution is to store only the unique vertices and then specify the order at which we want to draw these vertices in. In that case we would only have to store 4 vertices for the rectangle, and then just specify at which order we'd like to draw them.


    float vertices[] = {
     0.5f,  0.5f, 0.0f,  // top right
     0.5f, -0.5f, 0.0f,  // bottom right
    -0.5f, -0.5f, 0.0f,  // bottom left
    -0.5f,  0.5f, 0.0f   // top left 
};
    unsigned int indices[] = {  // note that we start from 0!
    0, 1, 3,   // first triangle
    1, 2, 3    // second triangle
};  
    // WE use the element buffer objects to decide that even with 4 vertices and providing the order in which to draw those vertices we can typically work with the rectangle woithout either creating a triangle
    unsigned int EBO;
    glGenBuffers(1, &EBO);
    




    unsigned int VBO,VAO;
    glGenVertexArrays(1,&VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1,&VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // rememwber for he element buffer object it needs to bounded aftwer the bao is bounded , overall do it before the veretx shader
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); 

    
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

    while(!glfwWindowShouldClose(window)){
        // ✅ Add these two lines at the top of the loop
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);   // background color
        glClear(GL_COLOR_BUFFER_BIT);             // wipe the previous frame
        
        glUseProgram(shaderProgram); // activaing the link
        glBindVertexArray(VAO);
        // simialrly hee this time instead of using the draw arrays function we need tot use the draw elelements
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
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