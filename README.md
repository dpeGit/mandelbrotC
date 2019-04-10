# mandelbrotC

My fractal program ported to C

# Dependacies
## TinyPngOut
https://www.nayuki.io/page/tiny-png-output
Used to write the png out

# To Compile
At the moment this program is designed to be recompiled on settings changes
I don't know if this will change in the future since it is a small program
## A note on memory
Since this program allocates the array of iterations (floats) at compile time.
The minimium memory requirements will be dominated by 4 * imageHeight * imageWidth bytes.
This is a small enough footprint for most purposes so don't worry about it.
However if you allocate more memory then your system has it will immedately segfault.
But and issue arises when you allocate nearly all your memory. C only reports memory usage as it writes to the array.
So the program memory usage will slowly increase over time. If the system runs out of memory then it will crash.
Meaning, if the program needs 7GB to run, and your system has 8GB (total including swap space). And you are currently using 2GB base.
You will crash. So give the program a little breathing room.

# The Roadmap
I plan on adding several features over time. Caching progress to the disk, threading, gpu computing cheifly amoung them for now.
Starting at version 2.0 I will be maintaining 2 version. One that uses the database Yottadb for disk caching,
and another that writes to the disk it's self. I anticipate that the database version will be faster however will require
it to be installed.
After these features are done I will add zoom fucntionality

# TODO

## 1.1
Add iterative colouring

## 2.0a/b
Yottadb disk caching
Regular disk caching

## 3.0
cpu multi-threading

## 4.0
gpu computing

## 5.0
Produce zoom videos
