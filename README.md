# Spin Coater

## Functionality
### Major
	- Substrate dimensions up to 3 inch (76.2mm)
	- Rotation speed up to 6k RPM at least, 8k RPM preferable
	- Vacuum pump included in enclosure
	- Enclosure max diamter 240mm

### Minor
	- Automatic photoresist dosing system
	- Communication with external master device (probably by CAN)
	- Automatic opening/closing of lid

## Mechanical
### Major
	- Add rotor and splash shield

### Minor
	- Decide whether to add backup files to git ignore
	- Remove stray .STP files
	- Move .STP files to proper locations
	- Decide wheter to consolidate multiple parts into 1 .FCstd file
	- Fix parametric pulley relative geometry problem when changing parameters
	- Find better names for files and parts
	- Add belt tensioning system

## Programming
### Major
	- program MCU to control rotation speed (DC motor version)
		- Read RPM by hall sensor
		- Read user input by potentiometer
		- Set rotation speed by PWM
		- Display current speed in RPM on display

### Minor
	- One-click .STEP export

## Electronic
### Major
	-

### Minor
	-
