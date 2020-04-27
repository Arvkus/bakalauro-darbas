#pragma once
#include "common.hpp"

namespace Input
{
    namespace Keys
    {
        bool A = false;
        bool W = false;
        bool S = false;
        bool D = false;
        bool L = false;
        bool Minus = false;
        bool Equal = false;
    }

    namespace Mouse
    {
        bool Left = false;
        bool Middle = false;
        bool Right = false;

        glm::vec2 Delta(0);
        glm::vec2 Position(0);
        float Wheel = 0;
    }

    void mouse_move_callback(GLFWwindow* window, double x, double y)
    {
        Input::Mouse::Delta = glm::vec2(x, y) - Input::Mouse::Position;
        Input::Mouse::Position = glm::vec2(x, y);
    }

    void mouse_scroll_callback(GLFWwindow* window, double x, double y)
    {
        Input::Mouse::Wheel = y;
    }

    void mouse_press_callback(GLFWwindow* window, int button, int action, int mods)
    {
        if(action == GLFW_PRESS){
            if(button == GLFW_MOUSE_BUTTON_LEFT) Input::Mouse::Left = true; 
            if(button == GLFW_MOUSE_BUTTON_MIDDLE) Input::Mouse::Middle = true; 
            if(button == GLFW_MOUSE_BUTTON_RIGHT) Input::Mouse::Right = true; 
        }
        if(action == GLFW_RELEASE){
            if(button == GLFW_MOUSE_BUTTON_LEFT) Input::Mouse::Left = false; 
            if(button == GLFW_MOUSE_BUTTON_MIDDLE) Input::Mouse::Middle = false; 
            if(button == GLFW_MOUSE_BUTTON_RIGHT) Input::Mouse::Right = false; 
        }
    }

    void key_press_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if(action == GLFW_PRESS)
        {
            if(key == GLFW_KEY_UP) Input::Keys::W = true;
            if(key == GLFW_KEY_LEFT) Input::Keys::A = true;
            if(key == GLFW_KEY_DOWN) Input::Keys::S = true;
            if(key == GLFW_KEY_RIGHT) Input::Keys::D = true;

            if(key == GLFW_KEY_W) Input::Keys::W = true;
            if(key == GLFW_KEY_A) Input::Keys::A = true;
            if(key == GLFW_KEY_S) Input::Keys::S = true;
            if(key == GLFW_KEY_D) Input::Keys::D = true;
            if(key == GLFW_KEY_L) Input::Keys::L = true;

            if(key == GLFW_KEY_MINUS) Input::Keys::Minus = true;
            if(key == GLFW_KEY_EQUAL) Input::Keys::Equal = true;
        }

        if(action == GLFW_RELEASE)
        {
            if(key == GLFW_KEY_UP) Input::Keys::W = false;
            if(key == GLFW_KEY_LEFT) Input::Keys::A = false;
            if(key == GLFW_KEY_DOWN) Input::Keys::S = false;
            if(key == GLFW_KEY_RIGHT) Input::Keys::D = false;

            if(key == GLFW_KEY_W) Input::Keys::W = false;
            if(key == GLFW_KEY_A) Input::Keys::A = false;
            if(key == GLFW_KEY_S) Input::Keys::S = false;
            if(key == GLFW_KEY_D) Input::Keys::D = false;
            if(key == GLFW_KEY_L) Input::Keys::L = false;

            if(key == GLFW_KEY_MINUS) Input::Keys::Minus = false;
            if(key == GLFW_KEY_EQUAL) Input::Keys::Equal = false;
        }
    }

    void reset()
    {
        Input::Mouse::Wheel = 0;
        Input::Mouse::Delta = glm::vec3(0);
    }
};
