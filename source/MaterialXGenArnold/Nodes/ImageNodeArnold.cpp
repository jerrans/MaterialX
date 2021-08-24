//
// TM & (c) 2021 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#include <MaterialXGenArnold/Nodes/ImageNodeArnold.h>
#include <MaterialXGenShader/GenContext.h>
#include <MaterialXGenShader/ShaderNode.h>
#include <MaterialXGenShader/ShaderStage.h>
#include <MaterialXGenShader/ShaderGenerator.h>
#include <MaterialXGenShader/Util.h>
#include <MaterialXGenArnold/ArnoldShaderGenerator.h>

#include <iostream>

namespace MaterialX
{
string ImageNodeArnold::COLORSPACE = "colorspace";
string ImageNodeArnold::AUTO_COLORSPACE = "auto";

ShaderNodeImplPtr ImageNodeArnold::create()
{
    return std::make_shared<ImageNodeArnold>();
}

void ImageNodeArnold::addInputs(ShaderNode& node, GenContext& context) const
{
    ShaderGenerator& generator = context.getShaderGenerator();
    ArnoldShaderGenerator* agenererator = dynamic_cast<ArnoldShaderGenerator*>(&generator);
    if (agenererator->getWriteImageNodeColorSpace())
    {
        // Add additional colorspace argument, if string substitution is non-empty
        ShaderInput* input = node.addInput(COLORSPACE, Type::STRING);
        input->setValue(Value::createValue<string>(AUTO_COLORSPACE));
    }
}

void ImageNodeArnold::setValues(const Node& node, ShaderNode& shaderNode, GenContext& context) const
{
    ShaderGenerator& generator = context.getShaderGenerator();
    ArnoldShaderGenerator* agenererator = dynamic_cast<ArnoldShaderGenerator*>(&generator);
    if (agenererator->getWriteImageNodeColorSpace())
    {
        // Look for colorspace attribute on file, if string substitution is non-empty
        InputPtr file = node.getInput("file");
        if (file)
        {
            // Get the colorspace attribute off of the filename input
            const string& colorSpaceValue = AUTO_COLORSPACE;
            if (true)
            {
                ShaderInput* input = shaderNode.getInput(COLORSPACE);
                if (input)
                {
                    input->setValue(Value::createValue<string>(colorSpaceValue));
                }
            }
        }
    }
    SourceCodeNode::setValues(node, shaderNode, context);
}

} // namespace MaterialX
