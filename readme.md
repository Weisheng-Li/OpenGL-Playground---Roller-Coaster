Author: Weisheng Li
email: wjl5238@psu.edu

- How to run the project?
start: double click project_2.exe to run the program.
control:
	x - toggle the ontrack flag.
	other control keys is the same as starter code

- Where each part is located?

track: 
all code related to the track is on track.cpp. The function
I added or completed includes:
-get_points() line 93
-interpolate() line 134
-create_track() line 187
-make_face() line 262
-makeRailPart() line 294
-makePlank() line 353

camera movement:
I simply completed the ProcessTrackMovement function in camera.cpp
The physically accurate speed is also calculate in this part
-ProcessTrackMovement() line 86

cart on the rail:
The cart is drawn in the main file,
it gets transformed to the camera position on the track
by a function in camera.cpp
-getCartTrans() line 184

Main file (Project2.cpp):
- line 490  toggle onTrack flag 
- line 379	draw cart on the rail 


- A list of everything contained in the submission
- Project_2
	-media
	-Headers
	-Shaders
	-Sources
	-Project_2.exe
- video.mp4
- Readme.md

- Extra Credits part:
1. cart on the rail
Initially there is not cart on the rail. The cart will
show up and start running after hitting X once until the
program was closed down

2. super wild track
I use completely random points to generate the rail, and
it requires several modifications in order to make such
wild track to work. 

first, it's even more difficult to make the beginning and
the end match. I use a linear interpolation for both the 
camera and the rail near the end to accomplish a smooth
transition from the end to the beginning. (see line 238 in track.hpp)

second, based on the architecture given in the starter code,
camera and track traverse the spline independently, so when the
step size is not exactly equal, the normal and the right vectors
of these two may start to deviate from each other, especially
when there are many irregular turns on the track. I then calculate 
a set of constant to make sure the step size of these
two would stay close to each other. Aside from that, I also
use a method to make the orientation of track and camera match
at the end of each loop so that the difference wouldn't keep
getting larger.


