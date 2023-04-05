function DHSStart() {
	Socket.setCallback("s",get_data,"s");
	Socket.setCallback("d",get_data,"d");
	Socket.setCallback("x",get_data,"x");
	Socket.send("get s d x"); // get curretn field data
}

function DHSStop() {
}

function get_data(data,extra_data) { // get websocket data
  document.getElementById(extra_data).value=data; // set number field
  document.getElementById(extra_data+"s").value=data; // set slider field
	}
