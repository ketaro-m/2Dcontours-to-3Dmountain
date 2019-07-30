3D object creation from handwriting contours
====

## Description
You can learn how the contour map and 3D object are correspond by your **ORIGINAL** handwriting picture.  
This program recognizes the contours, creates a 3D object from it and add colors corresponding to the hight.

Your steps are only ...  
・Draw a contour map.  
・Take a picture of it.  
・Run this program on your PC.

## Demo
<img src="https://github.com/ketaro-m/3D-object-creation-from-handwriting-contours/blob/master/img/demo.gif" width="320px">

## Usage
Please compile "3d_create.cpp" like
```
$ g++ -o 3d_create 3d_create.cpp `pkg-config opencv ‒cflags` `pkg-config opencv ‒ libs` -lglut -lGLU -lGL -lm
```
Then if you have a picture named "contour1.JPG" (as below),  
<img src="https://github.com/ketaro-m/3D-object-creation-from-handwriting-contours/blob/master/img/contour1.JPG" width="320px">

please run
```
$ ./3d_create contour1.JPG
```
Then you can see the 3D object like this!  
<img src="https://github.com/ketaro-m/3D-object-creation-from-handwriting-contours/blob/master/img/contour1to3d.png" width="320px">


 
### <Additional Functions>
・Change image types on the left window by clicking ...  
    scroll wheel - original handwriting picture  
    right button - contours image  
    left  button - colored contours image  

・Look at 3D object from various angles by dragging it with the left button.

・Change the size of 3D Object by dragging it with the right button.

・Change the color range by typing  
    "r" on keyboard - narrow the range  
    "b" on keyboard - widen the range

・Change the mode of 3D object by typing  
    "d" on keyboard - "default mode"  
    "s" on keyboard - "smooth mode" (the slope will be smoother)
