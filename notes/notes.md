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

```

new descriptor pseudo code
```cpp

class Descriptor{...};

Memory::init(&Instance)
Descriptor::init(&Instance);
Descriptor md; // model descriptor

// add_buffer(type, count, size)
// add_sampler(type, count, width, height)
// bindings in order
md.add_buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, sizeof(Material));
md.add_buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 32, 256);
md.add_sampler(VK_DESCRIPTOR_TYPE_UNIFORM_SAMPLER, 32, 512, 512);
md.create_sets();

```

```cpp



```



10h pradejo laseti
13h skundimas

nuo 11h vanduo nenaudojamas

14h toletas nuleistas 1 karta