#define QUOTE(...) #__VA_ARGS__
static const char* tasksModuleSource = QUOTE(

// tasks: backend-process and data plumbing for the deparameterization UI
// adapter. Spawns worker processes in their OWN process group so CancelRun
// can retire exactly one task's process tree, and exposes the small file /
// json helpers the adapter needs (no CAD here -- geometry stays behind the
// cadapp / brepkit modules).

foreign class Tasks
{
	// args: list of argv strings, argv[0] = program. log_path: per-run stdout
	// + stderr capture file (nil -> /dev/null). cwd: working directory
	// (nil -> inherit). Returns pid > 0, or 0 when the spawn failed.
	foreign static spawn(args, log_path, cwd)

	// [alive, exit_code]. exit_code is valid only once alive is false
	// (process exit observed by poll; -1 when the status is unknowable,
	// e.g. the pid was never ours or was reaped elsewhere).
	foreign static poll(pid)

	// Signal the task's whole process group (15 = SIGTERM, then 9 = SIGKILL
	// from the script side once the graceful window elapsed). Returns false
	// when the group no longer exists.
	foreign static kill_tree(pid, signal_num)

	foreign static mkdir_p(path)
	// Lowercase hex digest of the file at path, or nil when unreadable.
	foreign static sha256(path)
	foreign static now_ms()
}

foreign class Json
{
	// Parse one JSON document into Vessel maps / lists / num / bool / nil.
	// Returns nil on any parse error -- callers treat that as "unparseable",
	// never as an empty result.
	foreign static parse(text)
}

);

