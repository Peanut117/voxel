glslc voxel.comp -o shader.spv

echo Shaders compiled!

cd ../

bin/vulkan
