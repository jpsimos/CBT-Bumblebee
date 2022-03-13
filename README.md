# CBT-Bumblebee
This project was my attempt at enhancing the experience of my vehicle in college and to gain experience with embedded processors.
It runs on the CanBusTriple (8-bit). 

Objectives:
	1. To poll both vehicle busses exposed at the OBD-II port.
	2. Make standard OBD-II service requests for some vehicle data.
	3. Interpret non-standard OBD-II CAN data to usable vehicle data in almost real time.
	4. Encode vehicle data and update the driver information center with formatted vehicle data.
	5. Initiate device control sessions with the body control module and transmission control module. 
	6. With the addition of two momentary contact buttons on the shifter,
	    I should be able to change what vehicle data is shown on the DIC,
		enable a diagnostic session with the transmission control module to allow shifting
		the gear up or down while in drive.
	7. Dispatch steering wheel button commands over CAN, like on the fully loaded vehicle variant.
	8. Only operate all project features if the vehicle reports the correct VIN.
	9. Responsibly operate on both CAN busses, applicable project features use as-needed trigger logic to minimize traffic.
	10. Practice writing C/C++ without dynamic memory allocation and limited RAM. (The mess is not an objective.)
	11. Removable and non-invasive connection to the vehicle's wiring.
	
Overall, development was a fun learning experience. Code design was not the primary concern, but smooth and bug-free operation
was the idea. The programmed device could remain connected to the vehicle for months without crashing, freezing, or rebooting.

Please excuse misspelled words and random comments - I've left them intact to preserve history.

Remember, safety is priority.

-Jacob
