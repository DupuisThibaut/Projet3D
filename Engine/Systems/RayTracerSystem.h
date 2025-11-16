#ifndef RAYTRACERSYSTEM_H
#define RAYTRACERSYSTEM_H
#include <glm/fwd.hpp>
#include <unordered_map>
#include <cfloat>
#include <vector>

#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <string_view>


struct sp{
    float x,y,z,rayon,ra,ga,ba,b,rd,gd,bd,c,rs,gs,bs,s;
};

struct sq{
    float blx,bly,blz,p1,rx,ry,rz,p2,ux,uy,uz,p3,nx,ny,nz,p4,ra,ga,ba,b,rd,gd,bd,c,rs,gs,bs,s;
    // uvec2 texture;
};

struct l{
    float x,y,z,r;
};

struct v{
    float px,py,pz,u,nx,ny,nz,v;
};

struct tri{
    int a,b,c,d;
};

struct m{
    int pv,nbv,pt,nbt;
    float ar,ag,ab,p1,dr,dg,db,p2,sr,sg,sb,s;
};

struct bvh{
    float minx,miny,minz;int nb;float maxx,maxy,maxz;int prof;int left,right,start,count;
};

struct world{
    glm::mat4 modelMat;
    glm::mat4 invModelMatrix;
    glm::mat3 normalMat;
};

std::vector<std::vector<unsigned short>> newTriangles;
std::vector<bvh> creerBVH(std::vector<glm::vec3> vertices, std::vector<std::vector<unsigned short>> triangles){
    std::vector<bvh> bvhs;
    float minx=FLT_MAX,miny=FLT_MAX,minz=FLT_MAX,maxx=-FLT_MAX,maxy=-FLT_MAX,maxz=-FLT_MAX;
    for(unsigned int i=0;i<vertices.size();i++){
        glm::vec3 vertex=vertices[i];
        if(vertex[0]<minx)minx=vertex[0];
        if(vertex[1]<miny)miny=vertex[1];
        if(vertex[2]<minz)minz=vertex[2];
        if(vertex[0]>maxx)maxx=vertex[0];
        if(vertex[1]>maxy)maxy=vertex[1];
        if(vertex[2]>maxz)maxz=vertex[2];
    }
    bvh b;
    b.minx=minx;b.miny=miny;b.minz=minz;b.maxx=maxx;b.maxy=maxy;b.maxz=maxz;b.left=1,b.right=2;b.prof=0;b.nb=0;b.count=0;b.start=0;
    bvhs.push_back(b);
    std::vector<int> test;
    test.push_back(0);
    int prof=10;
    std::vector<std::vector<int>> triangleNode;
    std::vector<int> premier;
    for(unsigned int i=0;i<triangles.size();i++){
        premier.push_back(i);
    }
    triangleNode.push_back(premier);
    int nb1=0,nb2=0,nb3=0,debut=0;
    while(test.size()>0){
        bvh* current=&bvhs[test[0]];
        if(current->prof>=prof || triangleNode[current->nb].size()<=4){
            test.erase(test.begin());
            continue;
        }
        int axe;
        float x=current->maxx-current->minx;
        float y=current->maxy-current->miny;
        float z=current->maxz-current->minz;
        if(x>=y && x>=z)axe=0;else if(y>=x && y>=z)axe=1;else axe=2;
        bvh l,r;
        l.count=0;r.count=0;l.start=0;r.start=0;
        l.minx=current->minx;l.miny=current->miny;l.minz=current->minz;l.maxx=current->maxx;l.maxy=current->maxy;l.maxz=current->maxz;
        r.minx=current->minx;r.miny=current->miny;r.minz=current->minz;r.maxx=current->maxx;r.maxy=current->maxy;r.maxz=current->maxz;
        int nba=0,nbb=0,nbc=0;
        float milieu;
        if(axe==0){
            nba++;
            milieu=(current->minx+current->maxx)/2.0f;
            l.maxx=milieu;
            r.minx=milieu;
        }else if(axe==1){
            nbb++;
            milieu=(current->miny+current->maxy)/2.0f;
            l.maxy=milieu;
            r.miny=milieu;
        }else{
            nbc++;
            milieu=(current->minz+current->maxz)/2.0f;
            l.maxz=milieu;
            r.minz=milieu;
        }
        std::vector<int> lTri;std::vector<int> rTri;std::vector<float>milieux;
        for(unsigned int i=0;i<triangleNode[current->nb].size();i++){
            int ind=triangleNode[current->nb][i];
            std::vector<unsigned short> triangle=triangles[ind];
            glm::vec3 p0=vertices[triangle[0]];
            glm::vec3 p1=vertices[triangle[1]];
            glm::vec3 p2=vertices[triangle[2]];
            glm::vec3 centreGravite=(p0+p1+p2)/3.0f;
            milieux.push_back(centreGravite[axe]);
        }
        std::sort(milieux.begin(),milieux.end());
        milieu=milieux[milieux.size()/2];
        for(unsigned int i=0;i<triangleNode[current->nb].size();i++){
            int ind=triangleNode[current->nb][i];
            if(milieux[i]<milieu){
                lTri.push_back(ind);
            }else{
                rTri.push_back(ind);
            }
        }
        if(lTri.size()>0){
            minx=FLT_MAX;miny=FLT_MAX;minz=FLT_MAX;maxx=-FLT_MAX;maxy=-FLT_MAX;maxz=-FLT_MAX;
            for(unsigned int i=0;i<lTri.size();i++){
                std::vector<unsigned short> triangle=triangles[lTri[i]];
                for(int j=0;j<3;j++){
                    glm::vec3 vertex=vertices[triangle[j]];
                    if(vertex[0]<minx)minx=vertex[0];
                    if(vertex[1]<miny)miny=vertex[1];
                    if(vertex[2]<minz)minz=vertex[2];
                    if(vertex[0]>maxx)maxx=vertex[0];
                    if(vertex[1]>maxy)maxy=vertex[1];
                    if(vertex[2]>maxz)maxz=vertex[2];
                }
            }
            l.minx=minx;l.miny=miny;l.minz=minz;l.maxx=maxx;l.maxy=maxy;l.maxz=maxz;
        }
        if(rTri.size()>0){
            minx=FLT_MAX;miny=FLT_MAX;minz=FLT_MAX;maxx=-FLT_MAX;maxy=-FLT_MAX;maxz=-FLT_MAX;
            for(unsigned int i=0;i<rTri.size();i++){
                std::vector<unsigned short> triangle=triangles[rTri[i]];
                for(unsigned short j=0;j<3;j++){
                    glm::vec3 vertex=vertices[triangle[j]];
                    if(vertex[0]<minx)minx=vertex[0];
                    if(vertex[1]<miny)miny=vertex[1];
                    if(vertex[2]<minz)minz=vertex[2];
                    if(vertex[0]>maxx)maxx=vertex[0];
                    if(vertex[1]>maxy)maxy=vertex[1];
                    if(vertex[2]>maxz)maxz=vertex[2];
                }
            }
            r.minx=minx;r.miny=miny;r.minz=minz;r.maxx=maxx;r.maxy=maxy;r.maxz=maxz;
        }
        if(lTri.size()>0 && rTri.size()>0){
            nb1++;
            current->left=bvhs.size();
            l.nb=triangleNode.size();
            l.prof=current->prof+1;
            l.left=-1;
            l.right=-1;
            triangleNode.push_back(lTri);
            bvhs.push_back(l);
            test.push_back(bvhs.size()-1);
            current->right=bvhs.size();
            r.nb=triangleNode.size();
            r.prof=current->prof+1;
            r.left=-1;
            r.right=-1;
            triangleNode.push_back(rTri);
            bvhs.push_back(r);
            test.push_back(bvhs.size()-1);
        }else if(lTri.size()>0){
            nb2++;
            current->left=bvhs.size();
            l.nb=triangleNode.size();
            l.prof=current->prof+1;
            l.left=-1;
            l.right=-1;
            triangleNode.push_back(lTri);
            bvhs.push_back(l);
            test.push_back(bvhs.size()-1);
        }else if(rTri.size()>0){
            nb3++;
            current->right=bvhs.size();
            r.nb=triangleNode.size();
            r.prof=current->prof+1;
            r.left=-1;
            r.right=-1;
            triangleNode.push_back(rTri);
            bvhs.push_back(r);
            test.push_back(bvhs.size()-1);
        }
        test.erase(test.begin());
    }
    for(unsigned int i=0;i<bvhs.size();i++){
        bvh* bv=&bvhs[i];
        if(bv->left==-1 && bv->right==-1 && triangleNode[bv->nb].size()>0){
            bv->count=triangleNode[bv->nb].size();
            bv->start=newTriangles.size();
            for(unsigned int j=0;j<triangleNode[bv->nb].size();j++){
                newTriangles.push_back(triangles[triangleNode[bv->nb][j]]);
            }
        }
    }
    return bvhs;
}

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

class RayTracerSystem {
public:
    EntityManager* entityManager;
    GLuint computeProg;
    GLuint quadProg;
    GLuint ssboSpheres=0;
    GLuint ssboSquares=0;
    GLuint ssboLights=0;
    GLuint ssboVertices=0;
    GLuint ssboTriangles=0;
    GLuint ssboMeshes=0;
    GLuint ssboBVH=0;
    GLuint ssboWorld=0;
    GLuint texture;

    std::vector<sp> sps;        
    std::vector<sq> sqs;
    std::vector<l> ls;
    std::vector<v> vs;
    std::vector<tri> ts;
    std::vector<m> ms;
    std::vector<bvh> bvhs;
    std::vector<world> worlds;

    unsigned int TEXTURE_WIDTH = 512;
    unsigned int TEXTURE_HEIGHT = 512;

    unsigned int groups_x = 1;
    unsigned int groups_y = 1;


    RayTracerSystem(EntityManager* em) : entityManager(em){

    }

    bool initialize(){
        glGenTextures(1, &texture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA,GL_FLOAT, NULL);
        glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        const char* path="Shaders/computeShader.glsl";
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
            return false;
        }


        const char* vpath="Shaders/vertex.glsl";
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

        const char* fpath="Shaders/fragment.glsl";
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
        unsigned int LOCAL_X = 16;
        unsigned int LOCAL_Y = 16;
        groups_x = (TEXTURE_WIDTH  + LOCAL_X - 1) / LOCAL_X;
        groups_y = (TEXTURE_HEIGHT + LOCAL_Y - 1) / LOCAL_Y;
        return true;
    }

    bool resize(int width, int height){
        if(width<=0 || height<=0) return false;
        TEXTURE_WIDTH = width;
        TEXTURE_HEIGHT = height;
        if(texture)
            glDeleteTextures(1, &texture);
        glGenTextures(1, &texture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA,GL_FLOAT, NULL);
        glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindTexture(GL_TEXTURE_2D, 0);
        unsigned int LOCAL_X = 16;
        unsigned int LOCAL_Y = 16;
        groups_x = (TEXTURE_WIDTH  + LOCAL_X - 1) / LOCAL_X;
        groups_y = (TEXTURE_HEIGHT + LOCAL_Y - 1) / LOCAL_Y;
        return true;
    }

    void update(const std::vector<Entity>& entities) {
        const unsigned int LOCAL_X = 16;
        const unsigned int LOCAL_Y = 16;
        unsigned int groups_x = (TEXTURE_WIDTH  + LOCAL_X - 1) / LOCAL_X;
        unsigned int groups_y = (TEXTURE_HEIGHT + LOCAL_Y - 1) / LOCAL_Y;

        // calculer et uploader les matrices + cam pos depuis la Camera actuelle
        for(auto& camera : entityManager->GetComponents<CameraComponent>()){
            GLfloat mv[16], proj[16];
            glGetFloatv(GL_MODELVIEW_MATRIX, mv);
            glGetFloatv(GL_PROJECTION_MATRIX, proj);
            glm::mat4 view = camera.second.getViewMatrix(glm::vec3(entityManager->GetComponent<TransformComponent>(camera.first).worldMatrix[3]));
            glm::mat4 projM = camera.second.getProjectionMatrix();
            glm::mat4 invVP = glm::inverse(projM * view);

            glm::vec3 cpos = entityManager->GetComponent<TransformComponent>(camera.first).position;
            glm::vec3 camPosGLM(cpos[0], cpos[1], cpos[2]);

            // std::cout<<"camPos : "<<camPosGLM.x<<" camPos : "<<camPosGLM.y<<" camPos : "<<camPosGLM.z<<std::endl;

            glUseProgram(computeProg);
            // upload uniforms every frame
            GLint locRes = glGetUniformLocation(computeProg, "uResolution");
            if (locRes >= 0) glUniform2i(locRes, TEXTURE_WIDTH, TEXTURE_HEIGHT);
            GLint locCam = glGetUniformLocation(computeProg, "uCamPos");
            if (locCam >= 0) glUniform3f(locCam, camPosGLM.x, camPosGLM.y, camPosGLM.z);
            GLint locInv = glGetUniformLocation(computeProg, "uInvViewProj");
            if (locInv >= 0) glUniformMatrix4fv(locInv, 1, GL_FALSE, &invVP[0][0]);
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
        // glutSwapBuffers();
    }

    void onCreate(std::vector<Entity> entities){
        // std::unordered_map<glm::uint32_t,MeshComponents> meshesCompo = entityManager->GetComponents<MeshComponent>();
        // std::unordered_map<glm::uint32_t,LightComponent> lightsCompo = entityManager->GetComponents<LightComponent>();
        // std::unordered_map<glm::uint32_t,MaterialComponent> matCompo = entityManager->GetComponents<MaterialComponent>();
        // std::unordered_map<glm::uint32_t,TransformComponent> tCompo = entityManager->GetComponents<TransformComponent>();

        // std::cout<<"debut fonction"<<std::endl;
        std::vector<bvh> bvhs;
        bvh root;bvhs.push_back(root);
        int nbBVH=1,nbV=0,nbT=0;
        float minx=FLT_MAX,miny=FLT_MAX,minz=FLT_MAX,maxx=-FLT_MAX,maxy=-FLT_MAX,maxz=-FLT_MAX;
        for(auto& e : entities){
            // std::cout<<"debut de la boucle"<<std::endl;
            if(!entityManager->HasComponent<CameraComponent>(e.id)){
                MaterialComponent mat = entityManager->GetComponent<MaterialComponent>(e.id);
                TransformComponent t = entityManager->GetComponent<TransformComponent>(e.id);
                if(entityManager->HasComponent<MeshComponent>(e.id)){
                    MeshComponent M = entityManager->GetComponent<MeshComponent>(e.id);
                    //Spheres
                    if(M.type==PrimitiveType::SPHERE){
                        sp b;
                        b.x=t.position[0];
                        b.y=t.position[1];
                        b.z=t.position[2];
                        b.rayon=t.scale[0];
                        b.ra=mat.ambient_material[0];
                        b.ga=mat.ambient_material[1];
                        b.ba=mat.ambient_material[2];
                        b.rd=mat.diffuse_material[0];
                        b.gd=mat.diffuse_material[1];
                        b.bd=mat.diffuse_material[2];
                        b.rs=mat.specular_material[0];
                        b.gs=mat.specular_material[1];
                        b.bs=mat.specular_material[2];
                        b.s=mat.shininess;
                        b.b=0.0f;
                        b.c=0.0f;
                        sps.push_back(b);
                    }
                    //Squares
                    if(M.type==PrimitiveType::PLANE){
                        sq b;
                        glm::mat4 model=t.worldMatrix;
                        // std::cout<<"rota : "<<t.rotation[0]<<" rota : "<<t.rotation[1]<<" rota : "<<t.rotation[2]<<std::endl;
                        glm::mat3 rotaScale(model);
                        // std::cout<<"worldMatrix : "<<model[0][0]<<" worldMatrix : "<<model[0][1]<<" worldMatrix : "<<model[0][2]<<" worldMatrix : "<<model[0][3]<<std::endl;
                        // std::cout<<"worldMatrix : "<<model[1][0]<<" worldMatrix : "<<model[1][1]<<" worldMatrix : "<<model[1][2]<<" worldMatrix : "<<model[1][3]<<std::endl;
                        // std::cout<<"worldMatrix : "<<model[2][0]<<" worldMatrix : "<<model[2][1]<<" worldMatrix : "<<model[2][2]<<" worldMatrix : "<<model[2][3]<<std::endl;
                        // std::cout<<"worldMatrix : "<<model[3][0]<<" worldMatrix : "<<model[3][1]<<" worldMatrix : "<<model[3][2]<<" worldMatrix : "<<model[3][3]<<std::endl;
                        glm::vec3 m_right_vector=rotaScale*M.m_right_vector;
                        glm::vec3 m_up_vector=rotaScale*M.m_up_vector;
                        float lengthUV=length(m_up_vector);
                        float lengthRV=length(m_right_vector);
                        m_up_vector=normalize(m_up_vector);
                        m_right_vector=normalize(m_right_vector);
                        // std::cout<<"rv : "<<m_right_vector[0]<<" rv : "<<m_right_vector[1]<<" rv : "<<m_right_vector[2]<<std::endl;
                        // std::cout<<"uv : "<<m_up_vector[0]<<" uv : "<<m_up_vector[1]<<" uv : "<<m_up_vector[2]<<std::endl;
                        b.blx=t.position[0];
                        b.bly=t.position[1];
                        b.blz=t.position[2];
                        b.rx=m_right_vector[0];
                        b.ry=m_right_vector[1];
                        b.rz=m_right_vector[2];
                        b.ux=m_up_vector[0];
                        b.uy=m_up_vector[1];
                        b.uz=m_up_vector[2];
                        glm::vec3 m_normal=cross(m_right_vector,m_up_vector);
                        m_normal=normalize(m_normal);
                        b.nx=m_normal[0];
                        b.ny=m_normal[1];
                        b.nz=m_normal[2];
                        // std::cout<<"centre x : "<<b.blx<<" centre y : "<<b.bly<<" centre z : "<<b.blz<<std::endl;
                        // std::cout<<"m_right_vector x : "<<b.rx<<" m_right_vector y : "<<b.ry<<" m_right_vector z : "<<b.rz<<std::endl;
                        // std::cout<<"m_up_vector x : "<<b.ux<<" m_up_vector y : "<<b.uy<<" m_up_vector z : "<<b.uz<<std::endl;
                        // std::cout<<"m_normal x : "<<b.nx<<" m_normal y : "<<b.ny<<" m_normal z : "<<b.nz<<std::endl;
                        // std::cout<<"length right : "<<lengthRV<<std::endl;
                        // std::cout<<"length up : "<<lengthUV<<std::endl;
                        b.ra=mat.ambient_material[0];
                        b.ga=mat.ambient_material[1];
                        b.ba=mat.ambient_material[2];
                        b.rd=mat.diffuse_material[0];
                        b.gd=mat.diffuse_material[1];
                        b.bd=mat.diffuse_material[2];
                        b.rs=mat.specular_material[0];
                        b.gs=mat.specular_material[1];
                        b.bs=mat.specular_material[2];
                        b.s=mat.shininess;
                        // std::cout<<"ambient_material x : "<<b.ra<<" ambient_material y : "<<b.ga<<" ambient_material z : "<<b.ba<<std::endl;
                        // std::cout<<"diffuse_material x : "<<b.rd<<" diffuse_material y : "<<b.gd<<" diffuse_material z : "<<b.bd<<std::endl;
                        // std::cout<<"specular_material x : "<<b.rs<<" specular_material y : "<<b.gs<<" specular_material z : "<<b.bs<<std::endl;
                        // std::cout<<"shininess : "<<b.s<<std::endl;
                        b.b=0.0f;
                        b.c=0.0f;
                        b.p3=lengthUV;
                        b.p2=lengthRV;
                        b.p1=length(m_right_vector);
                        b.p4=length(m_up_vector);
                        // if (GLEW_ARB_bindless_texture) {
                        //     std::cout << "Bindless active!" << std::endl;
                        // }
                        // GLuint textPlane;
                        // glGenTextures(1, &textPlane);
                        // glBindTexture(GL_TEXTURE_2D, textPlane);
                        // int w,h,c;
                        // unsigned char* img=stbi_load(mat.texturePath.c_str(),&w,&h,&c,4);
                        // glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,img);
                        // glGenerateMipmap(GL_TEXTURE_2D);
                        // stbi_image_free(img);
                        // GLuint64 text=glGetTextureHandleARB(textPlane);
                        // glMakeTextureHandleResidentARB(text);
                        // uint32_t lo=uint32_t(text & 0xFFFFFFFFull);
                        // uint32_t hi=uint32_t(text >> 32);
                        // b.texture[0]=lo;
                        // b.texture[1]=hi;
                        // b.texture=text;
                        sqs.push_back(b);
                    }
                    //Meshes
                    if(M.type==PrimitiveType::MESH){
                        world w;w.modelMat=t.worldMatrix;w.invModelMatrix=inverse(t.worldMatrix);w.normalMat=transpose(inverse(mat3(t.worldMatrix)));worlds.push_back(w);
                        std::vector<bvh> bvhsYep=creerBVH(M.vertices,M.triangles);
                        M.triangles=newTriangles;
                        int nbV=0,nbT=0;
                        for(unsigned int j=0;j<M.vertices.size();j++){
                            glm::vec3 vertex=M.vertices[j];
                            v ve;
                            ve.px=vertex[0];
                            ve.py=vertex[1];
                            ve.pz=vertex[2];
                            if(vertex[0]<minx)minx=vertex[0];
                            if(vertex[1]<miny)miny=vertex[1];
                            if(vertex[2]<minz)minz=vertex[2];
                            if(vertex[0]>maxx)maxx=vertex[0];
                            if(vertex[1]>maxy)maxy=vertex[1];
                            if(vertex[2]>maxz)maxz=vertex[2];
                            ve.nx=M.normals[j][0];
                            ve.ny=M.normals[j][1];
                            ve.nz=M.normals[j][2];
                            ve.u=M.uvs[j][0];
                            ve.v=M.uvs[j][1];
                            vs.push_back(ve);
                        }
                        for(unsigned int j=0;j<M.triangles.size();j++){
                            std::vector<unsigned short> triangle=M.triangles[j];
                            tri tr;
                            tr.a=triangle[0];
                            tr.b=triangle[1];
                            tr.c=triangle[2];
                            tr.d=0;
                            ts.push_back(tr);
                        }
                        m mesh;
                        mesh.nbv=M.vertices.size();
                        mesh.nbt=M.triangles.size();
                        mesh.pv=nbV;
                        mesh.pt=nbT;
                        nbV+=mesh.nbv;
                        nbT+=mesh.nbt;
                        mesh.nbv=nbBVH;
                        mesh.ar=mat.ambient_material[0];
                        mesh.ag=mat.ambient_material[1];
                        mesh.ab=mat.ambient_material[2];
                        mesh.p1=bvhs.size();
                        mesh.dr=mat.diffuse_material[0];
                        mesh.dg=mat.diffuse_material[1];
                        mesh.db=mat.diffuse_material[2];
                        mesh.p2=bvhsYep.size();
                        mesh.sr=mat.specular_material[0];
                        mesh.sg=mat.specular_material[1];
                        mesh.sb=mat.specular_material[2];
                        mesh.s=mat.shininess;
                        ms.push_back(mesh);
                        for(unsigned int j=0;j<bvhsYep.size();j++){
                            if(bvhsYep[j].left!=-1)bvhsYep[j].left+=nbBVH;
                            if(bvhsYep[j].right!=-1)bvhsYep[j].right+=nbBVH;
                            bvhs.push_back(bvhsYep[j]);
                        }
                        nbBVH+=bvhsYep.size();
                    }
                }
                //Lights
                if(entityManager->HasComponent<LightComponent>(e.id)){
                    LightComponent light = entityManager->GetComponent<LightComponent>(e.id);
                    l b;
                    b.x=t.position[0];
                    b.y=t.position[1];
                    b.z=t.position[2];
                    b.r=t.scale[0];
                    ls.push_back(b);
                    std::cout<<"light x : "<<b.x<<" light y : "<<b.y<<" light z : "<<b.z<<std::endl;
                }
            }
        }
        std::cout<<"creation donnees finies !"<<std::endl;
        bvhs[0].minx=minx;bvhs[0].miny=miny;bvhs[0].minz=minz;bvhs[0].maxx=maxx;bvhs[0].maxy=maxy;bvhs[0].maxz=maxz;bvhs[0].left=-1;bvhs[0].right=-1;bvhs[0].prof=0;bvhs[0].nb=0;bvhs[0].count=0;bvhs[0].start=0;

        
        // std::cout<<"min : "<<minx<<" min : "<<miny<<" min : "<<minz<<std::endl;
        // std::cout<<"max : "<<maxx<<" max : "<<maxy<<" max : "<<maxz<<std::endl;

         //Spheres
        glGenBuffers(1, &ssboSpheres);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboSpheres);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sps.size()*sizeof(sp), sps.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboSpheres);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        //Squares
        glGenBuffers(1,&ssboSquares);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboSquares);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sqs.size()*sizeof(sq), sqs.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboSquares);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        //Lights
        glGenBuffers(1,&ssboLights);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboLights);
        glBufferData(GL_SHADER_STORAGE_BUFFER, ls.size()*sizeof(l), ls.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboLights);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        //Meshes
        glGenBuffers(1,&ssboVertices);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVertices);
        glBufferData(GL_SHADER_STORAGE_BUFFER, vs.size()*sizeof(v), vs.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssboVertices);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        
        glGenBuffers(1,&ssboTriangles);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboTriangles);
        glBufferData(GL_SHADER_STORAGE_BUFFER, ts.size()*sizeof(tri), ts.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssboTriangles);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        
        glGenBuffers(1,&ssboMeshes);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboMeshes);
        glBufferData(GL_SHADER_STORAGE_BUFFER, ms.size()*sizeof(m), ms.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssboMeshes);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        
        glGenBuffers(1,&ssboBVH);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboBVH);
        glBufferData(GL_SHADER_STORAGE_BUFFER, bvhs.size()*sizeof(bvh), bvhs.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssboBVH);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        glGenBuffers(1,&ssboWorld);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboWorld);
        glBufferData(GL_SHADER_STORAGE_BUFFER, worlds.size()*sizeof(world), worlds.data(), GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, ssboWorld);
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
        std::cout<<"nb sphere : "<<sps.size()<<" nb squares : "<<sqs.size()<<" nb lights : "<<ls.size()<<" nb meshes : "<<ms.size()<<std::endl;
        glUseProgram(0);
    }
};

#endif // RAYTRACERSYSTEM_H

// ,
// {
//     "id": 4,
//     "transform": {
//     "position": [-1.5, -9.0, 0.0],
//     "rotation": [0.0, 0.0, 0.0],
//     "scale": [0.5, 0.5, 0.5],
//     "parent": -1,
//     "children": []
//     },
//     "mesh": {
//     "type": "primitive",
//     "mesh_type": "BOX",
//     "subdivisions": 100,
//     "normal": [0.0, 1.0, 0.0],
//     "height": 2.0,
//     "width": 0.4
//     },
//     "material":{
//     "type":"color",
//     "color":[0.9, 0.9, 1.0],
//     "ambient":[1.0, 0.0, 0.0],
//     "diffuse":[1.0, 0.0, 0.0],
//     "specular":[1.0, 0.0, 0.0],
//     "shininess":0.5
//     }
// }