//
// TM & (c) 2021 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#ifndef MATERIALX_CGLTFLOADER_H
#define MATERIALX_CGLTFLOADER_H

/// @file 
/// GLTF format loader using the cgltf library

#include <MaterialXRender/GeometryHandler.h>

namespace MaterialX
{

/// Shared pointer to a GLTFLoader
using GLTFLoader2Ptr = std::shared_ptr<class GLTFLoader2>;

/// @class GLTFLoader2
/// Wrapper for loader to read in GLTF files using the cgltf library.
class MX_RENDER_API GLTFLoader2 : public GeometryLoader
{
  public:
    GLTFLoader2()
    : _debugLevel(0)
    {
        _extensions = { "glb", "GLB", "gltf", "GLTF" };
    }
    virtual ~GLTFLoader2() { }

    /// Create a new GLTFLoader
    static GLTFLoader2Ptr create() { return std::make_shared<GLTFLoader2>(); }

    /// Load geometry from file path
    bool load(const FilePath& filePath, MeshList& meshList) override;

private:
    unsigned int _debugLevel;
};

} // namespace MaterialX

#endif
