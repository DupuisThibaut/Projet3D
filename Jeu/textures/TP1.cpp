// Include standard headers
#include <GL/glew.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <unistd.h>

// Include GLEW

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

using namespace glm;

#include <common/shader.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
#include <common/texture.hpp>
#include <common/text2D.hpp>

#include "scene.hpp"
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
// #include "stb_image.h"
#include <common/stb_image.h>
#endif

void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
glm::vec3 camera_position   = glm::vec3(0.0f, 0.0f,  40.0f);
glm::vec3 camera_target = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camera_up    = glm::vec3(0.0f, 1.0f,  0.0f);

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

//rotation
float angle = 0.;
float zoom = 1.;
float cameraS=2.5;
bool modeCamera=true;
int resolution=16;
float currentOrbitalPos = 0.0f;
float orbitalSpeed = 1.0f;
float orbitalRadius = 3.0f;
/*******************************************************************************/


int scaleSurface=20;
void dessineSurfacePlane(std::vector<unsigned short> &indices, std::vector<std::vector<unsigned short> > &triangles, std::vector<glm::vec3> &indexed_vertices, int nb, std::vector<glm::vec2> &UV){
    indices.clear();triangles.clear();indexed_vertices.clear();indexed_vertices.resize(0);
    std::vector<unsigned short> t(3);
    glm::vec3 iv;
    glm::vec2 uv;
    iv[2]=0;
    // int width=1,height=1;
    float m=1.0f/(nb-1);
    for(float i=0;i<=1;i+=m){
        iv[0]=10-scaleSurface*i;uv[0]=1.0-i;
        for(float j=0;j<=1;j+=m){
            iv[1]=10-scaleSurface*j;uv[1]=1.0-j;
            //iv[2]=100*static_cast <float> (rand()) / static_cast <float> (RAND_MAX)/10.;
            //Trouvé sur stackOverflow pour générer un nombre aléatoire entre 0.0 et 1.0
            indexed_vertices.push_back(iv);
            UV.push_back(uv);
        }
    }
    for(unsigned short i=1;i<nb;i++){
        for(unsigned short j=0;j<nb-1;j++){
            t[0]=i*nb+j;
            t[1]=(i-1)*nb+j+1;
            t[2]=(i-1)*nb+j;
            triangles.push_back(t);
            indices.push_back(t[0]);
            indices.push_back(t[1]);
            indices.push_back(t[2]);
            t[2]=i*nb+j+1;
            triangles.push_back(t);
            indices.push_back(t[0]);
            indices.push_back(t[1]);
            indices.push_back(t[2]);
        }
    }
}

void lireHeightMap(std::string filename, std::vector<glm::vec3> &indexed_vertices, std::vector<glm::vec2> &UV){
    int width, height, numComponents;
    unsigned char * data = stbi_load (filename.c_str (),
                                    &width,
                                    &height,
                                    &numComponents, 
                                    0);
    for(int i=0;i<UV.size();i++){
        int x = static_cast<int>(UV[i][0] * (width - 1));
        int y = static_cast<int>((1.0f - UV[i][1]) * (height - 1));
        int index = (y * width + x) * numComponents;
        // std::cout<<(int)data[index]/256.<<std::endl;
        indexed_vertices[i][2]+=((float)data[index]/256.*20.);
        // std::cout<<indexed_vertices[i][2]<<std::endl;
    }
    stbi_image_free(data);
}

// GLuint texture(const std::string & filename){
//     GLuint texID;
//     int width, height, numComponents;
//     unsigned char * data = stbi_load (filename.c_str (),
//                                     &width,
//                                     &height,
//                                     &numComponents, 
//                                     0);
    
    
//     glGenTextures (1, &texID);
//     glBindTexture (GL_TEXTURE_2D, texID);
//     glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//     glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//     glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//     glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//     glTexImage2D (GL_TEXTURE_2D,
//                 0,
//                 (numComponents == 1 ? GL_RED : numComponents == 3 ? GL_RGB : GL_RGBA), 
//                 width,
//                 height,
//                 0,
//                 (numComponents == 1 ? GL_RED : numComponents == 3 ? GL_RGB : GL_RGBA), 
//                 GL_UNSIGNED_BYTE,
//                 data);
//     glGenerateMipmap(GL_TEXTURE_2D);
//     stbi_image_free(data);
//     glBindTexture (GL_TEXTURE_2D, 0);
//     return texID;
// }

int main( void )
{
    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        getchar();
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow( 1024, 768, "TP1 - GLFW", NULL, NULL);
    if( window == NULL ){
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    //  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, 1024/2, 768/2);

    // Dark blue background
    glClearColor(0.8f, 0.8f, 0.8f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Cull triangles which normal is not towards the camera
    //glEnable(GL_CULL_FACE);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    // GLuint programID = LoadShaders( "vertex_shader.glsl", "fragment_shader.glsl" );

    /*****************TODO***********************/
    // Get a handle for our "Model View Projection" matrices uniforms

    /****************************************/
    std::vector<unsigned short> indices; //Triangles concaténés dans une liste
    std::vector<std::vector<unsigned short> > triangles;
    std::vector<glm::vec3> indexed_vertices;
    std::vector<glm::vec2> UV;

    //Chargement du fichier de maillage
    // std::string filename("chair.off");
    // loadOFF(filename, indexed_vertices, indices, triangles );
    // indexed_vertices.resize(3);
    // indexed_vertices[0]=glm::vec3(0,0,0);
    // indexed_vertices[1]=glm::vec3(1,0,0);
    // indexed_vertices[2]=glm::vec3(0,1,0);
    // triangles.resize(3);
    // indices.resize(3);
    // indices[0]=0;
    // indices[1]=1;
    // indices[2]=2;
    // std::vector<unsigned short> t(3);
    // t[0]=0;
    // t[0]=1;
    // t[0]=2;
    // triangles[0]=t;
    // std::cout<<"i"<<std::endl;
    // std::cout<<"indices :"<<indices[0]<<std::endl;
    // std::cout<<"triangles :"<<triangles[0][0]<<triangles[0][1]<<triangles[0][2]<<std::endl;
    // std::cout<<"indexed_vertices :"<<indexed_vertices[0][0]<<indexed_vertices[0][1]<<indexed_vertices[0][2]<<std::endl;
    dessineSurfacePlane(indices,triangles,indexed_vertices,resolution,UV);lireHeightMap("../textures/i.png",indexed_vertices,UV);
    Scene scene;
    GLuint programID = LoadShaders( "vertex_shader.glsl", "fragment_shader.glsl" );
    // GLuint programID=scene.programID;
    scene.programID=programID;

    scene.objets.programID=programID;
    GameObject a,b;
    Mesh a1("../meshes/suzanne.off","../textures/grass.png"),b1("../meshes/suzanne.off","../textures/rock.png");
    a.setTransform(Transform(glm::mat3x3(),glm::vec3(2.0,0.0,0.0)));
    b.setTransform(Transform(glm::mat3x3(),glm::vec3(-2.0,0.0,0.0)));
    a.setMesh(a1);
    b.setMesh(b1);
    // a.setMesh("./suzanne.off");
    // b.setMesh("./chair.off");
    // objets.addEnfant(a);
    // std::cout<<objets.enfants[0].mesh.vertices.size()<<std::endl;
    // objets.addEnfant(b);
    GameObject c,d,e;
    Mesh c1("../meshes/sphere.off","../textures/s2.ppm"),d1("../meshes/sphere.off","../textures/s1.ppm"),e1("../meshes/sphere.off","../textures/2k_moon.jpg");
    c.setTransform(Transform(glm::mat3x3(1.0),glm::vec3(0.0,0.0,35.0),1.0));
    // d.setTransform(Transform(glm::mat3x3(),glm::vec3(0.0,-2.0,0.0),2.0));
    // e.setTransform(Transform(glm::mat3x3(),glm::vec3(0.0,0.0,2.0),1.0));
    c.setMesh(c1);
    d.setMesh(d1);
    e.setMesh(e1);
    Transform scale = d.transform.scale(5.0);
    d.setEspace(Transform(glm::mat3x3(1.0),glm::vec3(0.0,0.0,0.0),1.0));
    d.setTransform(scale);
    Transform inclinaison = d.transform.rotation(glm::vec3(0.0,0.0,1.0),23.44);
    Transform Distance = d.espace.translation(glm::vec3(1.0,0.0,0.0),10);
    scale = d.transform.scale(2.0);
    d.setEspace(Distance);
    d.setTransform(inclinaison.combineWith(scale));
    inclinaison = e.transform.rotation(glm::vec3(0.0,0.0,1.0),6.68);
    Transform inclinaisonAxe = e.espace.rotation(glm::vec3(0.0,0.0,1.0),5.14);  
    Distance = e.espace.translation(glm::vec3(1.0,0.0,0.),-3.0);
    Transform tmp = e.espace.combineWith(inclinaisonAxe).combineWith(Distance);
    e.setEspace(tmp);
    e.setTransform(inclinaison);
    // scene.objets.addEnfant(&c);
    // c.addEnfant(&d);
    // d.addEnfant(&e);

    GameObject f;
    Mesh f1;
    f1.setMesh(indexed_vertices,indices,triangles,UV);
    f1.setScale(scaleSurface);
    f1.ajouterTexture("../textures/i.png","TextHeight");
    f1.ajouterTexture("../textures/grass.png","TextHerbe");
    f1.ajouterTexture("../textures/rock.png","TextRoche");
    f1.ajouterTexture("../textures/snowrocks.png","TextNeige");
    f1.creerTextures(programID);
    f1.updateAreaAndNormal();
    f.setMesh(f1);
    // scene.objets.addEnfant(&f);
    // loadOFF("suzanne.off",scene.objets.vertices,scene.objets.indices,scene.objets.triangles);
    // GLuint texIDHeight=texture("./i.png");
    // GLuint texIDHerbe=texture("./grass.png");
    // GLuint texIDRoche=texture("./rock.png");
    // GLuint texIDNeige=texture("./snowrocks.png");
    // GLuint vertexbuffer;GLuint uvbuffer;GLuint elementbuffer;
    

    // Load it into a VBO

    
    // glGenBuffers(1, &vertexbuffer);
    // glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    // glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);


    
    // glGenBuffers(1, &uvbuffer);
    // glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    // glBufferData(GL_ARRAY_BUFFER, UV.size() * sizeof(glm::vec2), &UV[0], GL_STATIC_DRAW);

    
    // // Generate a buffer for the indices as well
    
    // glGenBuffers(1, &elementbuffer);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0] , GL_STATIC_DRAW);
    

    // Get a handle for our "LightPosition" uniform
    // glUseProgram(programID);
    // GLuint Text2DUniformIDHeight = glGetUniformLocation( programID, "TextHeight" );
    // GLuint Text2DUniformIDHerbe = glGetUniformLocation( programID, "TextHerbe" );
    // GLuint Text2DUniformIDRoche = glGetUniformLocation( programID, "TextRoche" );
    // GLuint Text2DUniformIDNeige = glGetUniformLocation( programID, "TextNeige" );

    Mesh Mchateau;GameObject GOchateau;
    // Mchateau.setMeshOBJ("../meshes/Peaches Castle.obj");
    // Mchateau.setScale(0.1);
    // GOchateau.setMesh(Mchateau);
    scene.lireOBJ("../meshes/Peaches_Castle.obj", &GOchateau);
    GOchateau.setTransform(Transform(glm::mat3x3(1.0),glm::vec3(0.0,0.0,0.0),1.0));
    GOchateau.setEspace(Transform(glm::mat3x3(1.0),glm::vec3(0.0,0.0,0.0),1.0));
    scene.objets.addEnfantOBJ(&GOchateau);
    // scene.lireMTL("../meshes/Peaches Castle.mtl");
    // std::cout<<scene.mtls.size()<<std::endl;



    GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");



    // For speed computation
    double lastTime = glfwGetTime();
    int nbFrames = 0;

    do{

        
        
        

        // Measure speed
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use our shader
        glUseProgram(programID);


        /*****************TODO***********************/
        // Model matrix : an identity matrix (model will be at the origin) then change

        // View matrix : camera/view transformation lookat() utiliser camera_position camera_target camera_up

        // Projection matrix : 45 Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units

        // Send our transformation to the currently bound shader,
        // in the "Model View Projection" to the shader uniforms
        glm::mat4 view;
        if(modeCamera)view=glm::lookAt(camera_position,camera_target,camera_up);
        else {
            //view=glm::mat4(1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.);
            currentOrbitalPos += deltaTime * orbitalSpeed;
            float camX = sin(currentOrbitalPos) * orbitalRadius;
            float camY = orbitalRadius;
            float camZ = cos(currentOrbitalPos) * orbitalRadius;
            view = glm::lookAt(glm::vec3(camZ,camX,camY),glm::vec3(0.0f, 0.0f, 0.0f),glm::vec3(0.0f, 0.0f, 1.0f));
        }
        glm::mat4 projection=glm::perspective(45.f, 4.f/3.f, 0.1f, 100.f);
        glm::mat4 model=glm::mat4(1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.);
        GLuint ModelUni = glGetUniformLocation(programID, "Model");
        GLuint ViewUni = glGetUniformLocation(programID, "View");
        GLuint ProjectionUni = glGetUniformLocation(programID, "Projection");
        glUniformMatrix4fv(ModelUni, 1, GL_FALSE, &model[0][0]);
        glUniformMatrix4fv(ViewUni, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(ProjectionUni, 1, GL_FALSE, &projection[0][0]);


        /****************************************/
        // std::cout<<"dessine !"<<std::endl;
        scene.draw(deltaTime,window);



        // // std::cout<<"i"<<std::endl;
        // // 1rst attribute buffer : vertices
        // glEnableVertexAttribArray(0);
        // glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        // glVertexAttribPointer(
        //             0,                  // attribute
        //             3,                  // size
        //             GL_FLOAT,           // type
        //             GL_FALSE,           // normalized?
        //             0,                  // stride
        //             (void*)0            // array buffer offset
        //             );

        // // Index buffer
        // glEnableVertexAttribArray(1);
        // glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        // glVertexAttribPointer(
        //             1,                  // attribute
        //             2,                  // size
        //             GL_FLOAT,           // typeglTe
        //             GL_FALSE,           // normalized?
        //             0,                  // stride
        //             (void*)0            // array buffer offset
        //             );

        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_2D, texIDHeight);
        // glUniform1i(Text2DUniformIDHeight, 0);
        // glActiveTexture(GL_TEXTURE1);
        // glBindTexture(GL_TEXTURE_2D, texIDHerbe);
        // glUniform1i(Text2DUniformIDHerbe, 1);
        // glActiveTexture(GL_TEXTURE2);
        // glBindTexture(GL_TEXTURE_2D, texIDRoche);
        // glUniform1i(Text2DUniformIDRoche, 2);
        // glActiveTexture(GL_TEXTURE3);
        // glBindTexture(GL_TEXTURE_2D, texIDNeige);
        // glUniform1i(Text2DUniformIDNeige, 3);


        // // Draw the triangles !
        // glDrawElements(
        //             GL_TRIANGLES,      // mode
        //             indices.size(),    // count
        //             GL_UNSIGNED_SHORT,   // type
        //             (void*)0           // element array buffer offset
        //             );
        // glDisableVertexAttribArray(1);
        // glDisableVertexAttribArray(0);


        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0 );

    // Cleanup VBO and shader
    // glDeleteBuffers(1, &vertexbuffer);
    // glDeleteBuffers(1, &uvbuffer);
    // glDeleteBuffers(1, &elementbuffer);
    glDeleteProgram(programID);
    glDeleteVertexArrays(1, &VertexArrayID);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    //Camera zoom in and out
    float cameraSpeed = cameraS * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera_position += cameraSpeed * camera_target;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera_position -= cameraSpeed * camera_target;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera_position += cameraSpeed * glm::vec3(-1.0f, 0.0f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera_position -= cameraSpeed * glm::vec3(-1.0f, 0.0f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS){
        cameraS+=0.1;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS){
        if(cameraS>0.0)cameraS-=0.1;
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS){
        modeCamera=!modeCamera;
    }
    if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS){
        resolution++;
        std::cout<<resolution<<std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS){
        if(resolution>1)resolution--;
        std::cout<<resolution<<std::endl;
    }

    //TODO add translations

    

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
