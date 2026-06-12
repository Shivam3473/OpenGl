// Coordinate Systems : Going 3D

// we must know that there exists various coordinate systems , we switch between them using matrices certainly , those are Local space (or Object space), World space, View space (or Eye space), Clip space, Screen space , howver thes etransformations are already perofrmed by the oepnGl and we pass the coordiantes and they finally turn out to be on the screen space wehich is the final stage and are ready to be sent for the rasterization

//Local space --> The vertices of the container we've been using were specified as coordinates between -0.5 and 0.5 with 0.0 as its origin. These are local coordinates. and these coordiantes are in local space

// if something was built and now imported to the world space , so it has to been transformed when model matrix transformation is applied on the local space , this transforms it such that it fits perfectly in the world space

// Clip space --> Here all the coordinates which were out of the bound are clipped , this is done using the projection matrix , in this we can also set our own intuitive coordinates and those r converted back to Ndc with projection matrix. now inside the clip space itself Once all the vertices are transformed to clip space a final operation called perspective division is performed where we divide the x, y and z components of the position vectors by the vector's homogeneous w component; perspective division is what transforms the 4D clip space coordinates to 3D normalized device coordinates. This step is performed automatically at the end of the vertex shader step.

// remember this projection matrix which is used in clip space could be of two types , orthographc projection matrix or the perspective projection matrix

// now let us do our final job in making an object look in 3d , so we construct our model matreix using glm , and this will converet the local space to the world space , in layman terms it rotates the body around x axis , then we use the view matrix which will take it ot he veiw space and then we use the projection matrix which takes it to the clip space and makes add perspective to it .


#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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


    //------------------- defining transformation matrices --------------------------


    // here we will define all the matrices that are used to transform 
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f)); //model matrix to take it to world space(rotates around x axis)

    // now we use the view matrix which tsakesd it the view space , basically in this we want to take the camera utwards so that is positive z direction , now we dont know camera handling yet so we do so by scaling it to the negative z direction

    glm::mat4 view = glm::mat4(1.0f);
    // note that we're translating the scene in the reverse direction of where we want to move
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f)); 

    // now we will prepare a projection matrix(perspective) and this would work to add the 3d effect to the screen and take it to the clip space
    // Its first parameter defines the fov value, that stands for field of view and sets how large the viewspace is. For a realistic view it is usually set to 45 degrees, but for more doom-style results you could set it to a higher value. The second parameter sets the aspect ratio which is calculated by dividing the viewport's width by its height. The third and fourth parameter set the near and far plane of the frustum. We usually set the near distance to 0.1 and the far distance to 100.0. All the vertices between the near and far plane and inside the frustum will be rendered.
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    glm::mat4 trans=projection*view*model;

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