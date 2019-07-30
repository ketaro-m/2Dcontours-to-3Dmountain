3D object creation from handwriting contours
====

You can create 3D object from a handwriting contours picutre.

## Demo
![contour1](https://user-images.githubusercontent.com/52503908/62114039-bf20be00-b2f0-11e9-8c10-0338291a3f7c.JPG)

## Usage
Please compile "3d_create.cpp" like
```
$ g++ -o 3d_create 3d_create.cpp `pkg-config opencv ‒cflags` `pkg-config opencv ‒ libs` -lglut -lGLU -lGL -lm
```
Then if you have a picture named "contour1.JPG", please 
```
$ ./3d_create contour1.JPG
```
