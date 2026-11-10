r"""Prove what kind of change the sources of src/ went through: only comments, or only names. Rename, and format.

Usage, from any folder (the checkout is the one this file is in):
  python -I tools/source_check.py comments [<old> [<new>]]
  python -I tools/source_check.py names [<old> [<new>]] Old=New [Old=New ...]
  python -I tools/source_check.py rename Old=New [Old=New ...]
  python -I tools/source_check.py format [--fix] [<old>]

<old> is a commit, HEAD when left out. <new> is a commit too; left out, it is the working tree. The sources are the
.cpp and .h files under src/, without src/Extern.

comments  Every source that differs between <old> and <new> is the same code: the same lines once the comments are
          removed, trailing white space is removed and empty lines are dropped. White space inside a line of code
          counts, because the assertions store their arguments as written. Added, deleted and renamed sources fail.
          Changed files that are not sources are listed and not checked.
names     Every source of <new> is <old>'s with the names replaced, white space apart: the same tokens once each old
          name is mapped to its new one (in code, comments and strings alike). No new name is in <old>, and no old name
          is left in <new>. Added, deleted and renamed sources fail.
rename    Replaces each old name, as a whole word, in every source of the working tree: code, comments and strings.
          The replacements inside string literals are listed, since a text the player sees may be among them.
format    The sources that differ between <old> and the working tree are in the style of .clang-format (clang-format
          --dry-run --Werror; Visual Studio's clang-format, or the one on PATH). A file that was not in the style in
          <old> either is named and does not fail. --fix formats the others in place.

Exit code 0: the check holds. 1: it does not (each finding is printed), or a command failed.
"""

import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW_STRING = re.compile(r'R"([^()\\ ]{0,16})\(')
ENCODING_PREFIX = re.compile(r"(?:^|[^A-Za-z0-9_])(?:u8|u|U|L)$")
# An identifier, a number with its suffix, a run of white space, or any other single character.
TOKEN = re.compile(r"[A-Za-z_]\w*|\d[\w.]*|\s+|.", re.DOTALL)


class CheckError(Exception):
	pass


def spans(text):
	"""The comments and the string literals of a C++ text: [(start, end, "comment" or "string")]. A `//` comment ends
	before its newline."""
	found = []
	i = 0
	n = len(text)
	while i < n:
		c = text[i]
		if c == "/" and text.startswith("//", i):
			end = text.find("\n", i)
			end = n if end < 0 else end
			found.append((i, end, "comment"))
			i = end
		elif c == "/" and text.startswith("/*", i):
			end = text.find("*/", i + 2)
			end = n if end < 0 else end + 2
			found.append((i, end, "comment"))
			i = end
		elif c == '"':
			start = i
			raw = RAW_STRING.match(text, i - 1) if i > 0 and text[i - 1] == "R" else None
			if raw:
				close = ")" + raw.group(1) + '"'
				end = text.find(close, raw.end())
				i = n if end < 0 else end + len(close)
			else:
				i += 1
				while i < n and text[i] != '"' and text[i] != "\n":
					i += 2 if text[i] == "\\" else 1
				i += 1
			found.append((start, min(i, n), "string"))
		elif c == "'":
			# After a digit or a letter it separates digits (1'000), unless the letters are an encoding prefix (u8'a').
			if i > 0 and (text[i - 1].isalnum() or text[i - 1] == "_") and not ENCODING_PREFIX.search(text[max(0, i - 3):i]):
				i += 1
			else:
				i += 1
				while i < n and text[i] != "'" and text[i] != "\n":
					i += 2 if text[i] == "\\" else 1
				i += 1
		else:
			i += 1
	return found


def code_lines(text):
	"""The text's lines without comments, trailing white space and empty lines."""
	out = []
	last = 0
	for start, end, kind in spans(text):
		if kind == "comment":
			out.append(text[last:start])
			last = end
	out.append(text[last:])
	lines = [line.rstrip() for line in "".join(out).split("\n")]
	return [line for line in lines if line]


def tokens(text, names=None):
	"""The text's tokens without white space, each name of `names` replaced."""
	found = [t for t in TOKEN.findall(text) if not t.isspace()]
	return [names.get(t, t) for t in found] if names else found


def git(*arguments, binary=False):
	result = subprocess.run(["git"] + list(arguments), cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
	if result.returncode != 0:
		raise CheckError("git %s: %s" % (" ".join(arguments), result.stderr.decode("utf-8", "replace").strip()))
	return result.stdout if binary else result.stdout.decode("utf-8")


def is_source(path):
	return path.startswith("src/") and not path.startswith("src/Extern/") and path.endswith((".cpp", ".h"))


def read(commit, path):
	"""The file's text in the commit, or in the working tree when commit is None."""
	if commit is None:
		with open(os.path.join(ROOT, path), encoding="utf-8", newline="") as f:
			return f.read()
	return git("show", "%s:%s" % (commit, path), binary=True).decode("utf-8")


def sources(commit):
	"""The sources of the commit, or of the working tree when commit is None (the files git knows that exist)."""
	if commit is None:
		listed = git("ls-files", "-z", "--cached", "--others", "--exclude-standard", "--", "src").split("\0")
		return sorted(p for p in listed if p and is_source(p) and os.path.isfile(os.path.join(ROOT, p)))
	return sorted(p for p in git("ls-tree", "-r", "-z", "--name-only", commit, "--", "src").split("\0") if p and is_source(p))


def changed(old, new):
	"""[(status, path)] of the files that differ between the commit `old` and `new` (the working tree when None),
	untracked files of the working tree included."""
	arguments = ["diff", "--name-status", "--no-renames", "-z", old] + ([new] if new else []) + ["--"]
	fields = git(*arguments).split("\0")
	entries = [(fields[i], fields[i + 1]) for i in range(0, len(fields) - 1, 2)]
	if new is None:
		for path in git("ls-files", "-z", "--others", "--exclude-standard").split("\0"):
			if path:
				entries.append(("A", path))
	return entries


def commits_and_names(arguments, most):
	"""Splits the arguments into at most `most` leading commits and the Old=New pairs after them."""
	commits = []
	while arguments and "=" not in arguments[0]:
		commits.append(arguments.pop(0))
	if len(commits) > most:
		raise CheckError("too many commits: %s" % " ".join(commits))
	names = {}
	for argument in arguments:
		old, _, new = argument.partition("=")
		if not re.fullmatch(r"[A-Za-z_]\w*", old) or not re.fullmatch(r"[A-Za-z_]\w*", new) or old in names:
			raise CheckError("not a rename, or given twice: %s" % argument)
		names[old] = new
	old = commits[0] if commits else "HEAD"
	new = commits[1] if len(commits) > 1 else None
	return old, new, names


def check_comments(arguments):
	old, new, names = commits_and_names(arguments, 2)
	if names:
		raise CheckError("comments takes no names")
	bad = 0
	checked = 0
	skipped = []
	for status, path in changed(old, new):
		if not is_source(path):
			skipped.append(path)
			continue
		if status != "M":
			print("NOT A MODIFICATION (%s): %s" % (status, path))
			bad += 1
			continue
		before = code_lines(read(old, path))
		after = code_lines(read(new, path))
		checked += 1
		if before == after:
			continue
		bad += 1
		for x, y in zip(before, after):
			if x != y:
				print("CODE DIFFERS: %s\n  was: %s\n  is:  %s" % (path, x, y))
				break
		else:
			print("CODE DIFFERS: %s (%d lines of code, were %d)" % (path, len(after), len(before)))
	for path in skipped:
		print("not checked (not a source of src/): %s" % path)
	if bad:
		print("comments: %d of %d changed sources differ in more than comments" % (bad, checked + bad))
		return 1
	print("comments: %d changed sources, the same code in each (only comments, line ends and empty lines differ)" % checked)
	return 0


def check_names(arguments):
	old, new, names = commits_and_names(arguments, 2)
	if not names:
		raise CheckError("names needs Old=New pairs")
	new_names = set(names.values())
	before = sources(old)
	after = sources(new)
	bad = 0
	differing = 0
	for path in sorted(set(before) ^ set(after)):
		print("%s: %s" % ("ADDED" if path in after else "DELETED", path))
		bad += 1
	for path in sorted(set(before) & set(after)):
		was = read(old, path)
		now = read(new, path)
		was_tokens = tokens(was)
		found = new_names.intersection(was_tokens)
		if found:
			print("NEW NAME ALREADY IN %s: %s" % (path, ", ".join(sorted(found))))
			bad += 1
		left = set(names).intersection(tokens(now))
		if left:
			print("OLD NAME LEFT IN %s: %s" % (path, ", ".join(sorted(left))))
			bad += 1
		if [names.get(t, t) for t in was_tokens] != tokens(now):
			print("OTHER DIFFERENCE IN %s" % path)
			bad += 1
		if was != now:
			differing += 1
	if bad:
		print("names: %d problems" % bad)
		return 1
	print("names: %d sources differ, in nothing but the %d names (and white space)" % (differing, len(names)))
	return 0


def rename(arguments):
	_, _, names = commits_and_names(arguments, 0)
	if not names:
		raise CheckError("rename needs Old=New pairs")
	pattern = re.compile(r"\b(%s)\b" % "|".join(map(re.escape, names)))
	total = 0
	files = 0
	for path in sources(None):
		text = read(None, path)
		strings = [(start, end) for start, end, kind in spans(text) if kind == "string"]
		for match in pattern.finditer(text):
			if any(start <= match.start() < end for start, end in strings):
				print("in a string literal: %s:%d: %s" % (path, text.count("\n", 0, match.start()) + 1, match.group(1)))
		replaced, count = pattern.subn(lambda m: names[m.group(1)], text)
		if count:
			with open(os.path.join(ROOT, path), "w", encoding="utf-8", newline="") as f:
				f.write(replaced)
			total += count
			files += 1
	print("rename: %d replacements in %d sources" % (total, files))
	return 0


def clang_format():
	"""Visual Studio's clang-format (the newest Visual Studio with the C++ tools, as tools/build.py finds it), or the
	one on PATH."""
	installer = os.path.join(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"), "Microsoft Visual Studio", "Installer")
	vswhere = os.path.join(installer, "vswhere.exe")
	if os.path.isfile(vswhere):
		result = subprocess.run([vswhere, "-latest", "-products", "*", "-requires",
			"Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"],
			stdout=subprocess.PIPE, text=True, check=False)
		for folder in result.stdout.splitlines():
			path = os.path.join(folder.strip(), "VC", "Tools", "Llvm", "x64", "bin", "clang-format.exe")
			if os.path.isfile(path):
				return path
	path = shutil.which("clang-format")
	if path is None:
		raise CheckError("clang-format not found (Visual Studio's VC\\Tools\\Llvm\\x64\\bin, or PATH)")
	return path


def check_format(arguments):
	fix = "--fix" in arguments
	old, _, names = commits_and_names([a for a in arguments if a != "--fix"], 1)
	if names:
		raise CheckError("format takes no names")
	tool = clang_format()

	def in_style(path, text=None):
		command = [tool, "--dry-run", "--Werror", "--assume-filename=" + path] if text is not None else [tool, "--dry-run", "--Werror", path]
		return subprocess.run(command, cwd=ROOT, input=text, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False).returncode == 0

	bad = 0
	checked = 0
	for status, path in changed(old, None):
		if status == "D" or not is_source(path):
			continue
		checked += 1
		if in_style(path):
			continue
		if status == "M" and not in_style(path, git("show", "%s:%s" % (old, path), binary=True)):
			print("not in the style in %s either, left alone: %s" % (old, path))
		elif fix:
			subprocess.run([tool, "-i", path], cwd=ROOT, check=True)
			print("formatted: %s" % path)
		else:
			print("NOT IN THE STYLE: %s" % path)
			bad += 1
	if bad:
		print("format: %d of %d changed sources are not in the style of .clang-format (--fix formats them)" % (bad, checked))
		return 1
	print("format: %d changed sources, all in the style of .clang-format" % checked)
	return 0


def main(argv):
	commands = {"comments": check_comments, "names": check_names, "rename": rename, "format": check_format}
	if not argv or argv[0] in ("-h", "--help") or argv[0] not in commands:
		print(__doc__)
		return 0 if argv and argv[0] in ("-h", "--help") else 2
	return commands[argv[0]](argv[1:])


if __name__ == "__main__":
	try:
		sys.exit(main(sys.argv[1:]))
	except (CheckError, subprocess.CalledProcessError) as e:
		print("source_check.py: %s" % e, file=sys.stderr)
		sys.exit(1)
