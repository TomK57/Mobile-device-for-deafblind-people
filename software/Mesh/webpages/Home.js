var lineLength = 0;

function DHSStart() {
	Socket.setCallback("mesh",mesh_data);
}

function DHSStop() {
}

function mesh_data(data,extra_data) {
  lineLength ++;
	if (lineLength > 20) {
	  lineLength = 0;
		document.getElementById("r").innerHTML+='\n';
	}
  document.getElementById("r").innerHTML+=data;
	if (data === '\n') lineLength = 0;
}

function mesh_send() {
 Socket.send(document.getElementById("s").value);
 document.getElementById("s").value = "";
}