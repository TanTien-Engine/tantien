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

void w_ShaderGen_add_uniform()
{
    const char* name = ves_tostring(1);
    const char* type = ves_tostring(2);

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto unif = linker->AddUniform(name, type);
    ves_set_number(0, pointer2double(unif));
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

void w_ShaderGen_op_const_float()
{
    float v = (float)ves_tonumber(1);

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto f = linker->ConstFloat(v);
    ves_set_number(0, pointer2double(f));
}

void w_ShaderGen_op_const_float2()
{
    float x = (float)ves_tonumber(1);
    float y = (float)ves_tonumber(2);

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto f2 = linker->ConstFloat2(x, y);
    ves_set_number(0, pointer2double(f2));
}

void w_ShaderGen_op_const_float3()
{
    float x = (float)ves_tonumber(1);
    float y = (float)ves_tonumber(2);
    float z = (float)ves_tonumber(3);

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto f3 = linker->ConstFloat3(x, y, z);
    ves_set_number(0, pointer2double(f3));
}

void w_ShaderGen_op_const_float4()
{
    float x = (float)ves_tonumber(1);
    float y = (float)ves_tonumber(2);
    float z = (float)ves_tonumber(3);
    float w = (float)ves_tonumber(4);

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto f4 = linker->ConstFloat4(x, y, z, w);
    ves_set_number(0, pointer2double(f4));
}

void w_ShaderGen_op_call()
{
    spvgentwo::Function* func = (spvgentwo::Function*)double2pointer(ves_tonumber(1));

    std::vector<spvgentwo::Instruction*> args;
    GD_ASSERT(ves_type(2) == VES_TYPE_LIST, "error type");
    const int num = ves_len(2);
    args.resize(num);
    for (int i = 0; i < num; ++i) {
        ves_geti(2, i);
        args[i] = (spvgentwo::Instruction*)double2pointer(ves_tonumber(-1));
    }

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->Call(func, args);

    ves_set_number(0, pointer2double(ret));
}

void w_ShaderGen_op_access_chain()
{
    spvgentwo::Instruction* base = (spvgentwo::Instruction*)double2pointer(ves_tonumber(1));
    int index = (int)ves_tonumber(2);

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->AccessChain(base, index);

    ves_set_number(0, pointer2double(ret));
}

void w_shadergen_op_compose_float2()
{
    spvgentwo::Instruction* x = (spvgentwo::Instruction*)double2pointer(ves_tonumber(1));
    spvgentwo::Instruction* y = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->ComposeFloat2(x, y);

    ves_set_number(0, pointer2double(ret));
}

void w_shadergen_op_compose_float3()
{
    spvgentwo::Instruction* x = (spvgentwo::Instruction*)double2pointer(ves_tonumber(1));
    spvgentwo::Instruction* y = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));
    spvgentwo::Instruction* z = (spvgentwo::Instruction*)double2pointer(ves_tonumber(3));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->ComposeFloat3(x, y, z);

    ves_set_number(0, pointer2double(ret));
}

void w_shadergen_op_compose_float4()
{
    spvgentwo::Instruction* x = (spvgentwo::Instruction*)double2pointer(ves_tonumber(1));
    spvgentwo::Instruction* y = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));
    spvgentwo::Instruction* z = (spvgentwo::Instruction*)double2pointer(ves_tonumber(3));
    spvgentwo::Instruction* w = (spvgentwo::Instruction*)double2pointer(ves_tonumber(4));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->ComposeFloat4(x, y, z, w);

    ves_set_number(0, pointer2double(ret));
}

void w_shadergen_op_compose_extract()
{
    spvgentwo::Instruction* comp = (spvgentwo::Instruction*)double2pointer(ves_tonumber(1));
    int index = (int)ves_tonumber(2);

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->ComposeExtract(comp, index);

    ves_set_number(0, pointer2double(ret));
}

void w_shadergen_op_dot()
{
    spvgentwo::Instruction* a = (spvgentwo::Instruction*)double2pointer(ves_tonumber(1));
    spvgentwo::Instruction* b = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->Dot(a, b);

    ves_set_number(0, pointer2double(ret));
}

void w_shadergen_op_add()
{
    spvgentwo::Instruction* a = (spvgentwo::Instruction*)double2pointer(ves_tonumber(1));
    spvgentwo::Instruction* b = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->Add(a, b);

    ves_set_number(0, pointer2double(ret));
}

void w_shadergen_op_sub()
{
    spvgentwo::Instruction* a = (spvgentwo::Instruction*)double2pointer(ves_tonumber(1));
    spvgentwo::Instruction* b = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->Sub(a, b);

    ves_set_number(0, pointer2double(ret));
}

void w_shadergen_op_mul()
{
    spvgentwo::Instruction* a = (spvgentwo::Instruction*)double2pointer(ves_tonumber(1));
    spvgentwo::Instruction* b = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->Mul(a, b);

    ves_set_number(0, pointer2double(ret));
}

void w_shadergen_op_div()
{
    spvgentwo::Instruction* a = (spvgentwo::Instruction*)double2pointer(ves_tonumber(1));
    spvgentwo::Instruction* b = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto ret = linker->Div(a, b);

    ves_set_number(0, pointer2double(ret));
}

void w_ShaderGen_op_store()
{
    spvgentwo::Instruction* dst = (spvgentwo::Instruction*)double2pointer(ves_tonumber(1));
    spvgentwo::Instruction* src = (spvgentwo::Instruction*)double2pointer(ves_tonumber(2));

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    linker->Store(dst, src);
}

void w_ShaderGen_op_return()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    linker->Return();
}

void w_ShaderGen_add_lib()
{
    const char* stage_str = ves_tostring(1);
    const char* code_str = ves_tostring(2);

    auto stage = to_shader_stage(stage_str);

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto lib = linker->AddLibrary(stage, code_str);
    ves_set_number(0, pointer2double(lib.get()));
}

void w_ShaderGen_get_func()
{
    spvgentwo::Module* module = (spvgentwo::Module*)double2pointer(ves_tonumber(1));
    int index = (int)ves_tonumber(2);

    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto func = linker->GetFunction(*module, index);
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
    spvgentwo::Instruction* inst = (spvgentwo::Instruction*)(double2pointer(ves_tonumber(2)));

    auto ret = linker->FuncReturn(func, inst);
    ves_set_number(0, pointer2double(ret));
}

void w_ShaderGen_module_const_float()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    spvgentwo::Module* module = (spvgentwo::Module*)double2pointer(ves_tonumber(1));
    float x = (float)ves_tonumber(2);
    
    auto ret = linker->ConstFloat(module, x);
    ves_set_number(0, pointer2double(ret));
}

void w_ShaderGen_module_const_float2()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    spvgentwo::Module* module = (spvgentwo::Module*)double2pointer(ves_tonumber(1));
    float x = (float)ves_tonumber(2);
    float y = (float)ves_tonumber(3);

    auto ret = linker->ConstFloat2(module, x, y);
    ves_set_number(0, pointer2double(ret));
}

void w_ShaderGen_module_const_float3()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    spvgentwo::Module* module = (spvgentwo::Module*)double2pointer(ves_tonumber(1));
    float x = (float)ves_tonumber(2);
    float y = (float)ves_tonumber(3);
    float z = (float)ves_tonumber(4);

    auto ret = linker->ConstFloat3(module, x, y, z);
    ves_set_number(0, pointer2double(ret));
}

void w_ShaderGen_module_const_float4()
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

void w_ShaderGen_link()
{
    auto linker = ((tt::Proxy<shadertrans::ShaderLink>*)ves_toforeign(0))->obj;
    auto glsl = linker->Link();
    ves_set_lstring(0, glsl.c_str(), glsl.size());
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
    if (strcmp(signature, "ShaderGen.add_uniform(_,_)") == 0) return w_ShaderGen_add_uniform;
    if (strcmp(signature, "ShaderGen.add_input(_,_)") == 0) return w_ShaderGen_add_input;
    if (strcmp(signature, "ShaderGen.add_output(_,_)") == 0) return w_ShaderGen_add_output;

    if (strcmp(signature, "ShaderGen.op_const_float(_)") == 0) return w_ShaderGen_op_const_float;
    if (strcmp(signature, "ShaderGen.op_const_float2(_,_)") == 0) return w_ShaderGen_op_const_float2;
    if (strcmp(signature, "ShaderGen.op_const_float3(_,_,_)") == 0) return w_ShaderGen_op_const_float3;
    if (strcmp(signature, "ShaderGen.op_const_float4(_,_,_,_)") == 0) return w_ShaderGen_op_const_float4;
    if (strcmp(signature, "ShaderGen.op_call(_,_)") == 0) return w_ShaderGen_op_call;
    if (strcmp(signature, "ShaderGen.op_access_chain(_,_)") == 0) return w_ShaderGen_op_access_chain;
    if (strcmp(signature, "ShaderGen.op_compose_float2(_,_)") == 0) return w_shadergen_op_compose_float2;
    if (strcmp(signature, "ShaderGen.op_compose_float3(_,_,_)") == 0) return w_shadergen_op_compose_float3;
    if (strcmp(signature, "ShaderGen.op_compose_float4(_,_,_,_)") == 0) return w_shadergen_op_compose_float4;
    if (strcmp(signature, "ShaderGen.op_compose_extract(_,_)") == 0) return w_shadergen_op_compose_extract;
    if (strcmp(signature, "ShaderGen.op_dot(_,_)") == 0) return w_shadergen_op_dot;
    if (strcmp(signature, "ShaderGen.op_add(_,_)") == 0) return w_shadergen_op_add;
    if (strcmp(signature, "ShaderGen.op_sub(_,_)") == 0) return w_shadergen_op_sub;
    if (strcmp(signature, "ShaderGen.op_mul(_,_)") == 0) return w_shadergen_op_mul;
    if (strcmp(signature, "ShaderGen.op_div(_,_)") == 0) return w_shadergen_op_div;
    if (strcmp(signature, "ShaderGen.op_store(_,_)") == 0) return w_ShaderGen_op_store;
    if (strcmp(signature, "ShaderGen.op_return()") == 0) return w_ShaderGen_op_return;

    if (strcmp(signature, "ShaderGen.add_lib(_,_)") == 0) return w_ShaderGen_add_lib;
    if (strcmp(signature, "ShaderGen.get_func(_,_)") == 0) return w_ShaderGen_get_func;

    if (strcmp(signature, "ShaderGen.func_replace(_,_)") == 0) return w_ShaderGen_func_replace;

    if (strcmp(signature, "ShaderGen.get_main_module()") == 0) return w_ShaderGen_get_main_module;
    if (strcmp(signature, "ShaderGen.create_decl_func(_,_)") == 0) return w_ShaderGen_create_decl_func;
    if (strcmp(signature, "ShaderGen.add_link_decl(_,_,_)") == 0) return w_ShaderGen_add_link_decl;

    if (strcmp(signature, "ShaderGen.create_func(_,_,_,_)") == 0) return w_ShaderGen_create_func;
    if (strcmp(signature, "ShaderGen.get_func_param(_,_)") == 0) return w_ShaderGen_get_func_param;
    if (strcmp(signature, "ShaderGen.get_func_args(_)") == 0) return w_ShaderGen_get_func_args;
    if (strcmp(signature, "ShaderGen.func_call(_,_,_)") == 0) return w_ShaderGen_func_call;
    if (strcmp(signature, "ShaderGen.func_return(_,_)") == 0) return w_ShaderGen_func_return;

    if (strcmp(signature, "ShaderGen.module_const_float(_,_)") == 0) return w_ShaderGen_module_const_float;
    if (strcmp(signature, "ShaderGen.module_const_float2(_,_,_)") == 0) return w_ShaderGen_module_const_float2;
    if (strcmp(signature, "ShaderGen.module_const_float3(_,_,_,_)") == 0) return w_ShaderGen_module_const_float3;
    if (strcmp(signature, "ShaderGen.module_const_float4(_,_,_,_,_)") == 0) return w_ShaderGen_module_const_float4;

    if (strcmp(signature, "ShaderGen.import_all()") == 0) return w_ShaderGen_import_all;
    if (strcmp(signature, "ShaderGen.finish_main()") == 0) return w_ShaderGen_finish_main;
    if (strcmp(signature, "ShaderGen.link()") == 0) return w_ShaderGen_link;

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