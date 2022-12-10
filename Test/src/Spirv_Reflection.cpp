#include <spirv_glsl.hpp>
#include <fstream>
#include <iostream>

std::vector<uint32_t> LoadShaderFromFile(const std::string& path)
{
    std::ifstream   file(path, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint32_t> data(size/4);

    if (!file.read((char*)data.data(), size))
    {
        printf("Error Loading Shader: %s", path.c_str());
        assert(false);
    }

    return data;
}

int main()
{
    //auto vs = LoadShaderFromFile("D:\\Programming\\ZephyrEngine\\asset\\shader\\spv\\static_mesh_pbr.vert.spv");
    //auto fs = LoadShaderFromFile("D:\\Programming\\ZephyrEngine\\asset\\shader\\spv\\static_mesh_pbr.frag.spv");

    //std::string a((char*)vs.data(), vs.size());
    //std::cout << a << std::endl;

    //spirv_cross::Compiler vertexCompiler((uint32_t*)vs.data(), vs.size() / 4);
    //spirv_cross::Compiler fragmentCompiler((uint32_t*)fs.data(), fs.size() / 4);
    //
    //spirv_cross::ShaderResources res = vertexCompiler.get_shader_resources();
    //spirv_cross::ShaderResources res2 = fragmentCompiler.get_shader_resources();

    
    FILE* f = fopen("D:\\Programming\\ZephyrEngine\\asset\\shader\\spv\\test.vert.spv", "rb");
    fseek(f, 0, SEEK_END);
    const auto fsz = ftell(f);
    fseek(f, 0, SEEK_SET);
    auto buf = new char[fsz];
    fread(buf, 1, fsz, f);
    fclose(f);

    spirv_cross::CompilerGLSL compiler((uint32_t*)buf, fsz / 4);
    auto                      res = compiler.get_shader_resources();
    for (auto& ub : res.storage_buffers)
    {
        const spirv_cross::SPIRType& type       = compiler.get_type(ub.type_id);
        const spirv_cross::SPIRType& basetype       = compiler.get_type(ub.base_type_id);
        auto set = compiler.get_decoration(ub.id, spv::DecorationDescriptorSet);
        auto binding = compiler.get_decoration(ub.id, spv::DecorationBinding);
        auto arrayDepth = type.array.size();
        auto arraySize  = type.array[0];
        auto                         size           = compiler.get_declared_struct_size(basetype);
        123;
    }
    
    for (auto& ub : res.stage_inputs)
    {
        const spirv_cross::SPIRType& type       = compiler.get_type(ub.type_id);
        const spirv_cross::SPIRType& basetype   = compiler.get_type(ub.base_type_id);
        auto                         set        = compiler.get_decoration(ub.id, spv::DecorationDescriptorSet);
        auto                         binding    = compiler.get_decoration(ub.id, spv::DecorationBinding);
        auto                         location    = compiler.get_decoration(ub.id, spv::DecorationLocation);
        auto                         arrayDepth = type.array.size();
        auto                         arraySize  = type.array[0];
    }
}