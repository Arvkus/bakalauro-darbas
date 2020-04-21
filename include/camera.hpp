#pragma once
#include "common.hpp"
#include "input.hpp"

class Camera{

private:

    // change position based on distance/origin/rotation
    glm::mat4 calculate_cframe(){ // TODO: broken
        glm::vec3 pos; // = glm::vec3(0,0,1);

        pos.x = cos(glm::radians((float)yaw)) * cos(glm::radians((float)pitch));
        pos.y = sin(glm::radians((float)pitch));
        pos.z = sin(glm::radians((float)yaw)) * cos(glm::radians((float)pitch));
        
        return glm::lookAt(origin + pos*distance, origin, glm::vec3(0,1,0));
    }

public:
    glm::vec3 origin = glm::vec3(0,0,0);

    float speed = .1;
    float scroll_speed = 1;
    float distance = 20;

    int yaw   = 0;
    int pitch = 0; 
    int roll  = 0;

    //----------------------------------------------

    glm::mat4 cframe(){ return calculate_cframe(); }

    void set_region(glm::vec3 min, glm::vec3 max)
    {
        distance = (max.length() + min.length()) / 2 ;
        scroll_speed = distance/10;
    }
    
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

        distance -= (distance - Input::Mouse::Wheel*scroll_speed) <= scroll_speed? 0.0 : Input::Mouse::Wheel*scroll_speed;
    }
};