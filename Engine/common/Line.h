#ifndef LINE_H
#define LINE_H

#include <cmath>
#include <iostream>

#include <glm/glm.hpp>

class Line {
private:
    glm::vec3 m_origin , m_direction;
public:
    Line() {}
    Line( glm::vec3 const & o , glm::vec3 const & d )  {
        m_origin = o;
        m_direction = d; m_direction = glm::normalize(m_direction);
    }
    glm::vec3 & origin() { return m_origin; }
    glm::vec3 const & origin() const { return m_origin; }
    glm::vec3 & direction() { return m_direction; }
    glm::vec3 const & direction() const { return m_direction; }
    glm::vec3 project( glm::vec3 const & p ) const {
        glm::vec3 result;
        return result;
    }
    float squareDistance( glm::vec3 const & p ) const {
        float result;
        return result;
    }
    float distance( glm::vec3 const & p ) const {
        return sqrt( squareDistance(p) );
    }
};

// Pour lire un vec3 depuis un fichier
static inline std::istream& operator>>(std::istream& in, glm::vec3& v) {
    return in >> v.x >> v.y >> v.z;
}

#endif
