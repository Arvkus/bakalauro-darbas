#pragma once
#include "common.hpp"
#include "input.hpp"

class Camera{

private:

    // change position based on distance/origin/rotation
    glm::mat4 calculate_cframe(){ // TODO: broken
        glm::vec3 pos = glm::vec3(0.0);
        
        /*
        pos1.x = sin(glm::radians((float)yaw));
        pos1.y = cos(glm::radians((float)yaw));
        pos1.z = 0;
        
        pos2.x = cos(glm::radians((float)pitch));
        pos2.y = 0;
        pos2.z = sin(glm::radians((float)pitch));
        */

        pos.x = sin(glm::radians((float)yaw)) * cos(glm::radians((float)pitch));
        pos.y = cos(glm::radians((float)yaw)) * cos(glm::radians((float)pitch));
        pos.z = sin(glm::radians((float)pitch));
        
        return glm::lookAt(origin + pos*distance, origin, glm::vec3(0,0,1));
    }

public:
    glm::vec3 origin = glm::vec3(0,0,0);

    float speed = .1;
    float distance = 20;

    int yaw   = 45;
    int pitch = 45; 
    int roll  = 0;

    //----------------------------------------------

    glm::mat4 cframe(){ return calculate_cframe(); }
    
    void move()
    {
        //------------------------
        // move origin

        float x = sin(glm::radians((float)yaw));
        float y = cos(glm::radians((float)yaw));

        this->origin.x -= x * (Input::Keys::W - Input::Keys::S)*speed;
        this->origin.y -= y * (Input::Keys::W - Input::Keys::S)*speed;

        this->origin.x += y * (Input::Keys::A - Input::Keys::D)*speed;
        this->origin.y -= x * (Input::Keys::A - Input::Keys::D)*speed;

        //-----------------------
        // rotate camera

        if(Input::Mouse::Middle || Input::Mouse::Left)
        {
            float x = Input::Mouse::Delta.x;
            float y = Input::Mouse::Delta.y;

            yaw = (yaw + (int)x ) % 360;
            pitch = (pitch + (int)y );
            
            if(pitch >  70) pitch =  70;
            if(pitch < -70) pitch = -70; 
        }

        //-----------------------
        // zoom 

        distance -= (distance - Input::Mouse::Wheel) <= 1.0? 0.0 : Input::Mouse::Wheel;
    }
};