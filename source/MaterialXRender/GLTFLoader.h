//
// TM & (c) 2021 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#ifndef MATERIALX_TINYGLTFLOADER_H
#define MATERIALX_TINYGLTFLOADER_H

/// @file 
/// GLTF geometry format loader using the TinyGLTF library

#include <MaterialXRender/GeometryHandler.h>

namespace MaterialX
{

/// Shared pointer to a GLTFLoader
using GLTFLoaderPtr = std::shared_ptr<class GLTFLoader>;

/// @class GLTFLoader
/// Wrapper for geometry loader to read in GLTF files using the TinyGLTF library.
class MX_RENDER_API GLTFLoader : public GeometryLoader
{
  public:
    GLTFLoader()
    : _debugLevel(0)
    {
        _extensions = { "glb", "GLB", "gltf", "GLTF" };
    }
    virtual ~GLTFLoader() { }

    /// Create a new GLTFLoader
    static GLTFLoaderPtr create() { return std::make_shared<GLTFLoader>(); }

    /// Load geometry from file path
    bool load(const FilePath& filePath, MeshList& meshList) override;

private:
    unsigned int _debugLevel;
};

} // namespace MaterialX

#endif
