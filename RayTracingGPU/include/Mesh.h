#ifndef MESH_H
#define MESH_H


#include <cstdlib>
#include <vector>
#include <string>
#include "Vec3.h"
#include "Ray.h"
#include "Triangle.h"
#include "Material.h"

#include <GL/glut.h>

#include <cfloat>

#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <string_view>

// -------------------------------------------
// Basic Mesh class
// -------------------------------------------

struct MeshVertex {
    inline MeshVertex () {}
    inline MeshVertex (const Vec3 & _p, const Vec3 & _n) : position (_p), normal (_n) , u(0) , v(0) {}
    inline MeshVertex (const MeshVertex & vertex) : position (vertex.position), normal (vertex.normal) , u(vertex.u) , v(vertex.v) {}
    inline virtual ~MeshVertex () {}
    inline MeshVertex & operator = (const MeshVertex & vertex) {
        position = vertex.position;
        normal = vertex.normal;
        u = vertex.u;
        v = vertex.v;
        return (*this);
    }
    // membres :
    Vec3 position; // une position
    Vec3 normal; // une normale
    float u,v; // coordonnees uv
};

struct MeshTriangle {
    inline MeshTriangle () {
        v[0] = v[1] = v[2] = 0;
    }
    inline MeshTriangle (const MeshTriangle & t) {
        v[0] = t.v[0];   v[1] = t.v[1];   v[2] = t.v[2];
    }
    inline MeshTriangle (unsigned int v0, unsigned int v1, unsigned int v2) {
        v[0] = v0;   v[1] = v1;   v[2] = v2;
    }
    unsigned int & operator [] (unsigned int iv) { return v[iv]; }
    unsigned int operator [] (unsigned int iv) const { return v[iv]; }
    inline virtual ~MeshTriangle () {}
    inline MeshTriangle & operator = (const MeshTriangle & t) {
        v[0] = t.v[0];   v[1] = t.v[1];   v[2] = t.v[2];
        return (*this);
    }
    // membres :
    unsigned int v[3];
};

struct bvh{
    float minx,miny,minz;int nb;float maxx,maxy,maxz;int prof;int left,right,start,count;
};



class Mesh {
protected:
    void build_positions_array() {
        positions_array.resize( 3 * vertices.size() );
        for( unsigned int v = 0 ; v < vertices.size() ; ++v ) {
            positions_array[3*v + 0] = vertices[v].position[0];
            positions_array[3*v + 1] = vertices[v].position[1];
            positions_array[3*v + 2] = vertices[v].position[2];
        }
    }
    void build_normals_array() {
        normalsArray.resize( 3 * vertices.size() );
        for( unsigned int v = 0 ; v < vertices.size() ; ++v ) {
            normalsArray[3*v + 0] = vertices[v].normal[0];
            normalsArray[3*v + 1] = vertices[v].normal[1];
            normalsArray[3*v + 2] = vertices[v].normal[2];
        }
    }
    void build_UVs_array() {
        uvs_array.resize( 2 * vertices.size() );
        for( unsigned int vert = 0 ; vert < vertices.size() ; ++vert ) {
            uvs_array[2*vert + 0] = vertices[vert].u;
            uvs_array[2*vert + 1] = vertices[vert].v;
        }
    }
    void build_triangles_array() {
        triangles_array.resize( 3 * triangles.size() );
        for( unsigned int t = 0 ; t < triangles.size() ; ++t ) {
            triangles_array[3*t + 0] = triangles[t].v[0];
            triangles_array[3*t + 1] = triangles[t].v[1];
            triangles_array[3*t + 2] = triangles[t].v[2];
        }
    }
public:
    std::vector<MeshVertex> vertices;
    std::vector<MeshTriangle> triangles;

    std::vector<bvh> bvhs;

    std::vector< float > positions_array;
    std::vector< float > normalsArray;
    std::vector< float > uvs_array;
    std::vector< unsigned int > triangles_array;

    Material material;

    void loadOFF (const std::string & filename);
    void recomputeNormals ();
    void centerAndScaleToUnit ();
    void scaleUnit ();


    virtual
    void build_arrays() {
        recomputeNormals();
        build_positions_array();
        build_normals_array();
        build_UVs_array();
        build_triangles_array();
        // creerBVH();
    }


    void translate( Vec3 const & translation ){
        for( unsigned int v = 0 ; v < vertices.size() ; ++v ) {
            vertices[v].position += translation;
        }
    }

    void apply_transformation_matrix( Mat3 transform ){
        for( unsigned int v = 0 ; v < vertices.size() ; ++v ) {
            vertices[v].position = transform*vertices[v].position;
        }

        //        recomputeNormals();
        //        build_positions_array();
        //        build_normals_array();
    }

    void scale( Vec3 const & scale ){
        Mat3 scale_matrix(scale[0], 0., 0.,
                0., scale[1], 0.,
                0., 0., scale[2]); //Matrice de transformation de mise à l'échelle
        apply_transformation_matrix( scale_matrix );
    }

    void rotate_x ( float angle ){
        float x_angle = angle * M_PI / 180.;
        Mat3 x_rotation(1., 0., 0.,
                        0., cos(x_angle), -sin(x_angle),
                        0., sin(x_angle), cos(x_angle));
        apply_transformation_matrix( x_rotation );
    }

    void rotate_y ( float angle ){
        float y_angle = angle * M_PI / 180.;
        Mat3 y_rotation(cos(y_angle), 0., sin(y_angle),
                        0., 1., 0.,
                        -sin(y_angle), 0., cos(y_angle));
        apply_transformation_matrix( y_rotation );
    }

    void rotate_z ( float angle ){
        float z_angle = angle * M_PI / 180.;
        Mat3 z_rotation(cos(z_angle), -sin(z_angle), 0.,
                        sin(z_angle), cos(z_angle), 0.,
                        0., 0., 1.);
        apply_transformation_matrix( z_rotation );
    }


    void draw() const {
        if( triangles_array.size() == 0 ) return;
        GLfloat material_color[4] = {material.diffuse_material[0],
                                     material.diffuse_material[1],
                                     material.diffuse_material[2],
                                     1.0};

        GLfloat material_specular[4] = {material.specular_material[0],
                                        material.specular_material[1],
                                        material.specular_material[2],
                                        1.0};

        GLfloat material_ambient[4] = {material.ambient_material[0],
                                       material.ambient_material[1],
                                       material.ambient_material[2],
                                       1.0};

        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_color);
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material_ambient);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, material.shininess);

        glEnableClientState(GL_VERTEX_ARRAY) ;
        glEnableClientState (GL_NORMAL_ARRAY);
        glNormalPointer (GL_FLOAT, 3*sizeof (float), (GLvoid*)(normalsArray.data()));
        glVertexPointer (3, GL_FLOAT, 3*sizeof (float) , (GLvoid*)(positions_array.data()));
        glDrawElements(GL_TRIANGLES, triangles_array.size(), GL_UNSIGNED_INT, (GLvoid*)(triangles_array.data()));

    }

    RayTriangleIntersection intersect( Ray const & ray ) const {
        RayTriangleIntersection closestIntersection;
        closestIntersection.t = FLT_MAX;
        // Note :
        // Creer un objet Triangle pour chaque face
        // Vous constaterez des problemes de précision
        // solution : ajouter un facteur d'échelle lors de la création du Triangle : float triangleScaling = 1.000001;
        for(long unsigned int i=0;i<triangles.size();i++){
            Triangle tri=Triangle(vertices[triangles[i].v[0]].position,vertices[triangles[i].v[1]].position,vertices[triangles[i].v[2]].position);
            RayTriangleIntersection triInter=tri.getIntersection(ray);
            if(closestIntersection.t>triInter.t){closestIntersection=triInter;}
        }
        return closestIntersection;
    }

    void creerBVH(){
        float minx=FLT_MAX,miny=FLT_MAX,minz=FLT_MAX,maxx=-FLT_MAX,maxy=-FLT_MAX,maxz=-FLT_MAX;
        for(unsigned int i=0;i<vertices.size();i++){
            MeshVertex vertex=vertices[i];
            if(vertex.position[0]<minx)minx=vertex.position[0];
            if(vertex.position[1]<miny)miny=vertex.position[1];
            if(vertex.position[2]<minz)minz=vertex.position[2];
            if(vertex.position[0]>maxx)maxx=vertex.position[0];
            if(vertex.position[1]>maxy)maxy=vertex.position[1];
            if(vertex.position[2]>maxz)maxz=vertex.position[2];
        }
        // std::cout<<"min x : "<<minx<<"min y : "<<miny<<" min z : "<<minz<<std::endl;
        // std::cout<<"max x : "<<maxx<<"max y : "<<maxy<<" max z : "<<maxz<<std::endl;
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
            if(debut==0){
                debut=1;
                Vec3 minc=Vec3(current->minx,current->miny,current->minz);
                Vec3 maxc=Vec3(current->maxx,current->maxy,current->maxz);
                std::cout<<"min : "<<minc<<std::endl;
                std::cout<<"max : "<<maxc<<std::endl;
            }
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
                MeshTriangle triangle=triangles[ind];
                Vec3 p0=vertices[triangle.v[0]].position;
                Vec3 p1=vertices[triangle.v[1]].position;
                Vec3 p2=vertices[triangle.v[2]].position;
                Vec3 centreGravite=(p0+p1+p2)/3.0f;
                milieux.push_back(centreGravite[axe]);
            }
            std::sort(milieux.begin(),milieux.end());
            milieu=milieux[milieux.size()/2];
            for(unsigned int i=0;i<triangleNode[current->nb].size();i++){
                int ind=triangleNode[current->nb][i];
                // MeshTriangle triangle=triangles[ind];
                // Vec3 p0=vertices[triangle.v[0]].position;
                // Vec3 p1=vertices[triangle.v[1]].position;
                // Vec3 p2=vertices[triangle.v[2]].position;
                // Vec3 centreGravite=(p0+p1+p2)/3.0f;
                // if(centreGravite[axe]<milieu){
                if(milieux[i]<milieu){
                    lTri.push_back(ind);
                }else{
                    rTri.push_back(ind);
                }
            }
            if(lTri.size()>0){
                minx=FLT_MAX;miny=FLT_MAX;minz=FLT_MAX;maxx=-FLT_MAX;maxy=-FLT_MAX;maxz=-FLT_MAX;
                for(unsigned int i=0;i<lTri.size();i++){
                    MeshTriangle triangle=triangles[lTri[i]];
                    for(int j=0;j<3;j++){
                        MeshVertex vertex=vertices[triangle.v[j]];
                        if(vertex.position[0]<minx)minx=vertex.position[0];
                        if(vertex.position[1]<miny)miny=vertex.position[1];
                        if(vertex.position[2]<minz)minz=vertex.position[2];
                        if(vertex.position[0]>maxx)maxx=vertex.position[0];
                        if(vertex.position[1]>maxy)maxy=vertex.position[1];
                        if(vertex.position[2]>maxz)maxz=vertex.position[2];
                    }
                }
                l.minx=minx;l.miny=miny;l.minz=minz;l.maxx=maxx;l.maxy=maxy;l.maxz=maxz;
            }
            if(rTri.size()>0){
                minx=FLT_MAX;miny=FLT_MAX;minz=FLT_MAX;maxx=-FLT_MAX;maxy=-FLT_MAX;maxz=-FLT_MAX;
                for(unsigned int i=0;i<rTri.size();i++){
                    MeshTriangle triangle=triangles[rTri[i]];
                    for(unsigned int j=0;j<3;j++){
                        MeshVertex vertex=vertices[triangle.v[j]];
                        if(vertex.position[0]<minx)minx=vertex.position[0];
                        if(vertex.position[1]<miny)miny=vertex.position[1];
                        if(vertex.position[2]<minz)minz=vertex.position[2];
                        if(vertex.position[0]>maxx)maxx=vertex.position[0];
                        if(vertex.position[1]>maxy)maxy=vertex.position[1];
                        if(vertex.position[2]>maxz)maxz=vertex.position[2];
                    }
                }
                r.minx=minx;r.miny=miny;r.minz=minz;r.maxx=maxx;r.maxy=maxy;r.maxz=maxz;
            }
            std::cout<<"milieu : "<<milieu<<" axe : "<<axe<<std::endl;
            std::cout<<"l tri size : "<<lTri.size()<<" r tri size : "<<rTri.size()<<std::endl;
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
        std::cout<<"cas 1 : "<<nb1<<" cas 2 : "<<nb2<<" cas 3 : "<<nb3<<std::endl;
        std::vector<MeshTriangle> newTriangles;
        for(unsigned int i=0;i<bvhs.size();i++){
            bvh* bv=&bvhs[i];
            if(bv->left==-1 && bv->right==-1 && triangleNode[bv->nb].size()>0){
                bv->count=triangleNode[bv->nb].size();
                bv->start=newTriangles.size();
                for(unsigned int j=0;j<triangleNode[bv->nb].size();j++){
                    newTriangles.push_back(triangles[triangleNode[bv->nb][j]]);
                }
            }
            std::cout<<"bvh numero : "<<i<<" left : "<<bv->left<<" right : "<<bv->right<<" prof : "<<bv->prof<<" count : "<<bv->count<<" start : "<<bv->start<<std::endl;
        }
        std::cout<<"nb triangles avant : "<<triangles.size()<<" nb triangles apres : "<<newTriangles.size()<<std::endl;
        triangles=newTriangles;
    }
};




#endif
