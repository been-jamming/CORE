function update_filter(){
	results = document.getElementById("results").childNodes;
	var choice = document.getElementById("filter").value;
	var i;
	if(choice === "All"){
		for(i = 0; i < results.length; i++){
			results[i].style.display = "block";
		}
	} else if(choice === "Definitions"){
		for(i = 0; i < results.length; i++){
			if(results[i].innerHTML.indexOf("definition") != 0){
				results[i].style.display = "none";
			} else {
				results[i].style.display = "block";
			}
		}
	} else if(choice === "Axioms"){
		for(i = 0; i < results.length; i++){
			if(results[i].innerHTML.indexOf("axiom") != 0){
				results[i].style.display = "none";
			} else {
				results[i].style.display = "block";
			}
		}
	} else {
		for(i = 0; i < results.length; i++){
			if(results[i].innerHTML.indexOf("proof") != 0){
				results[i].style.display = "none";
			} else {
				results[i].style.display = "block";
			}
		}
	}
}
