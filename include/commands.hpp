#include "common.hpp"
#include "instance.hpp"

class CommandsManager{
public:
    void init(Instance *instance)
    {
        this->instance = instance;
    }

    void destroy()
    {

    }

private:
    Instance *instance;
};