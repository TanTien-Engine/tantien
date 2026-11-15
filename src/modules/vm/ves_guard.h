#pragma once

// Argument-slot foreign null-guards for the wrap_*.cpp binding layer.
//
// ves_toforeign(idx) returns NULL when slot `idx` holds nil or a non-foreign --
// e.g. a script calls a method with a wrong-typed / missing argument. The wrap
// functions then dereference that NULL (directly, or via wrapper::Proxy<T>::obj)
// and crash the whole editor. These macros null-check the ARGUMENT slot first
// and bail out of the (void) wrap function with a nil result instead: nil is
// falsy on the script side, so a caller bug fails safe rather than taking down
// the process.
//
// Scope: ARGUMENT slots only (idx >= 1). Receiver slot 0 is guaranteed to be the
// right foreign type by the VM's method dispatch, so it is never guarded here.
// Allocator (w_*_allocate) optional-argument reads are NOT a fit for these
// early-return macros -- guard those inline with `if (void* p = ves_toforeign(i))`
// so the object still gets constructed.
//
// TT_ARG_OBJ expands wrapper::Proxy<T>, so the including TU must already pull in
// <wrapper/Proxy.h> (every wrap_*.cpp that uses it already does).

#include <vessel.h>

// Typed value-pointer foreign arg (e.g. sm::Plane*, sm::mat4*).
#define TT_ARG_PTR(T, var, idx)                       \
    T* var = (T*)ves_toforeign(idx);                  \
    if (!var) { ves_set_nil(0); return; }

// wrapper::Proxy<T>-wrapped object arg -> its ->obj shared_ptr.
#define TT_ARG_OBJ(T, var, idx)                       \
    void* var##_f_ = ves_toforeign(idx);              \
    if (!var##_f_) { ves_set_nil(0); return; }        \
    auto var = ((wrapper::Proxy<T>*)var##_f_)->obj

// Foreign block holding a bare T* (the `*(T**)ves_toforeign(idx)` idiom, e.g.
// the spvgentwo shader objects). Guards the block pointer, not the T* it holds.
#define TT_ARG_PP(T, var, idx)                        \
    void* var##_f_ = ves_toforeign(idx);              \
    if (!var##_f_) { ves_set_nil(0); return; }        \
    T* var = *(T**)var##_f_
