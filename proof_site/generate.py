from yattag import Doc
import os

#Get the names of all of the files from the proofs folder
os.chdir("../proofs");
files = [f for f in os.listdir(".") if os.path.isfile(f)]
files.sort()
print(files)

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
os.chdir("../proof_site")
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
	file_text = file_text.replace("<", "&lt")
	file_text = file_text.replace(">", "&gt")
	file_text = file_text.replace("define ", "<b style=\"color: blue\">define </b>")
	file_text = file_text.replace("axiom ", "<b style=\"color: blue\">axiom </b>")
	file_text = file_text.replace("prove ", "<b style=\"color: blue\">prove </b>")
	file_text = file_text.replace("given ", "<b style=\"color: blue\">given </b>")
	file_text = file_text.replace("choose ", "<b style=\"color: blue\">choose </b>")
	file_text = file_text.replace("implies ", "<b style=\"color: blue\">implies </b>")
	file_text = file_text.replace("return ", "<b style=\"color: blue\">return </b>")
	file_text = file_text.replace("extract ", "<b style=\"color: blue\">extract </b>")
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
	os.chdir("../proof_site")
	with open("proofs/" + name, "w") as file_handle:
		file_handle.write(doc.getvalue())
	os.chdir("../proofs")

