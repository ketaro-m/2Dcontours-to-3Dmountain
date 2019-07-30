3D object creation from handwriting contours
====

You can create 3D object from a handwriting contours picutre.

## Demo
<img src="https://github.com/ketaro-m/3D-object-creation-from-handwriting-contours/blob/master/img/demo.gif" width="640px">

## Usage
Please compile "3d_create.cpp" like
```
$ g++ -o 3d_create 3d_create.cpp `pkg-config opencv ‒cflags` `pkg-config opencv ‒ libs` -lglut -lGLU -lGL -lm
```
Then if you have a picture named "contour1.JPG", please
```
$ ./3d_create contour1.JPG
```
