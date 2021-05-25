from yattag import Doc
import os
import re

#Replacements for the source files
source_replacements = [("<", "##LESSTHAN"),
		       (">", "##GREATERTHAN"),
		       (r"\Wdefine\W", "<b style=\"color: blue;\">define</b>"),
		       (r"\Waxiom\W", "<b style=\"color: blue;\">axiom</b>"),
		       (r"\Wprove\W", "<b style=\"color: blue;\">prove</b>"),
		       (r"\Wgiven\W", "<b style=\"color: blue;\">given</b>"),
		       (r"\Wchoose\W", "<b style=\"color: blue;\">choose</b>"),
		       (r"\Wimplies\W", "<b style=\"color: blue;\">implies</b>"),
		       (r"\Wreturn\W", "<b style=\"color: blue;\">return</b>"),
		       (r"\Wextract\W", "<b style=\"color: blue;\">extract</b>"),
		       (r"\Wleft\W", "<b style=\"color: green;\">left</b>"),
		       (r"\Wright\W", "<b style=\"color: green;\">right</b>"),
		       (r"\Wexpand\W", "<b style=\"color: green;\">expand</b>"),
		       (r"\Wand\W", "<b style=\"color: green;\">and</b>"),
		       (r"\Wor\W", "<b style=\"color: green;\">or</b>"),
		       (r"\Wswap\W", "<b style=\"color: green;\">swap</b>"),
		       (r"\Wiff\W", "<b style=\"color: green;\">iff</b>"),
		       (r"\Wbranch\W", "<b style=\"color: green;\">branch</b>"),
		       (r"\*", "<b style=\"color: red;\">*</b>"),
		       (r"\^", "<b style=\"color: red;\">^</b>"),
		       (r"\&", "<b style=\"color: red;\">&</b>"),
		       (r"\|", "<b style=\"color: red;\">|</b>"),
		       (r"##LESSTHAN-##GREATERTHAN", "<b style=\"color: red;\">&lt-&gt</b>"),
		       (r"-##GREATERTHAN", "<b style=\"color: red;\">-&gt</b>"),
		       (r"##LESSTHAN", "&lt"),
		       (r"##GREATERTHAN", "&gt")]

def source_replace(match, replacement):
	pattern = match.re.pattern
	if pattern[0:2] == r"\W":
		return match.group()[0] + replacement + match.group()[-1]
	else:
		return replacement

#Get the names of all of the files from the proofs folder
os.chdir("../proofs");
files = [f for f in os.listdir(".") if os.path.isfile(f)]
files.sort()

#Generate index.html
doc, tag, text = Doc().tagtext()
doc.asis("<!DOCTYPE html>")
with tag("html"):
	with tag("head"):
		with tag("title"):
			text("CORE home")
	with tag("body"):
		with tag("div", klass = "header"):
			text("Files")
		with tag("div", klass = "files"):
			for file in files:
				parts = file.split(".")
				parts.pop()
				name = ".".join(parts) + ".html"
				with tag("a", href = "proofs/" + name):
					text(file)
				with tag("br"):
					pass
os.chdir("../docs")
with open("index.html", "w") as index:
	index.write(doc.getvalue())
os.chdir("../proofs")

#Generate each proof file
for file in files:
	with open(file, "r") as file_handle:
		file_text = file_handle.read()
	parts = file.split(".")
	parts.pop()
	name = ".".join(parts) + ".html"
	for replacement in source_replacements:
		file_text = re.sub(replacement[0], lambda match: source_replace(match, replacement[1]), file_text)
	doc, tag, text = Doc().tagtext()
	doc.asis("<!DOCTYPE html>")
	with tag("html"):
		with tag("head"):
			with tag("title"):
				text("file")
		with tag("body"):
			with tag("div", klass = "header"):
				text("CORE Source")
			with tag("div", klass = "CORE_source"):
				with tag("code", style="white-space: pre-wrap;"):
					doc.asis(file_text)
	os.chdir("../docs")
	with open("proofs/" + name, "w") as file_handle:
		file_handle.write(doc.getvalue())
	os.chdir("../proofs")

