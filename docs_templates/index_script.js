function update_filter(){
	var results = document.getElementById("results").childNodes;
	var choice = document.getElementById("filter_type").value;
	var search_string = document.getElementById("filter_name").value;
	var i;
	for(i = 0; i < results.length; i++){
		results[i].style.display = "block";
	}
	if(choice === "Definitions"){
		for(i = 0; i < results.length; i++){
			if(results[i].innerHTML.indexOf("definition") != 0){
				results[i].style.display = "none";
			}
		}
	} else if(choice === "Axioms"){
		for(i = 0; i < results.length; i++){
			if(results[i].innerHTML.indexOf("axiom") != 0){
				results[i].style.display = "none";
			}
		}
	} else if(choice === "Proofs"){
		for(i = 0; i < results.length; i++){
			if(results[i].innerHTML.indexOf("proof") != 0){
				results[i].style.display = "none";
			}
		}
	}
	for(i = 0; i < results.length; i++){
		if(results[i].innerHTML.indexOf(search_string) == -1){
			results[i].style.display = "none";
		}
	}
}
