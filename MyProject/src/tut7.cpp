// Textures --> A texture is a 2D image (even 1D and 3D textures exist) used to add detail to an object; think of a texture as a piece of paper with a nice brick image (for example) on it neatly folded over your 3D house so it looks like your house has a stone exterior. Because we can insert a lot of detail in a single image, we can give the illusion the object is extremely detailed without having to specify extra vertices.

// In order to map a texture to the triangle we need to tell each vertex of the triangle which part of the texture it corresponds to. Each vertex should thus have a texture coordinate associated with them that specifies what part of the texture image to sample from. Fragment interpolation then does the rest for the other fragments.

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>


// stb_image.h is a very popular single header image loading library by Sean Barrett that is able to load most popular file formats and is easy to integrate in your project(s). stb_image.h can be downloaded , thus we can include and load the images to add them and include as textures , so we have this file which we downlaoded from opengl documentation and including it as a header file wold help us to convert the image to pixel byte arrays, then we use various functions wo load this and do the job
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


using namespace std;

void size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}
// this has to be written for vertex shaders
const char *vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"
                                "layout (location = 1) in vec2 aTexCoord;\n" // Added ; and \n
                                 "out vec2 TexCoord;\n"
                                 "void main()\n"
                                 "{\n"
                                 "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);"
                                 "TexCoord = aTexCoord;"
                                 "}\0";

// this code for defining the surce code in glsl for the fragment shader and there we have 4d values for defining the color to be seen

const char *fragmentShaderSource = "#version 330 core\n"
                                   "out vec4 FragColor;\n"
                                   "in vec2 TexCoord;"  // thus here we have "in" the texture coordinates from the vertex shader souce code glsl
                                   "uniform sampler2D ourTexture;" // yha we had to make a uniform and sampler2d is a glsl stype and we later pass n our texture object to it thorugh it
                                   "void main()\n"
                                   "{\n"
                                   "FragColor = texture(ourTexture, TexCoord);"

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

    

    // We specify 3 texture coordinate points for the triangle. We want the bottom-left side of the triangle to correspond with the bottom-left side of the texture so we use the (0,0) texture coordinate for the triangle's bottom-left vertex. The same applies to the bottom-right side with a (1,0) texture coordinate. The top of the triangle should correspond with the top-center of the texture image so we take (0.5,1.0) as its texture coordinate

    float texCoords[] = {
        0.0f, 0.0f, // lower-left corner
        1.0f, 0.0f, // lower-right corner
        0.5f, 1.0f  // top-center corner
    };

    //To load an image using stb_image.h we use its stbi_load function
    int width, height, nrChannels;
    unsigned char *data = stbi_load("D:/IMPORTANT FOLDERS/CGS RND/OpenGl/MyProject/wall.jpg", &width, &height, &nrChannels, 0); 

    // now like any previopus object we looked at like vertex buffer or vertex array pbject , these textures are also ojects so here is the code to create them
    unsigned int texture;
    glGenTextures(1, &texture);   // 1 texture to be created
    glBindTexture(GL_TEXTURE_2D, texture);

    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    // now we use the image data loaded and use it to create the textures after they are bounded
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);// here we had loaded the data paprameters above from the image separately and created the texture object of a certain type in which we will assocate the value of that data from the image , so when that is done we will define the use of tex coordinates for deployment, here 0 stands for 0 mipmap level , and rgp is the format
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }

    stbi_image_free(data);


    // here itself in the coordiantes as earlier when using the color variation with the position we also alotted the color code here , simialrly we will allot the texture coordinates here itself and update the loc = 1 in the fragment shader which will be used with the vertex attribute array later 
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,0.0f, 0.0f,
        0.5f, -0.5f, 0.0f,1.0f, 0.0f,
        0.0f, 0.5f, 0.0f,0.5f, 1.0f };


    unsigned int VBO;
    // we also need to make vertex array object , generate it and bind it
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO); // we need to bind this buffer to its object type which is GL_ARRAY_BUFFER
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


    // so here we have set the vertex atteribute accordingly since we also had the texture to me looked on
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);  

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
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
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
    { // showing the message log if the link fails
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    }

    while (!glfwWindowShouldClose(window))
    {

        // ✅ Add these two lines at the top of the loop
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // background color
        glClear(GL_COLOR_BUFFER_BIT);         // wipe the previous frame

        glUseProgram(shaderProgram); // activaing the link
        glBindTexture(GL_TEXTURE_2D, texture); // texture bind is essential at the last before the VAO bind


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