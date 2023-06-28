/*******************************************************************************
 * Copyright 2018 -- 2023 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <stdarg.h>
#include <limits.h>

#include <mpi.h>

#include "kmeans.hpp"


#ifdef DEBUG
#include <sys/time.h>
typedef unsigned long long timestamp_t;

timestamp_t t0, t1;

static timestamp_t get_timestamp ()
{
  struct timeval now;
  gettimeofday (&now, NULL);
  return  now.tv_usec + (timestamp_t)now.tv_sec * 1000000;
}
#endif


//// Distance**2 between d-vectors pointed to by v1, v2.
//float distance2(const float *v1, const float *v2, const int d) {
//  float dist = 0.0;
//  for (int i=0; i<d; i++) {
//    float diff = v1[i] - v2[i];
//    dist += diff * diff;
//  }
//  return dist;
//}

// simplified Distance**2 between 3D-vectors pointed to by v1, v2 for int arithmetic
int64_t my_distance2(int32_t *v1, int32_t *v2)
{
   int64_t diff = 0;
   int64_t dist = 0;
   int64_t diff2 = 0;

   for(int j = 0; j < VECTOR_DIM; j++)
   {
      diff = (int64_t) (v1[j] - v2[j]);
      diff2 = (int64_t) (diff * diff);
      dist += diff2;
   }

  //dist = dist >> SCALE_FACTOR;
  return (int64_t) dist;
}

// Assign a vector to the correct cluster by computing its distances to
// each cluster centroid.
int assign_vector(int32_t *vector, int centroids[MAX_CENTROIDS * VECTOR_DIM], int k)
{
  int best_cluster = 0;
  int64_t best_dist = LONG_MAX;
  for (int c = 0; c < k; c++)
  {
    int64_t dist = my_distance2(vector, &centroids[c * VECTOR_DIM]);
    //printf("dist: %+013lld\n", (long long int) dist);
    if (dist < best_dist) 
    {
      best_cluster = c;
      best_dist = dist;
    }
  }
  return best_cluster;
}


// Add a vector into a sum of vectors.
void add_vector(int32_t *vector, int32_t *sum)
{
  for(int j = 0; j<VECTOR_DIM; j++)
  {
    sum[j] += vector[j];
  }
}

void div_vector(int32_t *vector, int32_t dividend)
{
  if(dividend != 0)
  {
    for(int j = 0; j<VECTOR_DIM; j++)
    {
      vector[j] /= dividend;
    }
  }
}

// Print the centroids one per line.
void print_centroids(int32_t *centroids, int k) {
  //printf("Centroids:\n");
  for (int i = 0; i < k*VECTOR_DIM; i += VECTOR_DIM)
  {
    for(int j = 0; j<VECTOR_DIM; j++)
    {
      printf("%+013d ", centroids[i+j]);
    }
    printf("\n");
  }
}

void reading_vectors(FILE* file_stream, int32_t *all_vectors, int nr_vectors)
{
  char line[256];
  int li = 0;
#ifdef DEBUG
  printf("expecting %d dimensions per line\n", VECTOR_DIM);
#endif
  while (fgets(line, 256, file_stream) && li < nr_vectors)
  {
    char* tok;
    int32_t var[VECTOR_DIM];
    //for (tok = strtok(line, ";"); tok && *tok; tok = strtok(NULL, " ;\n"))
    //we now how the file is formated
    tok = strtok(line, " ");
    for (int i = 0; i < VECTOR_DIM; i++)
    {
      int itok = atoi(tok);
      //printf("processing tok %d\n",itok);
      var[i] = itok;
      tok = strtok(NULL, " ");
      all_vectors[li*VECTOR_DIM + i] = var[i];
    }
    //printf("read line [%04d]: %+013d %+013d %+013d\n", li, var[0], var[1], var[2]);
    li++;
  }
}

int main(int argc, char** argv) 
{


  // Initial MPI and find process rank and number of processes.
  MPI_Init(NULL, NULL);
  int rank, nprocs;
  MPI_Status status;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  //int vectors_per_proc = ceil((float) / (float) nprocs);
  //int vectors_per_proc = (NR_VECTORS + (nprocs-1))/nprocs;
#ifdef DEBUG
  if(NR_VECTORS % nprocs != 0)
  {
    //for now, we need even processes
    printf("ERROR: Vectors could not be split evenely (%d/%d) (this is a simple demo)!\n", NR_VECTORS, nprocs);
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
#endif
  int vectors_per_proc = NR_VECTORS/nprocs;
  int k = K_STATIC;

  // Data structures in all processes.
  // Vectors assigned for this process
  int32_t *vectors, *labels;
  vectors = (int32_t*) malloc(vectors_per_proc * sizeof(int32_t) * VECTOR_DIM);
  if(vectors == NULL)
  {
    perror("malloc");
    exit(1);
  }

  // The cluster assignments for each site.
  labels = (int32_t*) malloc(vectors_per_proc * sizeof(int32_t));
  if(labels == NULL)
  {
    perror("malloc");
    exit(1);
  }

  // The sum of vectors assigned to each cluster by this process.
  // k vectors of d elements.
  int32_t sums[MAX_CENTROIDS * VECTOR_DIM];

  // The number of vectors assigned to each cluster by this process. k integers.
  int counts[MAX_CENTROIDS];

  // The current centroids against which vectors are being compared.
  // These are shipped to the process by the root process.
  int32_t centroids[MAX_CENTROIDS * VECTOR_DIM];


  //
  // Data structures maintained only in root process.
  //
  // All the sites for all the processes.
  // site_per_proc * nprocs vectors of d floats.
  int32_t *all_vectors = NULL;
  // Sum of sites assigned to each cluster by all processes.
  int32_t *all_sums = NULL;
  // Number of sites assigned to each cluster by all processes.
  int32_t *all_counts = NULL;
  // Result of program: a cluster label for each site.
  int *all_labels = NULL;;

  if(rank == 0)
  {

    if (argc != 2) {
      fprintf(stderr,
          "Usage: %s <path-to-point-file>\n",argv[0]);
      exit(1);
    }

    //read file
    FILE* file_stream = fopen(argv[1], "r");
    char line[257];
    //read N
    fgets(line, 256, file_stream);
    int nr_vectors = atoi(line);
    if(NR_VECTORS != nr_vectors)
    {
      fprintf(stderr, "This binary is compiled for another number of vectors: %d. File contains %d.\n", NR_VECTORS, nr_vectors);
      exit(1);
    }
    // read k
    fgets(line, 256, file_stream);
    int file_k = atoi(line);
    //if(k > MAX_CENTROIDS)
    if(file_k != K_STATIC)
    {
      //fprintf(stderr, "This binary is compiled for a maximum number of centroids: %d. File contains %d.\n", MAX_CENTROIDS, k);
      fprintf(stderr, "This binary is compiled for a number of centroids: %d. File contains %d.\n", MAX_CENTROIDS, k);
      exit(1);
    }

    printf("Number of vectors: %d; maximum per proc: %d\n", nr_vectors, vectors_per_proc);

    all_vectors = (int32_t*) malloc(nr_vectors * sizeof(int32_t) * VECTOR_DIM);
    if(all_vectors == NULL)
    {
      perror("malloc");
      exit(1);
    }
   
    reading_vectors(file_stream, all_vectors, nr_vectors);
    fclose(file_stream);

    // Take evenly distributed vectors as the initial cluster centroids.
    int step = nr_vectors/k;
    for (int i = 0; i < k * VECTOR_DIM; i += VECTOR_DIM)
    {
      for(int j = 0; j<VECTOR_DIM; j++)
      {
        centroids[i+j] = all_vectors[i*step + j];
      }
    }
    printf("finished reading file...\n");
//#ifdef PRINTALL
//    print_centroids(centroids, k);
//#endif
    all_sums = (int32_t*) malloc(k * sizeof(int32_t) * VECTOR_DIM);
    if(all_sums == NULL)
    {
      perror("malloc");
      exit(1);
    }

    all_counts = (int32_t*) malloc(k * sizeof(int32_t));
    if(all_counts == NULL)
    {
      perror("malloc");
      exit(1);
    }

    all_labels = (int32_t*) malloc(nr_vectors * sizeof(int32_t));
    if(all_labels == NULL)
    {
      perror("malloc");
      exit(1);
    }
#ifdef DEBUG
    t0 = get_timestamp();
#endif
  }

  ////distribute k
  //if(rank == 0)
  //{
  //  for(int i = 1; i<nprocs; i++)
  //  {
  //    MPI_Send(&k, 1, MPI_INTEGER, i, 0, MPI_COMM_WORLD);
  //  }
  //  printf("Distributed k: %d\n",k);
  //} else {
  //  MPI_Recv(&k, 1, MPI_INTEGER, 0, 0, MPI_COMM_WORLD, &status); 
  //}

  //printf("[%d] k: %d\n", rank, k);

  // Root sends each process its share of sites.
  MPI_Scatter(all_vectors, vectors_per_proc*VECTOR_DIM, MPI_INTEGER, vectors, vectors_per_proc*VECTOR_DIM, MPI_INTEGER, 0, MPI_COMM_WORLD);


  int norm = INT_MAX;  // Will tell us if centroids have moved.
  int iteration = 0;

  while(norm > NORM_LOWER_LIMIT && iteration < MAX_ITERATIONS)
  { // While they've moved...

    // Each process reinitializes its cluster accumulators.
    for (int i = 0; i < k*VECTOR_DIM; i += VECTOR_DIM)
    {
      for(int j = 0; j < VECTOR_DIM; j++)
      {
        sums[i+j] = 0;
      }
      counts[i] = 0;
    }

    // Broadcast the current cluster centroids to all processes.
    MPI_Bcast(centroids, k*VECTOR_DIM, MPI_INTEGER, 0, MPI_COMM_WORLD);

    // Find the closest centroid to each site and assign to cluster.
    for (int i = 0; i < vectors_per_proc; i++)
    {
      int cluster = assign_vector(&vectors[i*VECTOR_DIM], centroids, k);
      // Record the assignment of the site to the cluster.
      counts[cluster]++;
      add_vector(&vectors[i*VECTOR_DIM], &sums[cluster*VECTOR_DIM]);
    }

#ifdef PRINTALL
    if(rank == 0)
    {
      printf("root sums:\n");
      print_centroids(sums, k);
    }
#endif

    // Gather and sum at root all cluster sums for individual processes.
    MPI_Reduce(sums, all_sums, k*VECTOR_DIM, MPI_INTEGER, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(counts, all_counts, k, MPI_INTEGER, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
#ifdef PRINTALL
      printf("all sums:\n");
      print_centroids(all_sums, k);
      printf("all counts:\n");
      for(int i = 0; i<k; i++)
      {
        printf("   %03d:   %d\n", i, all_counts[i]);
      }
#endif
      // Root process computes new centroids by dividing sums per cluster
      // by count per cluster.
      for (int i = 0; i<k; i++)
      {
        div_vector(&all_sums[i*VECTOR_DIM], all_counts[i]);
      }
      printf("after div\n");
      // Have the centroids changed much?
      //norm = distance2(grand_sums, centroids, vector_dim*k);
      norm = 0;
      int64_t norm_tmp = 0;
      for (int i = 0; i<k; i++)
      {
        norm_tmp += my_distance2(&all_sums[i*VECTOR_DIM], &centroids[i*VECTOR_DIM]);
      }
      if(norm_tmp >= INT_MAX || norm_tmp < 0)
      {
        norm = INT_MAX;
      } else {
        norm = (int32_t) norm_tmp;
      }
      printf("[%04d] norm: %d\n", iteration, norm);
      // Copy new centroids from grand_sums into centroids.
      for (int i = 0; i<k*VECTOR_DIM; i+=VECTOR_DIM)
      {
        for(int j = 0; j < VECTOR_DIM; j++)
        {
          centroids[i+j] = all_sums[i+j];
        }
      }
#ifdef PRINTALL
      printf("current centroids: \n");
      print_centroids(centroids, k);
#endif
    }
    // Broadcast the norm.  All processes will use this in the loop test.
    MPI_Bcast(&norm, 1, MPI_INTEGER, 0, MPI_COMM_WORLD);
    iteration++;
  }

  // Now centroids are fixed, so compute a final label for each site.
  for (int i = 0; i < vectors_per_proc; i++)
  {
    labels[i] = assign_vector(&vectors[i*VECTOR_DIM], centroids, k);
  }

  // Gather all labels into root process.
  MPI_Gather(labels, vectors_per_proc, MPI_INTEGER, all_labels, vectors_per_proc, MPI_INTEGER, 0, MPI_COMM_WORLD);

  // Root can print out all labels and labels.
  if (rank == 0)
  {
    if(iteration == MAX_ITERATIONS)
    {
      printf("...max iterations reached!\n");
    } else {
      printf("...converted (critearia: norm < %d)!\n", NORM_LOWER_LIMIT);
    }
    //printf("Results:\n    Vector_Nr\t   Label\n");
    //for (int i = 0; i < nprocs * vectors_per_proc; i++) 
    //{
    //  printf("\t%d \t   %4d\n",i, all_labels[i]);
    //}
    printf("Final counts:\n");
    for(int i = 0; i<k; i++)
    {
      printf("   %03d:   %d\n", i, all_counts[i]);
    }
    //printf("Final centroids:\n");
    //print_centroids(centroids_x, centroids_y, centroids_z,k);
#ifdef DEBUG 
    t1 = get_timestamp();
    double secs = (t1 - t0) / 1000000.0L;
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>\tMPI C runtime: %lfs\n", secs);
#endif

  free(all_vectors);
  free(all_sums);
  free(all_counts);
  free(all_labels);
  }

  free(vectors);
  free(labels);
  MPI_Finalize();

}


