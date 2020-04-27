```cpp
// https://github.com/samhocevar/portable-file-dialogs


class Animal{
public:
    std::string name = "hohho";
    virtual void sound(){
        msg::printl("angry animal sounds");
    }
};

class Dog : public Animal{
public:
    int age = 34;
    void sound(){
        msg::printl("bark");
    }
};

int main(int argc, char *argv[])
{

    std::vector<std::unique_ptr<Animal>> animals;
    std::unique_ptr<Animal> anim(new Animal());


    animals.push_back(std::move(anim));
    animals.push_back(std::unique_ptr<Dog>(new Dog()));

 
    
    for(std::unique_ptr<Animal>& a : animals){
        a->sound();
    }
    