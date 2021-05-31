from yattag import Doc
import os
import re

#Copy "style.css" into docs
with open("docs_templates/style.css", "r") as style_template:
	with open("docs/style.css", "w") as style_file:
		style_file.write(style_template.read())
#Copy "index_script.js" into docs
with open("docs_templates/index_script.js", "r") as index_script_template:
	with open("docs/index_script.js", "w") as index_script:
		index_script.write(index_script_template.read())

os.chdir("docs/")

popup_script = """
<div id="popup" style="display: none; position: absolute; padding: 2px; border: 2px solid black; border-radius: 2px; background: white;"></div>
<script type="text/javascript">
popup = document.getElementById("popup");
function popup_window(dom, text){
	rect = dom.getBoundingClientRect()
	popup.innerHTML = text;
	popup.style.top = rect.bottom + window.scrollY + 5 + "px";
	popup.style.left = rect.left + window.scrollX + "px";
	popup.style.display = "inline-block";
}
function close_popup(){
	popup.style.display = "none";
}
</script>
"""

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
			definition_description = definition_description.replace("\n", "<br>")
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
			axiom_description = axiom_description.replace("\n", "<br>")
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
			proof_description = proof_description.replace("\n", "<br>")
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
		       (r"\~", "<b style=\"color: red;\">~</b>"),
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
	

def highlight_source(text, relative_head):
	text = " " + text + " "
	for replacement in source_replacements:
		text = re.sub(replacement[0], lambda match: source_replace(match, replacement[1]), text)
	for result in results_index:
		text = re.sub(r"\W" + result[1] + r"\W", lambda match: source_replace(match, "<b style=\"cursor: pointer;\" onclick=\"window.location.href = '" + relative_head + result[4] + "/" + result[1] + ".html';\" onmouseover=\"popup_window(this, '" + result[1] + " | " + result[4] + "');\" onmouseout=\"close_popup();\">" + result[1] + "</b>"), text)
	return text[1:-1]

#she was trying to sabatoge my typing
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
						text("CORE result")
					with tag("link", rel = "stylesheet", href="../style.css"):
						pass
					with tag("link", rel = "stylesheet", href="https://fontlibrary.org/face/pixelated", type="text/css"):
						pass
				with tag("body"):
					with tag("h1", style = "font-size: 4vw"):
						text("CORE library")
					with tag("div", klass = "page_body"):
						with tag("div", klass = "card"):
							doc.asis("<b>Result type</b>: " + result[0])
							doc.asis("<br><b>Result name</b>: " + result[1])
							if(result[2] == ""):
								doc.asis("<br><b>Description</b>: None given")
							else:
								doc.asis("<br><b>Description</b>: " + result[2] + "")
							doc.asis("<br><b>Source</b>:<br><br>")
							with tag("code", style="white-space: pre-wrap; cursor: default;"):
								doc.asis(highlight_source(result[3], "../"))
				doc.asis(popup_script)
			file_handle.write(doc.getvalue())
	os.chdir("../../proofs")

#Get the names of all of the files from the proofs folder
os.chdir("../proofs");
files = [f for f in os.listdir(".") if os.path.isfile(f)]
files.sort()

#Parse through each proof file
for file in files:
	with open(file, "r") as file_handle:
		file_lines = file_handle.readlines()
	file_text = "\n".join(file_lines)
	parts = file.split(".")
	parts.pop()
	name = ".".join(parts)
	results = index_file(file_lines, name)
	results_index.extend(results)
	write_results(results, name)
	name = name + ".html"
	file_text = highlight_source(file_text, "../")
	doc, tag, text = Doc().tagtext()

#Generate index.html
doc, tag, text = Doc().tagtext()
doc.asis("<!DOCTYPE html>")
with tag("html"):
	with tag("head"):
		with tag("title"):
			text("CORE home")
		with tag("link", rel = "stylesheet", href="style.css"):
			pass
		with tag("link", rel = "stylesheet", href="https://fontlibrary.org/face/pixelated", type="text/css"):
			pass
	with tag("body"):
		with tag("h1", style = "font-size: 4vw"):
			text("CORE library")
		with tag("div", klass = "page_body"):
			with tag("script", src = "index_script.js", type = "text/javascript"):
				pass
			with tag("div", klass = "card"):
				with tag("p"):
					text("Click on each result to see its proof")
				with tag("p"):
					text("Filter by type: ")
					with tag("select", id = "filter_type", onchange = "update_filter();"):
						with tag("option", value = "All"):
							text("All")
						with tag("option", value = "Definitions"):
							text("Definitions")
						with tag("option", value = "Axioms"):
							text("Axioms")
						with tag("option", value = "Proofs"):
							text("Proofs")
					text(" Filter by name: ")
					with tag("input", id = "filter_name", onchange = "update_filter();"):
						pass;
			with tag("div", klass = "card", id = "results"):
				for result in results_index:
					with tag("p"):
						text(result[0] + ": ")
						with tag("a", href = result[4] + "/" + result[1] + ".html"):
							text(result[4] + " " + result[1])
os.chdir("../docs")
with open("index.html", "w") as index:
	index.write(doc.getvalue())
os.chdir("../proofs")
