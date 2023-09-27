#include "wrap_VM.h"
#include "Bytecodes.h"
#include "modules/script/TransHelper.h"
#include "modules/vm/Bytecodes.h"
#include "modules/vm/Compiler.h"
#include "modules/vm/VM.h"
#include "modules/vm/math_opcodes.h"
#include "modules/vm/geo_opcodes.h"
#include "modules/vm/stl_opcodes.h"

#include <SM_Plane.h>
#include <polymesh3/Polytope.h>
#include <easyvm/VM.h>
#include <easyvm/OpCodes.h>

#include <string>

namespace
{

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

void w_Bytecodes_set_ret_reg()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(0))->obj;
    uint8_t reg = (uint8_t)ves_tonumber(1);
    code->SetRetReg(reg);
}

void w_Bytecodes_get_ret_reg()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(0))->obj;
    ves_set_number(0, code->GetRetReg());
}

void w_Bytecodes_store_num()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(0))->obj;

    uint8_t op = evm::OP_NUMBER_STORE;
    code->Write(reinterpret_cast<const char*>(&op), sizeof(uint8_t));

    uint8_t reg = (uint8_t)ves_tonumber(1);
    code->Write(reinterpret_cast<const char*>(&reg), sizeof(uint8_t));

    double num = ves_tonumber(2);
    code->Write(reinterpret_cast<const char*>(&num), sizeof(double));
}

void w_Bytecodes_print_num()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(0))->obj;

    uint8_t op = evm::OP_NUMBER_PRINT;
    code->Write(reinterpret_cast<const char*>(&op), sizeof(uint8_t));

    uint8_t reg = (uint8_t)ves_tonumber(1);
    code->Write(reinterpret_cast<const char*>(&reg), sizeof(uint8_t));
}

void w_Bytecodes_add()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(0))->obj;

    uint8_t op = evm::OP_ADD;
    code->Write(reinterpret_cast<const char*>(&op), sizeof(uint8_t));

    uint8_t reg_dst = (uint8_t)ves_tonumber(1);
    code->Write(reinterpret_cast<const char*>(&reg_dst), sizeof(uint8_t));

    uint8_t reg_src1 = (uint8_t)ves_tonumber(2);
    code->Write(reinterpret_cast<const char*>(&reg_src1), sizeof(uint8_t));

    uint8_t reg_src2 = (uint8_t)ves_tonumber(3);
    code->Write(reinterpret_cast<const char*>(&reg_src2), sizeof(uint8_t));
}

void w_Bytecodes_store_vec3()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(0))->obj;

    uint8_t op = tt::OP_VEC3_STORE;
    code->Write(reinterpret_cast<const char*>(&op), sizeof(uint8_t));

    uint8_t reg = (uint8_t)ves_tonumber(1);
    code->Write(reinterpret_cast<const char*>(&reg), sizeof(uint8_t));

    auto pos = tt::list_to_vec3(2);
    code->Write(reinterpret_cast<const char*>(&pos.x), sizeof(float));
    code->Write(reinterpret_cast<const char*>(&pos.y), sizeof(float));
    code->Write(reinterpret_cast<const char*>(&pos.z), sizeof(float));
}

void w_Bytecodes_print_vec3()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(0))->obj;

    uint8_t op = tt::OP_VEC3_PRINT;
    code->Write(reinterpret_cast<const char*>(&op), sizeof(uint8_t));

    uint8_t reg = (uint8_t)ves_tonumber(1);
    code->Write(reinterpret_cast<const char*>(&reg), sizeof(uint8_t));
}

void w_Bytecodes_create_vector()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(0))->obj;

    uint8_t op = tt::StlOpCode::OP_VECTOR_CREATE;
    code->Write(reinterpret_cast<const char*>(&op), sizeof(uint8_t));

    uint8_t reg = (uint8_t)ves_tonumber(1);
    code->Write(reinterpret_cast<const char*>(&reg), sizeof(uint8_t));
}

void w_Bytecodes_add_vector()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(0))->obj;

    uint8_t op = tt::StlOpCode::OP_VECTOR_ADD;
    code->Write(reinterpret_cast<const char*>(&op), sizeof(uint8_t));

    uint8_t dst_reg = (uint8_t)ves_tonumber(1);
    code->Write(reinterpret_cast<const char*>(&dst_reg), sizeof(uint8_t));

    uint8_t src_reg = (uint8_t)ves_tonumber(2);
    code->Write(reinterpret_cast<const char*>(&src_reg), sizeof(uint8_t));
}

void w_Bytecodes_create_plane()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(0))->obj;

    uint8_t op = tt::OP_CREATE_PLANE;
    code->Write(reinterpret_cast<const char*>(&op), sizeof(uint8_t));

    uint8_t reg_dst = (uint8_t)ves_tonumber(1);
    code->Write(reinterpret_cast<const char*>(&reg_dst), sizeof(uint8_t));

    uint8_t reg_p0 = (uint8_t)ves_tonumber(2);
    code->Write(reinterpret_cast<const char*>(&reg_p0), sizeof(uint8_t));
    uint8_t reg_p1 = (uint8_t)ves_tonumber(3);
    code->Write(reinterpret_cast<const char*>(&reg_p1), sizeof(uint8_t));
    uint8_t reg_p2 = (uint8_t)ves_tonumber(4);
    code->Write(reinterpret_cast<const char*>(&reg_p2), sizeof(uint8_t));
}

void w_Bytecodes_create_polyface()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(0))->obj;

    uint8_t op = tt::OP_CREATE_POLYFACE;
    code->Write(reinterpret_cast<const char*>(&op), sizeof(uint8_t));

    uint8_t reg_dst = (uint8_t)ves_tonumber(1);
    code->Write(reinterpret_cast<const char*>(&reg_dst), sizeof(uint8_t));

    uint8_t reg_plane = (uint8_t)ves_tonumber(2);
    code->Write(reinterpret_cast<const char*>(&reg_plane), sizeof(uint8_t));
}

void w_Bytecodes_create_polytope()
{
    auto code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(0))->obj;

    uint8_t op = tt::OP_CREATE_POLYTOPE;
    code->Write(reinterpret_cast<const char*>(&op), sizeof(uint8_t));

    uint8_t reg_dst = (uint8_t)ves_tonumber(1);
    code->Write(reinterpret_cast<const char*>(&reg_dst), sizeof(uint8_t));

    uint8_t reg_faces = (uint8_t)ves_tonumber(2);
    code->Write(reinterpret_cast<const char*>(&reg_faces), sizeof(uint8_t));
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
        ves_set_number(0, reg);
    } else {
        ves_set_nil(0);
    }
}

void w_Compiler_free_reg()
{
    auto c = ((tt::Proxy<tt::Compiler>*)ves_toforeign(0))->obj;
    uint8_t reg = (uint8_t)ves_tonumber(1);
    c->FreeRegister(reg);
}

void w_VM_allocate()
{
    auto proxy = (tt::Proxy<evm::VM>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<evm::VM>));
    auto b_code = ((tt::Proxy<tt::Bytecodes>*)ves_toforeign(1))->obj;
    auto& code = b_code->GetCode();
    proxy->obj = std::make_shared<evm::VM>((const char*)code.data(), code.size());
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

    tt::InitVM(vm);

    vm->Run();
}

void w_VM_load_boolean()
{
    auto vm = ((tt::Proxy<evm::VM>*)ves_toforeign(0))->obj;
    uint8_t reg = (uint8_t)ves_tonumber(1);

    evm::Value val;
    if (vm->GetRegister(reg, val) && val.type == evm::ValueType::BOOLEAN) {
        ves_set_boolean(0, val.as.boolean);
    } else {
        ves_set_nil(0);
    }
}

void w_VM_load_number()
{
    auto vm = ((tt::Proxy<evm::VM>*)ves_toforeign(0))->obj;
    uint8_t reg = (uint8_t)ves_tonumber(1);

    evm::Value val;
    if (vm->GetRegister(reg, val) && val.type == evm::ValueType::NUMBER) {
        ves_set_number(0, val.as.number);
    } else {
        ves_set_nil(0);
    }
}

void w_VM_load_string()
{
    auto vm = ((tt::Proxy<evm::VM>*)ves_toforeign(0))->obj;
    uint8_t reg = (uint8_t)ves_tonumber(1);

    evm::Value val;
    if (vm->GetRegister(reg, val) && val.type == evm::ValueType::STRING) {
        ves_set_lstring(0, val.as.string, strlen(val.as.string));
    } else {
        ves_set_nil(0);
    }
}

void w_VM_load_plane()
{
    auto vm = ((tt::Proxy<evm::VM>*)ves_toforeign(0))->obj;
    uint8_t reg = (uint8_t)ves_tonumber(1);

    evm::Value val;
    if (!vm->GetRegister(reg, val) || val.type != evm::ValueType::HANDLE) {
        ves_set_nil(0);
        return;
    }

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("maths", "Plane");
    auto proxy = (tt::Proxy<sm::Plane>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<sm::Plane>));
    proxy->obj = static_cast<evm::Handle<sm::Plane>*>(val.as.handle)->obj;
    ves_pop(1);
}

void w_VM_load_polyface()
{
    auto vm = ((tt::Proxy<evm::VM>*)ves_toforeign(0))->obj;
    uint8_t reg = (uint8_t)ves_tonumber(1);

    evm::Value val;
    if (!vm->GetRegister(reg, val) || val.type != evm::ValueType::HANDLE) {
        ves_set_nil(0);
        return;
    }

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("geometry", "PolyFace");
    auto proxy = (tt::Proxy<pm3::Polytope::Face>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<pm3::Polytope::Face>));
    proxy->obj = static_cast<evm::Handle<pm3::Polytope::Face>*>(val.as.handle)->obj;
    ves_pop(1);
}

void w_VM_load_polytope()
{
    auto vm = ((tt::Proxy<evm::VM>*)ves_toforeign(0))->obj;
    uint8_t reg = (uint8_t)ves_tonumber(1);

    evm::Value val;
    if (!vm->GetRegister(reg, val) || val.type != evm::ValueType::HANDLE) {
        ves_set_nil(0);
        return;
    }

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("geometry", "Polytope");
    auto proxy = (tt::Proxy<pm3::Polytope>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<pm3::Polytope>));
    proxy->obj = static_cast<evm::Handle<pm3::Polytope>*>(val.as.handle)->obj;
    ves_pop(1);
}

}

namespace tt
{

VesselForeignMethodFn VmBindMethod(const char* signature)
{
    // base
    if (strcmp(signature, "Bytecodes.set_ret_reg(_)") == 0) return w_Bytecodes_set_ret_reg;
    if (strcmp(signature, "Bytecodes.get_ret_reg()") == 0) return w_Bytecodes_get_ret_reg;
    if (strcmp(signature, "Bytecodes.store_num(_,_)") == 0) return w_Bytecodes_store_num;
    if (strcmp(signature, "Bytecodes.print_num(_)") == 0) return w_Bytecodes_print_num;
    if (strcmp(signature, "Bytecodes.add(_,_,_)") == 0) return w_Bytecodes_add;
    // math
    if (strcmp(signature, "Bytecodes.store_vec3(_,_)") == 0) return w_Bytecodes_store_vec3;
    if (strcmp(signature, "Bytecodes.print_vec3(_)") == 0) return w_Bytecodes_print_vec3;
    // stl
    if (strcmp(signature, "Bytecodes.create_vector(_)") == 0) return w_Bytecodes_create_vector;
    if (strcmp(signature, "Bytecodes.add_vector(_,_)") == 0) return w_Bytecodes_add_vector;
    // geo
    if (strcmp(signature, "Bytecodes.create_plane(_,_,_,_)") == 0) return w_Bytecodes_create_plane;
    if (strcmp(signature, "Bytecodes.create_polyface(_,_)") == 0) return w_Bytecodes_create_polyface;
    if (strcmp(signature, "Bytecodes.create_polytope(_,_)") == 0) return w_Bytecodes_create_polytope;

    if (strcmp(signature, "Compiler.new_reg()") == 0) return w_Compiler_new_reg;
    if (strcmp(signature, "Compiler.free_reg(_)") == 0) return w_Compiler_free_reg;

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