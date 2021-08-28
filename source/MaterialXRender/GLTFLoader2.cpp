//
// TM & (c) 2021 Lucasfilm Entertainment Company Ltd. and Lucasfilm Ltd.
// All rights reserved.  See LICENSE.txt for license.
//

#include <MaterialXRender/GLTFLoader2.h>
#include <MaterialXCore/Util.h>


#if defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wswitch"
#endif

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

#define CGLTF_IMPLEMENTATION
#include <MaterialXRender/External/cgltf/cgltf.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#if defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

#include <iostream>
#include <algorithm>
#include <stack>

namespace MaterialX
{

namespace {

const float MAX_FLOAT = std::numeric_limits<float>::max();
const size_t FACE_VERTEX_COUNT = 3;

// List of transforms which match to model meshes
using MeshMatrixList = std::unordered_map<size_t, Matrix44>;
// Use to cache object-to-world transforms during traversal. 
using Matrix44Stack = std::stack<Matrix44>;

const float PI = std::acos(-1.0f);

} // anonymous namespace

#if 0
void gltfReadFloat(const float* _attributeData, cgltf_size _accessorNumComponents, cgltf_size _index, cgltf_float* _out, cgltf_size _outElementSize)
{
    const float* input = &_attributeData[_accessorNumComponents * _index];

    for (cgltf_size ii = 0; ii < _outElementSize; ++ii)
    {
        _out[ii] = (ii < _accessorNumComponents) ? input[ii] : 0.0f;
    }
}
#endif

bool GLTFLoader2::load(const FilePath& filePath, MeshList& meshList)
{	
    _debugLevel = 1;
	const std::string input_filename = filePath.asString();

	const std::string ext = filePath.getExtension();
    const std::string BINARY_EXTENSION = "glb";
    const std::string ASCII_EXTENSION = "gltf";

    cgltf_options options = { 0 };
    cgltf_data* data = nullptr;

    // Read file
    cgltf_result result = cgltf_parse_file(&options, input_filename.c_str(), &data);
    if (result != cgltf_result_success)
    {
        return false;
    }
    if (cgltf_load_buffers(&options, data, input_filename.c_str()) != cgltf_result_success) 
    {
        return false;
    }

    for (cgltf_size sceneIndex = 0; sceneIndex < data->scenes_count; ++sceneIndex)
    {
        cgltf_scene* scene = &data->scenes[sceneIndex];

        for (cgltf_size nodeIndex = 0; nodeIndex < scene->nodes_count; ++nodeIndex)
        {
            cgltf_node* cnode = scene->nodes[nodeIndex];
            if (!cnode)
            {
                continue;
            }

            cgltf_mesh* cmesh = cnode->mesh;
            if (!cmesh)
            {
                continue;
            }
			Vector3 boxMin = { MAX_FLOAT, MAX_FLOAT, MAX_FLOAT };
			Vector3 boxMax = { -MAX_FLOAT, -MAX_FLOAT, -MAX_FLOAT };

			std::string meshName = cmesh->name;
			MeshPtr mesh = Mesh::create(meshName);
			if (_debugLevel > 0)
				std::cout << "Translate mesh: " << meshName << std::endl;
			meshList.push_back(mesh);
			mesh->setSourceUri(filePath);

			MeshStreamPtr positionStream = nullptr;
			MeshStreamPtr normalStream = nullptr;
            MeshStreamPtr colorStream = nullptr;
            MeshStreamPtr texcoordStream = nullptr;
			MeshStreamPtr tangentStream = nullptr;

			// Get matrices
			float nodeToWorld[16];
			cgltf_node_transform_world(cnode, nodeToWorld);
			//float nodeToWorldNormal[16];
			//bx::mtxCofactor(nodeToWorldNormal, nodeToWorld);

			for (cgltf_size primitiveIndex = 0; primitiveIndex < cmesh->primitives_count; ++primitiveIndex)
			{
				cgltf_primitive* primitive = &cmesh->primitives[primitiveIndex];

				if (!primitive)
				{
					continue;
				}

				// Read indexing
				MeshPartitionPtr part = MeshPartition::create();

				cgltf_accessor* indexAccessor = primitive->indices;
                if (!indexAccessor)
                {
                    continue;
                }
				size_t indexCount = indexAccessor->count;
				size_t faceCount = indexCount / FACE_VERTEX_COUNT;
				part->setFaceCount(faceCount);
				part->setIdentifier(meshName);

				MeshIndexBuffer& indices = part->getIndices();
                std::cout << "** Read indexing: " << std::endl;
				for (cgltf_size i = 0; i < indexCount; i++)
				{
					uint32_t vertexIndex = static_cast<uint32_t>
						(cgltf_accessor_read_index(indexAccessor, i));
					indices.push_back(vertexIndex);
				}

				//cgltf_size vertexCount = primitive->attributes[0].data->count;
                // Read in vertex streams
				for (cgltf_size prim = 0; prim < primitive->attributes_count; prim++)
				{
					cgltf_attribute* attribute = &primitive->attributes[prim];
					cgltf_accessor* accessor = attribute->data;
					if (!accessor)
					{
						continue;
					}
                    // Only load one stream of each type for now
                    cgltf_int streamIndex = attribute->index;
                    if (streamIndex != 0)
                    {
                        continue;
                    }

					// Get data as floats
					cgltf_size floatCount = cgltf_accessor_unpack_floats(accessor, NULL, 0);
					std::vector<float> attributeData;
					attributeData.resize(floatCount);
                    floatCount = cgltf_accessor_unpack_floats(accessor, &attributeData[0], floatCount);

					cgltf_size vectorSize = cgltf_num_components(accessor->type);
					size_t desiredVectorSize = 3;

					MeshStreamPtr geomStream = nullptr;

					bool isPositionStream = (attribute->type == cgltf_attribute_type_position);
					if (isPositionStream)
					{
						// Create position stream
						positionStream = MeshStream::create("i_" + MeshStream::POSITION_ATTRIBUTE, MeshStream::POSITION_ATTRIBUTE, streamIndex);
						geomStream = positionStream;
					}
					else if (attribute->type == cgltf_attribute_type_normal)
					{
						normalStream = MeshStream::create("i_" + MeshStream::NORMAL_ATTRIBUTE, MeshStream::NORMAL_ATTRIBUTE, streamIndex);
						geomStream = normalStream;
					}
                    else if (attribute->type == cgltf_attribute_type_tangent)
                    {
                        tangentStream = MeshStream::create("i_" + MeshStream::TANGENT_ATTRIBUTE, MeshStream::TANGENT_ATTRIBUTE, streamIndex);
                        geomStream = tangentStream;
                    }
                    else if (attribute->type == cgltf_attribute_type_color)
                    {
                        colorStream = MeshStream::create("i_" + MeshStream::COLOR_ATTRIBUTE, MeshStream::COLOR_ATTRIBUTE, streamIndex);
                        geomStream = colorStream;
                        if (vectorSize == 4)
                        {
                            desiredVectorSize = 4;
                        }
                    }
                    else if (attribute->type == cgltf_attribute_type_texcoord)
					{
						texcoordStream = MeshStream::create("i_" + MeshStream::TEXCOORD_ATTRIBUTE + "_0", MeshStream::TEXCOORD_ATTRIBUTE, 0);

						if (vectorSize == 2)
						{
							texcoordStream->setStride(MeshStream::STRIDE_2D);
							desiredVectorSize = 2;
						}
						geomStream = texcoordStream;
					}
                    else
                    {
                        std::cout << "Unknown stream type: " << std::to_string(attribute->type)
                            << std::endl;
                    }

					// Fill in stream 
					if (geomStream)
					{
						MeshFloatBuffer& buffer = geomStream->getData();
						cgltf_size vertexCount = accessor->count;

                        std::cout << "** Read stream: " << geomStream->getName() << std::endl;
                        std::cout << " - vertex count: " << std::to_string(vertexCount);
                        std::cout << " - vector size: " << std::to_string(vectorSize);

                        for (cgltf_size i = 0; i < vertexCount; i++)
						{
							const float* input = &attributeData[vectorSize * i];
							for (cgltf_size v = 0; v < desiredVectorSize; v++)
							{
								// Update bounding box
								float floatValue = (v < vectorSize) ? input[v] : 0.0f;
								if (isPositionStream)
								{
									boxMin[v] = std::min(floatValue, boxMin[v]);
									boxMax[v] = std::max(floatValue, boxMax[v]);
								}
								buffer.push_back(floatValue);
							}
						}
					}
				}

				// Assign streams to mesh.
				if (positionStream)
				{
					mesh->addStream(positionStream);
				}
				if (normalStream)
				{
					mesh->addStream(normalStream);
				}
				if (texcoordStream)
				{
					mesh->addStream(texcoordStream);
				}
				if (tangentStream)
				{
					mesh->addStream(tangentStream);
				}

				// Assign properties to mesh.
				if (positionStream)
				{
					mesh->setVertexCount(positionStream->getData().size() / MeshStream::STRIDE_3D);
				}
				mesh->setMinimumBounds(boxMin);
				mesh->setMaximumBounds(boxMax);
				Vector3 sphereCenter = (boxMax + boxMin) * 0.5;
				mesh->setSphereCenter(sphereCenter);
				mesh->setSphereRadius((sphereCenter - boxMin).getMagnitude());
			}
		}
    }

    if (data)
    {
        cgltf_free(data);
    }

#if 0
    Vector3 boxMin = { MAX_FLOAT, MAX_FLOAT, MAX_FLOAT };
    Vector3 boxMax = { -MAX_FLOAT, -MAX_FLOAT, -MAX_FLOAT };

    // Load model 
    // For each gltf mesh a new mesh is created
    // - A MeshStream == buffer view for an attribute + associated data.
    // - A MeshPartition == buffer view for indexing + associated data.
    for (size_t m=0; m< model.meshes.size(); m++)
    {
        tinygltf::Mesh& gMesh = model.meshes[m];

        // Create new mesh. Generate a name if the mesh does not have one.
        std::string meshName = gMesh.name;
        if (meshName.empty())
        {
            meshName = "generatedName_" + std::to_string(m);
        }
        MeshPtr mesh = Mesh::create(meshName);
        if (_debugLevel > 0)
            std::cout << "Translate mesh: " << meshName << std::endl;
        meshList.push_back(mesh);
        mesh->setSourceUri(filePath);

        MeshStreamPtr positionStream = nullptr; 
        MeshStreamPtr normalStream = nullptr;
        MeshStreamPtr texcoordStream = nullptr;
        MeshStreamPtr tangentStream = nullptr;

        // Scan primitives on the mesh
        for (tinygltf::Primitive& gPrim : gMesh.primitives)
        {
            // Get index accessor for the prim and create a partition
            // Only support triangle indexing for now
            int accessorIndex = gPrim.indices;
            if ((accessorIndex >= 0) &&
                (gPrim.mode == TINYGLTF_MODE_TRIANGLES))
            {
                const tinygltf::Accessor& gaccessor = model.accessors[accessorIndex];
                const tinygltf::BufferView& gBufferView = model.bufferViews[gaccessor.bufferView];
                const tinygltf::Buffer& gBuffer = model.buffers[gBufferView.buffer];

                size_t indexCount = gaccessor.count;
                MeshPartitionPtr part = MeshPartition::create();
                size_t faceCount = indexCount / FACE_VERTEX_COUNT;
                part->setFaceCount(faceCount);
                part->setIdentifier(meshName); 

                MeshIndexBuffer& indices = part->getIndices();
                size_t startLocation = gBufferView.byteOffset + gaccessor.byteOffset;
                size_t byteStride = gaccessor.ByteStride(gBufferView);

                bool isTriangleList = false;
                std::string indexingType = "invalid type";
                switch (gPrim.mode) {
                case TINYGLTF_MODE_POINTS:
                    indexingType = "point list";
                    break;
                case TINYGLTF_MODE_LINE:
                    indexingType = "line list";
                    break;
                case TINYGLTF_MODE_LINE_STRIP:
                    indexingType = "line string";
                    break;
                case TINYGLTF_MODE_TRIANGLES:
                    indexingType = "triangle list";
                    isTriangleList = true;
                    break;
                case TINYGLTF_MODE_TRIANGLE_STRIP:
                    indexingType = "triangle strip";
                    break;       
                default:
                    break;
                }
                if (!isTriangleList)
                {
                    if (_debugLevel > 0)
                    {
                        std::cout << "Skip unsupported prim type: " << indexingType << " on mesh" <<
                            meshName << std::endl;
                    }
                    continue;
                }

                if (_debugLevel > 0)
                {
                    std::cout << "*** Read mesh: " << meshName << std::endl;
                    std::cout << "Index start byte offset: " << std::to_string(startLocation) << std::endl;
                    std::cout << "-- Index byte stride: " << std::to_string(byteStride) << std::endl;
                    if (_debugLevel > 1)
                        std::cout << "{\n";
                }
                for (size_t i = 0; i < indexCount; i++)
                {
                    size_t offset = startLocation + (i * byteStride);
                    uint32_t bufferIndex = VALUE_AS_UINT32(gaccessor.componentType, &(gBuffer.data[offset]));
                    indices.push_back(bufferIndex);
                    if (_debugLevel > 1)
                        std::cout << "[" + std::to_string(i) + "] = " + std::to_string(bufferIndex) + "\n";
                }
                if (_debugLevel > 1)
                    std::cout << "}\n";
                mesh->addPartition(part);
            }

            // Check for any matrix transform for positions
            Matrix44 positionMatrix = Matrix44::IDENTITY;
            if (meshMatrices.find(m) != meshMatrices.end())
            {
                positionMatrix = meshMatrices[m];
            }

            // Get attributes. Note that bufferViews contain the content descriptioon
            for (auto& gattrib : gPrim.attributes)
            {
                // Find out the byteStride
                const tinygltf::Accessor& gAccessor = model.accessors[gattrib.second];
                const tinygltf::BufferView& gBufferView = model.bufferViews[gAccessor.bufferView];
                const tinygltf::Buffer& gBuffer = model.buffers[gBufferView.buffer];
                size_t byteStride = gAccessor.ByteStride(gBufferView);
                size_t floatStride = byteStride / sizeof(float);

                // Make sure to offset by both view and accessor
                size_t byteOffset = gBufferView.byteOffset + gAccessor.byteOffset;
                size_t startLocation = byteOffset / sizeof(float);

                unsigned int vectorSize = 3;
                if (gAccessor.type == TINYGLTF_TYPE_VEC2)
                {

                    vectorSize = 2;
                }
                else if (gAccessor.type == TINYGLTF_TYPE_VEC4)
                {
                    vectorSize = 4;
                }

                if (_debugLevel > 0)
                {
                    std::cout << "** READ ATTRIB: " << gattrib.first <<
                        " from buffer: " << std::to_string(gBufferView.buffer) << std::endl;
                    std::cout << "-- Buffer start byte offset: " << std::to_string(byteOffset) << std::endl;
                    std::cout << "-- Buffer start float offset: " << std::to_string(startLocation) << std::endl;
                    std::cout << "-- Byte stride: " << std::to_string(byteStride) << std::endl;
                    std::cout << "-- Float stride: " << std::to_string(floatStride) << std::endl;
                    std::cout << "-- Vector size: " << std::to_string(vectorSize) << std::endl;
                }

                bool isPositionStream = gattrib.first.compare("POSITION") == 0;
                MeshStreamPtr geomStream = nullptr;
                if (isPositionStream)
                {
                    if (!positionStream)
                    {
                        positionStream = MeshStream::create("i_" + MeshStream::POSITION_ATTRIBUTE, MeshStream::POSITION_ATTRIBUTE, 0);
                    }
                    geomStream = positionStream;
                }
                else if (gattrib.first.compare("NORMAL") == 0)
                {
                    normalStream = MeshStream::create("i_" + MeshStream::NORMAL_ATTRIBUTE, MeshStream::NORMAL_ATTRIBUTE, 0);
                    geomStream = normalStream;
                }
                else if (gattrib.first.compare("TEXCOORD_0") == 0)
                {
                    if (!texcoordStream)
                    {
                        texcoordStream = MeshStream::create("i_" + MeshStream::TEXCOORD_ATTRIBUTE + "_0", MeshStream::TEXCOORD_ATTRIBUTE, 0);
                    }

                    if (vectorSize == 2)
                    {
                        texcoordStream->setStride(MeshStream::STRIDE_2D);
                    }
                    geomStream = texcoordStream;
                }
                else if (gattrib.first.compare("TANGENT") == 0)
                {
                    tangentStream = MeshStream::create("i_" + MeshStream::TANGENT_ATTRIBUTE, MeshStream::NORMAL_ATTRIBUTE, 0);
                    geomStream = tangentStream;

                    // 4-channel tangents are not supported. Drop the 4th coordinate for now
                    if (vectorSize > 3)
                    {
                        vectorSize = 3;
                    }
                }
                if (geomStream)
                {
                    // Fill in stream 
                    MeshattributeData& buffer = geomStream->getData();

                    size_t dataCount = gAccessor.count;
                    const unsigned char* charPointer = &(gBuffer.data[byteOffset]);
                    float* floatPointer = const_cast<float *>(reinterpret_cast<const float*>(charPointer));
                    if (_debugLevel > 1)
                        std::cout << "{\n";
                    for (size_t i = 0; i < dataCount; i++)
                    {
                        // Copy the vector over
                        if (_debugLevel > 1)
                            std::cout << "[" + std::to_string(i) + "] = { ";

                        if (!isPositionStream)
                        {
                            for (size_t v = 0; v < vectorSize; v++)
                            {
                                float bufferData = *(floatPointer + v);
                                if (geomStream == texcoordStream && v==1)
                                {
                                    bufferData = 1.0f - bufferData;
                                }
                                buffer.push_back(bufferData);
                                if (_debugLevel > 1)
                                    std::cout << std::to_string(buffer[i]) + " ";
                            }
                        }

                        // Transform positions by an appropriate matrix
                        if (isPositionStream)
                        {
                            Vector3 position(*(floatPointer + 0), *(floatPointer + 1), *(floatPointer + 2));
                            position = positionMatrix.transformPoint(position);

                            // Update bounds.
                            for (size_t v = 0; v < 3; v++)
                            {
                                buffer.push_back(position[v]);
                                boxMin[v] = std::min(position[v], boxMin[v]);
                                boxMax[v] = std::max(position[v], boxMax[v]);
                            }
                        }
                        if (_debugLevel > 1)
                            std::cout << " }" << std::endl;

                        // Jump to next vector
                        floatPointer += floatStride;
                    }
                    if (_debugLevel > 1)
                        std::cout << "}\n";
                }
            }
        }

        // General noramsl if none provided
        if (!normalStream && positionStream)
        {
            normalStream = mesh->generateNormals(positionStream);
        }

        // Generate tangents if none provided
        if (!tangentStream && texcoordStream && positionStream && normalStream)
        {
            tangentStream = mesh->generateTangents(positionStream, normalStream, texcoordStream);
        }

        // Assign streams to mesh.
        if (positionStream)
        {
            mesh->addStream(positionStream);
        }
        if (normalStream)
        {
            mesh->addStream(normalStream);
        }
        if (texcoordStream)
        {
            mesh->addStream(texcoordStream);
        }
        if (tangentStream)
        {
            mesh->addStream(tangentStream);
        }

        // Assign properties to mesh.
        if (positionStream)
        {
            mesh->setVertexCount(positionStream->getData().size() / MeshStream::STRIDE_3D);
        }
        mesh->setMinimumBounds(boxMin);
        mesh->setMaximumBounds(boxMax);
        Vector3 sphereCenter = (boxMax + boxMin) * 0.5;
        mesh->setSphereCenter(sphereCenter);
        mesh->setSphereRadius((sphereCenter - boxMin).getMagnitude());
    }
#endif
	return true;
}

} // namespace MaterialX
