#ifndef RAY_H
#define RAY_H
#include "Line.h"
class Ray : public Line {
public:
    Ray() : Line() {}
    Ray( glm::vec3 const & o , glm::vec3 const & d ) : Line(o,d) {}
};
#endif
