# CAP-Practica2-RayTracing
Paralelización de un programa de RayTracing con OpenMP y MPI

g++ Crystalline.cpp Metallic.cpp random.cpp Scene.cpp Sphere.cpp utils.cpp main.cpp -o main.exe

g++ Crystalline.cpp Metallic.cpp random.cpp Scene.cpp Sphere.cpp utils.cpp main.cpp -o main.exe -O2

mpiCC Crystalline.cpp Metallic.cpp random.cpp Scene.cpp Sphere.cpp utils.cpp main.cpp -o main.exe -O2 -fopenmp

+--mpirun -np 4 ./main.exe
|
or
|
+--mpirun --use-hwthread-cpus -np 8 ./main.exe
|
or
|
+--mpirun --oversubscribe --hostfile ../myhostfile --use-hwthread-cpus -np 16 ./main.exe