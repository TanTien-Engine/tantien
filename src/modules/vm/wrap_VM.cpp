#include "wrap_VM.h"
#include "Bytecodes.h"
#include "VMHelper.h"
#include "CodesBuilder.h"
#include "Decompiler.h"
#include "CodesOptimize.h"
#include "modules/script/TransHelper.h"
#include "modules/vm/Bytecodes.h"
#include "modules/vm/Compiler.h"
#include "modules/vm/VM.h"
#include "modules/vm/math_opcodes.h"
#include "modules/vm/geo_opcodes.h"
#include "modules/vm/stl_opcodes.h"
#include "ValueType.h"

#include <SM_Plane.h>
#include <polymesh3/Polytope.h>
#include <easyvm/VM.h>
#include <easyvm/OpCodes.h>
#include <easyvm/VMHelper.h>

#include <string>

namespace
{

void bytecodes_write(uint8_t op, int num)
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(0))->obj;

    tt::CodesBuilder builder(code, tt::VM::Instance()->GetOpFields());

    builder.WriteType(op);

    for (int i = 0; i < num; ++i) {
        builder.WriteReg((uint8_t)ves_tonumber(i + 1));
    }
}

void w_Bytecodes_allocate()
{
    auto proxy = (tt::Proxy<tt::Bytecodes>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<tt::Bytecodes>));
    proxy->obj = std::make_shared<tt::Bytecodes>();
}

int w_Bytecodes_finalize(void* data)
{
    auto proxy = (tt::Proxy<tt::Bytecodes>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<tt::Bytecodes>);
}

void w_Bytecodes_store_bool()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(0))->obj;

    tt::CodesBuilder builder(code, tt::VM::Instance()->GetOpFields());

    builder.WriteType(evm::OP_BOOL_STORE);
    builder.WriteReg((uint8_t)ves_tonumber(1));
    builder.WriteBool(ves_toboolean(2));
}

void w_Bytecodes_store_num()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(0))->obj;

    tt::CodesBuilder builder(code, tt::VM::Instance()->GetOpFields());

    builder.WriteType(evm::OP_NUMBER_STORE);
    builder.WriteReg((uint8_t)ves_tonumber(1));
    builder.WriteDouble(ves_tonumber(2));
}

void w_Bytecodes_print_num()
{
    bytecodes_write(evm::OP_NUMBER_PRINT, 1);
}

void w_Bytecodes_negate_num()
{
    bytecodes_write(evm::OP_NUMBER_NEGATE, 2);
}

void w_Bytecodes_add_num()
{
    bytecodes_write(evm::OP_ADD, 3);
}

void w_Bytecodes_sub_num()
{
    bytecodes_write(evm::OP_SUB, 3);
}

void w_Bytecodes_mul_num()
{
    bytecodes_write(evm::OP_MUL, 3);
}

void w_Bytecodes_div_num()
{
    bytecodes_write(evm::OP_DIV, 3);
}

void w_Bytecodes_inc_num()
{
    bytecodes_write(evm::OP_INC, 1);
}

void w_Bytecodes_dec_num()
{
    bytecodes_write(evm::OP_DEC, 1);
}

void w_Bytecodes_cmp_num()
{
    bytecodes_write(evm::OP_CMP, 3);
}

void w_Bytecodes_jump_if_not()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(0))->obj;

    tt::CodesBuilder builder(code, tt::VM::Instance()->GetOpFields());

    builder.WriteType(evm::OP_JUMP_IF_NOT);
    builder.WriteInt((int)ves_tonumber(1));
    builder.WriteReg((uint8_t)ves_tonumber(2));
}

void w_Bytecodes_vec2_create_i()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(0))->obj;

    tt::CodesBuilder builder(code, tt::VM::Instance()->GetOpFields());

    builder.WriteType(tt::OP_VEC2_CREATE_I);
    builder.WriteReg((uint8_t)ves_tonumber(1));
    builder.WriteFloat((float)ves_tonumber(2));
    builder.WriteFloat((float)ves_tonumber(3));
}

void w_Bytecodes_vec3_create_r()
{
    bytecodes_write(tt::OP_VEC3_CREATE_R, 4);
}

void w_Bytecodes_vec3_create_i()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(0))->obj;

    tt::CodesBuilder builder(code, tt::VM::Instance()->GetOpFields());

    builder.WriteType(tt::OP_VEC3_CREATE_I);

    builder.WriteReg((uint8_t)ves_tonumber(1));

    builder.WriteFloat((float)ves_tonumber(2));
    builder.WriteFloat((float)ves_tonumber(3));
    builder.WriteFloat((float)ves_tonumber(4));
}

void w_Bytecodes_vec3_print()
{
    bytecodes_write(tt::OP_VEC3_PRINT, 1);
}

void w_Bytecodes_vec3_add()
{
    bytecodes_write(tt::OP_VEC3_ADD, 3);
}

void w_Bytecodes_vec3_sub()
{
    bytecodes_write(tt::OP_VEC3_SUB, 3);
}

void w_Bytecodes_vec3_transform()
{
    bytecodes_write(tt::OP_VEC3_TRANSFORM, 2);
}

void w_Bytecodes_vec3_get_x()
{
    bytecodes_write(tt::OP_VEC3_GET_X, 2);
}

void w_Bytecodes_vec3_get_y()
{
    bytecodes_write(tt::OP_VEC3_GET_Y, 2);
}

void w_Bytecodes_vec3_get_z()
{
    bytecodes_write(tt::OP_VEC3_GET_Z, 2);
}

void w_Bytecodes_vec4_create_i()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(0))->obj;

    tt::CodesBuilder builder(code, tt::VM::Instance()->GetOpFields());

    builder.WriteType(tt::OP_VEC4_CREATE_I);

    builder.WriteReg((uint8_t)ves_tonumber(1));

    builder.WriteFloat((float)ves_tonumber(2));
    builder.WriteFloat((float)ves_tonumber(3));
    builder.WriteFloat((float)ves_tonumber(4));
    builder.WriteFloat((float)ves_tonumber(5));
}

void w_Bytecodes_create_mat4()
{
    bytecodes_write(tt::OP_MATRIX_CREATE, 1);
}

void w_Bytecodes_mat4_rotate()
{
    bytecodes_write(tt::OP_MATRIX_ROTATE, 2);
}

void w_Bytecodes_mat4_translate()
{
    bytecodes_write(tt::OP_MATRIX_TRANSLATE, 2);
}

void w_Bytecodes_create_cube()
{
    bytecodes_write(tt::OP_CREATE_CUBE, 3);
}

void w_Bytecodes_create_vector()
{
    bytecodes_write(tt::StlOpCode::OP_VECTOR_CREATE, 1);
}

void w_Bytecodes_vector_add()
{
    bytecodes_write(tt::StlOpCode::OP_VECTOR_ADD, 2);
}

void w_Bytecodes_vector_concat()
{
    bytecodes_write(tt::StlOpCode::OP_VECTOR_CONCAT, 2);
}

void w_Bytecodes_vector_get()
{
    bytecodes_write(tt::StlOpCode::OP_VECTOR_GET, 3);
}

void w_Bytecodes_create_plane()
{
    bytecodes_write(tt::OP_CREATE_PLANE, 4);
}

void w_Bytecodes_create_plane_2()
{
    bytecodes_write(tt::OP_CREATE_PLANE_2, 3);
}

void w_Bytecodes_create_polyface()
{
    bytecodes_write(tt::OP_CREATE_POLYFACE, 2);
}

void w_Bytecodes_create_polytope()
{
    bytecodes_write(tt::OP_CREATE_POLYTOPE, 2);
}

void w_Bytecodes_create_polyface_2()
{
    bytecodes_write(tt::OP_CREATE_POLYFACE_2, 3);
}

void w_Bytecodes_create_polytope_2()
{
    bytecodes_write(tt::OP_CREATE_POLYTOPE_2, 3);
}

void w_Bytecodes_polytope_transform()
{
    bytecodes_write(tt::OP_POLYTOPE_TRANSFORM, 2);
}

void w_Bytecodes_polytope_subtract()
{
    bytecodes_write(tt::OP_POLYTOPE_SUBTRACT, 3);
}

void w_Bytecodes_polytope_extrude()
{
    bytecodes_write(tt::OP_POLYTOPE_EXTRUDE, 2);
}

void w_Bytecodes_polytope_clip()
{
    bytecodes_write(tt::OP_POLYTOPE_CLIP, 4);
}

void w_Bytecodes_polytope_set_dirty()
{
    bytecodes_write(tt::OP_POLYTOPE_SET_DIRTY, 1);
}

void w_Bytecodes_polypoint_select()
{
    bytecodes_write(tt::OP_POLYPOINT_SELECT, 3);
}

void w_Bytecodes_polyface_select()
{
    bytecodes_write(tt::OP_POLYFACE_SELECT, 4);
}

void w_Bytecodes_add()
{
    bytecodes_write(tt::OP_ADD, 3);
}

void w_Bytecodes_sub()
{
    bytecodes_write(tt::OP_SUB, 3);
}

void w_Bytecodes_mul()
{
    bytecodes_write(tt::OP_MUL, 3);
}

void w_Bytecodes_div()
{
    bytecodes_write(tt::OP_DIV, 3);
}

void w_Bytecodes_transform_unknown()
{
    bytecodes_write(tt::OP_TRANSFORM_UNKNOWN, 2);
}

void w_CodeStats_stat_call()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(1))->obj;
    const char* name = ves_tostring(2);
    code->StatCall(name);
}

void w_CodeStats_add_code_block()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(1))->obj;

    int begin = (int)ves_tonumber(2);
    int end = (int)ves_tonumber(3);

    int reg = (int)ves_tonumber(4);

    tt::Decompiler dc(code, tt::VM::Instance()->GetOpFields());
    size_t hash = dc.Hash(begin, end);

    code->GetOptimizer()->AddBlock(hash, begin, end, reg);
}

void w_CodeStats_add_cost()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(1))->obj;
    int cost = (int)ves_tonumber(2);
    code->AddCost(cost);
}

void w_CodeStats_get_cost()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(1))->obj;
    ves_set_number(0, code->GetCost());
}

void w_CodeTools_get_size()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(1))->obj;
    ves_set_number(0, static_cast<double>(code->GetCode().size()));
}

void w_CodeTools_decompiler()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(1))->obj;

    int begin = (int)ves_tonumber(2);
    int end = (int)ves_tonumber(3);
    if (begin == 0 && end == 0) {
        begin = 0;
        end = static_cast<int>(code->GetCode().size());
    }

    tt::Decompiler dc(code, tt::VM::Instance()->GetOpFields());
    dc.Print(begin, end);
}

void w_CodeTools_hash()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(1))->obj;

    int begin = (int)ves_tonumber(2);
    int end = (int)ves_tonumber(3);
    if (begin == 0 && end == 0) {
        begin = 0;
        end = static_cast<int>(code->GetCode().size());
    }

    tt::Decompiler dc(code, tt::VM::Instance()->GetOpFields());
    size_t hash = dc.Hash(begin, end);

    std::string s_hash = std::to_string(hash);
    ves_set_lstring(0, s_hash.c_str(), s_hash.length());
}

void w_CodeRegen_optimize()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(1))->obj;
    auto new_code = code->GetOptimizer()->RmDupCodes(code);

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("vm", "Bytecodes");
    auto proxy = (tt::Proxy<tt::Bytecodes>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<tt::Bytecodes>));
    proxy->obj = new_code;
    ves_pop(1);
}

void w_CodeRegen_write_num()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(1))->obj;
    int pos = (int)ves_tonumber(2);
    float f = (float)ves_tonumber(3);

    // todo: write outside
    if (!code->GetOptimizer()->WriteNumber(pos, f)) 
    {
        code->SetCurrPos(pos);
        code->Write(reinterpret_cast<const char*>(&f), sizeof(float));
    }
}

void w_CodeRegen_flush()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(1))->obj;
    code->GetOptimizer()->FlushCache();
}

void w_Compiler_allocate()
{
    auto proxy = (tt::Proxy<tt::Compiler>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<tt::Compiler>));
    proxy->obj = std::make_shared<tt::Compiler>();
}

int w_Compiler_finalize(void* data)
{
    auto proxy = (tt::Proxy<tt::Compiler>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<tt::Compiler>);
}

void w_Compiler_new_reg()
{
    auto c = ((tt::Proxy<tt::Compiler>*)ves_toforeign(0))->obj;
    int reg = c->NewRegister();
    if (reg >= 0) {

        //if (reg == 10)
        //{
        //    ves_traceback();
        //    printf("++++++++++++++++++++++++\n");
        //}

        ves_set_number(0, reg);
    } else {
        ves_set_nil(0);
    }
}

void w_Compiler_free_reg()
{
    auto c = ((tt::Proxy<tt::Compiler>*)ves_toforeign(0))->obj;
    int reg = (int)ves_tonumber(1);
    if (reg >= 0) {
        c->FreeRegister(reg);
    }
}

void w_Compiler_keep_reg()
{
    auto c = ((tt::Proxy<tt::Compiler>*)ves_toforeign(0))->obj;
    int reg = (int)ves_tonumber(1);
    bool keep = ves_toboolean(2);
    if (reg >= 0) {
        c->SetRegKeep(reg, keep);
    }
}

void w_Compiler_expect_reg_free()
{
    auto c = ((tt::Proxy<tt::Compiler>*)ves_toforeign(0))->obj;
    c->ExpectRegFree();
}

void w_VM_allocate()
{
    auto proxy = (tt::Proxy<evm::VM>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<evm::VM>));
    auto b_code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(1))->obj;
    auto& code = b_code->GetCode();
    proxy->obj = tt::VM::Instance()->CreateVM(code);
}

int w_VM_finalize(void* data)
{
    auto proxy = (tt::Proxy<evm::VM>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<evm::VM>);
}

void w_VM_run()
{
    auto vm = ((tt::Proxy<evm::VM>*)ves_toforeign(0))->obj;
    vm->Run();
}

void w_VM_load_boolean()
{
    auto vm = ((tt::Proxy<evm::VM>*)ves_toforeign(0))->obj;
    uint8_t reg = (uint8_t)ves_tonumber(1);
    if (reg == 0xff) {
        ves_set_nil(0);
    } else {
        ves_set_boolean(0, evm::VMHelper::GetRegBool(vm.get(), reg));
    }
}

void w_VM_load_number()
{
    auto vm = ((tt::Proxy<evm::VM>*)ves_toforeign(0))->obj;
    uint8_t reg = (uint8_t)ves_tonumber(1);
    if (reg == 0xff) {
        ves_set_nil(0);
    } else {
        ves_set_number(0, evm::VMHelper::GetRegNumber(vm.get(), reg));
    }
}

void w_VM_load_string()
{
    auto vm = ((tt::Proxy<evm::VM>*)ves_toforeign(0))->obj;
    uint8_t reg = (uint8_t)ves_tonumber(1);
    if (reg == 0xff) {
        ves_set_nil(0);
    } else {
        auto str = evm::VMHelper::GetRegString(vm.get(), reg);
        ves_set_lstring(0, str, strlen(str));
    }
}

void w_VM_load_plane()
{
    auto vm = ((tt::Proxy<evm::VM>*)ves_toforeign(0))->obj;

    uint8_t reg = (uint8_t)ves_tonumber(1);
    if (reg == 0xff) {
        ves_set_nil(0);
        return;
    }

    auto& val = vm->GetRegister(reg);
    if (val.type != tt::ValueType::V_PLANE) {
        ves_set_nil(0);
        return;
    }

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("maths", "Plane");
    auto proxy = (tt::Proxy<sm::Plane>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<sm::Plane>));
    proxy->obj = evm::VMHelper::GetHandleValue<sm::Plane>(val);
    ves_pop(1);
}

void w_VM_load_polyface()
{
    auto vm = ((tt::Proxy<evm::VM>*)ves_toforeign(0))->obj;

    uint8_t reg = (uint8_t)ves_tonumber(1);
    if (reg == 0xff) {
        ves_set_nil(0);
        return;
    }

    auto& val = vm->GetRegister(reg);
    if (val.type != tt::ValueType::V_POLY_FACE) {
        ves_set_nil(0);
        return;
    }

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("geometry", "PolyFace");
    auto proxy = (tt::Proxy<pm3::Polytope::Face>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<pm3::Polytope::Face>));
    proxy->obj = evm::VMHelper::GetHandleValue<pm3::Polytope::Face>(val);
    ves_pop(1);
}

void w_VM_load_polytope()
{
    auto vm = ((tt::Proxy<evm::VM>*)ves_toforeign(0))->obj;
    uint8_t reg = (uint8_t)ves_tonumber(1);

    auto polys = tt::VMHelper::LoadPolys(vm.get(), reg);
    if (polys.empty())
    {
        ves_set_nil(0);
    }
    else if (polys.size() == 1)
    {
        ves_pop(ves_argnum());

        ves_pushnil();
        ves_import_class("geometry", "Polytope");
        auto proxy = (tt::Proxy<pm3::Polytope>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<pm3::Polytope>));
        proxy->obj = polys[0];
        ves_pop(1);
    }
    else
    {
        ves_pop(ves_argnum());

        const int num = (int)polys.size();
        ves_newlist(num);
        for (int i = 0; i < num; ++i)
        {
            ves_pushnil();
            ves_import_class("geometry", "Polytope");
            auto proxy = (tt::Proxy<pm3::Polytope>*)ves_set_newforeign(1, 2, sizeof(tt::Proxy<pm3::Polytope>));
            proxy->obj = polys[i];
            ves_pop(1);
            ves_seti(-2, i);
            ves_pop(1);
        }
    }
}

}

namespace tt
{

VesselForeignMethodFn VmBindMethod(const char* signature)
{
    // base
    if (strcmp(signature, "Bytecodes.store_bool(_,_)") == 0) return w_Bytecodes_store_bool;
    if (strcmp(signature, "Bytecodes.store_num(_,_)") == 0) return w_Bytecodes_store_num;
    if (strcmp(signature, "Bytecodes.print_num(_)") == 0) return w_Bytecodes_print_num;
    if (strcmp(signature, "Bytecodes.negate_num(_,_)") == 0) return w_Bytecodes_negate_num;
    if (strcmp(signature, "Bytecodes.add_num(_,_,_)") == 0) return w_Bytecodes_add_num;
    if (strcmp(signature, "Bytecodes.sub_num(_,_,_)") == 0) return w_Bytecodes_sub_num;
    if (strcmp(signature, "Bytecodes.mul_num(_,_,_)") == 0) return w_Bytecodes_mul_num;
    if (strcmp(signature, "Bytecodes.div_num(_,_,_)") == 0) return w_Bytecodes_div_num;
    if (strcmp(signature, "Bytecodes.inc_num(_)") == 0) return w_Bytecodes_inc_num;
    if (strcmp(signature, "Bytecodes.dec_num(_)") == 0) return w_Bytecodes_dec_num;
    if (strcmp(signature, "Bytecodes.cmp_num(_,_,_)") == 0) return w_Bytecodes_cmp_num;
    if (strcmp(signature, "Bytecodes.jump_if_not(_,_)") == 0) return w_Bytecodes_jump_if_not;
    // math
    if (strcmp(signature, "Bytecodes.vec2_create_i(_,_,_)") == 0) return w_Bytecodes_vec2_create_i;
    if (strcmp(signature, "Bytecodes.vec3_create_r(_,_,_,_)") == 0) return w_Bytecodes_vec3_create_r;
    if (strcmp(signature, "Bytecodes.vec3_create_i(_,_,_,_)") == 0) return w_Bytecodes_vec3_create_i;
    if (strcmp(signature, "Bytecodes.vec3_print(_)") == 0) return w_Bytecodes_vec3_print;
    if (strcmp(signature, "Bytecodes.vec3_add(_,_,_)") == 0) return w_Bytecodes_vec3_add;
    if (strcmp(signature, "Bytecodes.vec3_sub(_,_,_)") == 0) return w_Bytecodes_vec3_sub;
    if (strcmp(signature, "Bytecodes.vec3_transform(_,_)") == 0) return w_Bytecodes_vec3_transform;
    if (strcmp(signature, "Bytecodes.vec3_get_x(_,_)") == 0) return w_Bytecodes_vec3_get_x;
    if (strcmp(signature, "Bytecodes.vec3_get_y(_,_)") == 0) return w_Bytecodes_vec3_get_y;
    if (strcmp(signature, "Bytecodes.vec3_get_z(_,_)") == 0) return w_Bytecodes_vec3_get_z;
    if (strcmp(signature, "Bytecodes.vec4_create_i(_,_,_,_,_)") == 0) return w_Bytecodes_vec4_create_i;
    if (strcmp(signature, "Bytecodes.create_mat4(_)") == 0) return w_Bytecodes_create_mat4;
    if (strcmp(signature, "Bytecodes.mat4_rotate(_,_)") == 0) return w_Bytecodes_mat4_rotate;
    if (strcmp(signature, "Bytecodes.mat4_translate(_,_)") == 0) return w_Bytecodes_mat4_translate;
    if (strcmp(signature, "Bytecodes.create_cube(_,_,_)") == 0) return w_Bytecodes_create_cube;
    // stl
    if (strcmp(signature, "Bytecodes.create_vector(_)") == 0) return w_Bytecodes_create_vector;
    if (strcmp(signature, "Bytecodes.vector_add(_,_)") == 0) return w_Bytecodes_vector_add;
    if (strcmp(signature, "Bytecodes.vector_concat(_,_)") == 0) return w_Bytecodes_vector_concat;
    if (strcmp(signature, "Bytecodes.vector_get(_,_,_)") == 0) return w_Bytecodes_vector_get;
    // geo
    if (strcmp(signature, "Bytecodes.create_plane(_,_,_,_)") == 0) return w_Bytecodes_create_plane;
    if (strcmp(signature, "Bytecodes.create_plane_2(_,_,_)") == 0) return w_Bytecodes_create_plane_2;
    if (strcmp(signature, "Bytecodes.create_polyface(_,_)") == 0) return w_Bytecodes_create_polyface;
    if (strcmp(signature, "Bytecodes.create_polytope(_,_)") == 0) return w_Bytecodes_create_polytope;
    if (strcmp(signature, "Bytecodes.create_polyface_2(_,_,_)") == 0) return w_Bytecodes_create_polyface_2;
    if (strcmp(signature, "Bytecodes.create_polytope_2(_,_,_)") == 0) return w_Bytecodes_create_polytope_2;
    if (strcmp(signature, "Bytecodes.polytope_transform(_,_)") == 0) return w_Bytecodes_polytope_transform;
    if (strcmp(signature, "Bytecodes.polytope_subtract(_,_,_)") == 0) return w_Bytecodes_polytope_subtract;
    if (strcmp(signature, "Bytecodes.polytope_extrude(_,_)") == 0) return w_Bytecodes_polytope_extrude;
    if (strcmp(signature, "Bytecodes.polytope_clip(_,_,_,_)") == 0) return w_Bytecodes_polytope_clip;
    if (strcmp(signature, "Bytecodes.polytope_set_dirty(_)") == 0) return w_Bytecodes_polytope_set_dirty;
    if (strcmp(signature, "Bytecodes.polypoint_select(_,_,_)") == 0) return w_Bytecodes_polypoint_select;
    if (strcmp(signature, "Bytecodes.polyface_select(_,_,_,_)") == 0) return w_Bytecodes_polyface_select;
    // multi
    if (strcmp(signature, "Bytecodes.add(_,_,_)") == 0) return w_Bytecodes_add;
    if (strcmp(signature, "Bytecodes.sub(_,_,_)") == 0) return w_Bytecodes_sub;
    if (strcmp(signature, "Bytecodes.mul(_,_,_)") == 0) return w_Bytecodes_mul;
    if (strcmp(signature, "Bytecodes.div(_,_,_)") == 0) return w_Bytecodes_div;
    if (strcmp(signature, "Bytecodes.transform_unknown(_,_)") == 0) return w_Bytecodes_transform_unknown;

    if (strcmp(signature, "static CodeStats.stat_call(_,_)") == 0) return w_CodeStats_stat_call;
    if (strcmp(signature, "static CodeStats.add_code_block(_,_,_,_)") == 0) return w_CodeStats_add_code_block;
    if (strcmp(signature, "static CodeStats.add_cost(_,_)") == 0) return w_CodeStats_add_cost;
    if (strcmp(signature, "static CodeStats.get_cost(_)") == 0) return w_CodeStats_get_cost;

    if (strcmp(signature, "static CodeTools.get_size(_)") == 0) return w_CodeTools_get_size;
    if (strcmp(signature, "static CodeTools.decompiler(_,_,_)") == 0) return w_CodeTools_decompiler;
    if (strcmp(signature, "static CodeTools.hash(_,_,_)") == 0) return w_CodeTools_hash;

    if (strcmp(signature, "static CodeRegen.optimize(_)") == 0) return w_CodeRegen_optimize;
    if (strcmp(signature, "static CodeRegen.write_num(_,_,_)") == 0) return w_CodeRegen_write_num;
    if (strcmp(signature, "static CodeRegen.flush(_)") == 0) return w_CodeRegen_flush;

    if (strcmp(signature, "Compiler.new_reg()") == 0) return w_Compiler_new_reg;
    if (strcmp(signature, "Compiler.free_reg(_)") == 0) return w_Compiler_free_reg;
    if (strcmp(signature, "Compiler.keep_reg(_,_)") == 0) return w_Compiler_keep_reg;
    if (strcmp(signature, "Compiler.expect_reg_free()") == 0) return w_Compiler_expect_reg_free;

    if (strcmp(signature, "VM.run()") == 0) return w_VM_run;

    if (strcmp(signature, "VM.load_boolean(_)") == 0) return w_VM_load_boolean;
    if (strcmp(signature, "VM.load_number(_)") == 0) return w_VM_load_number;
    if (strcmp(signature, "VM.load_string(_)") == 0) return w_VM_load_string;
    // geo
    if (strcmp(signature, "VM.load_plane(_)") == 0) return w_VM_load_plane;
    if (strcmp(signature, "VM.load_polyface(_)") == 0) return w_VM_load_polyface;
    if (strcmp(signature, "VM.load_polytope(_)") == 0) return w_VM_load_polytope;

	return nullptr;
}

void VmBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
    if (strcmp(class_name, "Bytecodes") == 0)
    {
        methods->allocate = w_Bytecodes_allocate;
        methods->finalize = w_Bytecodes_finalize;
        return;
    }

    if (strcmp(class_name, "Compiler") == 0)
    {
        methods->allocate = w_Compiler_allocate;
        methods->finalize = w_Compiler_finalize;
        return;
    }

    if (strcmp(class_name, "VM") == 0)
    {
        methods->allocate = w_VM_allocate;
        methods->finalize = w_VM_finalize;
        return;
    }
}

}