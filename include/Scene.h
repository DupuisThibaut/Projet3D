#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <string>
#include "Mesh.h"
#include "Sphere.h"
#include "Square.h"
#include "Vec3.h"


#include <GL/glut.h>


enum LightType {
    LightType_Spherical,
    LightType_Quad
};


struct Light {
    Vec3 material;
    bool isInCamSpace;
    LightType type;

    Vec3 pos;
    float radius;

    Mesh quad;

    float powerCorrection;

    Light() : powerCorrection(1.0) {}

};

struct RaySceneIntersection{
    bool intersectionExists;
    unsigned int typeOfIntersectedObject;
    unsigned int objectIndex;
    float t;
    RayTriangleIntersection rayMeshIntersection;
    RaySphereIntersection raySphereIntersection;
    RaySquareIntersection raySquareIntersection;
    RaySceneIntersection() : intersectionExists(false) , t(FLT_MAX) {}
};



class Scene {
    
public:
    std::vector< Mesh > meshes;
    std::vector< Sphere > spheres;
    std::vector< Square > squares;
    std::vector< Light > lights;


    Scene() {
    }

    void draw() {
        // iterer sur l'ensemble des objets, et faire leur rendu :
        for( unsigned int It = 0 ; It < meshes.size() ; ++It ) {
            Mesh const & mesh = meshes[It];
            mesh.draw();
        }
        for( unsigned int It = 0 ; It < spheres.size() ; ++It ) {
            Sphere const & sphere = spheres[It];
            sphere.draw();
        }
        for( unsigned int It = 0 ; It < squares.size() ; ++It ) {
            Square const & square = squares[It];
            square.draw();
        }
    }




    RaySceneIntersection computeIntersection(Ray const & ray) {
        RaySceneIntersection result;
        result.raySphereIntersection.t = std::numeric_limits<float>::infinity(); // Initialiser Ã l'infini
        result.raySquareIntersection.t = std::numeric_limits<float>::infinity();
        result.rayMeshIntersection.t = std::numeric_limits<float>::infinity();
        result.t = FLT_MAX;
        result.intersectionExists = false;
        //TODO calculer les intersections avec les objets de la scene et garder la plus proche
        for(int i = 0;i<meshes.size();i++){
            RayTriangleIntersection resTest = meshes[i].intersect(ray);
            if(resTest.t<result.rayMeshIntersection.t && resTest.intersectionExists){
                result.rayMeshIntersection = resTest;
                result.objectIndex = i;
                result.intersectionExists = true;
                result.typeOfIntersectedObject = 2;
            }
        }
        for(size_t i=0;i<spheres.size();i++){
            RaySphereIntersection resTest = spheres[i].intersect(ray);
            if(resTest.t<result.t && resTest.intersectionExists){
                result.raySphereIntersection = resTest;
                result.objectIndex = i;
                result.intersectionExists = true;
                result.typeOfIntersectedObject = 0;
                result.t = resTest.t;
            }
        }
        for(size_t i=0;i<squares.size();i++){
            RaySquareIntersection resTest = squares[i].intersect(ray);
            if(resTest.t<result.t && resTest.intersectionExists){
                result.raySquareIntersection = resTest;
                result.objectIndex = i;
                result.intersectionExists = true;
                result.typeOfIntersectedObject = 1;
                result.raySquareIntersection.t = resTest.t;
            }
        }
        return result;
    }

    Vec3 rayTraceRecursive( Ray ray , int NRemainingBounces ) {
        RaySceneIntersection intersect = computeIntersection(ray);
        Vec3 color;
        if (intersect.intersectionExists==true) {
            Vec3 l=Vec3(0.8,0.8,0.8);
            if(intersect.raySphereIntersection.intersectionExists && intersect.typeOfIntersectedObject==0){
                Vec3 L=lights[0].pos-intersect.raySphereIntersection.intersection;
                Vec3 normal=intersect.raySphereIntersection.normal;
                Vec3 V=ray.origin()-intersect.raySphereIntersection.intersection;
                V.normalize();L.normalize();normal.normalize();
                float cosT=Vec3::dot(normal,L);
                Vec3 R=2*cosT*normal-L;
                R.normalize();
                float cosA=Vec3::dot(R,V);
                float n=spheres[intersect.objectIndex].material.shininess;
                Vec3 ambient=spheres[intersect.objectIndex].material.ambient_material;
                Vec3 diffuse=spheres[intersect.objectIndex].material.diffuse_material;
                Vec3 specular=spheres[intersect.objectIndex].material.specular_material;
                color[0]=l[0]*ambient[0]+l[0]*diffuse[0]*cosT+l[0]*specular[0]*pow(cosA,n);
                color[1]=l[1]*ambient[1]+l[1]*diffuse[1]*cosT+l[1]*specular[1]*pow(cosA,n);
                color[2]=l[2]*ambient[2]+l[2]*diffuse[2]*cosT+l[2]*specular[2]*pow(cosA,n);
                // std::cout<<color<<std::endl;
                // Ray rayon=Ray(lights[0].pos,intersect.raySquareIntersection.intersection-lights[0].pos);
                float nombreRayonOmbreDouce=0.;float nombreRayon=10.;float pourcentageOmbre=0.;
                Ray rayon = Ray(intersect.raySphereIntersection.intersection+normal*0.001,L);
                // RaySceneIntersection intersectionLumiere=computeIntersection(rayon);
                // if(intersectionLumiere.intersectionExists && intersectionLumiere.t<rayon.direction().length())nombreRayonOmbreDouce++;//color = Vec3(0.0,0.0,0.0);
                Vec3 newRay;
                for(int i=0;i<nombreRayon;i++){
                    newRay=lights[0].pos+Vec3(float(std::rand()%100)/100-0.5,float(std::rand()%100)/100.-0.5,float(std::rand()%100)/100.-0.5)*lights[0].radius;
                    rayon=Ray(intersect.raySphereIntersection.intersection+normal*0.001,newRay-intersect.raySphereIntersection.intersection);
                    RaySceneIntersection intersectionLumiere=computeIntersection(rayon);
                    if(intersectionLumiere.intersectionExists && intersectionLumiere.t<rayon.direction().length())nombreRayonOmbreDouce++;
                }
                pourcentageOmbre=nombreRayonOmbreDouce/nombreRayon;
                color *= (1.0-pourcentageOmbre);
                // if(intersectionLumiere.intersectionExists==true){
                //     if(intersectionLumiere.raySphereIntersection.intersectionExists && intersectionLumiere.typeOfIntersectedObject==0){
                //         if(intersectionLumiere.raySphereIntersection.intersection[0]!=intersect.raySphereIntersection.intersection[0] ||
                //         intersectionLumiere.raySphereIntersection.intersection[1]!=intersect.raySphereIntersection.intersection[1] ||
                //         intersectionLumiere.raySphereIntersection.intersection[2]!=intersect.raySphereIntersection.intersection[2]){
                //             float a=(lights[0].pos-intersectionLumiere.raySphereIntersection.intersection).length();
                //             float b=(intersectionLumiere.raySphereIntersection.intersection-intersect.raySphereIntersection.intersection).length();
                //             float c=(lights[0].pos-intersect.raySphereIntersection.intersection).length();
                //             if(a+b==c)color=Vec3(0.,0.,0.);
                //         }
                //     }
                //     if(intersectionLumiere.raySquareIntersection.intersectionExists && intersectionLumiere.typeOfIntersectedObject==0){
                //         if(intersectionLumiere.raySquareIntersection.intersection[0]!=intersect.raySphereIntersection.intersection[0] ||
                //         intersectionLumiere.raySquareIntersection.intersection[1]!=intersect.raySphereIntersection.intersection[1] ||
                //         intersectionLumiere.raySquareIntersection.intersection[2]!=intersect.raySphereIntersection.intersection[2]){
                //             float a=(lights[0].pos-intersectionLumiere.raySquareIntersection.intersection).length();
                //             float b=(intersectionLumiere.raySquareIntersection.intersection-intersect.raySphereIntersection.intersection).length();
                //             float c=(lights[0].pos-intersect.raySphereIntersection.intersection).length();
                //             if(a+b==c)color=Vec3(0.,0.,0.);
                //         }
                //     }
                // }
            }
            else if(intersect.raySquareIntersection.intersectionExists && intersect.typeOfIntersectedObject==1){
                Vec3 L=lights[0].pos-intersect.raySquareIntersection.intersection;
                Vec3 normal=intersect.raySquareIntersection.normal;
                Vec3 V=ray.origin()-intersect.raySquareIntersection.intersection;
                V.normalize();L.normalize();normal.normalize();
                float cosT=Vec3::dot(normal,L);
                Vec3 R=2*cosT*normal-L;
                R.normalize();
                float cosA=Vec3::dot(R,V);
                float n=squares[intersect.objectIndex].material.shininess;
                Vec3 ambient=squares[intersect.objectIndex].material.ambient_material;
                Vec3 diffuse=squares[intersect.objectIndex].material.diffuse_material;
                Vec3 specular=squares[intersect.objectIndex].material.specular_material;
                color[0]=l[0]*ambient[0]+l[0]*diffuse[0]*cosT+l[0]*specular[0]*pow(cosA,n);
                color[1]=l[1]*ambient[1]+l[1]*diffuse[1]*cosT+l[1]*specular[1]*pow(cosA,n);
                color[2]=l[2]*ambient[2]+l[2]*diffuse[2]*cosT+l[2]*specular[2]*pow(cosA,n);
                // std::cout<<color<<std::endl;
                //Ray rayon=Ray(lights[0].pos,intersect.raySquareIntersection.intersection-lights[0].pos);
                float nombreRayonOmbreDouce=0.;float nombreRayon=10.;float pourcentageOmbre=0.;
                Ray rayon = Ray(intersect.raySquareIntersection.intersection+normal*0.001,L);
                // RaySceneIntersection intersectionLumiere=computeIntersection(rayon);
                // if(intersectionLumiere.intersectionExists && intersectionLumiere.t<rayon.direction().length())nombreRayonOmbreDouce++;//color = Vec3(0.0,0.0,0.0);
                Vec3 newRay;
                for(int i=0;i<nombreRayon;i++){
                    newRay=lights[0].pos+Vec3(float(std::rand()%100)/100.-0.5,float(std::rand()%100)/100.-0.5,float(std::rand()%100)/100.-0.5)*lights[0].radius;
                    rayon=Ray(intersect.raySquareIntersection.intersection+normal*0.001,newRay-intersect.raySquareIntersection.intersection);
                    RaySceneIntersection intersectionLumiere=computeIntersection(rayon);
                    if(intersectionLumiere.intersectionExists && intersectionLumiere.t<rayon.direction().length())nombreRayonOmbreDouce++;
                }
                pourcentageOmbre=nombreRayonOmbreDouce/nombreRayon;
                color *= (1.0-pourcentageOmbre);
                // if(intersectionLumiere.intersectionExists==true){
                //     if(intersectionLumiere.raySphereIntersection.intersectionExists && intersectionLumiere.typeOfIntersectedObject==0){
                //         if(intersectionLumiere.raySphereIntersection.intersection[0]!=intersect.raySquareIntersection.intersection[0] ||
                //         intersectionLumiere.raySphereIntersection.intersection[1]!=intersect.raySquareIntersection.intersection[1] ||
                //         intersectionLumiere.raySphereIntersection.intersection[2]!=intersect.raySquareIntersection.intersection[2]){
                //             float a=(lights[0].pos-intersectionLumiere.raySphereIntersection.intersection).length();
                //             float b=(intersectionLumiere.raySphereIntersection.intersection-intersect.raySquareIntersection.intersection).length();
                //             float c=(lights[0].pos-intersect.raySquareIntersection.intersection).length();
                //             if(a+b==c)color=Vec3(0.,0.,0.);
                //         }
                //     }
                //     if(intersectionLumiere.raySquareIntersection.intersectionExists && intersectionLumiere.typeOfIntersectedObject==0){
                //         if(intersectionLumiere.raySquareIntersection.intersection[0]!=intersect.raySquareIntersection.intersection[0] ||
                //         intersectionLumiere.raySquareIntersection.intersection[1]!=intersect.raySquareIntersection.intersection[1] ||
                //         intersectionLumiere.raySquareIntersection.intersection[2]!=intersect.raySquareIntersection.intersection[2]){
                //             float a=(lights[0].pos-intersectionLumiere.raySquareIntersection.intersection).length();
                //             float b=(intersectionLumiere.raySquareIntersection.intersection-intersect.raySquareIntersection.intersection).length();
                //             float c=(lights[0].pos-intersect.raySquareIntersection.intersection).length();
                //             if(a+b==c)color=Vec3(0.,0.,0.);
                //         }
                //     }
                // }
            }
            else if(intersect.rayMeshIntersection.intersectionExists && intersect.typeOfIntersectedObject==2){
                Vec3 L=lights[0].pos-intersect.rayMeshIntersection.intersection;
                Vec3 normal=intersect.rayMeshIntersection.normal;
                Vec3 V=ray.origin()-intersect.rayMeshIntersection.intersection;
                V.normalize();L.normalize();normal.normalize();
                float cosT=Vec3::dot(normal,L);
                Vec3 R=2*cosT*normal-L;
                R.normalize();
                float cosA=Vec3::dot(R,V);
                float n=meshes[intersect.objectIndex].material.shininess;
                Vec3 ambient=meshes[intersect.objectIndex].material.ambient_material;
                Vec3 diffuse=meshes[intersect.objectIndex].material.diffuse_material;
                Vec3 specular=meshes[intersect.objectIndex].material.specular_material;
                color[0]=l[0]*ambient[0]+l[0]*diffuse[0]*cosT+l[0]*specular[0]*pow(cosA,n);
                color[1]=l[1]*ambient[1]+l[1]*diffuse[1]*cosT+l[1]*specular[1]*pow(cosA,n);
                color[2]=l[2]*ambient[2]+l[2]*diffuse[2]*cosT+l[2]*specular[2]*pow(cosA,n);
                float nombreRayonOmbreDouce=0.;float nombreRayon=10.;float pourcentageOmbre=0.;
                Ray rayon = Ray(intersect.rayMeshIntersection.intersection+normal*0.001,L);
                Vec3 newRay;
                for(int i=0;i<nombreRayon;i++){
                    newRay=lights[0].pos+Vec3(float(std::rand()%100)/100.-0.5,float(std::rand()%100)/100.-0.5,float(std::rand()%100)/100.-0.5)*lights[0].radius;
                    rayon=Ray(intersect.rayMeshIntersection.intersection+normal*0.001,newRay-intersect.rayMeshIntersection.intersection);
                    RaySceneIntersection intersectionLumiere=computeIntersection(rayon);
                    if(intersectionLumiere.intersectionExists && intersectionLumiere.t<rayon.direction().length())nombreRayonOmbreDouce++;
                }
                pourcentageOmbre=nombreRayonOmbreDouce/nombreRayon;
                color *= (1.0-pourcentageOmbre);
            }
            return color;
        } else {
            auto a = 0.5*(ray.direction()[1] + 1.0);
            return (1.0-a)*Vec3(1.0f, 1.0f, 1.0f)+a*Vec3(0.5,0.7,1.0);
        }
    }



    Vec3 rayTrace( Ray const & rayStart ) {
        //TODO appeler la fonction recursive
        Vec3 color=rayTraceRecursive(rayStart,1);
        return color;
    }

    void setup_single_sphere() {
        meshes.clear();
        spheres.clear();
        squares.clear();
        lights.clear();

        {
            lights.resize( lights.size() + 1 );
            Light & light = lights[lights.size() - 1];
            light.pos = Vec3(0.,5.,5.);
            light.radius = 2.5f;
            light.powerCorrection = 2.f;
            light.type = LightType_Spherical;
            light.material = Vec3(1,1,1);
            light.isInCamSpace = false;
        }
        // { //Back Wall
        //     squares.resize( squares.size() + 1 );
        //     Square & s = squares[squares.size() - 1];
        //     s.setQuad(Vec3(-1., -1., 0.), Vec3(1., 0, 0.), Vec3(0., 1, 0.), 2., 2.);
        //     s.scale(Vec3(2., 2., 1.));
        //     s.translate(Vec3(0., 0., -2.));
        //     s.build_arrays();
        //     s.material.diffuse_material = Vec3( 0.5,0.,1. );
        //     s.material.specular_material = Vec3( 1.,1.,1. );
        //     s.material.shininess = 16;
        // }

        // { //Left Wall

        //     squares.resize( squares.size() + 1 );
        //     Square & s = squares[squares.size() - 1];
        //     s.setQuad(Vec3(-1., -1., 0.), Vec3(1., 0, 0.), Vec3(0., 1, 0.), 2., 2.);
        //     s.scale(Vec3(2., 2., 1.));
        //     s.translate(Vec3(0., 0., -2.));
        //     s.rotate_y(90);
        //     s.build_arrays();
        //     s.material.diffuse_material = Vec3( 0.,0.5,1. );
        //     s.material.specular_material = Vec3( 1.,0.,0. );
        //     s.material.shininess = 16;
        // }

        // { //Right Wall
        //     squares.resize( squares.size() + 1 );
        //     Square & s = squares[squares.size() - 1];
        //     s.setQuad(Vec3(-1., -1., 0.), Vec3(1., 0, 0.), Vec3(0., 1, 0.), 2., 2.);
        //     s.translate(Vec3(0., 0., -2.));
        //     s.scale(Vec3(2., 2., 1.));
        //     s.rotate_y(-90);
        //     s.build_arrays();
        //     s.material.diffuse_material = Vec3( 0.5,0.5,1. );
        //     s.material.specular_material = Vec3( 0.0,1.0,0.0 );
        //     s.material.shininess = 16;
        // }

        // { //Floor
        //     squares.resize( squares.size() + 1 );
        //     Square & s = squares[squares.size() - 1];
        //     s.setQuad(Vec3(-1., -1., 0.), Vec3(1., 0, 0.), Vec3(0., 1, 0.), 2., 2.);
        //     s.translate(Vec3(0., 0., -2.));
        //     s.scale(Vec3(2., 2., 1.));
        //     s.rotate_x(-90);
        //     s.build_arrays();
        //     s.material.diffuse_material = Vec3( 0.,0.,1. );
        //     s.material.specular_material = Vec3( 1.0,1.0,1.0 );
        //     s.material.shininess = 16;
        // }

        // { //Ceiling
        //     squares.resize( squares.size() + 1 );
        //     Square & s = squares[squares.size() - 1];
        //     s.setQuad(Vec3(-1., -1., 0.), Vec3(1., 0, 0.), Vec3(0., 1, 0.), 2., 2.);
        //     s.translate(Vec3(0., 0., -2.));
        //     s.scale(Vec3(2., 2., 1.));
        //     s.rotate_x(90);
        //     s.build_arrays();
        //     s.material.diffuse_material = Vec3( 0.,0.,0. );
        //     s.material.specular_material = Vec3( 1.0,1.0,1.0 );
        //     s.material.shininess = 16;
        // }

        // { //Front Wall
        //     squares.resize( squares.size() + 1 );
        //     Square & s = squares[squares.size() - 1];
        //     s.setQuad(Vec3(-1., -1., 0.), Vec3(1., 0, 0.), Vec3(0., 1, 0.), 2., 2.);
        //     s.translate(Vec3(0., 0., -3.));
        //     s.scale(Vec3(2., 2., 1.));
        //     s.rotate_y(180);
        //     s.build_arrays();
        //     s.material.diffuse_material = Vec3( 0.5,0.5,0.5 );
        //     s.material.specular_material = Vec3( 1.0,1.0,1.0 );
        //     s.material.shininess = 16;
        // }
        // {
        //     spheres.resize( spheres.size() + 1 );
        //     Sphere & s = spheres[spheres.size() - 1];
        //     s.m_center = Vec3(1. , 0. , 0.);
        //     s.m_radius = 1.f;
        //     s.build_arrays();
        //     s.material.type = Material_Mirror;
        //     s.material.diffuse_material = Vec3( 0.,1.,0. );
        //     s.material.specular_material = Vec3( 0.2,0.2,0.2 );
        //     s.material.shininess = 20;
        // }
        {
            spheres.resize( spheres.size() + 1 );
            Sphere & s = spheres[spheres.size() - 1];
            s.m_center = Vec3(0. , 0. , 0.);
            s.m_radius = 1.f;
            s.build_arrays();
            s.material.type = Material_Mirror;
            s.material.diffuse_material = Vec3( 0.58,0.,0.82 );
            s.material.specular_material = Vec3( 0.2,0.2,0.2 );
            s.material.shininess = 20;
        }
    }

    void setup_single_square() {
        meshes.clear();
        spheres.clear();
        squares.clear();
        lights.clear();

        {
            lights.resize( lights.size() + 1 );
            Light & light = lights[lights.size() - 1];
            light.pos = Vec3(-5,5,5);
            light.radius = 2.5f;
            light.powerCorrection = 2.f;
            light.type = LightType_Spherical;
            light.material = Vec3(1,1,1);
            light.isInCamSpace = false;
        }

        {
            squares.resize( squares.size() + 1 );
            Square & s = squares[squares.size() - 1];
            s.setQuad(Vec3(-1., -1., 0.), Vec3(1., 0, 0.), Vec3(0., 1, 0.), 2., 2.);
            s.build_arrays();
            s.material.diffuse_material = Vec3( 1.,0.,0. );
            s.material.specular_material = Vec3( 0.2,0.2,0.2 );
            s.material.shininess = 20;
        }
    }

    void setup_cornell_box(){
        meshes.clear();
        spheres.clear();
        squares.clear();
        lights.clear();

        {
            lights.resize( lights.size() + 1 );
            Light & light = lights[lights.size() - 1];
            light.pos = Vec3( 0.0, 1.5, 0.0 );
            light.radius = 2.5f;
            light.powerCorrection = 2.f;
            light.type = LightType_Spherical;
            light.material = Vec3(1,1,1);
            light.isInCamSpace = false;
        }

        { //Back Wall
            squares.resize( squares.size() + 1 );
            Square & s = squares[squares.size() - 1];
            s.setQuad(Vec3(-1., -1., 0.), Vec3(1., 0, 0.), Vec3(0., 1, 0.), 2., 2.);
            s.scale(Vec3(2., 2., 1.));
            s.translate(Vec3(0., 0., -2.));
            s.build_arrays();
            s.material.diffuse_material = Vec3( 1.,1.,1. );
            s.material.specular_material = Vec3( 1.,1.,1. );
            s.material.shininess = 16;
        }

        { //Left Wall

            squares.resize( squares.size() + 1 );
            Square & s = squares[squares.size() - 1];
            s.setQuad(Vec3(-1., -1., 0.), Vec3(1., 0, 0.), Vec3(0., 1, 0.), 2., 2.);
            s.scale(Vec3(2., 2., 1.));
            s.translate(Vec3(0., 0., -2.));
            s.rotate_y(90);
            s.build_arrays();
            s.material.diffuse_material = Vec3( 1.,0.,0. );
            s.material.specular_material = Vec3( 1.,0.,0. );
            s.material.shininess = 16;
        }

        { //Right Wall
            squares.resize( squares.size() + 1 );
            Square & s = squares[squares.size() - 1];
            s.setQuad(Vec3(-1., -1., 0.), Vec3(1., 0, 0.), Vec3(0., 1, 0.), 2., 2.);
            s.translate(Vec3(0., 0., -2.));
            s.scale(Vec3(2., 2., 1.));
            s.rotate_y(-90);
            s.build_arrays();
            s.material.diffuse_material = Vec3( 0.0,1.0,0.0 );
            s.material.specular_material = Vec3( 0.0,1.0,0.0 );
            s.material.shininess = 16;
        }

        { //Floor
            squares.resize( squares.size() + 1 );
            Square & s = squares[squares.size() - 1];
            s.setQuad(Vec3(-1., -1., 0.), Vec3(1., 0, 0.), Vec3(0., 1, 0.), 2., 2.);
            s.translate(Vec3(0., 0., -2.));
            s.scale(Vec3(2., 2., 1.));
            s.rotate_x(-90);
            s.build_arrays();
            s.material.diffuse_material = Vec3( 1.0,1.0,1.0 );
            s.material.specular_material = Vec3( 1.0,1.0,1.0 );
            s.material.shininess = 16;
        }

        { //Ceiling
            squares.resize( squares.size() + 1 );
            Square & s = squares[squares.size() - 1];
            s.setQuad(Vec3(-1., -1., 0.), Vec3(1., 0, 0.), Vec3(0., 1, 0.), 2., 2.);
            s.translate(Vec3(0., 0., -2.));
            s.scale(Vec3(2., 2., 1.));
            s.rotate_x(90);
            s.build_arrays();
            s.material.diffuse_material = Vec3( 1.0,1.0,1.0 );
            s.material.specular_material = Vec3( 1.0,1.0,1.0 );
            s.material.shininess = 16;
        }

        { //Front Wall
            squares.resize( squares.size() + 1 );
            Square & s = squares[squares.size() - 1];
            s.setQuad(Vec3(-1., -1., 0.), Vec3(1., 0, 0.), Vec3(0., 1, 0.), 2., 2.);
            s.translate(Vec3(0., 0., -2.));
            s.scale(Vec3(2., 2., 1.));
            s.rotate_y(180);
            s.build_arrays();
            s.material.diffuse_material = Vec3( 1.0,1.0,1.0 );
            s.material.specular_material = Vec3( 1.0,1.0,1.0 );
            s.material.shininess = 16;
        }


        { //GLASS Sphere

            spheres.resize( spheres.size() + 1 );
            Sphere & s = spheres[spheres.size() - 1];
            s.m_center = Vec3(1.0, -1.25, 0.5);
            s.m_radius = 0.75f;
            s.build_arrays();
            s.material.type = Material_Mirror;
            s.material.diffuse_material = Vec3( 1.,0.,0. );
            s.material.specular_material = Vec3( 1.,0.,0. );
            s.material.shininess = 16;
            s.material.transparency = 1.0;
            s.material.index_medium = 1.4;
        }


        { //MIRRORED Sphere
            spheres.resize( spheres.size() + 1 );
            Sphere & s = spheres[spheres.size() - 1];
            s.m_center = Vec3(-1.0, -1.25, -0.5);
            s.m_radius = 0.75f;
            s.build_arrays();
            s.material.type = Material_Glass;
            s.material.diffuse_material = Vec3( 1.,1.,1. );
            s.material.specular_material = Vec3(  1.,1.,1. );
            s.material.shininess = 16;
            s.material.transparency = 0.;
            s.material.index_medium = 0.;
        }

    }

    void setup_single_meshes() {
        meshes.clear();
        spheres.clear();
        squares.clear();
        lights.clear();

        {
            lights.resize( lights.size() + 1 );
            Light & light = lights[lights.size() - 1];
            light.pos = Vec3(-5,5,5);
            light.radius = 2.5f;
            light.powerCorrection = 2.f;
            light.type = LightType_Spherical;
            light.material = Vec3(1,1,1);
            light.isInCamSpace = false;
        }

        {
            meshes.resize( meshes.size() + 1 );
            Mesh & m = meshes[meshes.size() - 1];
            m.loadOFF("../meshes/space_shuttle.off");
            m.scale(Vec3(0.01,0.01,0.01));
            m.translate(Vec3(1.0,0.0,0.0));
            m.build_arrays();
            m.creerBVH();
            m.material.diffuse_material = Vec3( 1.,0.,0. );
            m.material.specular_material = Vec3( 0.2,0.2,0.2 );
            m.material.shininess = 20;
        }

        // {
        //     meshes.resize( meshes.size() + 1 );
        //     Mesh & m = meshes[meshes.size() - 1];
        //     m.loadOFF("../meshes/suzanne.off");
        //     m.translate(Vec3(1.2,1.0,0.0));
        //     m.build_arrays();
        //     m.creerBVH();
        //     m.material.diffuse_material = Vec3( 1.,0.,0. );
        //     m.material.specular_material = Vec3( 0.2,0.2,0.2 );
        //     m.material.shininess = 20;
        // }

        // {
        //     meshes.resize( meshes.size() + 1 );
        //     Mesh & m = meshes[meshes.size() - 1];
        //     m.loadOFF("../meshes/suzanne.off");
        //     m.translate(Vec3(1.2,-1.0,0.0));
        //     m.build_arrays();
        //     m.creerBVH();
        //     m.material.diffuse_material = Vec3( 1.,0.,0. );
        //     m.material.specular_material = Vec3( 0.2,0.2,0.2 );
        //     m.material.shininess = 20;
        // }
    }

};



#endif
