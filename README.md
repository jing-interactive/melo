# MeshViewer
OBJ / GLTF mesh viewer for Windows and macOS.

To build this project, you need:

* [Cinder](https://github.com/cinder/Cinder)
* [Cinder-VNM](https://github.com/jing-interactive/Cinder-VNM)
* [Cinder-Nodes](https://github.com/jing-interactive/Cinder-Nodes)

The folder structure should appear like this:

```
Cinder/
    blocks/
        Cinder-VNM/
        Cinder-Nodes/
    include/
MeshViewer/
    assets/
        Cube/
            Cube.gltf
        Amazing-gltf-files/
            scene.gltf
            scene.bin
        Symbolic-link-is-also-supported