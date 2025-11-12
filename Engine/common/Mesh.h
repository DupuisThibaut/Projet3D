#ifndef MESH_H
#define MESH_H


#include <vector>
#include <string>
#include "Ray.h"

#include <GL/glut.h>

#include <cfloat>

#include <glm/glm.hpp>


// -------------------------------------------
// Basic Mesh class
// -------------------------------------------

struct MeshVertex {
    inline MeshVertex () {}
    inline MeshVertex (const glm::vec3 & _p, const glm::vec3 & _n) : position (_p), normal (_n) , u(0) , v(0) {}
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
    glm::vec3 position; // une position
    glm::vec3 normal; // une normale
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

    std::vector< float > positions_array;
    std::vector< float > normalsArray;
    std::vector< float > uvs_array;
    std::vector< unsigned int > triangles_array;

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
    }


    void translate( glm::vec3 const & translation ){
        for( unsigned int v = 0 ; v < vertices.size() ; ++v ) {
            vertices[v].position += translation;
        }
    }

    void apply_transformation_matrix( glm::mat3 transform ){
        for( unsigned int v = 0 ; v < vertices.size() ; ++v ) {
            vertices[v].position = transform*vertices[v].position;
        }

        //        recomputeNormals();
        //        build_positions_array();
        //        build_normals_array();
    }

    void scale( glm::vec3 const & scale ){
        glm::mat3 scale_matrix(scale[0], 0., 0.,
                0., scale[1], 0.,
                0., 0., scale[2]); //Matrice de transformation de mise à l'échelle
        apply_transformation_matrix( scale_matrix );
    }

    void rotate_x ( float angle ){
        float x_angle = angle * M_PI / 180.;
        glm::mat3 x_rotation(1., 0., 0.,
                        0., cos(x_angle), -sin(x_angle),
                        0., sin(x_angle), cos(x_angle));
        apply_transformation_matrix( x_rotation );
    }

    void rotate_y ( float angle ){
        float y_angle = angle * M_PI / 180.;
        glm::mat3 y_rotation(cos(y_angle), 0., sin(y_angle),
                        0., 1., 0.,
                        -sin(y_angle), 0., cos(y_angle));
        apply_transformation_matrix( y_rotation );
    }

    void rotate_z ( float angle ){
        float z_angle = angle * M_PI / 180.;
        glm::mat3 z_rotation(cos(z_angle), -sin(z_angle), 0.,
                        sin(z_angle), cos(z_angle), 0.,
                        0., 0., 1.);
        apply_transformation_matrix( z_rotation );
    }
    // // Calcul de l'intersection entre le rayon et le Mesh :
    // RayTriangleIntersection intersect( Ray const & ray ) const {
    //     RayTriangleIntersection closestIntersection;
    //     closestIntersection.t = FLT_MAX;
    //     // Note :
    //     // Creer un objet Triangle pour chaque face
    //     // Vous constaterez des problemes de précision
    //     // solution : ajouter un facteur d'échelle lors de la création du Triangle : float triangleScaling = 1.000001;
    //     float triangleScaling = 1.000001;
    //     // Pour chaque triangle du Mesh :
    //     for (size_t e = 0; e<triangles_array.size();e+=3){
    //         Triangle current = Triangle(vertices[triangles_array[e]].position*triangleScaling,vertices[triangles_array[e+1]].position*triangleScaling,vertices[triangles_array[e+2]].position*triangleScaling);
    //         RayTriangleIntersection tmp = current.getIntersection(ray);
    //         // On récupère l'intersection (si elle existe) et on teste si c'est le point le plus proche :
    //         if(tmp.t >1e-6 && tmp.t<closestIntersection.t && tmp.intersectionExists){
    //             // Interpolation de la normale en utilisant les coordonnées barycentriques :
    //             closestIntersection.normal = tmp.w0 * vertices[triangles_array[e]].normal + tmp.w1 * vertices[triangles_array[e+1]].normal + tmp.w2 *vertices[triangles_array[e+2]].normal;
    //             closestIntersection.normal.normalize();  // Normaliser la normale interpolée
    //             closestIntersection.intersectionExists = true;
    //             closestIntersection.t = tmp.t;
    //             closestIntersection.intersection = tmp.intersection;
    //             closestIntersection.w0 = tmp.w0;
    //             closestIntersection.w1 = tmp.w1;
    //             closestIntersection.w2 = tmp.w2;  
    //             // Interpolation des u v en utilisant les coordonnées barycentriques :         
    //             float theta = atan2(closestIntersection.normal[2], closestIntersection.normal[0]);
    //             float phi = acos(closestIntersection.normal[1]); 
    //             closestIntersection.u = (theta + M_PI) / (2.0 * M_PI); 
    //             closestIntersection.v = phi / M_PI;
    //             // Interpolation de la couleur en utilisant les coordonnées barycentriques : 
    //             closestIntersection.color = tmp.w0 * vertices[triangles_array[e]].normal + tmp.w1 * vertices[triangles_array[e+1]].normal + tmp.w2 *vertices[triangles_array[e+2]].normal;
    //         }
    //     }
    //     return closestIntersection;
    // }
};




#endif
