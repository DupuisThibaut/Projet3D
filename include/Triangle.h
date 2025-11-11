#ifndef TRIANGLE_H
#define TRIANGLE_H
#include "Vec3.h"
#include "Ray.h"
#include "Plane.h"
#include <cfloat>

struct RayTriangleIntersection{
    bool intersectionExists;
    float t;
    float w0,w1,w2;
    unsigned int tIndex;
    Vec3 intersection;
    Vec3 normal;
};

class Triangle {
private:
    Vec3 m_c[3] , m_normal;
    float area;
public:
    Triangle() {}
    Triangle( Vec3 const & c0 , Vec3 const & c1 , Vec3 const & c2 ) {
        m_c[0] = c0;
        m_c[1] = c1;
        m_c[2] = c2;
        updateAreaAndNormal();
    }
    void updateAreaAndNormal() {
        Vec3 nNotNormalized = Vec3::cross( m_c[1] - m_c[0] , m_c[2] - m_c[0] );
        float norm = nNotNormalized.length();
        m_normal = nNotNormalized / norm;
        area = norm / 2.f;
    }
    void setC0( Vec3 const & c0 ) { m_c[0] = c0; } // remember to update the area and normal afterwards!
    void setC1( Vec3 const & c1 ) { m_c[1] = c1; } // remember to update the area and normal afterwards!
    void setC2( Vec3 const & c2 ) { m_c[2] = c2; } // remember to update the area and normal afterwards!
    Vec3 const & normal() const { return m_normal; }
    Vec3 projectOnSupportPlane( Vec3 const & p ) const {
        Vec3 result;
        //TODO completer
        Vec3 proj=p-m_c[0];
        float d=Vec3::dot(proj,m_normal);
        result=p-d*m_normal;
        return result;
    }
    float squareDistanceToSupportPlane( Vec3 const & p ) const {
        float result;
        //TODO completer
        Vec3 proj=projectOnSupportPlane(p);
        result=(proj[0]-p[0])*(proj[0]-p[0])*(proj[1]-p[1])*(proj[1]-p[1])*(proj[2]-p[2])*(proj[2]-p[2]);
        return result;
    }
    float distanceToSupportPlane( Vec3 const & p ) const { return sqrt( squareDistanceToSupportPlane(p) ); }
    bool isParallelTo( Line const & L ) const {
        bool result=false;
        //TODO completer
        if(Vec3::dot(m_normal,L.direction())==0)result=true;
        return result;
    }
    Vec3 getIntersectionPointWithSupportPlane( Line const & L ) const {
        // you should check first that the line is not parallel to the plane!
        Vec3 result;
        //TODO completer
        if(isParallelTo(L)==false){
            float denominator =Vec3::dot(L.direction(),m_normal);
            float t = ((Vec3::dot(m_c[1]-m_c[0],m_normal)-(Vec3::dot(L.origin(),m_normal)))/denominator);
            result=L.origin()+t*L.direction();
        }
        return result;
    }
    void computeBarycentricCoordinates( Vec3 const & p , float & u0 , float & u1 , float & u2 ) const {
        //TODO Complete
        Vec3 v0 = m_c[1] - m_c[0];
        Vec3 v1 = m_c[2] - m_c[0];
        Vec3 v2 = p - m_c[0];

        float d00 = Vec3::dot(v0, v0);
        float d01 = Vec3::dot(v0, v1);
        float d11 = Vec3::dot(v1, v1);
        float d20 = Vec3::dot(v2, v0);
        float d21 = Vec3::dot(v2, v1);
        float denom = d00 * d11 - d01 * d01;

        u1 = (d11 * d20 - d01 * d21) / denom;
        u2 = (d00 * d21 - d01 * d20) / denom;
        u0 = 1.0f - u1 - u2;
    }

    RayTriangleIntersection getIntersection( Ray const & ray ) const {
        RayTriangleIntersection result;
        // 1) check that the ray is not parallel to the triangle:
        bool parallel=isParallelTo(ray);
        if(parallel){
            result.t=FLT_MAX;
            result.intersectionExists=false;
            return result;
        }
        // 2) check that the triangle is "in front of" the ray:
        float denominator =Vec3::dot(ray.direction(),m_normal);
        float t = ((Vec3::dot(m_c[0] - ray.origin(),m_normal))/denominator);

        Vec3 intersectionPoint = ray.origin()+t*ray.direction();
        if(t<=0){
            result.t = FLT_MAX;
            result.intersectionExists = false;
            return result;
        }
        // 3) check that the intersection point is inside the triangle:
        // CONVENTION: compute u,v such that p = w0*c0 + w1*c1 + w2*c2, check that 0 <= w0,w1,w2 <= 1
        float a,b,g;
        computeBarycentricCoordinates(intersectionPoint, a, b, g);
        if(a<0||b<0||g<0||a>1||b>1||g>1){
            result.t=FLT_MAX;
            result.intersectionExists = false;
            return result;
        }
        // 4) Finally, if all conditions were met, then there is an intersection! :
        result.intersectionExists=true;
        result.intersection=intersectionPoint;
        result.t = t;     
        result.w0=a;
        result.w1=b;
        result.w2=g;
        result.normal=m_normal;
        return result;
    }
};
#endif
