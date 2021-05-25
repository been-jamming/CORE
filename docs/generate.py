from yattag import Doc
import os
import re

results_index = []

def index_file(lines, name):
	results = []
	lines_iter = iter(lines)
	line = next(lines_iter, None)
	line_num = 0
	while line:
		if line[:7] == "define ":
			definition_name = line.split(":")[0].split()[1]
			if "(" in definition_name:
				definition_name = definition_name.split("(")[0]
			definition_description = ""
			definition_line_num = line_num - 1
			while definition_line_num and lines[definition_line_num][:2] == "//":
				definition_description = lines[definition_line_num][2:] + definition_description
				definition_line_num -= 1
			results.append(("definition", definition_name, definition_description, line, name))
		if line[:6] == "axiom ":
			axiom_name = line.split(":")[0].split()[1]
			if "[" in axiom_name:
				axiom_name = axiom_name.split("[")[0]
			axiom_description = ""
			axiom_line_num = line_num - 1
			while axiom_line_num and lines[axiom_line_num][:2] == "//":
				axiom_description = lines[axiom_line_num][2:] + axiom_description
				axiom_line_num -= 1
			results.append(("axiom", axiom_name, axiom_description, line, name))
		if line[:6] == "prove ":
			proof_name = line.split(":")[0].split()[1]
			if "[" in proof_name:
				proof_name = proof_name.split("[")[0]
			proof_description = ""
			proof_line_num = line_num - 1
			while proof_line_num and lines[proof_line_num][:2] == "//":
				proof_description = lines[proof_line_num][2:] + proof_description
				proof_line_num -= 1
			proof_code = line
			line = next(lines_iter, None)
			line_num += 1
			while line and line[0] != "}":
				proof_code += line
				line = next(lines_iter, None)
				line_num += 1
			proof_code += "}"
			results.append(("proof", proof_name, proof_description, proof_code, name))
		line = next(lines_iter, None)
		line_num += 1
	return results

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

def highlight_source(text):
	for replacement in source_replacements:
		text = re.sub(replacement[0], lambda match: source_replace(match, replacement[1]), text)
	return text

def write_results(results, name):
	os.chdir("../docs")
	if not os.path.exists(name):
		os.makedirs(name)
	os.chdir(name)
	for result in results:
		with open(result[1] + ".html", "w") as file_handle:
			doc, tag, text = Doc().tagtext()
			doc.asis("<!DOCTYPE html>")
			with tag("html"):
				with tag("head"):
					with tag("title"):
						text("result")
				with tag("body"):
					with tag("div", klass = "result_type"):
						text(result[0])
					with tag("div", klass = "result_name"):
						text(result[1])
					with tag("div", klass = "result_description"):
						text("Description: " + result[2])
					with tag("code", style="white-space: pre-wrap;"):
						doc.asis(highlight_source(result[3]))
			file_handle.write(doc.getvalue())
	os.chdir("../../proofs")

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

#Parse through each proof file
for file in files:
	with open(file, "r") as file_handle:
		file_lines = file_handle.readlines()
	file_text = "\n".join(file_lines)
	parts = file.split(".")
	parts.pop()
	name = ".".join(parts)
	results = index_file(file_lines, name)
	write_results(results, name)
	results_index.extend(results)
	name = name + ".html"
	file_text = highlight_source(file_text)
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

