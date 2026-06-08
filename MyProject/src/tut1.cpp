// in this tutorial we will learn to set up our first screens in the oepn gl by ourself
#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>


// Be sure to include GLAD before GLFW. The include file for GLAD includes the required OpenGL headers behind the scenes (like GL/gl.h) so be sure to include GLAD before other header files that require OpenGL (like GLFW).

using namespace std;

void size_callback(GLFWwindow* window,int width,int height){

    glViewport(0,0,width,height);
};
int main(){


    // here we will first initialize the GLFW window using these 4 functions
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,4);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

    //HERE THE ifrst one of them is compulsory , glfwInit() helps in starting the library , then other 3 r just the hints, so they tell that we will focus on 4 version of openGl in some way

    // Next we're required to create a window object. This window object holds all the windowing data and is required by most of GLFW's other functions.
    GLFWwindow* window = glfwCreateWindow(800,600,"tut1",NULL,NULL);

    if(window==NULL){
        cout<<"FAIlure to creATE the window!!"<<endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // so above we have created a window object which is by the createwindow function , also we have checked if the window is created or not , then we tell the glfw to make the context of our window!!


    // now we wil introduce the GLAD 
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    // nwo finalllly after enough of the functions associated wityh the glfw and the glad we have a function associated with the openGl , look at it


    // Before we can start rendering we have to do one last thing. We have to tell OpenGL the size of the rendering window so OpenGL knows how we want to display the data and coordinates with respect to the window. We can set those dimensions via the glViewport function:
    glViewport(0,0,800,600);


    // The first two parameters of glViewport set the location of the lower left corner of the window. The third and fourth parameter set the width and height of the rendering window in pixels, which we set equal to GLFW's window size.




    // now think that the user would be again and again re sizing the window acctording to their use so for that we will need a call back function and make it above int main
    glfwSetFramebufferSizeCallback(window,size_callback);



    // We don't want the application to draw a single image and then immediately quit and close the window. We want the application to keep drawing images and handling user input until the program has been explicitly told to stop. For this reason we have to create a while loop, that we now call the render loop, that keeps on running until we tell GLFW to stop. The following code shows a very simple render loop:


    while(!glfwWindowShouldClose(window))
    {
        glfwSwapBuffers(window);
        glfwPollEvents();    
    }

    glfwTerminate();


    return 0;
}