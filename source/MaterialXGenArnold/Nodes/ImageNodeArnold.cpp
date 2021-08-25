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
#include <MaterialXFormat/Util.h>
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

void ImageNodeArnold::initialize(const InterfaceElement& element, GenContext& context)
{
    ShaderGenerator& generator = context.getShaderGenerator();
    ArnoldShaderGenerator* agenererator = dynamic_cast<ArnoldShaderGenerator*>(&generator);
    _addColorSpaceArgument = agenererator->getWriteImageNodeColorSpace();

    FilePath file;
    if (_addColorSpaceArgument)
    {
        if (!element.isA<Implementation>())
        {
            throw ExceptionShaderGenError("Element '" + element.getName() + "' is not an Implementation element");
        }
        file = element.getAttribute("file");
        file = context.resolveSourceFile(file);
        _functionSource = readFile(file);
        if (_functionSource.empty())
            _addColorSpaceArgument = false;
    }

    if (!_addColorSpaceArgument)
    {
        InterfaceElement* nonConstElement = const_cast<InterfaceElement*>(&element);
        if (element.getName() == "IM_image_color3_arnold")
        {
            nonConstElement->setAttribute("file", "stdlib/genosl/mx_image_color3.osl");
        }
        else
        {
            nonConstElement->setAttribute("file", "stdlib/genosl/mx_image_color4.osl");
        }
        _addColorSpaceArgument = false;
    }

    SourceCodeNode::initialize(element, context);

    if (!_addColorSpaceArgument)
    {
        const string declaration("string colorspace, ");
        const string texturearg(", \"colorspace\", colorspace");
        StringMap removeCMMap;
        removeCMMap[declaration] = EMPTY_STRING;
        removeCMMap[texturearg] = EMPTY_STRING;
        replaceSubstrings(_functionSource, removeCMMap);
    }
}

void ImageNodeArnold::addInputs(ShaderNode& node, GenContext&) const
{
    if (_addColorSpaceArgument)
    {
        // Add additional colorspace argument, if string substitution is non-empty
        ShaderInput* input = node.addInput(COLORSPACE, Type::STRING);
        input->setValue(Value::createValue<string>(AUTO_COLORSPACE));
    }
}

void ImageNodeArnold::setValues(const Node& node, ShaderNode& shaderNode, GenContext& context) const
{
    if (_addColorSpaceArgument)
    {
        // Look for colorspace attribute on file, if string substitution is non-empty
        InputPtr fileInput = node.getInput("file");
        if (fileInput)
        {
            const string& sourceColorSpace = fileInput->getActiveColorSpace();
            std::cout << "Found colorspace: " << sourceColorSpace << " on input" << fileInput->getNamePath() << std::endl;

            // Get the colorspace attribute off of the filename input
            const string& colorSpaceValue = sourceColorSpace.empty() ? AUTO_COLORSPACE : sourceColorSpace;
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
