glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv

echo Shaders compiled!

cd ../

bin/vulkan
