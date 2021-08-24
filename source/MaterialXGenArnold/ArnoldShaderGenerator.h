//
// TM & (c) 2017 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#ifndef MATERIALX_ARNOLDSHADERGENERATOR_H
#define MATERIALX_ARNOLDSHADERGENERATOR_H

/// @file
/// Arnold OSL shader generator

#include <MaterialXGenOsl/OslShaderGenerator.h>

namespace MaterialX
{

using ArnoldShaderGeneratorPtr = shared_ptr<class ArnoldShaderGenerator>;

/// @class ArnoldShaderGenerator 
/// An OSL shader generator targeting the Arnold renderer
class ArnoldShaderGenerator : public OslShaderGenerator
{
  public:
    ArnoldShaderGenerator(bool writeImageNodeColorSpace=false);

    static ShaderGeneratorPtr create(bool writeImageNodeColorSpace=false) { return std::make_shared<ArnoldShaderGenerator>(writeImageNodeColorSpace); }

    /// Return a unique identifyer for the target this generator is for
    const string& getTarget() const override { return TARGET; }

    /// Unique identifyer for this generator target
    static const string TARGET;

    /// Get whether to write "colorspace" information for image nodes.
    bool getWriteImageNodeColorSpace() const
    {
        return _writeImageNodeColorSpace;
    }

  private:
     bool _writeImageNodeColorSpace;
};

}

#endif
