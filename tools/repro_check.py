r"""Prove that the Release client of this checkout is the same executable as a commit's, byte for byte.

Usage, from any folder (the checkout is the one this file is in):
  python -I tools/repro_check.py [<commit>]              the commit to compare with; default HEAD
  python -I tools/repro_check.py --compare <a.exe> <b.exe>

The proof for a change that must not change the program at all: one that moves no line of a .cpp, renames no function
and touches no assertion (the assertions store their expression's text, file, line and function). With a clean working
tree and no argument it is the check cmake/Reproducible.cmake names: one commit, built in two folders, gives one
executable.

What it does:
  1. Builds this checkout's working tree: `python -I tools/build.py windows-x64` (it fails on any warning), after
     deleting Patella.exe and Patella.pdb so that the link is a first link. The build tree is configured first if it is
     not yet. A worktree has no Tibia.exe (git does not have it): the configure then reads, in place, the one this
     checkout's build tree already names, PATELLA_TIBIA_EXE's, or the main checkout's. Nothing is copied.
  2. Builds the commit the same way in a second folder, build/base, a detached worktree of this repository that the
     script makes and then keeps, so that the next commit compiles only what differs. The executable is kept as
     build/base.cache/<commit>.exe and not built again. (`git worktree remove --force build/base` removes the folder.)
  3. Compares the two files. They must be equal in every byte. When they are not, the script says whether they differ
     only in the build's identifiers (below), which means the code is the same and one of the links was not a first
     link, or in other bytes too, each range listed with its section and address.

MSVC now and then compiles Communication.cpp to other code from the same source (measured several times with 14.51:
std::_Integer_to_chars<unsigned char>, "mov rcx,rax" for "mov ecx,eax"), and compiling it again gives the usual code.
So a difference is believed only after the client's own objects of both folders have been compiled once more.

--compare only compares two executables. The build identifiers, each found through the PE headers of both files and
never guessed from where the bytes differ: the COFF header's TimeDateStamp, the TimeDateStamp of every debug directory
entry, the GUID and age of the CodeView record (the PDB's), and the debug directory's REPRO entry (a hash of the
build).

Exit code 0: the executables are equal in every byte. 1: they differ, or a build failed. Windows and MSVC only.
"""

import os
import shutil
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PRESET = "windows-x64"
BASE = os.path.join(ROOT, "build", "base")
CACHE = os.path.join(ROOT, "build", "base.cache")


class CheckError(Exception):
	pass


def identifier_ranges(data):
	"""The byte ranges of the build identifiers {(start, end): name} and the sections [(name, rva, vsize, raw,
	rawsize)] of a PE32+ file."""
	if data[:2] != b"MZ":
		raise CheckError("not an MZ file")
	pe = struct.unpack_from("<I", data, 0x3c)[0]
	if data[pe:pe + 4] != b"PE\0\0":
		raise CheckError("not a PE file")
	sections_count, = struct.unpack_from("<H", data, pe + 6)
	optional_size, = struct.unpack_from("<H", data, pe + 20)
	optional = pe + 24
	magic, = struct.unpack_from("<H", data, optional)
	if magic != 0x20b:
		raise CheckError("not a PE32+ file")
	ranges = {(pe + 8, pe + 12): "COFF header TimeDateStamp"}
	sections = []
	table = optional + optional_size
	for i in range(sections_count):
		entry = table + 40 * i
		name = data[entry:entry + 8].rstrip(b"\0").decode("ascii", "replace")
		vsize, rva, rawsize, raw = struct.unpack_from("<IIII", data, entry + 8)
		sections.append((name, rva, vsize, raw, rawsize))

	def offset_of(rva):
		for _, section_rva, vsize, raw, rawsize in sections:
			if section_rva <= rva < section_rva + max(vsize, rawsize):
				return raw + rva - section_rva
		raise CheckError("RVA %#x is in no section" % rva)

	# Data directory 6: the debug directory.
	debug_rva, debug_size = struct.unpack_from("<II", data, optional + 112 + 8 * 6)
	if debug_rva:
		directory = offset_of(debug_rva)
		for i in range(debug_size // 28):
			entry = directory + 28 * i
			_, _, _, _, kind, size, _, raw = struct.unpack_from("<IIHHIIII", data, entry)
			ranges[(entry + 4, entry + 8)] = "debug directory entry %d (type %d) TimeDateStamp" % (i, kind)
			if kind == 2 and data[raw:raw + 4] == b"RSDS":
				ranges[(raw + 4, raw + 24)] = "CodeView record's GUID and age (the PDB's)"
			elif kind == 16:
				ranges[(raw, raw + size)] = "debug directory REPRO hash"
	return ranges, sections


def compare(path_a, path_b):
	"""Prints how the two executables differ. 0 when they are equal in every byte, else 1."""
	with open(path_a, "rb") as f:
		a = f.read()
	with open(path_b, "rb") as f:
		b = f.read()
	if a == b:
		print("IDENTICAL: every byte (%d bytes)" % len(a))
		return 0
	if len(a) != len(b):
		print("DIFFERENT: sizes %d and %d" % (len(a), len(b)))
		return 1
	ranges_a, sections = identifier_ranges(a)
	ranges_b, sections_b = identifier_ranges(b)
	if ranges_a != ranges_b or sections != sections_b:
		print("DIFFERENT: the section tables or the debug directories are laid out differently")
		return 1

	def where(offset):
		for name, rva, _, raw, rawsize in sections:
			if raw <= offset < raw + rawsize:
				return "%s RVA %#x" % (name, rva + offset - raw)
		return "headers"

	differing = []
	start = None
	for i in range(len(a)):
		if a[i] != b[i]:
			if start is None:
				start = i
		elif start is not None:
			differing.append((start, i))
			start = None
	if start is not None:
		differing.append((start, len(a)))
	other = 0
	listed = 0
	seen = set()
	for start, end in differing:
		names = [name for (low, high), name in ranges_a.items() if low <= start and end <= high]
		if names:
			seen.add(names[0])
			continue
		other += end - start
		if listed < 40:
			listed += 1
			print("  %#x-%#x (%d bytes, %s): %s -> %s" % (start, end, end - start, where(start),
				a[start:min(end, start + 16)].hex(), b[start:min(end, start + 16)].hex()))
	for name in sorted(seen):
		print("  differs: " + name)
	if other:
		print("DIFFERENT: %d bytes of code or data differ, in %d ranges" % (other, len(differing) - len(seen)))
	else:
		print("DIFFERENT only in the build identifiers above: the same code, but one link was not a first link")
	return 1


def git(*arguments, cwd=ROOT):
	result = subprocess.run(["git"] + list(arguments), cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
		check=False)
	if result.returncode != 0:
		raise CheckError("git %s: %s" % (" ".join(arguments), result.stderr.strip()))
	return result.stdout.strip()


def cached_tibia_exe(root):
	"""PATELLA_TIBIA_EXE of the folder's build tree, or None."""
	cache = os.path.join(root, "build", PRESET, "CMakeCache.txt")
	if os.path.isfile(cache):
		with open(cache, encoding="utf-8", errors="replace") as f:
			for line in f:
				if line.startswith("PATELLA_TIBIA_EXE:"):
					return line.split("=", 1)[1].strip()
	return None


def tibia_exe():
	"""The Tibia.exe a folder without one of its own configures with (the module comment), or None: this checkout has
	its own, which is CMake's default."""
	if os.path.isfile(os.path.join(ROOT, "Tibia.exe")):
		return None
	main_checkout = os.path.dirname(git("rev-parse", "--path-format=absolute", "--git-common-dir"))
	for path in (cached_tibia_exe(ROOT), os.environ.get("PATELLA_TIBIA_EXE"), os.path.join(main_checkout, "Tibia.exe")):
		if path and os.path.isfile(path):
			return path.replace("\\", "/")
	raise CheckError("no Tibia.exe: not here, not PATELLA_TIBIA_EXE's, not in %s" % main_checkout)


def run_build(root, command, what):
	result = subprocess.run([sys.executable, "-I", os.path.join(root, "tools", "build.py")] + command,
		stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace", check=False)
	if result.returncode != 0:
		lines = result.stdout.splitlines()
		interesting = [line for line in lines if "error" in line.lower() or "warning" in line.lower()]
		print("\n".join(interesting[:80] if interesting else lines[-60:]))
		raise CheckError("%s failed (exit %d); `python -I tools/build.py %s` in %s shows all of it"
			% (what, result.returncode, " ".join(command), root))


def build(root, tibia, what):
	"""Builds the Release client of the folder from a first link and returns the executable's path."""
	if not os.path.isfile(os.path.join(root, "build", PRESET, "CMakeCache.txt")):
		command = ["--env", PRESET, "cmake", "--preset", PRESET]
		if not os.path.isfile(os.path.join(root, "Tibia.exe")):
			command.append("-DPATELLA_TIBIA_EXE=" + (tibia or os.path.join(ROOT, "Tibia.exe").replace("\\", "/")))
		print("repro_check: configuring %s" % what, flush=True)
		run_build(root, command, "the configure of " + what)
	release = os.path.join(root, "build", PRESET, "Release")
	for name in ("Patella.exe", "Patella.pdb"):
		if os.path.isfile(os.path.join(release, name)):
			os.remove(os.path.join(release, name))
	print("repro_check: building %s" % what, flush=True)
	run_build(root, [PRESET], "the build of " + what)
	executable = os.path.join(release, "Patella.exe")
	if not os.path.isfile(executable):
		raise CheckError("the build of %s made no Patella.exe" % what)
	return executable


def remove_own_objects(root):
	"""Deletes the objects of the client's own sources (not the libraries') so that the next build compiles them again."""
	removed = 0
	for folder, _, names in os.walk(os.path.join(root, "build", PRESET, "CMakeFiles")):
		if os.sep + "Release" + os.sep + "src" not in folder + os.sep:
			continue
		for name in names:
			if name.endswith(".obj"):
				os.remove(os.path.join(folder, name))
				removed += 1
	return removed


def build_base(commit, tibia):
	"""Builds the commit in build/base and returns the path of its executable in build/base.cache."""
	if os.path.isdir(BASE):
		git("checkout", "--force", "--detach", commit, cwd=BASE)
	else:
		os.makedirs(os.path.dirname(BASE), exist_ok=True)
		git("worktree", "prune")
		git("worktree", "add", "--detach", BASE, commit)
	executable = build(BASE, tibia, "%s in build/base" % commit[:7])
	os.makedirs(CACHE, exist_ok=True)
	kept = os.path.join(CACHE, commit + ".exe")
	shutil.copyfile(executable, kept)
	return kept


def main(argv):
	if argv and argv[0] in ("-h", "--help"):
		print(__doc__)
		return 0
	if argv and argv[0] == "--compare":
		if len(argv) != 3:
			print("repro_check.py: --compare <a.exe> <b.exe>", file=sys.stderr)
			return 2
		return compare(argv[1], argv[2])
	if len(argv) > 1:
		print("repro_check.py: [<commit>]", file=sys.stderr)
		return 2
	if os.name != "nt":
		raise CheckError("the comparison is of the Windows client (MSVC)")
	commit = git("rev-parse", "--verify", (argv[0] if argv else "HEAD") + "^{commit}")
	tibia = tibia_exe()
	mine = build(ROOT, tibia, "this checkout")
	kept = os.path.join(CACHE, commit + ".exe")
	if os.path.isfile(kept):
		print("repro_check: %s was built before (build/base.cache)" % commit[:7], flush=True)
	else:
		kept = build_base(commit, tibia)
	print("repro_check: this checkout against %s" % commit[:7], flush=True)
	code = compare(kept, mine)
	if code == 0:
		return 0
	print("repro_check: compiling the client's own objects of both folders again, to rule the compiler out", flush=True)
	remove_own_objects(ROOT)
	mine = build(ROOT, tibia, "this checkout")
	if os.path.isdir(BASE) and git("rev-parse", "HEAD", cwd=BASE) == commit:
		remove_own_objects(BASE)
	kept = build_base(commit, tibia)
	print("repro_check: this checkout against %s, both compiled again" % commit[:7], flush=True)
	return compare(kept, mine)


if __name__ == "__main__":
	try:
		sys.exit(main(sys.argv[1:]))
	except CheckError as e:
		print("repro_check.py: %s" % e, file=sys.stderr)
		sys.exit(1)
