3D object creation from handwriting contours
====

You can create 3D object from a handwriting contours picutre.

## Demo

## Usage
Please compile this 
```
$ g++ -o 3d_create 3d_create.cpp `pkg-config opencv ‒cflags` `pkg-config opencv ‒ libs` -lglut -lGLU -lGL -lm
```
Then if you have a picture named "contour1.JPG", please 
```
$ ./3d_create contour1.JPG
```
