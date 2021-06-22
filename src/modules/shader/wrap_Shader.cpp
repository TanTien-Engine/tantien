#include "modules/shader/wrap_Shader.h"
#include "modules/script/Proxy.h"

#include <shadertrans/ShaderStage.h>
#include <shadertrans/ShaderTrans.h>
#include <shadertrans/SpirvTools.h>
#include <shadertrans/ShaderReflection.h>
#include <shadertrans/spirv_Linker.h>
#include <shadertrans/ShaderBuilder.h>
#include <shadertrans/SpirvGenTwo.h>
#include <shadertrans/ShaderPreprocess.h>
#include <vessel/src/value.h>
#include <guard/check.h>

#include <vector>

#include <string.h>

namespace
{

void return_module(spvgentwo::Module* module)
{
    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("shader", "ShaderModule");
    spvgentwo::Module** ptr = (spvgentwo::Module**)ves_set_newforeign(0, 1, sizeof(spvgentwo::Module*));
    *ptr = module;
    ves_pop(1);
}

void return_func(spvgentwo::Function* func)
{
    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("shader", "ShaderFunc");
    spvgentwo::Function** ptr = (spvgentwo::Function**)ves_set_newforeign(0, 1, sizeof(spvgentwo::Function*));
    *ptr = func;
    ves_pop(1);
}

void return_block(spvgentwo::BasicBlock* bb)
{
    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("shader", "ShaderBlock");
    spvgentwo::BasicBlock** ptr = (spvgentwo::BasicBlock**)ves_set_newforeign(0, 1, sizeof(spvgentwo::BasicBlock*));
    *ptr = bb;
    ves_pop(1);
}

void return_inst(spvgentwo::Instruction* inst)
{
    if (!inst) {
        ves_set_nil(0);
        return;
    }

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("shader", "ShaderInst");
    spvgentwo::Instruction** ptr = (spvgentwo::Instruction**)ves_set_newforeign(0, 1, sizeof(spvgentwo::Instruction*));
    *ptr = inst;
    ves_pop(1);
}

spvgentwo::Instruction* to_inst(int idx)
{
    auto inst = ves_toforeign(idx);
    if (!inst) {
        return nullptr;
    } else {
        return *(spvgentwo::Instruction**)inst;
    }
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

// ShaderTools

void w_ShaderTools_code2spirv()
{
    const char* stage_str = ves_tostring(1);
    const char* code_str = ves_tostring(2);
    const char* lang_str = ves_tostring(3);
    
    std::vector<unsigned int> spirv;
    auto stage = to_shader_stage(stage_str);
    if (strcmp(lang_str, "glsl") == 0) {
        shadertrans::ShaderTrans::GLSL2SpirV(stage, code_str, spirv, true);
    } else if (strcmp(lang_str, "hlsl") == 0) {
        shadertrans::ShaderTrans::HLSL2SpirV(stage, code_str, "main", spirv);
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

void w_ShaderTools_disassemble()
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

void w_ShaderTools_hlsl2glsl()
{
    const char* stage_str = ves_tostring(1);
    const char* hlsl = ves_tostring(2);
    const char* entry_point = ves_tostring(3);

    std::vector<unsigned int> spirv;
    auto stage = to_shader_stage(stage_str);
    shadertrans::ShaderTrans::HLSL2SpirV(stage, hlsl, entry_point, spirv);
    std::string glsl;
    shadertrans::ShaderTrans::SpirV2GLSL(stage, spirv, glsl);
    glsl = shadertrans::ShaderPreprocess::PrepareHLSL(glsl, entry_point);

    ves_set_lstring(0, glsl.c_str(), glsl.size());
}

// ShaderModule

void w_ShaderModule_const_bool()
{
    spvgentwo::Module* module = *(spvgentwo::Module**)ves_toforeign(0);
    bool b = ves_toboolean(1);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::ConstBool(module, b);
    return_inst(inst);
}

void w_ShaderModule_const_int()
{
    spvgentwo::Module* module = *(spvgentwo::Module**)ves_toforeign(0);
    int x = (int)ves_tonumber(1);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::ConstInt(module, x);
    return_inst(inst);
}

void w_ShaderModule_const_float()
{
    spvgentwo::Module* module = *(spvgentwo::Module**)ves_toforeign(0);
    float x = (float)ves_tonumber(1);
    
    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::ConstFloat(module, x);
    return_inst(inst);
}

void w_ShaderModule_const_float2()
{
    spvgentwo::Module* module = *(spvgentwo::Module**)ves_toforeign(0);
    float x = (float)ves_tonumber(1);
    float y = (float)ves_tonumber(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::ConstFloat2(module, x, y);
    return_inst(inst);
}

void w_ShaderModule_const_float3()
{
    spvgentwo::Module* module = *(spvgentwo::Module**)ves_toforeign(0);
    float x = (float)ves_tonumber(1);
    float y = (float)ves_tonumber(2);
    float z = (float)ves_tonumber(3);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::ConstFloat3(module, x, y, z);
    return_inst(inst);
}

void w_ShaderModule_const_float4()
{
    spvgentwo::Module* module = *(spvgentwo::Module**)ves_toforeign(0);
    float x = (float)ves_tonumber(1);
    float y = (float)ves_tonumber(2);
    float z = (float)ves_tonumber(3);
    float w = (float)ves_tonumber(4);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::ConstFloat4(module, x, y, z, w);
    return_inst(inst);
}

void w_ShaderModule_const_mat2()
{
    spvgentwo::Module* module = *(spvgentwo::Module**)ves_toforeign(0);

    float m[4];
    const int len = ves_len(1);
    GD_ASSERT(len == 4, "error m size");
    std::vector<unsigned int> spirv(len);
    for (int i = 0; i < len; ++i)
    {
        ves_geti(1, i);
        m[i] = (float)ves_tonumber(-1);
        ves_pop(1);
    }

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::ConstMatrix2(module, m);
    return_inst(inst);
}

void w_ShaderModule_const_mat3()
{
    spvgentwo::Module* module = *(spvgentwo::Module**)ves_toforeign(0);

    float m[9];
    const int len = ves_len(1);
    GD_ASSERT(len == 9, "error m size");
    std::vector<unsigned int> spirv(len);
    for (int i = 0; i < len; ++i)
    {
        ves_geti(1, i);
        m[i] = (float)ves_tonumber(-1);
        ves_pop(1);
    }

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::ConstMatrix3(module, m);
    return_inst(inst);
}

void w_ShaderModule_const_mat4()
{
    spvgentwo::Module* module = *(spvgentwo::Module**)ves_toforeign(0);

    float m[16];
    const int len = ves_len(1);
    GD_ASSERT(len == 16, "error m size");
    std::vector<unsigned int> spirv(len);
    for (int i = 0; i < len; ++i)
    {
        ves_geti(1, i);
        m[i] = (float)ves_tonumber(-1);
        ves_pop(1);
    }

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::ConstMatrix4(module, m);
    return_inst(inst);
}

void w_ShaderModule_query_func()
{
    spvgentwo::Module* module = *(spvgentwo::Module**)ves_toforeign(0);
    const char* name = ves_tostring(1);

    spvgentwo::Function* func = shadertrans::SpirvGenTwo::QueryFunction(*module, name);
    return_func(func);
}

void w_ShaderModule_create_decl_func()
{
    spvgentwo::Module* module = *(spvgentwo::Module**)ves_toforeign(0);
    spvgentwo::Function* func = *(spvgentwo::Function**)ves_toforeign(1);

    spvgentwo::Function* decl_func = shadertrans::SpirvGenTwo::CreateDeclFunc(module, func);
    return_func(decl_func);
}

void w_ShaderModule_create_func()
{
    spvgentwo::Module* module = *(spvgentwo::Module**)ves_toforeign(0);
    const char* name = ves_tostring(1);
    const char* ret = ves_tostring(2);

    const int len = ves_len(3);
    std::vector<std::string> args(len);
    for (int i = 0; i < len; ++i)
    {
        ves_geti(3, i);
        args[i] = ves_tostring(-1);
        ves_pop(1);
    }

    spvgentwo::Function* func = shadertrans::SpirvGenTwo::CreateFunc(module, name, ret, args);
    return_func(func);
}

// ShaderFunc

void w_ShaderFunc_add_variable()
{
    spvgentwo::Function* func = *(spvgentwo::Function**)ves_toforeign(0);
    const char* name = ves_tostring(1);
    spvgentwo::Instruction* value = to_inst(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::AddVariable(func, name, value);
    return_inst(inst);
}

void w_ShaderFunc_variable_float()
{
    spvgentwo::Function* func = *(spvgentwo::Function**)ves_toforeign(0);
    const char* name = ves_tostring(1);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::VariableFloat(func, name);
    return_inst(inst);
}

void w_ShaderFunc_variable_float2()
{
    spvgentwo::Function* func = *(spvgentwo::Function**)ves_toforeign(0);
    const char* name = ves_tostring(1);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::VariableFloat2(func, name);
    return_inst(inst);
}

void w_ShaderFunc_variable_float3()
{
    spvgentwo::Function* func = *(spvgentwo::Function**)ves_toforeign(0);
    const char* name = ves_tostring(1);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::VariableFloat3(func, name);
    return_inst(inst);
}

void w_ShaderFunc_variable_float4()
{
    spvgentwo::Function* func = *(spvgentwo::Function**)ves_toforeign(0);
    const char* name = ves_tostring(1);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::VariableFloat4(func, name);
    return_inst(inst);
}

void w_ShaderFunc_get_block()
{
    spvgentwo::Function* func = *(spvgentwo::Function**)ves_toforeign(0);
    spvgentwo::BasicBlock* bb = shadertrans::SpirvGenTwo::GetFuncBlock(func);
    return_block(bb);
}

void w_ShaderFunc_add_block()
{
    spvgentwo::Function* func = *(spvgentwo::Function**)ves_toforeign(0);
    const char* name = ves_tostring(1);

    auto bb = shadertrans::SpirvGenTwo::AddBlock(func, name);
    return_block(bb);
}

void w_ShaderFunc_get_param()
{
    spvgentwo::Function* func = *(spvgentwo::Function**)ves_toforeign(0);
    int index = (int)ves_tonumber(1);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::GetFuncParam(func, index);
    return_inst(inst);
}

void w_ShaderFunc_get_args()
{
    spvgentwo::Function* func = *(spvgentwo::Function**)ves_toforeign(0);

    std::vector<std::string> names;
    shadertrans::SpirvGenTwo::GetFuncParamNames(func, names);

    ves_pop(1);

    ves_newlist((int)names.size());
    for (int i = 0; i < names.size(); ++i)
    {
        ves_pushstring(names[i].c_str());
        ves_seti(-2, i);
        ves_pop(1);
    }
}

void w_ShaderFunc_call()
{
    spvgentwo::Function* caller = *(spvgentwo::Function**)ves_toforeign(0);
    spvgentwo::Function* callee = *(spvgentwo::Function**)ves_toforeign(1);

    const int len = ves_len(2);
    std::vector<spvgentwo::Instruction*> args(len);
    for (int i = 0; i < len; ++i)
    {
        ves_geti(2, i);
        args[i] = to_inst(-1);
        ves_pop(1);
    }

    for (auto& arg : args) {
        if (!arg) {
            ves_set_nil(0);
            return;
        }
    }

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::FuncCall(caller, callee, args);
    return_inst(inst);
}

// ShaderBlock

void w_ShaderBlock_access_chain()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* base = to_inst(1);
    int index = (int)ves_tonumber(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::AccessChain(bb, base, index);
    return_inst(inst);
}

void w_ShaderBlock_compose_float2()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* x = to_inst(1);
    spvgentwo::Instruction* y = to_inst(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::ComposeFloat2(bb, x, y);
    return_inst(inst);
}

void w_ShaderBlock_compose_float3()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* x = to_inst(1);
    spvgentwo::Instruction* y = to_inst(2);
    spvgentwo::Instruction* z = to_inst(3);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::ComposeFloat3(bb, x, y, z);
    return_inst(inst);
}

void w_ShaderBlock_compose_float4()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* x = to_inst(1);
    spvgentwo::Instruction* y = to_inst(2);
    spvgentwo::Instruction* z = to_inst(3);
    spvgentwo::Instruction* w = to_inst(4);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::ComposeFloat4(bb, x, y, z, w);
    return_inst(inst);
}

void w_ShaderBlock_compose_extract()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* comp = to_inst(1);
    int index = (int)ves_tonumber(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::ComposeExtract(bb, comp, index);
    return_inst(inst);
}

void w_ShaderBlock_dot()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* a = to_inst(1);
    spvgentwo::Instruction* b = to_inst(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::Dot(bb, a, b);
    return_inst(inst);
}

void w_ShaderBlock_cross()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* a = to_inst(1);
    spvgentwo::Instruction* b = to_inst(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::Cross(bb, a, b);
    return_inst(inst);
}

void w_ShaderBlock_add()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* a = to_inst(1);
    spvgentwo::Instruction* b = to_inst(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::Add(bb, a, b);
    return_inst(inst);
}

void w_ShaderBlock_sub()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* a = to_inst(1);
    spvgentwo::Instruction* b = to_inst(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::Sub(bb, a, b);
    return_inst(inst);
}

void w_ShaderBlock_mul()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* a = to_inst(1);
    spvgentwo::Instruction* b = to_inst(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::Mul(bb, a, b);
    return_inst(inst);
}

void w_ShaderBlock_div()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* a = to_inst(1);
    spvgentwo::Instruction* b = to_inst(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::Div(bb, a, b);
    return_inst(inst);
}

void w_ShaderBlock_negate()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* v = to_inst(1);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::Negate(bb, v);
    return_inst(inst);
}

void w_ShaderBlock_reflect()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* I = to_inst(1);
    spvgentwo::Instruction* N = to_inst(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::Reflect(bb, I, N);
    return_inst(inst);
}

void w_ShaderBlock_sqrt()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* v = to_inst(1);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::Sqrt(bb, v);
    return_inst(inst);
}

void w_ShaderBlock_pow()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* x = to_inst(1);
    spvgentwo::Instruction* y = to_inst(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::Pow(bb, x, y);
    return_inst(inst);
}

void w_ShaderBlock_normalize()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* v = to_inst(1);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::Normalize(bb, v);
    return_inst(inst);
}

void w_ShaderBlock_length()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* v = to_inst(1);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::Length(bb, v);
    return_inst(inst);
}

void w_ShaderBlock_max()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* x = to_inst(1);
    spvgentwo::Instruction* y = to_inst(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::Max(bb, x, y);
    return_inst(inst);
}

void w_ShaderBlock_min()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* x = to_inst(1);
    spvgentwo::Instruction* y = to_inst(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::Min(bb, x, y);
    return_inst(inst);
}

void w_ShaderBlock_clamp()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* x = to_inst(1);
    spvgentwo::Instruction* min = to_inst(2);
    spvgentwo::Instruction* max = to_inst(3);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::Clamp(bb, x, min, max);
    if (!inst) {
        ves_set_nil(0);
    } else {
        return_inst(inst);
    }
}

void w_ShaderBlock_mix()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* x = to_inst(1);
    spvgentwo::Instruction* y = to_inst(2);
    spvgentwo::Instruction* a = to_inst(3);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::Mix(bb, x, y, a);
    return_inst(inst);
}

void w_ShaderBlock_is_equal()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* a = to_inst(1);
    spvgentwo::Instruction* b = to_inst(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::IsEqual(bb, a, b);
    return_inst(inst);
}

void w_ShaderBlock_is_not_equal()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* a = to_inst(1);
    spvgentwo::Instruction* b = to_inst(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::IsNotEqual(bb, a, b);
    return_inst(inst);
}

void w_ShaderBlock_is_greater()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* a = to_inst(1);
    spvgentwo::Instruction* b = to_inst(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::IsGreater(bb, a, b);
    return_inst(inst);
}

void w_ShaderBlock_is_greater_equal()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* a = to_inst(1);
    spvgentwo::Instruction* b = to_inst(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::IsGreaterEqual(bb, a, b);
    return_inst(inst);
}

void w_ShaderBlock_is_less()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* a = to_inst(1);
    spvgentwo::Instruction* b = to_inst(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::IsLess(bb, a, b);
    return_inst(inst);
}

void w_ShaderBlock_is_less_equal()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* a = to_inst(1);
    spvgentwo::Instruction* b = to_inst(2);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::IsLessEqual(bb, a, b);
    return_inst(inst);
}

void w_ShaderBlock_kill()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);

    shadertrans::SpirvGenTwo::Kill(bb);
}

void w_ShaderBlock_return_null()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);

    shadertrans::SpirvGenTwo::Return(bb);
}

void w_ShaderBlock_return_value()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* inst = to_inst(1);

    shadertrans::SpirvGenTwo::ReturnValue(bb, inst);
}

void w_ShaderBlock_load()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* val = to_inst(1);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::Load(bb, val);
    return_inst(inst);
}

void w_ShaderBlock_store()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* dst = to_inst(1);
    spvgentwo::Instruction* src = to_inst(2);

    shadertrans::SpirvGenTwo::Store(bb, dst, src);
}

void w_ShaderBlock_image_sample()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* img = to_inst(1);
    spvgentwo::Instruction* uv = to_inst(2);
    spvgentwo::Instruction* lod = to_inst(3);

    spvgentwo::Instruction* inst = shadertrans::SpirvGenTwo::ImageSample(bb, img, uv, lod);
    return_inst(inst);
}

void w_ShaderBlock_branch_if()
{
    spvgentwo::BasicBlock* bb = *(spvgentwo::BasicBlock**)ves_toforeign(0);
    spvgentwo::Instruction* cond = to_inst(1);
    spvgentwo::BasicBlock* true_bb  = *(spvgentwo::BasicBlock**)ves_toforeign(2);
    spvgentwo::BasicBlock* false_bb = *(spvgentwo::BasicBlock**)ves_toforeign(3);
    spvgentwo::BasicBlock* merge_bb = *(spvgentwo::BasicBlock**)ves_toforeign(4);

    merge_bb = shadertrans::SpirvGenTwo::If(bb, cond, true_bb, false_bb, merge_bb);
    return_block(merge_bb);
}

// ShaderInst

void w_ShaderInst_get_type()
{
    spvgentwo::Instruction* inst = to_inst(0);
    if (!inst) {
        ves_set_nil(0);
    } else {
        const char* type = shadertrans::SpirvGenTwo::GetType(*inst);
        ves_set_lstring(0, type, strlen(type));
    }
}

void w_ShaderInst_is_vector()
{
    spvgentwo::Instruction* inst = to_inst(0);
    if (!inst) {
        ves_set_nil(0);
    } else {
        ves_set_boolean(0, shadertrans::SpirvGenTwo::IsVector(*inst));
    }
}

void w_ShaderInst_get_vector_num()
{
    spvgentwo::Instruction* inst = to_inst(0);
    if (!inst) {
        ves_set_nil(0);
    } else {
        ves_set_number(0, shadertrans::SpirvGenTwo::GetVectorNum(*inst));
    }
}

// ShaderGen

void w_ShaderGen_allocate()
{
    auto builder = std::make_shared<shadertrans::ShaderBuilder>();
    auto proxy = (tt::Proxy<shadertrans::ShaderBuilder>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<shadertrans::ShaderBuilder>));
    proxy->obj = builder;
}

int w_ShaderGen_finalize(void* data)
{
    auto proxy = (tt::Proxy<shadertrans::ShaderBuilder>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<shadertrans::ShaderBuilder>);
}

void w_ShaderGen_add_input()
{
    const char* name = ves_tostring(1);
    const char* type = ves_tostring(2);

    auto builder = ((tt::Proxy<shadertrans::ShaderBuilder>*)ves_toforeign(0))->obj;
    auto input = builder->AddInput(name, type);
    return_inst(input);
}

void w_ShaderGen_add_output()
{
    const char* name = ves_tostring(1);
    const char* type = ves_tostring(2);

    auto builder = ((tt::Proxy<shadertrans::ShaderBuilder>*)ves_toforeign(0))->obj;
    auto output = builder->AddOutput(name, type);
    return_inst(output);
}

void w_ShaderGen_add_uniform()
{
    spvgentwo::Module* module = *(spvgentwo::Module**)ves_toforeign(1);
    const char* name = ves_tostring(2);
    const char* type = ves_tostring(3);

    auto builder = ((tt::Proxy<shadertrans::ShaderBuilder>*)ves_toforeign(0))->obj;
    auto unif = builder->AddUniform(module, name, type);
    return_inst(unif);
}

void w_ShaderGen_query_unif_name()
{
    spvgentwo::Instruction* unif = to_inst(1);

    auto builder = ((tt::Proxy<shadertrans::ShaderBuilder>*)ves_toforeign(0))->obj;
    auto name = builder->QueryUniformName(unif);
    ves_set_lstring(0, name, strlen(name));
}

void w_ShaderGen_add_module()
{
    const char* stage_str = ves_tostring(1);
    const char* code_str = ves_tostring(2);
    const char* lang_str = ves_tostring(3);
    const char* name = ves_tostring(4);
    const char* enter_point = ves_tostring(5);

    auto stage = to_shader_stage(stage_str);

    auto builder = ((tt::Proxy<shadertrans::ShaderBuilder>*)ves_toforeign(0))->obj;
    spvgentwo::Module* module = builder->AddModule(stage, code_str, lang_str, name, enter_point)->impl.get();
    return_module(module);
}

void w_ShaderGen_func_replace()
{
    auto builder = ((tt::Proxy<shadertrans::ShaderBuilder>*)ves_toforeign(0))->obj;
    spvgentwo::Function* from = *(spvgentwo::Function**)ves_toforeign(1);
    spvgentwo::Function* to = *(spvgentwo::Function**)ves_toforeign(2);

    builder->ReplaceFunc(from, to);
}

void w_ShaderGen_get_main_module()
{
    auto builder = ((tt::Proxy<shadertrans::ShaderBuilder>*)ves_toforeign(0))->obj;
    spvgentwo::Module* module = builder->GetMainModule();
    return_module(module);
}

void w_ShaderGen_get_main_func()
{
    auto builder = ((tt::Proxy<shadertrans::ShaderBuilder>*)ves_toforeign(0))->obj;
    spvgentwo::Function* func = builder->GetMainFunc();
    return_func(func);
}

void w_ShaderGen_add_link_decl()
{
    auto builder = ((tt::Proxy<shadertrans::ShaderBuilder>*)ves_toforeign(0))->obj;
    spvgentwo::Function* func = *(spvgentwo::Function**)ves_toforeign(1);
    const char* name = ves_tostring(2);
    bool is_export = ves_toboolean(3);

    builder->AddLinkDecl(func, name, is_export);
}

void w_ShaderGen_connect_cs_main()
{
    auto builder = ((tt::Proxy<shadertrans::ShaderBuilder>*)ves_toforeign(0))->obj;
    const char* main_glsl = ves_tostring(1);
    auto glsl = builder->ConnectCSMain(main_glsl);
    if (glsl.empty()) {
        ves_set_nil(0);
    } else {
        ves_set_lstring(0, glsl.c_str(), glsl.size());
    }
}

}

namespace tt
{

VesselForeignMethodFn ShaderBindMethod(const char* signature)
{
    // ShaderTools

    if (strcmp(signature, "static ShaderTools.code2spirv(_,_,_)") == 0) return w_ShaderTools_code2spirv;
    if (strcmp(signature, "static ShaderTools.disassemble(_)") == 0) return w_ShaderTools_disassemble;

    if (strcmp(signature, "static ShaderTools.hlsl2glsl(_,_,_)") == 0) return w_ShaderTools_hlsl2glsl;

    // ShaderModule

    if (strcmp(signature, "ShaderModule.const_bool(_)") == 0) return w_ShaderModule_const_bool;
    if (strcmp(signature, "ShaderModule.const_int(_)") == 0) return w_ShaderModule_const_int;

    if (strcmp(signature, "ShaderModule.const_float(_)") == 0) return w_ShaderModule_const_float;
    if (strcmp(signature, "ShaderModule.const_float2(_,_)") == 0) return w_ShaderModule_const_float2;
    if (strcmp(signature, "ShaderModule.const_float3(_,_,_)") == 0) return w_ShaderModule_const_float3;
    if (strcmp(signature, "ShaderModule.const_float4(_,_,_,_)") == 0) return w_ShaderModule_const_float4;

    if (strcmp(signature, "ShaderModule.const_mat2(_)") == 0) return w_ShaderModule_const_mat2;
    if (strcmp(signature, "ShaderModule.const_mat3(_)") == 0) return w_ShaderModule_const_mat3;
    if (strcmp(signature, "ShaderModule.const_mat4(_)") == 0) return w_ShaderModule_const_mat4;

    if (strcmp(signature, "ShaderModule.query_func(_)") == 0) return w_ShaderModule_query_func;

    if (strcmp(signature, "ShaderModule.create_decl_func(_)") == 0) return w_ShaderModule_create_decl_func;
    if (strcmp(signature, "ShaderModule.create_func(_,_,_)") == 0) return w_ShaderModule_create_func;

    // ShaderFunc

    if (strcmp(signature, "ShaderFunc.add_variable(_,_)") == 0) return w_ShaderFunc_add_variable;

    if (strcmp(signature, "ShaderFunc.variable_float(_)") == 0) return w_ShaderFunc_variable_float;
    if (strcmp(signature, "ShaderFunc.variable_float2(_)") == 0) return w_ShaderFunc_variable_float2;
    if (strcmp(signature, "ShaderFunc.variable_float3(_)") == 0) return w_ShaderFunc_variable_float3;
    if (strcmp(signature, "ShaderFunc.variable_float4(_)") == 0) return w_ShaderFunc_variable_float4;

    if (strcmp(signature, "ShaderFunc.get_block()") == 0) return w_ShaderFunc_get_block;
    if (strcmp(signature, "ShaderFunc.add_block(_)") == 0) return w_ShaderFunc_add_block;

    if (strcmp(signature, "ShaderFunc.get_param(_)") == 0) return w_ShaderFunc_get_param;
    if (strcmp(signature, "ShaderFunc.get_args()") == 0) return w_ShaderFunc_get_args;
    if (strcmp(signature, "ShaderFunc.call(_,_)") == 0) return w_ShaderFunc_call;

    // ShaderBlock

    if (strcmp(signature, "ShaderBlock.access_chain(_,_)") == 0) return w_ShaderBlock_access_chain;
    if (strcmp(signature, "ShaderBlock.compose_float2(_,_)") == 0) return w_ShaderBlock_compose_float2;
    if (strcmp(signature, "ShaderBlock.compose_float3(_,_,_)") == 0) return w_ShaderBlock_compose_float3;
    if (strcmp(signature, "ShaderBlock.compose_float4(_,_,_,_)") == 0) return w_ShaderBlock_compose_float4;
    if (strcmp(signature, "ShaderBlock.compose_extract(_,_)") == 0) return w_ShaderBlock_compose_extract;
    if (strcmp(signature, "ShaderBlock.dot_(_,_)") == 0) return w_ShaderBlock_dot;
    if (strcmp(signature, "ShaderBlock.cross_(_,_)") == 0) return w_ShaderBlock_cross;
    if (strcmp(signature, "ShaderBlock.add_(_,_)") == 0) return w_ShaderBlock_add;
    if (strcmp(signature, "ShaderBlock.sub_(_,_)") == 0) return w_ShaderBlock_sub;
    if (strcmp(signature, "ShaderBlock.mul(_,_)") == 0) return w_ShaderBlock_mul;
    if (strcmp(signature, "ShaderBlock.div(_,_)") == 0) return w_ShaderBlock_div;
    if (strcmp(signature, "ShaderBlock.negate_(_)") == 0) return w_ShaderBlock_negate;
    if (strcmp(signature, "ShaderBlock.reflect(_,_)") == 0) return w_ShaderBlock_reflect;
    if (strcmp(signature, "ShaderBlock.sqrt_(_)") == 0) return w_ShaderBlock_sqrt;
    if (strcmp(signature, "ShaderBlock.pow_(_,_)") == 0) return w_ShaderBlock_pow;
    if (strcmp(signature, "ShaderBlock.normalize(_)") == 0) return w_ShaderBlock_normalize;
    if (strcmp(signature, "ShaderBlock.length(_)") == 0) return w_ShaderBlock_length;
    if (strcmp(signature, "ShaderBlock.max(_,_)") == 0) return w_ShaderBlock_max;
    if (strcmp(signature, "ShaderBlock.min(_,_)") == 0) return w_ShaderBlock_min;
    if (strcmp(signature, "ShaderBlock.clamp(_,_,_)") == 0) return w_ShaderBlock_clamp;
    if (strcmp(signature, "ShaderBlock.mix_(_,_,_)") == 0) return w_ShaderBlock_mix;

    if (strcmp(signature, "ShaderBlock.is_equal(_,_)") == 0) return w_ShaderBlock_is_equal;
    if (strcmp(signature, "ShaderBlock.is_not_equal(_,_)") == 0) return w_ShaderBlock_is_not_equal;
    if (strcmp(signature, "ShaderBlock.is_greater(_,_)") == 0) return w_ShaderBlock_is_greater;
    if (strcmp(signature, "ShaderBlock.is_greater_equal(_,_)") == 0) return w_ShaderBlock_is_greater_equal;
    if (strcmp(signature, "ShaderBlock.is_less(_,_)") == 0) return w_ShaderBlock_is_less;
    if (strcmp(signature, "ShaderBlock.is_less_equal(_,_)") == 0) return w_ShaderBlock_is_less_equal;

    if (strcmp(signature, "ShaderBlock.kill()") == 0) return w_ShaderBlock_kill;

    if (strcmp(signature, "ShaderBlock.return_null()") == 0) return w_ShaderBlock_return_null;
    if (strcmp(signature, "ShaderBlock.return_value(_)") == 0) return w_ShaderBlock_return_value;

    if (strcmp(signature, "ShaderBlock.load(_)") == 0) return w_ShaderBlock_load;
    if (strcmp(signature, "ShaderBlock.store(_,_)") == 0) return w_ShaderBlock_store;

    if (strcmp(signature, "ShaderBlock.image_sample(_,_,_)") == 0) return w_ShaderBlock_image_sample;

    if (strcmp(signature, "ShaderBlock.branch_if(_,_,_,_)") == 0) return w_ShaderBlock_branch_if;

    // ShaderInst

    if (strcmp(signature, "ShaderInst.get_type()") == 0) return w_ShaderInst_get_type;

    if (strcmp(signature, "ShaderInst.is_vector()") == 0) return w_ShaderInst_is_vector;
    if (strcmp(signature, "ShaderInst.get_vector_num()") == 0) return w_ShaderInst_get_vector_num;

    // ShaderGen

    if (strcmp(signature, "ShaderGen.add_input(_,_)") == 0) return w_ShaderGen_add_input;
    if (strcmp(signature, "ShaderGen.add_output(_,_)") == 0) return w_ShaderGen_add_output;
    if (strcmp(signature, "ShaderGen.add_uniform(_,_,_)") == 0) return w_ShaderGen_add_uniform;
    if (strcmp(signature, "ShaderGen.query_unif_name(_)") == 0) return w_ShaderGen_query_unif_name;

    if (strcmp(signature, "ShaderGen.add_module(_,_,_,_,_)") == 0) return w_ShaderGen_add_module;

    if (strcmp(signature, "ShaderGen.func_replace(_,_)") == 0) return w_ShaderGen_func_replace;

    if (strcmp(signature, "ShaderGen.get_main_module()") == 0) return w_ShaderGen_get_main_module;
    if (strcmp(signature, "ShaderGen.get_main_func()") == 0) return w_ShaderGen_get_main_func;
    if (strcmp(signature, "ShaderGen.add_link_decl(_,_,_)") == 0) return w_ShaderGen_add_link_decl;
    
    if (strcmp(signature, "ShaderGen.connect_cs_main(_)") == 0) return w_ShaderGen_connect_cs_main;

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