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
- Support Linux / Android / iOS
- Support Sketchfab download API
- FrameGraph

# To build `samples/MeshViewer`, you need:

* [Cinder](https://github.com/cinder/Cinder)
* [Cinder-VNM](https://github.com/jing-interactive/Cinder-VNM)

The folder structure should appear like this:

```
Cinder/
    blocks/
        Cinder-VNM/
        melo/
            samples/MeshViewer/vc2015/MeshViewer.sln
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
```

# PBR shader macros

## vertex inputs

- HAS_NORMALS
- HAS_TANGENTS
- HAS_VERTEX_COLOR_VEC4
- HAS_UV_SET1
- HAS_UV_SET2

- USE_MORPHING
- USE_SKINNING

- HAS_TARGET_POSITION0
- HAS_TARGET_POSITION1
- HAS_TARGET_POSITION2
- HAS_TARGET_POSITION3
- HAS_TARGET_POSITION4
- HAS_TARGET_POSITION5
- HAS_TARGET_POSITION6
- HAS_TARGET_POSITION7
- HAS_TARGET_NORMAL0
- HAS_TARGET_NORMAL1
- HAS_TARGET_NORMAL2
- HAS_TARGET_NORMAL3
- HAS_TARGET_NORMAL4
- HAS_TARGET_TANGENT0
- HAS_TARGET_TANGENT1
- HAS_TARGET_TANGENT2
- HAS_TARGET_TANGENT3
- HAS_TARGET_TANGENT4
- HAS_JOINT_SET1
- HAS_JOINT_SET2
- HAS_JOINT_SET3
- HAS_JOINT_SET4
- HAS_WEIGHT_SET1
- HAS_WEIGHT_SET2


## material types
- MATERIAL_SPECULARGLOSSINESS
- MATERIAL_METALLICROUGHNESS
- MATERIAL_UNLIT
- MATERIAL_ANISOTROPY
- MATERIAL_SUBSURFACE
- MATERIAL_THIN_FILM
- MATERIAL_THICKNESS
- MATERIAL_ABSORPTION
- MATERIAL_IOR
- MATERIAL_TRANSMISSION

## texture maps
- HAS_BASE_COLOR_MAP
- HAS_NORMAL_MAP
- HAS_METALLIC_ROUGHNESS_MAP
- HAS_OCCLUSION_MAP
- HAS_EMISSIVE_MAP
- HAS_SUBSURFACE_COLOR_MAP
- HAS_SUBSURFACE_THICKNESS_MAP
- HAS_ANISOTROPY_MAP
- HAS_ANISOTROPY_DIRECTION_MAP
- HAS_SPECULAR_GLOSSINESS_MAP
- HAS_METALLICROUGHNESS_SPECULAROVERRIDE_MAP
- HAS_SHEEN_COLOR_INTENSITY_MAP
- HAS_THIN_FILM_MAP
- HAS_THIN_FILM_THICKNESS_MAP
- HAS_THICKNESS_MAP
- HAS_CLEARCOAT_TEXTURE_MAP
- HAS_CLEARCOAT_ROUGHNESS_MAP
- HAS_CLEARCOAT_NORMAL_MAP

## alpha mode
- ALPHAMODE_OPAQUE
- ALPHAMODE_MASK

## use misc
- USE_IBL
- USE_PUNCTUAL

## debug

- DEBUG_BASECOLOR
- DEBUG_ALPHA
- DEBUG_NORMAL
- DEBUG_TANGENT
- DEBUG_METALLIC
- DEBUG_ROUGHNESS
- DEBUG_BITANGENT
- DEBUG_OCCLUSION
- DEBUG_F0
- DEBUG_FEMISSIVE
- DEBUG_FSPECULAR
- DEBUG_FDIFFUSE
- DEBUG_FSHEEN
- DEBUG_FCLEARCOAT
- DEBUG_FSUBSURFACE
- DEBUG_THICKNESS
- DEBUG_FTRANSMISSION
