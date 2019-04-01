# melo
melo is mesh loader for OBJ, glTF2 and PLY, also includes a `Cinder` based mesh viewer on Windows and macOS.

# Features

- Support Windows and macOS
- Can be used outside of Cinder
- Loading OBJ meshes through [syoyo/tinyobjloader](https://github.com/syoyo/tinyobjloader)
- Loading glTF2 meshes through [syoyo/tinygltf](https://github.com/syoyo/tinygltf)
- PBR rendering w/ modified shaders from [KhronosGroup/glTF-WebGL-PBR](https://github.com/KhronosGroup/glTF-WebGL-PBR/tree/master/shaders)

# TODO
- Implement skinning animation
- Implement morph animation
- Support Linux
- Support mobile platforms
- Support Sketchfab download API

# To build `samples/MeshViewer`, you need:

* [Cinder](https://github.com/cinder/Cinder)
* [Cinder-VNM](https://github.com/jing-interactive/Cinder-VNM)

The folder structure should appear like this:

```
Cinder/
    blocks/
        Cinder-VNM/
        melo/
            samples/BasicSample/vc2015/MeshViewer.sln
            assets/
                Cube/
                    Cube.gltf
                cerberus/
                    Cerberus.obj
                Awesome-gltf-files/
                    scene.gltf
                    scene.bin
                Symbolic-links-are-also-supported
    include/