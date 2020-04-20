rem C:/VulkanSDK/1.2.131.2/Bin/glslc.exe shaders/shader.vert -o bin/shaders/vert.spv
rem C:/VulkanSDK/1.2.131.2/Bin/glslc.exe shaders/shader.frag -o bin/shaders/frag.spv
rem https://stackoverflow.com/questions/39615/how-to-loop-through-files-matching-wildcard-in-batch-file

FOR %%F in (shaders/*.vert) DO (
    echo Compiling: %%~nF
    C:/VulkanSDK/1.2.131.2/Bin/glslc.exe shaders/%%~nF.vert ^
        -o bin/shaders/%%~nF.spv
)

FOR %%F in (shaders/*.frag) DO (
    echo Compiling: %%~nF
    C:/VulkanSDK/1.2.131.2/Bin/glslc.exe shaders/%%~nF.frag ^
        -o bin/shaders/%%~nF.spv
)





