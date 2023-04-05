function DHSStart() {
	Socket.setCallback("mesh",mesh_data);
}

function DHSStop() {
}

function mesh_data(data,extra_data) {
  document.getElementById("r").innerHTML+=data;
}

function mesh_send() {
 Socket.send(document.getElementById("s").value);
 document.getElementById("s").value = "";
}