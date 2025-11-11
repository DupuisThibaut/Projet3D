#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <Camera.h>
#include <stb_image.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>

// -------------------------------------------
// gMini : a minimal OpenGL/GLUT application
// for 3D graphics.
// Copyright (C) 2006-2008 Tamy Boubekeur
// All rights reserved.
// -------------------------------------------

// -------------------------------------------
// Disclaimer: this code is dirty in the
// meaning that there is no attention paid to
// proper class attribute access, memory
// management or optimisation of any kind. It
// is designed for quick-and-dirty testing
// purpose.
// -------------------------------------------


#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <Vec3.h>
#include <Scene.h>
#include <GL/glut.h>

#include <matrixUtilities.h>

using namespace std;

#include <imageLoader.h>

#include <Material.h>


// -------------------------------------------
// OpenGL/GLUT application code.
// -------------------------------------------

static GLint window;
static unsigned int SCREENWIDTH = 800;
static unsigned int SCREENHEIGHT = 800;
static Camera camera;
static bool mouseRotatePressed = false;
static bool mouseMovePressed = false;
static bool mouseZoomPressed = false;
static int lastX=0, lastY=0, lastZoom=0;
static unsigned int FPS = 0;
static bool fullScreen = false;

std::vector<Scene> scenes;
unsigned int selected_scene;

std::vector< std::pair< Vec3 , Vec3 > > rays;

static GLuint computeProg = 0;
static GLuint quadProg = 0;
static GLuint texture = 0;
static const unsigned int TEXTURE_WIDTH = 512;
static const unsigned int TEXTURE_HEIGHT = 512;

GLuint debugBuffer;

void printUsage () {
    cerr << endl
         << "gMini: a minimal OpenGL/GLUT application" << endl
         << "for 3D graphics." << endl
         << "Author : Tamy Boubekeur (http://www.labri.fr/~boubek)" << endl << endl
         << "Usage : ./gmini [<file.off>]" << endl
         << "Keyboard commands" << endl
         << "------------------" << endl
         << " ?: Print help" << endl
         << " w: Toggle Wireframe Mode" << endl
         << " g: Toggle Gouraud Shading Mode" << endl
         << " f: Toggle full screen mode" << endl
         << " <drag>+<left button>: rotate model" << endl
         << " <drag>+<right button>: move model" << endl
         << " <drag>+<middle button>: zoom" << endl
         << " q, <esc>: Quit" << endl << endl;
}

void usage () {
    printUsage ();
    exit (EXIT_FAILURE);
}


// ------------------------------------
void initLight () {

    GLfloat light_position[4] = {0.0, 1.5, 0.0, 1.0};
    GLfloat color[4] = { 1.0, 1.0, 1.0, 1.0};
    GLfloat ambient[4] = { 1.0, 1.0, 1.0, 1.0};
    
    std::cout<<"light"<<std::endl;
    // glEnable(GL_DEPTH_TEST);
    // glEnable(GL_LIGHTING);
    // glEnable(GL_LIGHT1);
    glLightfv (GL_LIGHT1, GL_POSITION, light_position);
    glLightfv (GL_LIGHT1, GL_DIFFUSE, color);
    glLightfv (GL_LIGHT1, GL_SPECULAR, color);
    glLightModelfv (GL_LIGHT_MODEL_AMBIENT, ambient);
    glEnable (GL_LIGHT1);
    glEnable (GL_LIGHTING);
}

void init () {
    camera.resize (SCREENWIDTH, SCREENHEIGHT);
    initLight ();
    std::cout<<"init"<<std::endl;
    //glCullFace (GL_BACK);
    glDisable (GL_CULL_FACE);
    glDepthFunc (GL_LESS);
    glEnable (GL_DEPTH_TEST);
    glClearColor (0.2f, 0.2f, 0.3f, 1.0f);
}


// ------------------------------------
// Replace the code of this
// functions for cleaning memory,
// closing sockets, etc.
// ------------------------------------

void clear () {

}

// ------------------------------------
// Replace the code of this
// functions for alternative rendering.
// ------------------------------------



unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}


void draw () {
    glEnable(GL_LIGHTING);
    scenes[selected_scene].draw();

    // draw rays : (for debug)
    //  std::cout << rays.size() << std::endl;
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glLineWidth(6);
    glColor3f(1,0,0);
    glBegin(GL_LINES);
    for( unsigned int r = 0 ; r < rays.size() ; ++r ) {
        glVertex3f( rays[r].first[0],rays[r].first[1],rays[r].first[2] );
        glVertex3f( rays[r].second[0], rays[r].second[1], rays[r].second[2] );
    }
    glEnd();
}

// void display () {
//     glLoadIdentity ();
//     glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//     camera.apply ();
//     draw ();
//     glFlush ();
//     glutSwapBuffers ();
// }

void display () {
    const unsigned int LOCAL_X = 16;
    const unsigned int LOCAL_Y = 16;
    unsigned int groups_x = (TEXTURE_WIDTH  + LOCAL_X - 1) / LOCAL_X;
    unsigned int groups_y = (TEXTURE_HEIGHT + LOCAL_Y - 1) / LOCAL_Y;

    // calculer et uploader les matrices + cam pos depuis la Camera actuelle
    camera.apply(); // met à jour les matrices fixes
    GLfloat mv[16], proj[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    glGetFloatv(GL_PROJECTION_MATRIX, proj);
    glm::mat4 view = glm::make_mat4(mv);      // glm::make_mat4 est column-major compatible
    glm::mat4 projM = glm::make_mat4(proj);
    glm::mat4 invVP = glm::inverse(projM * view);

    Vec3 cpos;
    camera.getPos(cpos); // existing method in Camera.h
    glm::vec3 camPosGLM(cpos[0], cpos[1], cpos[2]);

    glUseProgram(computeProg);
    // upload uniforms every frame
    GLint locRes = glGetUniformLocation(computeProg, "uResolution");
    if (locRes >= 0) glUniform2i(locRes, TEXTURE_WIDTH, TEXTURE_HEIGHT);
    GLint locCam = glGetUniformLocation(computeProg, "uCamPos");
    if (locCam >= 0) glUniform3f(locCam, camPosGLM.x, camPosGLM.y, camPosGLM.z);
    GLint locInv = glGetUniformLocation(computeProg, "uInvViewProj");
    if (locInv >= 0) glUniformMatrix4fv(locInv, 1, GL_FALSE, glm::value_ptr(invVP));

    // optional time uniform if present
    GLint locT = glGetUniformLocation(computeProg, "t");
    if (locT >= 0) {
        float t = glutGet((GLenum)GLUT_ELAPSED_TIME) * 0.001f;
        glUniform1f(locT, t);
    }

    glDispatchCompute(groups_x, groups_y, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_SHADER_IMAGE_ACCESS_BARRIER_BIT|GL_TEXTURE_FETCH_BARRIER_BIT);
    glUseProgram(0);

    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, debugBuffer);
    // float* ptr = (float*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    // if(ptr) {
    //     for(int i = 0; i < TEXTURE_WIDTH * TEXTURE_HEIGHT; ++i)
    //         std::cout << ptr[i] << std::endl;
    //     glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    // }
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(quadProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    renderQuad();
    glUseProgram(0);
    glutSwapBuffers();
}

void idle () {
    static float lastTime = glutGet ((GLenum)GLUT_ELAPSED_TIME);
    static unsigned int counter = 0;
    counter++;
    float currentTime = glutGet ((GLenum)GLUT_ELAPSED_TIME);
    if (currentTime - lastTime >= 1000.0f) {
        FPS = counter;
        counter = 0;
        static char winTitle [64];
        sprintf (winTitle, "Raytracer - FPS: %d", FPS);
        glutSetWindowTitle (winTitle);
        lastTime = currentTime;
    }
    glutPostRedisplay ();
}


void ray_trace_from_camera() {
    int w = glutGet(GLUT_WINDOW_WIDTH)  ,   h = glutGet(GLUT_WINDOW_HEIGHT);
    std::cout << "Ray tracing a " << w << " x " << h << " image" << std::endl;
    camera.apply();
    Vec3 pos , dir;
    //    unsigned int nsamples = 100;
    unsigned int nsamples = 10;
    std::vector< Vec3 > image( w*h , Vec3(0,0,0) );
    for (int y=0; y<h; y++){
        for (int x=0; x<w; x++) {
            for( unsigned int s = 0 ; s < nsamples ; ++s ) {
                float u = ((float)(x) + (float)(rand())/(float)(RAND_MAX)) / w;
                float v = ((float)(y) + (float)(rand())/(float)(RAND_MAX)) / h;
                // this is a random uv that belongs to the pixel xy.
                screen_space_to_world_space_ray(u,v,pos,dir);
                Vec3 color = scenes[selected_scene].rayTrace( Ray(pos , dir) );
                image[x + y*w] += color;
            }
            image[x + y*w] /= nsamples;
        }
    }
    std::cout << "\tDone" << std::endl;

    std::string filename = "./rendu.ppm";
    ofstream f(filename.c_str(), ios::binary);
    if (f.fail()) {
        cout << "Could not open file: " << filename << endl;
        return;
    }
    f << "P3" << std::endl << w << " " << h << std::endl << 255 << std::endl;
    for (int i=0; i<w*h; i++)
        f << (int)(255.f*std::min<float>(1.f,image[i][0])) << " " << (int)(255.f*std::min<float>(1.f,image[i][1])) << " " << (int)(255.f*std::min<float>(1.f,image[i][2])) << " ";
    f << std::endl;
    f.close();
}


void key (unsigned char keyPressed, int x, int y) {
    Vec3 pos , dir;
    switch (keyPressed) {
    case 'f':
        if (fullScreen == true) {
            glutReshapeWindow (SCREENWIDTH, SCREENHEIGHT);
            fullScreen = false;
        } else {
            glutFullScreen ();
            fullScreen = true;
        }
        break;
    case 'q':
    case 27:
        clear ();
        exit (0);
        break;
    case 'w':
        GLint polygonMode[2];
        glGetIntegerv(GL_POLYGON_MODE, polygonMode);
        if(polygonMode[0] != GL_FILL)
            glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
        else
            glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
        break;

    case 'r':
        camera.apply();
        rays.clear();
        ray_trace_from_camera();
        break;
    case '+':
        selected_scene++;
        if( selected_scene >= scenes.size() ) selected_scene = 0;
        break;
    default:
        printUsage ();
        break;
    }
    idle ();
}

void mouse (int button, int state, int x, int y) {
    if (state == GLUT_UP) {
        mouseMovePressed = false;
        mouseRotatePressed = false;
        mouseZoomPressed = false;
    } else {
        if (button == GLUT_LEFT_BUTTON) {
            camera.beginRotate (x, y);
            mouseMovePressed = false;
            mouseRotatePressed = true;
            mouseZoomPressed = false;
        } else if (button == GLUT_RIGHT_BUTTON) {
            lastX = x;
            lastY = y;
            mouseMovePressed = true;
            mouseRotatePressed = false;
            mouseZoomPressed = false;
        } else if (button == GLUT_MIDDLE_BUTTON) {
            if (mouseZoomPressed == false) {
                lastZoom = y;
                mouseMovePressed = false;
                mouseRotatePressed = false;
                mouseZoomPressed = true;
            }
        }
    }
    idle ();
}

void motion (int x, int y) {
    if (mouseRotatePressed == true) {
        camera.rotate (x, y);
    }
    else if (mouseMovePressed == true) {
        camera.move ((x-lastX)/static_cast<float>(SCREENWIDTH), (lastY-y)/static_cast<float>(SCREENHEIGHT), 0.0);
        lastX = x;
        lastY = y;
    }
    else if (mouseZoomPressed == true) {
        camera.zoom (float (y-lastZoom)/SCREENHEIGHT);
        lastZoom = y;
    }
}


void reshape(int w, int h) {
    camera.resize (w, h);
    glViewport(0, 0, w, h);
}

void chargerScene(int nb){
    Scene s=scenes[nb];
    int taille=s.spheres.size()*sizeof(Sphere)+s.squares.size()*sizeof(Square)+s.meshes.size()*sizeof(Mesh)+s.lights.size()*sizeof(Light);
    
    //Spheres
    struct sp{
        float x,y,z,rayon,ra,ga,ba,b,rd,gd,bd,c,rs,gs,bs,s;
    };
    std::vector<sp> sps;
    for(unsigned int i=0;i<s.spheres.size();i++){
        Sphere a=s.spheres[i];
        sp b;
        b.x=a.m_center[0];
        b.y=a.m_center[1];
        b.z=a.m_center[2];
        b.rayon=a.m_radius;
        b.ra=a.material.ambient_material[0];
        b.ga=a.material.ambient_material[1];
        b.ba=a.material.ambient_material[2];
        b.rd=a.material.diffuse_material[0];
        b.gd=a.material.diffuse_material[1];
        b.bd=a.material.diffuse_material[2];
        b.rs=a.material.specular_material[0];
        b.gs=a.material.specular_material[1];
        b.bs=a.material.specular_material[2];
        b.s=a.material.shininess;
        b.b=0.0f;
        b.c=0.0f;
        sps.push_back(b);
    }
    std::cout<<"taille sphere : "<<sps.size()*sizeof(sp)<<std::endl;
    GLuint ssbo = 0;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sps.size()*sizeof(sp), sps.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    //Squares
    struct sq{
        float blx,bly,blz,p1,rx,ry,rz,p2,ux,uy,uz,p3,nx,ny,nz,p4,ra,ga,ba,b,rd,gd,bd,c,rs,gs,bs,s;
    };
    std::vector<sq> sqs;
    for(unsigned int i=0;i<s.squares.size();i++){
        s.squares[i].rebuild();
        Square a=s.squares[i];
        sq b;
        b.blx=a.m_bottom_left[0];
        b.bly=a.m_bottom_left[1];
        b.blz=a.m_bottom_left[2];
        b.rx=a.m_right_vector[0];
        b.ry=a.m_right_vector[1];
        b.rz=a.m_right_vector[2];
        b.ux=a.m_up_vector[0];
        b.uy=a.m_up_vector[1];
        b.uz=a.m_up_vector[2];
        b.nx=a.m_normal[0];
        b.ny=a.m_normal[1];
        b.nz=a.m_normal[2];
        b.ra=a.material.ambient_material[0];
        b.ga=a.material.ambient_material[1];
        b.ba=a.material.ambient_material[2];
        b.rd=a.material.diffuse_material[0];
        b.gd=a.material.diffuse_material[1];
        b.bd=a.material.diffuse_material[2];
        b.rs=a.material.specular_material[0];
        b.gs=a.material.specular_material[1];
        b.bs=a.material.specular_material[2];
        b.s=a.material.shininess;
        b.b=0.0f;
        b.c=0.0f;
        b.p1=0.0f;
        b.p2=0.0f;
        b.p3=0.0f;
        b.p4=0.0f;
        sqs.push_back(b);
    }
    std::cout<<"taille square : "<<sqs.size()*sizeof(sq)<<std::endl;
    ssbo=0;
    glGenBuffers(1,&ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sqs.size()*sizeof(sq), sqs.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    //Lights
    struct l{
        float x,y,z,r;
    };
    std::vector<l> ls;
    for(unsigned int i=0;i<s.lights.size();i++){
        Light a=s.lights[i];
        l b;
        b.x=a.pos[0];
        b.y=a.pos[1];
        b.z=a.pos[2];
        b.r=a.radius;
        ls.push_back(b);
    }
    std::cout<<"taille lights : "<<ls.size()*sizeof(l)<<std::endl;
    ssbo=0;
    glGenBuffers(1,&ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ls.size()*sizeof(l), ls.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    //Meshes
    struct v{
        float px,py,pz,u,nx,ny,nz,v;
    };
    std::vector<v> vs;
    struct t{
        int a,b,c,d;
    };
    std::vector<t> ts;
    struct m{
        int pv,nbv,pt,nbt;
        float ar,ag,ab,p1,dr,dg,db,p2,sr,sg,sb,s;
    };
    std::vector<m> ms;
    
    std::vector<bvh> bvhs;
    int nbV=0,nbT=0;
    float minx=FLT_MAX,miny=FLT_MAX,minz=FLT_MAX,maxx=-FLT_MAX,maxy=-FLT_MAX,maxz=-FLT_MAX;
    for(unsigned int i=0;i<s.meshes.size();i++){
        Mesh a=s.meshes[i];
        for(unsigned int j=0;j<a.vertices.size();j++){
            MeshVertex vertex=a.vertices[j];
            v ve;
            ve.px=vertex.position[0];
            ve.py=vertex.position[1];
            ve.pz=vertex.position[2];
            if(vertex.position[0]<minx)minx=vertex.position[0];
            if(vertex.position[1]<miny)miny=vertex.position[1];
            if(vertex.position[2]<minz)minz=vertex.position[2];
            if(vertex.position[0]>maxx)maxx=vertex.position[0];
            if(vertex.position[1]>maxy)maxy=vertex.position[1];
            if(vertex.position[2]>maxz)maxz=vertex.position[2];
            ve.nx=vertex.normal[0];
            ve.ny=vertex.normal[1];
            ve.nz=vertex.normal[2];
            ve.u=vertex.u;
            ve.v=vertex.v;
            vs.push_back(ve);
        }
    }
    bvh root;
    root.minx=minx;root.miny=miny;root.minz=minz;root.maxx=maxx;root.maxy=maxy;root.maxz=maxz;root.left=-1;root.right=-1;root.prof=0;root.nb=0;root.count=0;root.start=0;
    bvhs.push_back(root);
    int nbBVH=1;
    for(unsigned int i=0;i<s.meshes.size();i++){
        Mesh a=s.meshes[i];
        for(unsigned int j=0;j<a.triangles.size();j++){
            MeshTriangle triangle=a.triangles[j];
            t tr;
            tr.a=triangle.v[0];
            tr.b=triangle.v[1];
            tr.c=triangle.v[2];
            tr.d=0;
            ts.push_back(tr);
        }
        m mesh;
        mesh.nbv=a.vertices.size();
        mesh.nbt=a.triangles.size();
        mesh.pv=nbV;
        mesh.pt=nbT;
        nbV+=mesh.nbv;
        nbT+=mesh.nbt;
        mesh.nbv=nbBVH;
        std::cout<<"nbbvh : "<<mesh.nbv<<std::endl;
        mesh.ar=a.material.ambient_material[0];
        mesh.ag=a.material.ambient_material[1];
        mesh.ab=a.material.ambient_material[2];
        mesh.p1=bvhs.size();
        mesh.dr=a.material.diffuse_material[0];
        mesh.dg=a.material.diffuse_material[1];
        mesh.db=a.material.diffuse_material[2];
        mesh.p2=a.bvhs.size();
        mesh.sr=a.material.specular_material[0];
        mesh.sg=a.material.specular_material[1];
        mesh.sb=a.material.specular_material[2];
        mesh.s=a.material.shininess;
        ms.push_back(mesh);
        for(unsigned int j=0;j<a.bvhs.size();j++){
            if(a.bvhs[j].left!=-1)a.bvhs[j].left+=nbBVH;
            if(a.bvhs[j].right!=-1)a.bvhs[j].right+=nbBVH;
            bvhs.push_back(a.bvhs[j]);
            // if(a.bvhs[j].left==-1 && a.bvhs[j].right==-1)std::cout<<a.bvhs[j].count<<std::endl;
        }
        nbBVH+=a.bvhs.size();
        std::cout<<"info : nbV : "<<mesh.nbv<<" nbT : "<<mesh.nbt<<" premierV : "<<mesh.pv<<" premierT : "<<mesh.pt<<std::endl;
    }
    std::cout<<"taille meshes : "<<ms.size()*sizeof(m)+vs.size()*sizeof(v)+ts.size()*sizeof(t)<<std::endl;
    std::cout<<"taille bvh : "<<bvhs.size()*sizeof(bvh)<<std::endl;
    std::cout<<"nb bvh : "<<bvhs.size()<<std::endl;
    std::cout<<"minx : "<<minx<<" miny : "<<miny<<" minz : "<<minz<<std::endl;
    std::cout<<"maxx : "<<maxx<<" maxy : "<<maxy<<" maxz : "<<maxz<<std::endl;
    bvh bv=bvhs[10];
    std::cout<<"numero : "<<1<<" left : "<<bv.left<<" right : "<<bv.right<<" prof : "<<bv.prof<<" count : "<<bv.count<<" start : "<<bv.start<<std::endl;
    ssbo=0;
    glGenBuffers(1,&ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, vs.size()*sizeof(v), vs.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    ssbo=0;
    glGenBuffers(1,&ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ts.size()*sizeof(t), ts.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    ssbo=0;
    glGenBuffers(1,&ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ms.size()*sizeof(m), ms.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    ssbo=0;
    glGenBuffers(1,&ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, bvhs.size()*sizeof(bvh), bvhs.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // glGenBuffers(1, &debugBuffer);
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, debugBuffer);
    // glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * TEXTURE_WIDTH * TEXTURE_HEIGHT, nullptr, GL_DYNAMIC_COPY);
    // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, debugBuffer);
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    //heuu les uniforms la
    glUseProgram(computeProg);
    GLint locSp=glGetUniformLocation(computeProg,"nbSphere");
    if(locSp>=0)glUniform1i(locSp,(GLint)sps.size());
    GLint locSq=glGetUniformLocation(computeProg,"nbSquare");
    if(locSq>=0)glUniform1i(locSq,(GLint)sqs.size());
    GLint locL=glGetUniformLocation(computeProg,"nbLight");
    if(locL>=0)glUniform1i(locL,(GLint)ls.size());
    GLint locM=glGetUniformLocation(computeProg,"nbMesh");
    if(locM>=0)glUniform1i(locM,(GLint)ms.size());
    glUseProgram(0);
    std::cout<<"fin scene"<<std::endl;
}


int main (int argc, char ** argv) {
    if (argc > 2) {
        printUsage ();
        exit (EXIT_FAILURE);
    }
    glutInit (&argc, argv);
    glutInitDisplayMode (GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize (SCREENWIDTH, SCREENHEIGHT);
    window = glutCreateWindow ("gMini");

    if (!gladLoadGL()) {
        std::cerr << "Erreur d'initialisation de GLAD" << std::endl;
        exit(-1);
    }
    
    init ();
    glutIdleFunc (idle);
    glutDisplayFunc (display);
    glutKeyboardFunc (key);
    glutReshapeFunc (reshape);
    glutMotionFunc (motion);
    glutMouseFunc (mouse);
    key ('?', 0, 0);

    camera.move(0., 0., -3.1);
    selected_scene=0;
    scenes.resize(4);
    scenes[0].setup_single_sphere();
    scenes[1].setup_single_square();
    scenes[2].setup_cornell_box();
    scenes[3].setup_single_meshes();

    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA,GL_FLOAT, NULL);
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    const char* path="../shader/computeShader.glsl";
    std::ifstream sfile;
    sfile.open(path);
    std::stringstream sstream;
    sstream<<sfile.rdbuf();
    sfile.close();
    std::string ccode=sstream.str();
    const char* source=ccode.c_str();
    GLuint cs=glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(cs,1,&source,NULL);
    glCompileShader(cs);

    //erreur
    GLint success;
    glGetShaderiv(cs, GL_COMPILE_STATUS, &success);
    if(!success) {
        GLint maxLength = 0;
        glGetShaderiv(cs, GL_INFO_LOG_LENGTH, &maxLength);
        std::vector<GLchar> errorLog(maxLength);
        glGetShaderInfoLog(cs, maxLength, &maxLength, &errorLog[0]);
        std::cout << "Erreur de compilation du compute shader:" << std::endl;
        std::cout << &errorLog[0] << std::endl;
    }

    computeProg=glCreateProgram();
    glAttachShader(computeProg,cs);
    glLinkProgram(computeProg);

    //erreur
    glGetProgramiv(computeProg, GL_LINK_STATUS, &success);
    if(!success) {
        GLint maxLength = 0;
        glGetProgramiv(computeProg, GL_INFO_LOG_LENGTH, &maxLength);
        std::vector<GLchar> errorLog(maxLength);
        glGetProgramInfoLog(computeProg, maxLength, &maxLength, &errorLog[0]);
        std::cout << "Erreur de liaison du compute shader:" << std::endl;
        std::cout << &errorLog[0] << std::endl;
    }


    const char* vpath="../shader/vertex.glsl";
    std::ifstream vfile;
    vfile.open(vpath);
    std::stringstream vsstream;
    vsstream<<vfile.rdbuf();
    vfile.close();
    std::string vcode=vsstream.str();
    const char* vsource=vcode.c_str();
    GLuint vs=glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs,1,&vsource,NULL);
    glCompileShader(vs);

    const char* fpath="../shader/fragment.glsl";
    std::ifstream ffile;
    ffile.open(fpath);
    std::stringstream fsstream;
    fsstream<<ffile.rdbuf();
    ffile.close();
    std::string fcode=fsstream.str();
    const char* fsource=fcode.c_str();
    GLuint fs=glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs,1,&fsource,NULL);
    glCompileShader(fs);

    quadProg=glCreateProgram();
    glAttachShader(quadProg,vs);
    glAttachShader(quadProg,fs);
    glLinkProgram(quadProg);


    glUseProgram(quadProg);
    GLint loc = glGetUniformLocation(quadProg, "tex");
    if (loc >= 0) glUniform1i(loc, 0);
    glUseProgram(0);

    chargerScene(3);

    // struct SphereCPU { float cx,cy,cz,r; float cr,cg,cb,pad; };
    // std::vector<SphereCPU> sph = {
    //     { 0.0f, 0.0f, 0.0f, 0.6f,   0.9f,0.2f,0.2f,0.0f },
    //     { 1.0f, 0.2f, -1.0f, 0.5f,  0.2f,0.8f,0.3f,0.0f }
    // };
    // GLuint ssbo = 0;
    // glGenBuffers(1, &ssbo);
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    // glBufferData(GL_SHADER_STORAGE_BUFFER, sph.size()*sizeof(SphereCPU), sph.data(), GL_STATIC_DRAW);
    // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo); // binding = 1 dans le shader
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // glUseProgram(computeProg);
    // GLint locCount = glGetUniformLocation(computeProg, "uSphereCount");
    // if (locCount >= 0) glUniform1i(locCount, (GLint)sph.size());
    // glUseProgram(0);

    // int data[64];
    
    // GLuint ssbo;
    // glGenBuffers(1, &ssbo);
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    // glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STREAM_DRAW);
    // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // glDispatchCompute((unsigned int)TEXTURE_WIDTH,(unsigned int)TEXTURE_HEIGHT,1);
    // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    
    
    glutMainLoop ();
    return EXIT_SUCCESS;
}

