#define QUOTE(...) #__VA_ARGS__
static const char* regenModuleSource = QUOTE(

foreign class PolyDiff
{
	init() {}

	foreign build(input, output)
}

);