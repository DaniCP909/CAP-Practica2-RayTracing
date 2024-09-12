# RayTracing OpenMP + MPI

Thi project was made to test both Shared Memory and Distributed Memory Models I a C++ Ray Tracing program.
In that program, a certain number of frames in a specified resolution, are rendered using Ray Tracing. 
https://github.com/RayTracing

There are three solutions provided in this project, divided in different branches:
  - Main: solution to explore best configuration, in a specific terminal, to use OpenMP in some loops of the algorithm.
    *next explored solutions keep OpenMP usage.
  - 1frameNprocs: first explored path dividing each frame in equal portions sent to each process with MPI.
  - NframesMprocs: sends one full frame to each process.



g++ Crystalline.cpp Metallic.cpp random.cpp Scene.cpp Sphere.cpp utils.cpp main.cpp -o main.exe

g++ Crystalline.cpp Metallic.cpp random.cpp Scene.cpp Sphere.cpp utils.cpp main.cpp -o main.exe -O2


