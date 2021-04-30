#include "modules/shader/wrap_Shader.h"
#include "modules/script/Proxy.h"

#include <shadertrans/ShaderStage.h>
#include <shadertrans/ShaderTrans.h>
#include <shadertrans/SpirvTools.h>
#include <shadertrans/ShaderReflection.h>
#include <shadertrans/spirv_Linker.h>
#include <shadertrans/ShaderLink.h>
#include <vessel/src/value.h>
#include <guard/check.h>

#include <vector>

#include <string.h>

namespace
{

double pointer2double(void* pointer)
{
    return (double)(uintptr_t)(pointer);
}

void* double2pointer(double d)
{
    return (void*)(uintptr_t)(d);
}

shadertrans::ShaderStage to_shader_stage(const char* str)
{
    shadertrans::ShaderStage stage;
    if (strcmp(str, "vertex") == 0) {
        stage = shadertrans::ShaderStage::VertexShader;
    } else if (strcmp(str, "tess_ctrl") == 0) {
        stage = shadertrans::ShaderStage::TessCtrlShader;
    } else if (strcmp(str, "tess_eval") == 0) {
        stage = shadertrans::ShaderStage::TessEvalShader;
    } else if (strcmp(str, "geometry") == 0) {
        stage = shadertrans::ShaderStage::GeometryShader;
    } else if (strcmp(str, "pixel") == 0) {
        stage = shadertrans::ShaderStage::PixelShader;
    } else if (strcmp(str, "compute") == 0) {
        stage = shadertrans::ShaderStage::ComputeShader;
    }
    return stage;
}

void w_ShaderGen_allocate()
{
    auto linker = std::make_shared<shadertrans::ShaderLink>();
    auto proxy = (tt::Proxy<shadertrans::ShaderLink>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<shadertrans::ShaderLink>));
    proxy->obj = linker;
}

int w_ShaderGen_finalize(void* data)
{
    auto proxy = (tt::Proxy<shadertrans::ShaderLink>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<shadertrans::ShaderLink>);
}

void w_ShaderGen_add_input()
{
    const char* name = ves_tostring(1);
    const char* type = ves_tostring(2);

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto input = linker->AddInput(name, type);
    ves_set_number(0, pointer2double(input));
}

void w_ShaderGen_add_output()
{
    const char* name = ves_tostring(1);
    const char* type = ves_tostring(2);

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto output = linker->AddOutput(name, type);
    ves_set_number(0, pointer2double(output));
}

void w_ShaderGen_add_uniform()
{
    spvgentwo::Module* module = (spvgentwo::Module*)double2pointer(ves_tonumber(1));
    const char* name = ves_tostring(2);
    const char* type = ves_tostring(3);

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto unif = linker->AddUniform(module, name, type);
    ves_set_number(0, pointer2double(unif));
}

void w_ShaderGen_get_unif_num()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    ves_set_number(0, linker->GetUniformNum());
}

void w_ShaderGen_access_chain()
{
    spvgentwo::Function* func = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));
    spvgentwo::Instruction* base = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));
    int index = (int)ves_tonumber(3);

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->AccessChain(func, base, index);

    ves_set_number(0, pointer2double(ret));
}

void w_shadergen_compose_float2()
{
    spvgentwo::Function* func = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));
    spvgentwo::Instruction* x = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));
    spvgentwo::Instruction* y = (spvgentwo::Instruction*)double2pointer(ves_tonumber(3));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->ComposeFloat2(func, x, y);

    ves_set_number(0, pointer2double(ret));
}

void w_shadergen_compose_float3()
{
    spvgentwo::Function* func = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));
    spvgentwo::Instruction* x = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));
    spvgentwo::Instruction* y = (spvgentwo::Instruction*)double2pointer(ves_tonumber(3));
    spvgentwo::Instruction* z = (spvgentwo::Instruction*)double2pointer(ves_tonumber(4));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->ComposeFloat3(func, x, y, z);

    ves_set_number(0, pointer2double(ret));
}

void w_shadergen_compose_float4()
{
    spvgentwo::Function* func = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));
    spvgentwo::Instruction* x = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));
    spvgentwo::Instruction* y = (spvgentwo::Instruction*)double2pointer(ves_tonumber(3));
    spvgentwo::Instruction* z = (spvgentwo::Instruction*)double2pointer(ves_tonumber(4));
    spvgentwo::Instruction* w = (spvgentwo::Instruction*)double2pointer(ves_tonumber(5));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->ComposeFloat4(func, x, y, z, w);

    ves_set_number(0, pointer2double(ret));
}

void w_shadergen_compose_extract()
{
    spvgentwo::Function* func = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));
    spvgentwo::Instruction* comp = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));
    int index = (int)ves_tonumber(3);

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->ComposeExtract(func, comp, index);

    ves_set_number(0, pointer2double(ret));
}

void w_shadergen_dot()
{
    spvgentwo::Function* func = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));
    spvgentwo::Instruction* a = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));
    spvgentwo::Instruction* b = (spvgentwo::Instruction*)double2pointer(ves_tonumber(3));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->Dot(func, a, b);

    ves_set_number(0, pointer2double(ret));
}

void w_shadergen_add()
{
    spvgentwo::Function* func = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));
    spvgentwo::Instruction* a = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));
    spvgentwo::Instruction* b = (spvgentwo::Instruction*)double2pointer(ves_tonumber(3));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->Add(func, a, b);

    ves_set_number(0, pointer2double(ret));
}

void w_shadergen_sub()
{
    spvgentwo::Function* func = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));
    spvgentwo::Instruction* a = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));
    spvgentwo::Instruction* b = (spvgentwo::Instruction*)double2pointer(ves_tonumber(3));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->Sub(func, a, b);

    ves_set_number(0, pointer2double(ret));
}

void w_shadergen_mul()
{
    spvgentwo::Function* func = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));
    spvgentwo::Instruction* a = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));
    spvgentwo::Instruction* b = (spvgentwo::Instruction*)double2pointer(ves_tonumber(3));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->Mul(func, a, b);

    ves_set_number(0, pointer2double(ret));
}

void w_shadergen_div()
{
    spvgentwo::Function* func = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));
    spvgentwo::Instruction* a = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));
    spvgentwo::Instruction* b = (spvgentwo::Instruction*)double2pointer(ves_tonumber(3));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->Div(func, a, b);

    ves_set_number(0, pointer2double(ret));
}

void w_shadergen_negate()
{
    spvgentwo::Function* func = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));
    spvgentwo::Instruction* v = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->Negate(func, v);

    ves_set_number(0, pointer2double(ret));
}

void w_ShaderGen_store()
{
    spvgentwo::Function* func = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));
    spvgentwo::Instruction* dst = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));
    spvgentwo::Instruction* src = (spvgentwo::Instruction*)double2pointer(ves_tonumber(3));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    linker->Store(func, dst, src);
}

void w_ShaderGen_load()
{
    spvgentwo::Function* func = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));
    spvgentwo::Instruction* val = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->Load(func, val);
    ves_set_number(0, pointer2double(ret));
}

void w_ShaderGen_image_sample()
{
    spvgentwo::Function* func = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));
    spvgentwo::Instruction* img = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));
    spvgentwo::Instruction* uv = (spvgentwo::Instruction*)double2pointer(ves_tonumber(3));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->ImageSample(func, img, uv);
    ves_set_number(0, pointer2double(ret));
}

void w_ShaderGen_add_module()
{
    const char* stage_str = ves_tostring(1);
    const char* code_str = ves_tostring(2);
    const char* name = ves_tostring(3);

    auto stage = to_shader_stage(stage_str);

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto lib = linker->AddModule(stage, code_str, name);
    ves_set_number(0, pointer2double(lib.get()));
}

void w_ShaderGen_query_func()
{
    spvgentwo::Module* module = (spvgentwo::Module*)double2pointer(ves_tonumber(1));
    const char* name = ves_tostring(2);

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto func = linker->QueryFunction(*module, name);
    ves_set_number(0, pointer2double(func));
}

void w_ShaderGen_func_replace()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    spvgentwo::Function* from = (spvgentwo::Function*)double2pointer(ves_tonumber(1));
    spvgentwo::Function* to = (spvgentwo::Function*)double2pointer(ves_tonumber(2));

    linker->ReplaceFunc(from, to);
}

void w_ShaderGen_get_main_module()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    spvgentwo::Module* main = linker->GetMainModule();
    ves_set_number(0, pointer2double(main));
}

void w_ShaderGen_get_main_func()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    spvgentwo::Function* main = linker->GetMainFunc();
    ves_set_number(0, pointer2double(main));
}

void w_ShaderGen_create_decl_func()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    spvgentwo::Module* module = (spvgentwo::Module*)double2pointer(ves_tonumber(1));
    spvgentwo::Function* func = (spvgentwo::Function*)double2pointer(ves_tonumber(2));

    auto ret = linker->CreateDeclFunc(module, func);
    ves_set_number(0, pointer2double(ret));
}

void w_ShaderGen_add_link_decl()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    spvgentwo::Function* func = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));
    const char* name = ves_tostring(2);
    bool is_export = ves_toboolean(3);

    linker->AddLinkDecl(func, name, is_export);
}

void w_ShaderGen_create_func()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;

    spvgentwo::Module* module = (spvgentwo::Module*)double2pointer(ves_tonumber(1));
    const char* name = ves_tostring(2);
    const char* ret = ves_tostring(3);

    const int len = ves_len(4);
    std::vector<std::string> args(len);
    for (int i = 0; i < len; ++i)
    {
        ves_geti(4, i);
        args[i] = ves_tostring(-1);
        ves_pop(1);
    }

    auto func = linker->CreateFunc(module, name, ret, args);
    ves_set_number(0, pointer2double(func));
}

void w_ShaderGen_get_func_param()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    spvgentwo::Function* func = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));
    int index = (int)ves_tonumber(2);

    auto ret = linker->GetFuncParam(func, index);
    ves_set_number(0, pointer2double(ret));
}

void w_ShaderGen_get_func_args()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    spvgentwo::Function* func = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));

    std::vector<std::string> names;
    linker->GetFuncParamNames(func, names);

    ves_pop(2);

    ves_newlist((int)names.size());
    for (int i = 0; i < names.size(); ++i)
    {
        ves_pushstring(names[i].c_str());
        ves_seti(-2, i);
        ves_pop(1);
    }
}

void w_ShaderGen_func_call()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    spvgentwo::Function* caller = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));
    spvgentwo::Function* callee = (spvgentwo::Function*)(double2pointer(ves_tonumber(2)));

    const int len = ves_len(3);
    std::vector<spvgentwo::Instruction*> args(len);
    for (int i = 0; i < len; ++i)
    {
        ves_geti(3, i);
        args[i] = (spvgentwo::Instruction*)(double2pointer(ves_tonumber(-1)));
        ves_pop(1);
    }

    auto ret = linker->FuncCall(caller, callee, args);
    ves_set_number(0, pointer2double(ret));
}

void w_ShaderGen_func_return()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    spvgentwo::Function* func = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));

    linker->Return(func);
}

void w_ShaderGen_func_return_value()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    spvgentwo::Function* func = (spvgentwo::Function*)(double2pointer(ves_tonumber(1)));
    spvgentwo::Instruction* inst = (spvgentwo::Instruction*)(double2pointer(ves_tonumber(2)));

    linker->ReturnValue(func, inst);
}

void w_ShaderGen_const_float()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    spvgentwo::Module* module = (spvgentwo::Module*)double2pointer(ves_tonumber(1));
    float x = (float)ves_tonumber(2);
    
    auto ret = linker->ConstFloat(module, x);
    ves_set_number(0, pointer2double(ret));
}

void w_ShaderGen_const_float2()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    spvgentwo::Module* module = (spvgentwo::Module*)double2pointer(ves_tonumber(1));
    float x = (float)ves_tonumber(2);
    float y = (float)ves_tonumber(3);

    auto ret = linker->ConstFloat2(module, x, y);
    ves_set_number(0, pointer2double(ret));
}

void w_ShaderGen_const_float3()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    spvgentwo::Module* module = (spvgentwo::Module*)double2pointer(ves_tonumber(1));
    float x = (float)ves_tonumber(2);
    float y = (float)ves_tonumber(3);
    float z = (float)ves_tonumber(4);

    auto ret = linker->ConstFloat3(module, x, y, z);
    ves_set_number(0, pointer2double(ret));
}

void w_ShaderGen_const_float4()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    spvgentwo::Module* module = (spvgentwo::Module*)double2pointer(ves_tonumber(1));
    float x = (float)ves_tonumber(2);
    float y = (float)ves_tonumber(3);
    float z = (float)ves_tonumber(4);
    float w = (float)ves_tonumber(5);

    auto ret = linker->ConstFloat4(module, x, y, z, w);
    ves_set_number(0, pointer2double(ret));
}

void w_ShaderGen_const_mat3()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    spvgentwo::Module* module = (spvgentwo::Module*)double2pointer(ves_tonumber(1));

    float m[9];
    const int len = ves_len(2);
    GD_ASSERT(len == 9, "error m size");
    std::vector<unsigned int> spirv(len);
    for (int i = 0; i < len; ++i)
    {
        ves_geti(2, i);
        m[i] = (float)ves_tonumber(-1);
        ves_pop(1);
    }

    auto ret = linker->ConstMatrix3(module, m);
    ves_set_number(0, pointer2double(ret));
}

void w_ShaderGen_import_all()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    linker->ImportAll();
}

void w_ShaderGen_finish_main()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    linker->FinishMain();
}

void w_Shader_code2spirv()
{
    const char* stage_str = ves_tostring(1);
    const char* code_str = ves_tostring(2);
    const char* lang_str = ves_tostring(3);
    
    std::vector<unsigned int> spirv;
    auto stage = to_shader_stage(stage_str);
    if (strcmp(lang_str, "glsl") == 0) {
        shadertrans::ShaderTrans::GLSL2SpirV(stage, code_str, spirv, true);
    } else if (strcmp(lang_str, "hlsl") == 0) {
        shadertrans::ShaderTrans::HLSL2SpirV(stage, code_str, spirv);
    }

    ves_pop(4);

    ves_newlist((int)spirv.size());
    for (int i = 0; i < spirv.size(); ++i)
    {
        ves_pushnumber(spirv[i]);
        ves_seti(-2, i);
        ves_pop(1);
    }
}

void w_Shader_disassemble()
{
    const int len = ves_len(1);
    std::vector<unsigned int> spirv(len);
    for (int i = 0; i < len; ++i)
    {
        ves_geti(1, i);
        spirv[i] = (unsigned int)ves_tonumber(-1);
        ves_pop(1);
    }

    std::string text;
    shadertrans::SpirvTools::Disassemble(spirv.data(), spirv.size(), &text);
    ves_set_lstring(0, text.c_str(), text.size());
}

void w_Shader_print()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    spvgentwo::Module* module = (spvgentwo::Module*)double2pointer(ves_tonumber(1));
    linker->Print(*module);
}

}

namespace tt
{

VesselForeignMethodFn ShaderBindMethod(const char* signature)
{
    if (strcmp(signature, "ShaderGen.add_input(_,_)") == 0) return w_ShaderGen_add_input;
    if (strcmp(signature, "ShaderGen.add_output(_,_)") == 0) return w_ShaderGen_add_output;
    if (strcmp(signature, "ShaderGen.add_uniform(_,_,_)") == 0) return w_ShaderGen_add_uniform;
    if (strcmp(signature, "ShaderGen.get_unif_num()") == 0) return w_ShaderGen_get_unif_num;

    if (strcmp(signature, "ShaderGen.access_chain(_,_,_)") == 0) return w_ShaderGen_access_chain;
    if (strcmp(signature, "ShaderGen.compose_float2(_,_,_)") == 0) return w_shadergen_compose_float2;
    if (strcmp(signature, "ShaderGen.compose_float3(_,_,_,_)") == 0) return w_shadergen_compose_float3;
    if (strcmp(signature, "ShaderGen.compose_float4(_,_,_,_,_)") == 0) return w_shadergen_compose_float4;
    if (strcmp(signature, "ShaderGen.compose_extract(_,_,_)") == 0) return w_shadergen_compose_extract;
    if (strcmp(signature, "ShaderGen.dot(_,_,_)") == 0) return w_shadergen_dot;
    if (strcmp(signature, "ShaderGen.add(_,_,_)") == 0) return w_shadergen_add;
    if (strcmp(signature, "ShaderGen.sub(_,_,_)") == 0) return w_shadergen_sub;
    if (strcmp(signature, "ShaderGen.mul(_,_,_)") == 0) return w_shadergen_mul;
    if (strcmp(signature, "ShaderGen.div(_,_,_)") == 0) return w_shadergen_div;
    if (strcmp(signature, "ShaderGen.negate(_,_)") == 0) return w_shadergen_negate;
    if (strcmp(signature, "ShaderGen.store(_,_,_)") == 0) return w_ShaderGen_store;
    if (strcmp(signature, "ShaderGen.load(_,_)") == 0) return w_ShaderGen_load;
    if (strcmp(signature, "ShaderGen.image_sample(_,_,_)") == 0) return w_ShaderGen_image_sample;

    if (strcmp(signature, "ShaderGen.add_module(_,_,_)") == 0) return w_ShaderGen_add_module;
    if (strcmp(signature, "ShaderGen.query_func(_,_)") == 0) return w_ShaderGen_query_func;

    if (strcmp(signature, "ShaderGen.func_replace(_,_)") == 0) return w_ShaderGen_func_replace;

    if (strcmp(signature, "ShaderGen.get_main_module()") == 0) return w_ShaderGen_get_main_module;
    if (strcmp(signature, "ShaderGen.get_main_func()") == 0) return w_ShaderGen_get_main_func;
    if (strcmp(signature, "ShaderGen.create_decl_func(_,_)") == 0) return w_ShaderGen_create_decl_func;
    if (strcmp(signature, "ShaderGen.add_link_decl(_,_,_)") == 0) return w_ShaderGen_add_link_decl;

    if (strcmp(signature, "ShaderGen.create_func(_,_,_,_)") == 0) return w_ShaderGen_create_func;
    if (strcmp(signature, "ShaderGen.get_func_param(_,_)") == 0) return w_ShaderGen_get_func_param;
    if (strcmp(signature, "ShaderGen.get_func_args(_)") == 0) return w_ShaderGen_get_func_args;
    if (strcmp(signature, "ShaderGen.func_call(_,_,_)") == 0) return w_ShaderGen_func_call;
    if (strcmp(signature, "ShaderGen.func_return(_)") == 0) return w_ShaderGen_func_return;
    if (strcmp(signature, "ShaderGen.func_return_value(_,_)") == 0) return w_ShaderGen_func_return_value;

    if (strcmp(signature, "ShaderGen.const_float(_,_)") == 0) return w_ShaderGen_const_float;
    if (strcmp(signature, "ShaderGen.const_float2(_,_,_)") == 0) return w_ShaderGen_const_float2;
    if (strcmp(signature, "ShaderGen.const_float3(_,_,_,_)") == 0) return w_ShaderGen_const_float3;
    if (strcmp(signature, "ShaderGen.const_float4(_,_,_,_,_)") == 0) return w_ShaderGen_const_float4;

    if (strcmp(signature, "ShaderGen.const_mat3(_,_)") == 0) return w_ShaderGen_const_mat3;

    if (strcmp(signature, "ShaderGen.import_all()") == 0) return w_ShaderGen_import_all;
    if (strcmp(signature, "ShaderGen.finish_main()") == 0) return w_ShaderGen_finish_main;

    if (strcmp(signature, "ShaderGen.print(_)") == 0) return w_Shader_print;

    if (strcmp(signature, "static ShaderTools.code2spirv(_,_,_)") == 0) return w_Shader_code2spirv;
    if (strcmp(signature, "static ShaderTools.disassemble(_)") == 0) return w_Shader_disassemble;

    return NULL;
}

void ShaderBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
    if (strcmp(class_name, "ShaderGen") == 0)
    {
        methods->allocate = w_ShaderGen_allocate;
        methods->finalize = w_ShaderGen_finalize;
        return;
    }
}

}