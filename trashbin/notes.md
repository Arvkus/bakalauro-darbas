```cpp

Memory::init(&device, &command_pool);

Image image = Memory::create_image();
Buffer buffer = Memory::create_buffer();
//image.transition_layout();
image.fill_memory(&pixels, layout);
//image.transition_layout();
image.create_view();