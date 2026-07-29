#pragma once

// Vessel module host: the single source of truth for loading the statically
// linked .ves script modules and binding their foreign (C++) classes/methods.
//
// This used to live inline in src/editor/main.cpp. It was factored out so the
// windowed `editor` and the headless `ves_test` harness share ONE module
// registry and loader instead of duplicating (and drifting) it. Wire the four
// callbacks below into a VesselConfiguration (or call configure()), then
// ves_interpret/ves_call as usual.

#include <vessel.h>

#include <string>
#include <vector>

namespace tt_ves_host {

// Extra directories consulted by read_module AFTER the compiled-in registry and
// the src/ tree. The editor pushes the loaded editor file's own directory here
// so sibling modules resolve; the test harness pushes the test dir.
extern std::vector<std::string> search_paths;

// --- VesselConfiguration callbacks -------------------------------------------
VesselLoadModuleResult    read_module(const char* module);
VesselExpandModulesResult expand_modules(const char* path);
VesselForeignClassMethods bind_foreign_class(const char* module, const char* className);
VesselForeignMethodFn     bind_foreign_method(const char* module, const char* className,
                                              bool isStatic, const char* signature);

// Default write_fn: forwards VM output to stdout.
void write(const char* text);

// Fills cfg with the four callbacks above + the default stdout write_fn.
// Callers may overwrite cfg.write_fn afterward if they want to capture output.
void configure(VesselConfiguration* cfg);

} // namespace tt_ves_host
