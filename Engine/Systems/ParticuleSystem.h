#ifndef PARTICULESYSTEM_H
#define PARTICULESYSTEM_H
#include <glm/fwd.hpp>
#include <unordered_map>
#include <cfloat>
#include <vector>

#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <string_view>

#include "../Components/ParticuleComponent.h"

class ParticuleSystem {
public:
    EntityManager* entityManager;

    ParticuleSystem(EntityManager* em) : entityManager(em) {
        initialize();
    }

    void update(float t){
        for (auto& [id, p] : entityManager->GetComponents<ParticuleComponent>()) {
            // p.update(t);
            auto& particule=entityManager->GetComponent<ParticuleComponent>(id);
            // std::cout<<"particule x : "<<particule.pos[0].x<<" particule y : "<<particule.pos[0].y<<" particule z : "<<particule.pos[0].z<<std::endl;
            // std::cout<<"time : "<<t<<std::endl;
            for(int i=0;i<particule.nb;i++){
                particule.speed[i][2] -= t*9.81f;
                particule.pos[i] += 0.1f*particule.speed[i];
                // particule.pos[i] += particule.speed[i];

                if (particule.pos[i][2] < 0.0) {
                    particule.speed[i][2] = -0.8 * particule.speed[i][2];
                    particule.pos[i][2] = 0.0;
                }
                particule.age[i] += 1.0f;
                // age+=t/15.0f;
                // std::cout<<"age : "<<age<<std::endl;
                if(particule.age[i] >= particule.ageMax[i]) init(i,&particule);
            }
            // std::cout<<"particule x : "<<particule.pos[0].x<<" particule y : "<<particule.pos[0].y<<" particule z : "<<particule.pos[0].z<<std::endl;
        }
    }

    void init(int i, ParticuleComponent* p){
        p->pos[i] = p->position;
        float angle = 2.0 * M_PI * rand() / RAND_MAX;
        float norm = 0.04 * rand() / RAND_MAX;
        p->speed[i] = glm::vec3(norm * cos(angle), norm * sin(angle),
                    rand() / static_cast<float>(RAND_MAX));
        p->age[i] = 0.0f;
        p->ageMax[i] = 50.0f + (100.0f * rand() / float(RAND_MAX));
        p->rayon[i]=rand()/float(RAND_MAX)*0.05f;
    }

    void initialize(){
        for (auto& [id, p] : entityManager->GetComponents<ParticuleComponent>()) {
            // p.update(t);
            auto& particule=entityManager->GetComponent<ParticuleComponent>(id);
            particule.pos.resize(particule.nb);
            particule.speed.resize(particule.nb);
            particule.age.resize(particule.nb);
            particule.ageMax.resize(particule.nb);
            particule.rayon.resize(particule.nb);
            for(int i=0;i<particule.nb;i++){
                init(i,&particule);
            }
        }
    }
};

#endif // PARTICULESYSTEM_H