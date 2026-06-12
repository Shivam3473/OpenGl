// Transformations
// in transformations we use sveral stuff in which the most important is the matrix multiplication to the vector which proves to transform them -------> 1. this include the identity matrix multiplication which leaves the vector untpuched . 2. then there is scaling , for non uniform scaling we usually mulrtiply a matrix with only non zero diagonal elelemtns and not equal to 1 , if they were all 1 then it would be the identity itself.
// 3. Translation ----> Translation is the process of adding another vector on top of the original vector to return a new vector with a different position, thus moving the vector based on a translation vector. We've already discussed vector addition so this shouldn't be too new. Basically to do this , we talk an identity matrix but an alteration that top 3 va;ues of its 4th col are T_x,T_y,T_z
// 4. Rotation ----> Here when we talk about rotation then for that too , we will be having the special sort of matrices whuch are present on opengl documentagion that when ultiplied to the array will ro tate them around some x, y , z or even around some arbitrary axis
// 5. Combining matrices -> here we place all the matrices using which we need to collectively transofrm our array from right to left, it is advisedd to always go in this order of priorities to the matrices to be kept from right to left, that is scaling then rpotation then translkation and then multiply them to get the final matrix which needs to e further multiplied


#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// these libraries based on GLM need to  be included any how , they r important for dealing with all this matrix and vector related stuff
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// this has to be written for vertex shaders, this is very important to note that o add te transformation we have to edit the verte shader and add a uniform mat4 variable which we update as we prepare the transformation matrix , and this we use in the gl_positionn for the live location
const char *vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"
                                "layout (location = 1) in vec2 aTexCoord;\n" // Added ; and \n
                                 "out vec2 TexCoord;\n"
                                 "uniform mat4 transform;" // this is very important line to work with the transforms
                                 "void main()\n"
                                 "{\n"
                                 "  gl_Position = transform * vec4(aPos, 1.0f);"
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

using namespace std;

void size_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
}

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




    // this is the very example of the translation which is done suign the glm
    /*
    glm::vec4 vec(1.0f, 0.0f, 0.0f, 1.0f); // vector defined to be translated 
    glm::mat4 trans = glm::mat4(1.0f);     // identity matrix created 
    trans = glm::translate(trans, glm::vec3(1.0f, 1.0f, 0.0f));  // translation matrix created 
    vec = trans * vec;   // finally the output achieved
    std::cout << vec.x << vec.y << vec.z << std::endl; */



    // another example of transformation could be this where we r technically forst scaling it and then rotating it 
    glm::mat4 trans = glm::mat4(1.0f);
    trans = glm::rotate(trans, glm::radians(90.0f), glm::vec3(0.0, 0.0, 1.0));
    trans = glm::scale(trans, glm::vec3(0.5, 0.5, 0.5));  


    float texCoords[] = {
        0.0f, 0.0f, // lower-left corner
        1.0f, 0.0f, // lower-right corner
        0.5f, 1.0f  // top-center corner
    };
    int width, height, nrChannels;
    unsigned char *data = stbi_load("D:/IMPORTANT FOLDERS/CGS RND/OpenGl/MyProject/wall.jpg", &width, &height, &nrChannels, 0); 
    unsigned int texture;
    glGenTextures(1, &texture);   // 1 texture to be created
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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

 
        // here this is very important to be added basically here we update the value of the unuiform we created above inside the vertex shader
        unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));


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