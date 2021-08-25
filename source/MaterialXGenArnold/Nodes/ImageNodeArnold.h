//
// TM & (c) 2021 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#ifndef MATERIALX_IMAGECODENODEARNOLD_H
#define MATERIALX_IMAGECODENODEARNOLD_H

#include <MaterialXGenShader/Nodes/SourceCodeNode.h>

namespace MaterialX
{

/// Extending the HwSourceCodeNode with requirements for image nodes.
class MX_GENSHADER_API ImageNodeArnold : public SourceCodeNode
{
  public:
    static ShaderNodeImplPtr create();

    void initialize(const InterfaceElement& element, GenContext& context) override;
    void addInputs(ShaderNode& node, GenContext& context) const override;
    void setValues(const Node& node, ShaderNode& shaderNode, GenContext& context) const override;

  protected:
    // Additional colorspace argument and default "auto" value
    static string COLORSPACE;
    static string AUTO_COLORSPACE;

    /// Protected constructor
    bool _addColorSpaceArgument = false;
};

} // namespace MaterialX

#endif
