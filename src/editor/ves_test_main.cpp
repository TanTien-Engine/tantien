// Headless .ves test harness.
//
// Runs a single .ves test driver with NO window / GL context, and turns its
// result into a process exit code (0 = all asserts passed, nonzero = failures
// or a compile/runtime error). This is the automated way to exercise the
// script layer (topo_sort, reachability, Tree.build, ...) that previously could
// only be run by launching the windowed editor by hand.
//
// It shares the exact module registry + loader the editor uses, via
// ves_host.{h,cpp}, so tests import the REAL modules (blueprint, geograph, vm,
// ...). Foreign classes are bound lazily and never instantiated by these tests,
// so no render/GL backend is touched.
//
// Protocol: every driver module (test/ves/<name>.ves) exposes
//     class Main { static run() { ... ; return <failure_count> } }
// Invoke:  ves_test <name>            (run from the repo root — the loader uses
//                                      relative paths like "src/script/")

#include "ves_host.h"

#include <vessel.h>

#include <cstdio>
#include <string>

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: ves_test <driver-name> [<driver-name> ...]\n"
                             "  drivers resolve to test/ves/<name>.ves; run from the repo root\n");
        return 2;
    }

    ves_init_vm();

    VesselConfiguration cfg;
    tt_ves_host::configure(&cfg);           // module loader + foreign binders + stdout write
    ves_set_config(&cfg);

    // Resolve driver + harness modules out of test/ves/ (after the src/ tree).
    tt_ves_host::search_paths.push_back("test/ves/");

    int total_failures = 0;

    for (int a = 1; a < argc; ++a)
    {
        const char* driver = argv[a];
        std::printf("=== %s ===\n", driver);

        // Bootstrap: import the driver's Main class and stash its failure count
        // in a module global we can read back. Use a unique bootstrap module
        // name per driver so re-declaring _failures across iterations can't clash.
        std::string boot;
        boot += "import \"";
        boot += driver;
        boot += "\" for Main\nvar _failures = Main.run()\n";

        std::string boot_module = "ves_test$";
        boot_module += std::to_string(a);
        VesselInterpretResult r = ves_interpret(boot_module.c_str(), boot.c_str());
        if (r != VES_INTERPRET_OK) {
            std::printf("[%s] %s\n", driver,
                        r == VES_INTERPRET_COMPILE_ERROR ? "COMPILE ERROR" : "RUNTIME ERROR");
            total_failures += 1;
            continue;
        }

        int type = ves_getglobal("_failures");
        int failures = (type == VES_TYPE_NUM) ? (int)ves_tonumber(-1) : 1;
        ves_pop(1);

        if (type != VES_TYPE_NUM) {
            std::printf("[%s] driver did not return a number\n", driver);
        }
        std::printf("[%s] %d failure(s)\n", driver, failures);
        total_failures += failures;
    }

    ves_free_vm();

    std::printf("=== total: %d failure(s) ===\n", total_failures);
    return total_failures == 0 ? 0 : 1;
}
