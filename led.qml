// import QtQuick 1.0 // to target S60 5th Edition or Maemo 5
import QtQuick 1.1

Rectangle {
	id: rectangle1
	width: 20
	height: 20
	state:"off_state"
	radius: width/2
	opacity: 1
	border.color: "#000000"
	border.width: 2
	Timer {
		id: blink_off
		interval: 50
		onTriggered: turnOff()
	}

	function blinkOn(){
		turnOn();
		blink_off.start();
	}
	function blinkErr(){
		turnErr();
		blink_off.start();
	}
	function turnOn(){
		if(state != "on_state")
			state = "on_state";
	}
	function turnOff(){
		if(state != "off_state")
			state = "off_state";
	}
	function turnErr(){
		if(state != "error_state")
			state = "error_state";
	}
	function currentState(){
		return state;
	}

	states: [
		State {
			name: "on_state"

			PropertyChanges {
				target: rectangle1
				color: "#00ff00"
			}
		},
		State {
			name: "error_state"
			PropertyChanges {
				target: rectangle1
				color: "#ff0000"
			}
		},
		State {
			name: "off_state"
			PropertyChanges {
				target: rectangle1
				color: "#002d00"
			}
		}

	]
}
