# mandelbrotC

My fractal program ported to C

# Dependacies
##tinypngout
https://www.nayuki.io/page/tiny-png-output
Used to write the png out

# To Compile
At the moment this program is designed to be recompiled on settings changes
I don't know if this will change in the future since it is a small program

# The Roadmap
I plan on adding several features over time. Caching progress to the disk, threading, gpu computing cheifly amoung them for now.
Starting at version 2.0 I will be maintaining 2 version. One that uses the database Yottadb for disk caching
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

# Known Bugs
Segfualt on large images where none should happen. Issue is likely with the TinyPngOut image data being created on the stack
