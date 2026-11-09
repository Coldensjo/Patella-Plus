r"""Configure and build a CMake preset in one command, from any shell (issue #10).

Usage:
  python -I tools/build.py <workflow preset> [cmake --workflow options, e.g. --fresh]
  python -I tools/build.py --env <preset> <command> [arguments ...]
  python -I tools/build.py --list
  python -I tools/build.py --install-cmake <folder>

The first form runs `cmake --workflow --preset <workflow preset>` (CMakePresets.json: windows-x64,
windows-x64-debug, linux-x64-gcc, linux-x64-gcc-debug, package-windows-x64, package-linux-x64): configure
and build (the package presets also package). The second runs any command (cmake --build, ...) in the
compiler environment of <preset> (a configure, build or workflow preset), e.g.
  python -I tools/build.py --env windows-x64 cmake --build --preset windows-x64-debug

The MSVC presets mark their architecture and toolset `external`: CMake does not set up the compiler
for them. This script does what a developer prompt does: it runs Visual Studio's vcvarsall.bat with
the architecture of the preset (x64 for windows-x64; found through vswhere, the
newest Visual Studio with the C++ tools) and runs CMake in the environment it sets. A shell that is
already a developer prompt for that architecture is used as it is. The other presets (the Linux presets
on Linux) run CMake in the shell's own environment.

Docker (issue #7): on Windows, a preset whose condition asks for a Linux host (linux-x64-gcc,
package-linux-x64) runs in the image patella-modern/linux-x64:gcc16.2-clang23.1-libstdcxx16-cmake4.4.5-ninja1.13.2-sdl3-mesa25.0.7
(PATELLA_DOCKER_IMAGE overrides it; `docker build -t patella-modern/linux-x64:gcc16.2-clang23.1-libstdcxx16-cmake4.4.5-ninja1.13.2-sdl3-mesa25.0.7
docker/linux-x64` builds
it), in both forms. The repository is mounted read-only at /src; the files git knows (tracked, and
untracked ones that are not ignored: never Tibia.exe or the data files) are copied with their times
to /work, on the container's own case-sensitive file system, and the command runs there.
build/<preset> is mounted at /work/build/<preset>, so the build tree and the downloads stay between runs.

Warnings (issue #17): every cmake run (the first form, and the second when the command is cmake) is
checked for compiler, resource compiler, assembler and linker warnings (MSVC's "warning C4101",
"warning LNK4075", "warning RC4005", "warning A4018", "Command line warning D9025" ...; GCC's and Clang's
"file:line:col: warning:"). The output is passed through unchanged. A warning makes the script exit 1 after
the command ends (when the command itself succeeded), with the warnings listed again. The product presets
build with /WX or -Werror already. Ninja recompiles only what changed, so a build that is up to date shows no
warnings: --fresh checks every file.

CMake (issue #141): one version everywhere, 4.4.5. On Windows the script runs cmake, ctest and cpack from the
pinned portable CMake, Kitware's windows-x86_64 release in C:\Servers\Software\cmake (PATELLA_CMAKE_DIR overrides
the folder; hosted CI unpacks the same zip with --install-cmake and sets it), never the first cmake on PATH:
the machine PATH (C:\Program Files\CMake) comes before the user PATH, and vcvarsall.bat puts Visual Studio's own
CMake first. It refuses a missing or other version with a message. The image (docker/linux-x64) has the same version;
on Linux the command's cmake is the one on PATH. `--install-cmake <folder>` downloads that zip, checks it against
CMAKE_SHA256 (the release's cmake-4.4.5-SHA-256.txt) and unpacks it into <folder>, which must not exist. A timeout or
a 5xx is retried with a pause, and when GitHub keeps failing the same zip comes from Kitware's own site, under the
same check (issue #153: download_verified).

Exit code: the command's, or 1 when it succeeded but warned.
"""

import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PRESETS = os.path.join(ROOT, "CMakePresets.json")
VCVARSALL = r"VC\Auxiliary\Build\vcvarsall.bat"
# where the .cmd scripts found it before issue #10; vswhere's answer comes first
DEFAULT_VS = r"C:\Program Files\Microsoft Visual Studio\18\Community"
# The Linux build image (docker/linux-x64/Dockerfile).
DOCKER_IMAGE = "patella-modern/linux-x64:gcc16.2-clang23.1-libstdcxx16-cmake4.4.5-ninja1.13.2-sdl3-mesa25.0.7"
DOCKER_DIR = "docker/linux-x64"
# The pinned CMake of the Windows builds (issue #141; the module comment).
CMAKE_VERSION = "4.4.5"
CMAKE_DEFAULT_DIR = r"C:\Servers\Software\cmake"
CMAKE_TOOLS = ("cmake", "ctest", "cpack")
CMAKE_ARCHIVE = "https://github.com/Kitware/CMake/releases/download/v%s/cmake-%s-windows-x86_64.zip" % (
	CMAKE_VERSION, CMAKE_VERSION)
# Kitware's own site, the fallback when GitHub keeps failing (issue #153); the same file, the same SHA-256.
CMAKE_ARCHIVES = (CMAKE_ARCHIVE, "https://cmake.org/files/v%s/cmake-%s-windows-x86_64.zip" % (
	CMAKE_VERSION.rpartition(".")[0], CMAKE_VERSION))
# cmake-4.4.5-SHA-256.txt of the release: cmake-4.4.5-windows-x86_64.zip
CMAKE_SHA256 = "326c826486d46a23cf9d5a1c9ee3a567d427bdd02c4a34071937a2a675cff004"
# download_verified (issue #153), the same numbers as cmake/Download.cmake and docker/linux-x64/fetch.sh.
DOWNLOAD_TRIES = 10
DOWNLOAD_FIRST_TRIES = 4
DOWNLOAD_PAUSE = 10
DOWNLOAD_INACTIVITY = 60


# A warning of a tool of the build: MSVC's cl, link, lib, rc, cvtres and ml print "warning <code>" with a
# letter prefix and four or five digits (cl's command line warnings "Command line warning D9025");
# GCC and Clang "<file>:<line>[:<column>]: warning:".
WARNING = re.compile(r"\bwarning\s+[A-Z]{1,3}\d{4,5}\b|:\d+(?::\d+)?: warning: ")


class BuildError(Exception):
	pass


def load_presets():
	with open(PRESETS, "r", encoding="utf-8") as f:
		return json.load(f)


def configure_preset_of(presets, name):
	"""The configure preset behind `name`, a configure, build or workflow preset."""
	for kind in ("configurePresets", "buildPresets"):
		for preset in presets.get(kind, []):
			if preset["name"] == name:
				return name if kind == "configurePresets" else preset["configurePreset"]
	for preset in presets.get("workflowPresets", []):
		if preset["name"] == name:
			for step in preset["steps"]:
				if step["type"] == "configure":
					return step["name"]
	raise BuildError("no preset %r in CMakePresets.json" % name)


def resolved(presets, name):
	"""The configure preset `name` with what it inherits (the first parent wins, as in CMake)."""
	by_name = {p["name"]: p for p in presets.get("configurePresets", [])}
	if name not in by_name:
		raise BuildError("no configure preset %r in CMakePresets.json" % name)
	preset = by_name[name]
	result = {}
	parents = preset.get("inherits", [])
	if isinstance(parents, str):
		parents = [parents]
	for parent in reversed(parents):
		merged = resolved(presets, parent)
		cache = dict(result.get("cacheVariables", {}))
		cache.update(merged.get("cacheVariables", {}))
		result.update(merged)
		result["cacheVariables"] = cache
	cache = dict(result.get("cacheVariables", {}))
	cache.update(preset.get("cacheVariables", {}))
	result.update(preset)
	result["cacheVariables"] = cache
	return result


def value(field):
	if isinstance(field, dict):
		return field.get("value"), field.get("strategy", "set")
	return field, "set"


def vcvars_argument(preset):
	"""vcvarsall.bat's architecture argument for an MSVC preset with an external architecture, or
	None when the preset needs no developer environment."""
	compiler = preset.get("cacheVariables", {}).get("CMAKE_CXX_COMPILER", "")
	if isinstance(compiler, dict):
		compiler = compiler.get("value", "")
	if os.path.basename(str(compiler)).lower() not in ("cl", "cl.exe"):
		return None
	target, strategy = value(preset.get("architecture"))
	if not target or strategy != "external":
		return None
	toolset, _ = value(preset.get("toolset"))
	host = target
	for part in str(toolset or "").split(","):
		key, _, setting = part.partition("=")
		if key.strip() == "host" and setting.strip():
			host = setting.strip()
	target = target.lower()
	host = host.lower()
	return target if host == target else "%s_%s" % (host, target)


def find_vcvarsall():
	installer = os.path.join(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"),
		"Microsoft Visual Studio", "Installer")
	vswhere = os.path.join(installer, "vswhere.exe")
	candidates = []
	if os.path.isfile(vswhere):
		result = subprocess.run([vswhere, "-latest", "-products", "*", "-requires",
			"Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"],
			capture_output=True, text=True)
		candidates += [line.strip() for line in result.stdout.splitlines() if line.strip()]
	candidates.append(DEFAULT_VS)
	for folder in candidates:
		path = os.path.join(folder, VCVARSALL)
		if os.path.isfile(path):
			return path, installer
	raise BuildError("vcvarsall.bat not found (Visual Studio with the C++ workload; tried %s)"
		% ", ".join(candidates))


def developer_environment(argument):
	"""os.environ as vcvarsall.bat <argument> leaves it."""
	# a developer prompt for the same target and host already
	target = argument.split("_")[-1]
	host = argument.split("_")[0]
	if (os.environ.get("VSCMD_ARG_TGT_ARCH", "").lower() == target
			and os.environ.get("VSCMD_ARG_HOST_ARCH", "").lower() == host):
		return dict(os.environ)
	vcvarsall, installer = find_vcvarsall()
	env = dict(os.environ)
	# vcvarsall.bat finds vswhere.exe on PATH
	env["PATH"] = env.get("PATH", "") + os.pathsep + installer
	comspec = os.path.join(os.environ.get("SystemRoot", r"C:\Windows"), "System32", "cmd.exe")
	dump = 'import json, os; print(json.dumps(dict(os.environ)))'
	command = '"%s" /d /s /c ""%s" %s >nul 2>&1 && "%s" -I -c "%s""' % (comspec, vcvarsall, argument,
		sys.executable, dump)
	result = subprocess.run(command, env=env, capture_output=True, text=True, encoding="utf-8")
	if result.returncode != 0 or not result.stdout.strip():
		raise BuildError("vcvarsall.bat %s failed (exit %d): %s" % (argument, result.returncode,
			result.stderr.strip()))
	env = json.loads(result.stdout.strip().splitlines()[-1])
	if not shutil.which("cl", path=env.get("PATH") or env.get("Path")):
		raise BuildError("cl.exe is not on PATH after vcvarsall.bat %s" % argument)
	return env


_checked_cmake = set()


def pinned_cmake(command):
	"""`command` with a bare cmake, ctest or cpack replaced by the pinned CMake's executable (issue #141), on Windows;
	unchanged elsewhere, or when the command names another program or a path. Raises BuildError when the pinned CMake
	is missing or reports another version."""
	tool = os.path.splitext(command[0])[0].lower()
	if os.name != "nt" or tool not in CMAKE_TOOLS or os.path.dirname(command[0]):
		return command
	folder = os.environ.get("PATELLA_CMAKE_DIR") or CMAKE_DEFAULT_DIR
	executable = os.path.join(folder, "bin", tool + ".exe")
	if not os.path.isfile(executable):
		raise BuildError("the pinned CMake %s is missing: no %s. Install it: python -I tools/build.py --install-cmake "
			"%s (Kitware's %s, its SHA-256 checked), and add its bin folder to the user PATH; or set PATELLA_CMAKE_DIR "
			"(issue #141)" % (CMAKE_VERSION, executable, folder, CMAKE_ARCHIVE))
	if executable not in _checked_cmake:
		result = subprocess.run([executable, "--version"], capture_output=True, text=True)
		first = (result.stdout.splitlines() or [""])[0].strip()
		if result.returncode != 0 or first != "%s version %s" % (tool, CMAKE_VERSION):
			raise BuildError("%s reports %r, not %s version %s (issue #141: one CMake version everywhere)"
				% (executable, first, tool, CMAKE_VERSION))
		_checked_cmake.add(executable)
	return [executable] + list(command[1:])


def fetch_once(url, path):
	"""One attempt: `url` to `path`. Returns None, or the reason when the failure is one to retry (a timeout, HTTP 408,
	429 or 5xx, a refused or dropped connection, an unfinished transfer). Raises BuildError on any other failure."""
	import http.client
	import socket
	import ssl
	import urllib.error
	import urllib.request
	transient = (TimeoutError, ConnectionError, http.client.HTTPException, socket.gaierror, ssl.SSLError)
	try:
		with urllib.request.urlopen(url, timeout=DOWNLOAD_INACTIVITY) as response, open(path, "wb") as out:
			shutil.copyfileobj(response, out)
			# An early close ends the copy without an error when the body was read in blocks.
			if response.length:
				raise http.client.IncompleteRead(b"", response.length)
	except urllib.error.HTTPError as e:
		if e.code in (408, 429) or 500 <= e.code < 600:
			return "HTTP %d" % e.code
		raise BuildError("%s: HTTP %d; not retried" % (url, e.code))
	except (urllib.error.URLError, *transient) as e:
		reason = e.reason if isinstance(e, urllib.error.URLError) else e
		if isinstance(reason, transient) and not isinstance(reason, ssl.SSLCertVerificationError):
			return repr(reason)
		raise BuildError("%s: %r; not retried" % (url, reason))
	return None


def download_verified(urls, sha256, path, tries=DOWNLOAD_TRIES, first_tries=DOWNLOAD_FIRST_TRIES, pause=DOWNLOAD_PAUSE):
	"""Downloads the first of `urls` that answers to `path`, which only exists afterwards with the SHA-256 `sha256`
	(issue #153). A URL with a fallback after it gets `first_tries` attempts, the last one `tries`, with a pause of
	`pause` seconds times the attempt (at most six times `pause`) after each failure fetch_once retries. Raises
	BuildError on anything else, without a retry or a fallback: a 404 is a wrong pin, a wrong SHA-256 a wrong file."""
	import hashlib
	import time
	part = path + ".part"
	for position, url in enumerate(urls):
		limit = first_tries if position + 1 < len(urls) else tries
		for attempt in range(1, limit + 1):
			print("build.py: downloading %s (attempt %d of %d)" % (url, attempt, limit), flush=True)
			reason = fetch_once(url, part)
			if reason is None:
				digest = hashlib.sha256()
				with open(part, "rb") as f:
					for block in iter(lambda: f.read(1 << 20), b""):
						digest.update(block)
				if digest.hexdigest() != sha256:
					os.remove(part)
					raise BuildError("%s has SHA-256 %s, not %s; not retried" % (url, digest.hexdigest(), sha256))
				os.replace(part, path)
				return
			if os.path.exists(part):
				os.remove(part)
			if attempt < limit:
				wait = pause * min(attempt, 6)
				print("build.py: %s: %s; again in %d s" % (url, reason, wait), flush=True)
				time.sleep(wait)
			else:
				print("build.py: %s: %s; giving up on this URL" % (url, reason), flush=True)
	raise BuildError("every URL failed: %s" % ", ".join(urls))


def install_cmake(folder):
	"""Downloads Kitware's CMake zip (CMAKE_ARCHIVES, in order), checks its SHA-256 and unpacks it into `folder`
	(issues #141, #153)."""
	import zipfile
	folder = os.path.abspath(folder)
	if os.path.lexists(folder):
		raise BuildError("--install-cmake: %s exists already" % folder)
	parent = os.path.dirname(folder)
	os.makedirs(parent, exist_ok=True)
	work = tempfile.mkdtemp(prefix="cmake-", dir=parent)
	archive = os.path.join(work, "cmake.zip")
	try:
		download_verified(CMAKE_ARCHIVES, CMAKE_SHA256, archive)
	except BuildError as e:
		shutil.rmtree(work, ignore_errors=True)
		raise BuildError("--install-cmake: %s" % e)
	with zipfile.ZipFile(archive) as z:
		z.extractall(work)
	top = os.path.join(work, "cmake-%s-windows-x86_64" % CMAKE_VERSION)
	if not os.path.isfile(os.path.join(top, "bin", "cmake.exe")):
		raise BuildError("--install-cmake: no bin/cmake.exe in the archive; left in %s" % work)
	os.replace(top, folder)
	os.remove(archive)
	os.rmdir(work)
	print("build.py: CMake %s (SHA-256 %s) in %s" % (CMAKE_VERSION, CMAKE_SHA256, folder), flush=True)


def needs_linux(preset):
	"""Whether the configure preset's condition asks for a Linux host."""
	condition = preset.get("condition") or {}
	return (condition.get("type") == "equals" and condition.get("lhs") == "${hostSystemName}"
		and condition.get("rhs") == "Linux")


def in_docker(name):
	"""The configure preset behind `name` when it runs in the Docker image (a Linux preset on
	Windows), else None."""
	if os.name != "nt":
		return None
	presets = load_presets()
	configure = configure_preset_of(presets, name)
	return configure if needs_linux(resolved(presets, configure)) else None


def docker_command(configure, command):
	"""`docker run` for `command` in /work of the image, the repository's files copied there (module
	comment), and the file that names them, which the caller passes on stdin and deletes."""
	image = os.environ.get("PATELLA_DOCKER_IMAGE", DOCKER_IMAGE)
	if shutil.which("docker") is None:
		raise BuildError("docker not found on PATH (preset %s runs in the image %s)" % (configure, image))
	if subprocess.run(["docker", "image", "inspect", image], capture_output=True).returncode != 0:
		raise BuildError("Docker image %s not found; build it: docker build -t %s %s"
			% (image, image, DOCKER_DIR))
	listed = subprocess.run(["git", "ls-files", "-z", "--cached", "--others", "--exclude-standard"],
		cwd=ROOT, capture_output=True, check=True).stdout
	# tracked files deleted in the working tree are not there to copy
	names = [name for name in listed.split(b"\0")
		if name and os.path.lexists(os.path.join(ROOT, os.fsdecode(name)))]
	with tempfile.NamedTemporaryFile(prefix="build-docker-", suffix=".txt", delete=False) as listing:
		listing.write(b"".join(name + b"\0" for name in names))
	build_dir = os.path.join(ROOT, "build", configure)
	os.makedirs(build_dir, exist_ok=True)
	script = ('set -e; mkdir -p /work; (cd /src && tar --null -T - -cf -) | tar -x -C /work; '
		'cd /work; exec "$@"')
	mounts = shlex.split(os.environ.get("PATELLA_DOCKER_MOUNTS", ""))
	# The client's cursors (issue #131, cmake/LinuxX64.cmake): the local Tibia.exe read-only at /tibia, the environment
	# naming it for the configure. Never copied into the image or the build tree beyond the extracted cursor resources.
	tibia = os.path.join(ROOT, "Tibia.exe")
	if os.path.isfile(tibia):
		mounts += ["-v", "%s:/tibia/Tibia.exe:ro" % tibia, "-e", "PATELLA_TIBIA_EXE=/tibia/Tibia.exe"]
	return ["docker", "run", "--rm", "-i", "-v", "%s:/src:ro" % ROOT,
		"-v", "%s:/work/build/%s" % (build_dir, configure)] + mounts + [
		image, "bash", "-c", script, "bash"] + command, listing.name


def environment(name):
	"""The environment to run CMake in for the preset `name`."""
	presets = load_presets()
	preset = resolved(presets, configure_preset_of(presets, name))
	argument = vcvars_argument(preset)
	if argument is None:
		return dict(os.environ)
	if os.name != "nt":
		raise BuildError("preset %s needs MSVC (Windows)" % name)
	return developer_environment(argument)


def run(command, env, check_warnings=False, docker=None):
	"""Runs the command, in the Docker image when `docker` names the configure preset."""
	stdin = listing = None
	if docker is not None:
		print("build.py: preset %s in the Docker image: %s" % (docker, subprocess.list2cmdline(command)),
			flush=True)
		command, listing = docker_command(docker, command)
		stdin = open(listing, "rb")
	else:
		command = pinned_cmake(command)
	try:
		executable = shutil.which(command[0], path=env.get("PATH") or env.get("Path"))
		if executable is None:
			raise BuildError("%s not found on PATH" % command[0])
		print("build.py: %s" % subprocess.list2cmdline(command), flush=True)
		if not check_warnings:
			return subprocess.call([executable] + command[1:], env=env, cwd=ROOT, stdin=stdin)
		return run_checked([executable] + command[1:], env, stdin)
	finally:
		if stdin is not None:
			stdin.close()
			os.remove(listing)


def run_checked(command, env, stdin=None):
	"""Runs the command with its output passed through byte for byte and scanned for warnings; exit 1 when
	the command succeeded but warned."""
	sys.stdout.flush()
	out = sys.stdout.buffer
	warnings = []
	process = subprocess.Popen(command, env=env, cwd=ROOT, stdin=stdin, stdout=subprocess.PIPE,
		stderr=subprocess.STDOUT)
	try:
		for raw in process.stdout:
			out.write(raw)
			out.flush()
			# the tools write the console's code page; ASCII is all the patterns need
			line = raw.decode("latin-1").rstrip("\r\n")
			if WARNING.search(line):
				warnings.append(line)
	finally:
		process.stdout.close()
		code = process.wait()
	if warnings:
		print("build.py: %d warning%s in the build (any compiler or linker warning fails the build; "
			"issue #17):" % (len(warnings), "" if len(warnings) == 1 else "s"), file=sys.stderr)
		for line in warnings:
			print("build.py:   %s" % line, file=sys.stderr)
		sys.stderr.flush()
		if code == 0:
			return 1
	return code


def main(argv):
	if not argv or argv[0] in ("-h", "--help"):
		print(__doc__)
		return 0 if argv else 2
	if argv[0] == "--list":
		for preset in load_presets().get("workflowPresets", []):
			print("%-24s %s" % (preset["name"], preset.get("displayName", "")))
		return 0
	if argv[0] == "--install-cmake":
		if len(argv) != 2:
			print("build.py: --install-cmake <folder>", file=sys.stderr)
			return 2
		install_cmake(argv[1])
		return 0
	if argv[0] == "--env":
		if len(argv) < 3:
			print("build.py: --env <preset> <command> [arguments ...]", file=sys.stderr)
			return 2
		command = argv[2:]
		is_cmake = os.path.splitext(os.path.basename(command[0]))[0].lower() == "cmake"
		docker = in_docker(argv[1])
		env = dict(os.environ) if docker else environment(argv[1])
		return run(command, env, check_warnings=is_cmake, docker=docker)
	name = argv[0]
	if name not in [p["name"] for p in load_presets().get("workflowPresets", [])]:
		raise BuildError("no workflow preset %r (python -I tools/build.py --list)" % name)
	docker = in_docker(name)
	env = dict(os.environ) if docker else environment(name)
	return run(["cmake", "--workflow", "--preset", name] + argv[1:], env, check_warnings=True, docker=docker)


if __name__ == "__main__":
	try:
		sys.exit(main(sys.argv[1:]))
	except BuildError as e:
		print("build.py: %s" % e, file=sys.stderr)
		sys.exit(1)
