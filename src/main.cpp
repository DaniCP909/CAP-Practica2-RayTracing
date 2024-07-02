//==================================================================================================
// Written in 2016 by Peter Shirley <ptrshrl@gmail.com>
//
// To the extent possible under law, the author(s) have dedicated all copyright and related and
// neighboring rights to this software to the public domain worldwide. This software is distributed
// without any warranty.
//
// You should have received a copy (see file COPYING.txt) of the CC0 Public Domain Dedication along
// with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
//==================================================================================================

#include <float.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <limits>

#include "Camera.h"
#include "Object.h"
#include "Scene.h"
#include "Sphere.h"
#include "Diffuse.h"
#include "Metallic.h"
#include "Crystalline.h"

#include <iomanip>

#include "omp.h"
#include "mpi.h"

#include "random.h"
#include "utils.h"

// ->- MPI ->-
//
#define NPROCS 8
#define NFRAMES 10

// -*- OpenMP -*-
//mejor resultado: 4
int nThreads = 2;

Scene randomScene() {
	int n = 500;
	Scene list;
	list.add(new Object(
		new Sphere(Vec3(0, -1000, 0), 1000),
		new Diffuse(Vec3(0.5, 0.5, 0.5))
	));

	for (int a = -11; a < 11; a++) {
		for (int b = -11; b < 11; b++) {
			float choose_mat = randomCap();
			Vec3 center(a + 0.9f * randomCap(), 0.2f, b + 0.9f * randomCap());
			if ((center - Vec3(4, 0.2f, 0)).length() > 0.9f) {
				if (choose_mat < 0.8f) {  // diffuse
					list.add(new Object(
						new Sphere(center, 0.2f),
						new Diffuse(Vec3(randomCap() * randomCap(),
							randomCap() * randomCap(),
							randomCap() * randomCap()))
					));
				}
				else if (choose_mat < 0.95f) { // metal
					list.add(new Object(
						new Sphere(center, 0.2f),
						new Metallic(Vec3(0.5f * (1 + randomCap()),
							0.5f * (1 + randomCap()),
							0.5f * (1 + randomCap())),
							0.5f * randomCap())
					));
				}
				else {  // glass
					list.add(new Object(
						new Sphere(center, 0.2f),
						new Crystalline(1.5f)
					));
				}
			}
		}
	}

	list.add(new Object(
		new Sphere(Vec3(0, 1, 0), 1.0),
		new Crystalline(1.5f)
	));
	list.add(new Object(
		new Sphere(Vec3(-4, 1, 0), 1.0f),
		new Diffuse(Vec3(0.4f, 0.2f, 0.1f))
	));
	list.add(new Object(
		new Sphere(Vec3(4, 1, 0), 1.0f),
		new Metallic(Vec3(0.7f, 0.6f, 0.5f), 0.0f)
	));

	return list;
}

void rayTracingCPU(unsigned char* img, int w, int h, int ns = 10, int px = 0, int py = 0, int pw = -1, int ph = -1) {
	if (pw == -1) pw = w;
	if (ph == -1) ph = h;
	int patch_w = pw - px;
	Scene world = randomScene();
	world.setSkyColor(Vec3(0.5f, 0.7f, 1.0f));
	world.setInfColor(Vec3(1.0f, 1.0f, 1.0f));

	Vec3 lookfrom(13, 2, 3);
	Vec3 lookat(0, 0, 0);
	float dist_to_focus = 10.0;
	float aperture = 0.1f;

	Camera cam(lookfrom, lookat, Vec3(0, 1, 0), 20, float(w) / float(h), aperture, dist_to_focus);

	omp_set_num_threads(nThreads);
	#pragma omp parallel 
	
	#pragma omp for collapse(2)
	for (int j = 0; j < (ph - py); j++) {
		for (int i = 0; i < (pw - px); i++) {
			//if(i == 0 && j == 0) std::cout << "[OMP] num threads: " << omp_get_num_threads() << std::endl; 
			
			Vec3 col(0, 0, 0);
			for (int s = 0; s < ns; s++) {
				float u = float(i + px + randomCap()) / float(w);
				float v = float(j + py + randomCap()) / float(h);
				Ray r = cam.get_ray(u, v);
				col += world.getSceneColor(r);
			}
			col /= float(ns);
			col = Vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));

			img[(j * patch_w + i) * 3 + 2] = char(255.99 * col[0]);
			img[(j * patch_w + i) * 3 + 1] = char(255.99 * col[1]);
			img[(j * patch_w + i) * 3 + 0] = char(255.99 * col[2]);
		}
	}
}

int main() {
	MPI_Init(NULL, NULL);
	int rank, wrld_size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &wrld_size);

	int w = 1200;// 1200;
	int h = 800;// 800;
	int ns = 10;

	double patch_offset = 1.0/ (NPROCS);
	int patch_x_size = w;
	int patch_y_size = h * patch_offset;
	double patch_x_idx = 1;
	double patch_y_idx = patch_offset * (rank % (NPROCS));
	std::cout << "[" << rank << "] patch_x_size " << patch_x_size << " , patch_y_size " << patch_y_size << ", patch_x_idx " << patch_x_idx << ", patch_y_idx " << patch_y_idx << std::endl;

	double t0, t1;
	double elapsed;
	double t_film = 0.0;

	int seed = 0;

	int size = sizeof(unsigned char) * (patch_x_size) * (patch_y_size) * 3;
	unsigned char* data = (unsigned char*)calloc(size, 1);
	unsigned char* full_data;
	int full_size = size * NPROCS;

	if(rank == 0){
		full_data = (unsigned char*)calloc(full_size, 1);
	}

	int patch_x_start = 0;
	int patch_x_end = w;
	int patch_y_start = (h * patch_y_idx);
	int patch_y_end = (h * patch_y_idx) + patch_y_size;
	std::cout << "[" << rank << "] " << "patch_offset: " << patch_offset << " | "<< patch_x_start << ", " << patch_x_end << ", " << patch_y_start << ", " << patch_y_end << " ///" << std::endl; 

	for(int frame = 0; frame < NFRAMES; frame++){
		if(rank == 0){
			seed = (int)(randomCap() * 100);
		}
		MPI_Bcast(&seed, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
		if(rank == 0) t0 = MPI_Wtime();
		std::cout << "-------[" << rank << "]------SEED--->" << seed << std::endl;
		srand(seed);

		rayTracingCPU(data, w, h, ns, patch_x_start, patch_y_start, patch_x_end, patch_y_end);

		MPI_Gather(data, size, MPI_UNSIGNED_CHAR, full_data, size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

		MPI_Barrier(MPI_COMM_WORLD); //only for measure time
		if(rank == 0){
			t1 = MPI_Wtime();
			std::string file_name = "../images/frame" + std::to_string(frame) +".bmp";
			writeBMP(file_name.c_str(), full_data, (w), (h));
			printf("Imagen creada.\n");
			elapsed = (t1 - t0);
			std::cout << "Tiempo transcurrido: " << std::fixed << std::setprecision(9) << elapsed << "s" << std::endl;
			t_film += elapsed;
			if(frame == (NFRAMES - 1)) {
				std::cout << "[FINAL] Tiempo TOTAL pelicula: " << std::fixed << std::setprecision(9) << t_film << "s" << " NPROCS = " << NPROCS << " | nThreads = " << nThreads << std::endl;
				free(full_data);
				t_film = 0.0;
			}
		}
	}
	free(data);
	MPI_Finalize();
	return (0);
}
