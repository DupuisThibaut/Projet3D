#ifndef _H_2D_GEOMETRY
#define _H_2D_GEOMETRY
#include "matrices.h"
#include <cmath>
#include <cfloat>
#include <vector>

// 2D Point
typedef vec2 Point2D;

// 2D Line
typedef struct Line2D {
    Point2D start;
    Point2D end;

    inline Line2D(){ }
    inline Line2D(const Point2D& s, const Point2D& e) : start(s), end(e) { }
} Line2D;
// Helpers for Line2D
float Length(const Line2D& line) {
    return Magnitude(line.end - line.start);
}
float LengthSq(const Line2D& line) {
    return MagnitudeSq(line.end - line.start);
}

// 2D Circle
typedef struct Circle{
    Point2D position;
    float radius;

    inline Circle() : radius(1.0f) { }
    inline Circle(const Point2D& pos, float r) : position(pos), radius(r) { }
} Circle;

// 2D Rectangle
typedef struct Rectangle2D{
    Point2D origin;
    vec2 size;

    inline Rectangle2D() : size(1.0f, 1.0f) { }
    inline Rectangle2D(const Point2D& o, const vec2& s) : origin(o), size(s) { }
} Rectangle2D;
// Helpers for Rectangle2D
vec2 GetMin(const Rectangle2D& rect) {
    vec2 p1 = rect.origin;
    vec2 p2 = rect.origin + rect.size;
    return vec2(fmin(p1.x, p2.x), fmin(p1.y, p2.y));
}
vec2 GetMax(const Rectangle2D& rect) {
    vec2 p1 = rect.origin;
    vec2 p2 = rect.origin + rect.size;
    return vec2(fmax(p1.x, p2.x), fmax(p1.y, p2.y));
}
Rectangle2D FromMinMax(const vec2& min, const vec2& max) {
    return Rectangle2D(min, max - min);
}

// 2D Oriented Rectangle
typedef struct OrientedRectangle{
    Point2D position;
    vec2 halfExtents;
    float rotation;
    inline OrientedRectangle() : halfExtents(1.0f, 1.0f), rotation(0.0f) { }
    inline OrientedRectangle(const Point2D& pos, const vec2& extents) : position(pos), halfExtents(extents), rotation(0.0f) { }
    inline OrientedRectangle(const Point2D& pos, const vec2& extents, float rot) : position(pos), halfExtents(extents), rotation(rot) { }
} OrientedRectangle;

// Containment functions
// Point
bool PointOnLine(const Point2D& p, const Line2D& line){
    float dy = (line.end.y - line.start.y);
    float dx = (line.end.x - line.start.x);
    float M = dy / dx;
    float B = line.start.y - M * line.start.x;
    return CMP(p.y, M * p.x + B);
}
bool PointInCircle(const Point2D& point, const Circle& c){
    Line2D line(point, c.position);
    if(LengthSq(line) < c.radius * c.radius){
        return true;
    }
    return false;
}
bool PointInRectangle(const Point2D& point, const Rectangle2D& rectangle){
    vec2 min = GetMin(rectangle);
    vec2 max = GetMax(rectangle);
    return min.x <= point.x &&
           min.y <= point.y &&
           point.x <= max.x &&
           point.y <= max.y;
}
bool PointInOrientedRectangle(const Point2D& point, const OrientedRectangle& rectangle){
    vec2 rotVector = point - rectangle.position;
    float theta = -DEG2RAD(rectangle.rotation);
    mat2 zRotation2x2 = mat2(
        cosf(theta), sinf(theta),
        -sinf(theta),  cosf(theta)
    );
    vec2 localPoint = zRotation2x2 * rotVector;
    Rectangle2D localRectangle(Point2D(), rectangle.halfExtents * 2.0f);
    vec2 localPoint = localPoint + rectangle.halfExtents;
    return PointInRectangle(localPoint, localRectangle);
}
// Line
bool LineCircle(const Line2D& l, const Circle& c){
    vec2 ab = l.end - l.start;
    float t = glm::dot(c.position - l.start, ab) / glm::dot(ab, ab);
    if(t < 0.0f || t > 1.0f){
        return false;
    }
    Point2D closestPoint = l.start + ab * t;
    Line2D circleToClosest(c.position, closestPoint);
    return LengthSq(circleToClosest) < c.radius * c.radius;
}
bool LineRectangle(const Line2D& l, const Rectangle2D& r){
    if(PointInRectangle(l.start, r) || PointInRectangle(l.end, r)){
        return true;
    }
    vec2 norm = Normalized(l.end - l.start);
    norm.x = (norm.x != 0.0f) ? 1.0f / norm.x : 0.0f;
    norm.y = (norm.y != 0.0f) ? 1.0f / norm.y : 0.0f;
    vec2 min = (GetMin(r) - l.start) * norm;
    vec2 max = (GetMax(r) - l.start) * norm;
    float tmin = fmaxf(fminf(min.x, max.x), fminf(min.y, max.y));
    float tmax = fminf(fmaxf(min.x, max.x), fmaxf(min.y, max.y));
    if(tmax < 0.0f || tmin > tmax){
        return false;
    }
    float t = (tmin < 0.0f) ? tmax : tmin;
    return t > 0.0f && t * t < LengthSq(l);
}
bool LineOrientedRectangle(const Line2D& line, const OrientedRectangle& rectangle){
    float theta = -DEG2RAD(rectangle.rotation);
    mat2 zRotation2x2 = mat2(
        cosf(theta), sinf(theta),
        -sinf(theta),  cosf(theta)
    );
    Line2D localLine;
    vec2 rotVector = line.start - rectangle.position;
    localLine.start = zRotation2x2 * rotVector + rectangle.halfExtents;
    rotVector = line.end - rectangle.position;
    localLine.end = zRotation2x2 * rotVector + rectangle.halfExtents;
    Rectangle2D localRectangle(Point2D(), rectangle.halfExtents * 2.0f);
    return LineRectangle(localLine, localRectangle);
}

#define PointLine(point, line) PointOnLine(point, line)
#define LinePoint(line, point) PointOnLine(point, line)
#define CircleLine(circle, line) LineCircle(line, circle)
#define RectangleLine(rectangle, line) LineRectangle(line, rectangle)
#define OrientedRectangleLine(orectangle, line) LineOrientedRectangle(line, orectangle)

// Collisions 2D
bool CircleCircle(const Circle& c1, const Circle& c2){
    Line2D line(c1.position, c2.position);
    float radiusSum = c1.radius + c2.radius;
    return LengthSq(line) <= radiusSum * radiusSum;
}
bool CircleRectangle(const Circle& circle, const Rectangle2D& rect){
    vec2 min = GetMin(rect);
    vec2 max = GetMax(rect);
    Point2D closestPoint = circle.position;
    if(closestPoint.x < min.x) closestPoint.x = min.x;
    else if (closestPoint.x > max.x) closestPoint.x = max.x;
    closestPoint.y = (closestPoint.y < min.y) ? min.y : closestPoint.y;
    closestPoint.y = (closestPoint.y > max.y) ? max.y : closestPoint.y;
    Line2D line(circle.position, closestPoint);
    return LengthSq(line) <= circle.radius * circle.radius;
}
#define RectangleCircle(rectangle, circle) CircleRectangle(circle, rectangle)
#define CLAMP(number, minimum, maximum) number = (number < minimum) ? minimum : (number > maximum) ? maximum : number;

bool CircleOrientedRectangle(const Circle& circle, const OrientedRectangle& rect){
    vec2 r = circle.position - rect.position;
    float theta = -DEG2RAD(rect.rotation);
    mat2 zRotation2x2 = mat2(
        cosf(theta), sinf(theta),
        -sinf(theta),  cosf(theta)
    );
    Point2D localCirclePos = zRotation2x2 * r;
    Circle lCircle(r+rect.halfExtents, circle.radius);
    Rectangle2D lRect(Point2D(), rect.halfExtents * 2.0f);
    return CircleRectangle(lCircle, lRect);
}
#define OrientedRectangleCircle(rectangle, circle) CircleOrientedRectangle(circle, rectangle)

bool RectangleRectangle(const Rectangle2D& rect1, const Rectangle2D& rect2){
    vec2 aMin = GetMin(rect1);
    vec2 aMax = GetMax(rect1);
    vec2 bMin = GetMin(rect2);
    vec2 bMax = GetMax(rect2);
    bool overX = ((bMin.x <= aMax.x) && (aMin.x <= bMax.x));
    bool overY = ((bMin.y <= aMax.y) && (aMin.y <= bMax.y));
    return overX && overY;
}
#define OVERLAP(aMin, aMax, bMin, bMax) ((bMin <= aMax) && (aMin <= bMax))
// Interval 2D
typedef struct Interval2D{
    float min;
    float max;
} Interval2D;
// Helpers for Interval2D
Interval2D GetInterval(const Rectangle2D& rect, const vec2& axis){
    Interval2D result;
    vec2 min = GetMin(rect);
    vec2 max = GetMax(rect);
    vec2 vert[] = {
        min,
        vec2(min.x, max.y),
        max,
        vec2(max.x, min.y)
    };
    result.min = result.max = glm::dot(axis, vert[0]);
    for(int i = 1; i < 4; i++){
        float projection = glm::dot(axis, vert[i]);
        if(projection < result.min){
            result.min = projection;
        }
        if(projection > result.max){
            result.max = projection;
        }
    }
    return result;
}

bool OverlapOnAxis(const Rectangle2D& rect1, const Rectangle2D& rect2, const vec2& axis){
    Interval2D a = GetInterval(rect1, axis);
    Interval2D b = GetInterval(rect2, axis);
    return ((b.min <= a.max) && (a.min <= b.max));
}
bool RectangleRectangleSAT(const Rectangle2D& rect1, const Rectangle2D& rect2){
    vec2 axisToTest[] = {
        vec2(1.0f, 0.0f),
        vec2(0.0f, 1.0f)
    };
    for(int i = 0; i < 2; i++){
        if(!OverlapOnAxis(rect1, rect2, axisToTest[i])){
            return false;
        }
    }
    return true;
}
// bool GenericSAT(Rectangle2D* shape1, Rectangle2D* shape2){
//     std::vector<vec2>normals = GetFaceNormals(shape1);
//     for(int i = 0; i < normals.size(); i++){
//         if(!OverlapOnAxis(*shape1, *shape2, normals[i])){
//             return true;
//         }
//     }
//     normals = GetFaceNormals(shape2);
//     for(int i = 0; i < normals.size(); i++){
//         if(!OverlapOnAxis(*shape1, *shape2, normals[i])){
//             return true;
//         }
//     }
//     std::vector<vec2>edges1 = GetEdges(shape1);
//     std::vector<vec2>edges2 = GetEdges(shape2);
//     for(int i = 0; i < edges1.size(); i++){
//         for(int j = 0; j < edges2.size(); j++){
//             vec2 axis = glm::cross(edges1[i], edges2[j]);
//             if(!OverlapOnAxis(*shape1, *shape2, axis)){
//                 return true;
//             }
//         }
//     }
//     return false;
// }

Interval2D GetInterval(const OrientedRectangle& rect, const vec2& axis){
    Rectangle2D r = Rectangle2D(Point2D(rect.position - rect.halfExtents), rect.halfExtents * 2.0f);
    vec2 min = GetMin(r);
    vec2 max = GetMax(r);
    vec2 verts[]={
        min,max, vec2(min.x, max.y), vec2(max.x, min.y)
    };
    float t = DEG2RAD(rect.rotation);
    mat2 zRot = {
        cosf(t), -sinf(t),
        sinf(t),  cosf(t)
    };
    for(int i = 0; i < 4; i++){
        vec2 r = verts[i] - rect.position;
        r = zRot * r;
        verts[i] = r + rect.position;
    }
    Interval2D result;
    result.min = result.max = glm::dot(axis, verts[0]);
    for(int i = 1; i < 4; i++){
        float projection = glm::dot(axis, verts[i]);
        result.min = (projection<result.min)?projection :result.min;
        result.max = (projection>result.max)?projection :result.max;
    }   
    return result;
}

bool OverlapOnAxis(const Rectangle2D& rect1, const OrientedRectangle& rect2, const vec2& axis){
    Interval2D a = GetInterval(rect1, axis);
    Interval2D b = GetInterval(rect2, axis);
    return ((b.min <= a.max) && (a.min <= b.max));
}

bool RectangleOrientedRectangle(const Rectangle2D& rect1, const OrientedRectangle& rect2){
    vec2 axisToTest[] = {
        vec2(1.0f, 0.0f),
        vec2(0.0f, 1.0f),
        vec2(), vec2()
    };
    float t= DEG2RAD(rect2.rotation);
    mat2 zRot = {
        cosf(t), sinf(t),
        -sinf(t),  cosf(t)
    };
    vec2 axis = Normalized(vec2(rect2.halfExtents.x, 0.0f));
    axis = zRot * axis;
    axisToTest[2] = axis;
    axis = Normalized(vec2(0.0f, rect2.halfExtents.y));
    axis = zRot * axis;
    axisToTest[3] = axis;
    for(int i = 0; i < 4; i++){
        if(!OverlapOnAxis(rect1, rect2, axisToTest[i])){
            return false;
        }
    }
    return true;
}

// Oriented to Oriented
bool OrientedRectangleOrientedRectangle(const OrientedRectangle& rect1, const OrientedRectangle& rect2){
    Rectangle2D local1(Point2D(), rect1.halfExtents * 2.0f);
    vec2 r = rect2.position - rect1.position;
    OrientedRectangle local2(rect2.position, rect2.halfExtents, rect2.rotation);
    local2.rotation = rect2.rotation - rect1.rotation;
    float t = -DEG2RAD(rect1.rotation);
    mat2 zRot = {
        cosf(t), sinf(t),
        -sinf(t),  cosf(t)
    };
    vec2 localPos = zRot * r + rect1.halfExtents;
    return RectangleOrientedRectangle(local1, local2);
}

// Optimisations

Circle ContainingCircle(Point2D* pArray, int arrayCount){
    Point2D center;
    for(int i = 0; i < arrayCount; i++){
        center += pArray[i];
    }
    center = center * (1.0f / float(arrayCount));
    Circle result(center, 1.0f);
    result.radius = MagnitudeSq(center - pArray[0]);
    for(int i = 1; i < arrayCount; i++){
        float distSq = MagnitudeSq(center - pArray[i]);
        if(distSq > result.radius){
            result.radius = distSq;
        }
    }
    result.radius = sqrtf(result.radius);
    return result;
}

Rectangle2D ContainingRectangle(Point2D* pArray, int arrayCount){
    vec2 min = pArray[0];
    vec2 max = pArray[0];
    for(int i = 0; i < arrayCount; i++){
        min.x = pArray[i].x < min.x ? pArray[i].x : min.x;
        min.y = pArray[i].y < min.y ? pArray[i].y : min.y;
        max.x = pArray[i].x > max.x ? pArray[i].x : max.x;
        max.y = pArray[i].y > max.y ? pArray[i].y : max.y;
    }
    return FromMinMax(min, max);
}

typedef struct BoundingShape{
    int numCircles;
    Circle* circles;
    int numRectangles;
    Rectangle2D* rectangles;

    inline BoundingShape() : numCircles(0), circles(nullptr), numRectangles(0), rectangles(nullptr) { }
};
bool PointInShape(const BoundingShape& shape, const Point2D& point){
    for(int i = 0; i < shape.numCircles; i++){
        if(PointInCircle(point, shape.circles[i])){
            return true;
        }
    }
    for(int i = 0; i < shape.numRectangles; i++){
        if(PointInRectangle(point, shape.rectangles[i])){
            return true;
        }
    }
    return false;
}
#endif