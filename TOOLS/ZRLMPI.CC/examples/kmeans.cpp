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


//
//struct My_vector<D> {
//  int32_t n[D];
//}
//


#define VDIM 3
typedef struct Vector3D {
  int32_t x;
  int32_t y;
  int32_t z;
} Vector3D;


//// Distance**2 between d-vectors pointed to by v1, v2.
//float distance2(const float *v1, const float *v2, const int d) {
//  float dist = 0.0;
//  for (int i=0; i<d; i++) {
//    float diff = v1[i] - v2[i];
//    dist += diff * diff;
//  }
//  return dist;
//}

// Distance**2 between 3D-vectors pointed to by v1, v2.
uint32_t my_distance2(const Vector3D *v1, const Vector3D *v2)
{
  int64_t dist = 0;
  int64_t diff = 0;
  int64_t diff2 = 0;

  diff = (v1->x - v2->x) / SCALE_FACTOR;
  diff2 = (diff * diff);
  if (diff2 >= UINT_MAX)
  {
    return UINT_MAX;
  }
  dist += diff2;

  diff = (v1 -> y - v2 ->y) / SCALE_FACTOR;
  diff2 = (diff * diff);
  if (diff2 >= UINT_MAX)
  {
    return UINT_MAX;
  }
  dist += diff2;

  diff = (v1->z - v2->z) / SCALE_FACTOR;
  diff2 = (diff * diff);
  if (diff2 >= UINT_MAX)
  {
    return UINT_MAX;
  }
  dist += diff2;

#ifdef ZRLMPI_SW_ONLY
  if(dist < 0)
  {
    printf("DISTANCE < 0\n");
    printf("V1:\n");
    printf("%+013d ",  v1->x);
    printf("%+013d ",  v1->y);
    printf("%+013d\n", v1->z);
    printf("V2:\n");
    printf("%+013d ",  v2->x);
    printf("%+013d ",  v2->y);
    printf("%+013d\n", v2->z);
    printf("dist: %+013lld\n", (long long int) dist);
    exit(42);
  }
#endif
  if (dist >= UINT_MAX)
  {
    dist = UINT_MAX;
  }
  return (uint32_t) dist;
}

// Assign a vector to the correct cluster by computing its distances to
// each cluster centroid.
int assign_vector(const Vector3D *vector, Vector3D centroids[MAX_CENTROIDS], const int k)
{
  int best_cluster = 0;
  uint32_t best_dist = my_distance2(vector, &centroids[0]);
  for (int c = 1; c < k; c++)
  {
    uint32_t dist = my_distance2(vector, &centroids[c]);
    if (dist < best_dist) {
      best_cluster = c;
      best_dist = dist;
    }
  }
  return best_cluster;
}


// Add a vector into a sum of vectors.
void add_vector(const Vector3D *vector, Vector3D *sum)
{
  sum->x += vector->x;
  sum->y += vector->y;
  sum->z += vector->z;
}

void div_vector(Vector3D *vector, int32_t dividend)
{
  if(dividend != 0)
  {
    vector->x /= dividend;
    vector->y /= dividend;
    vector->z /= dividend;
  }
}

// Print the centroids one per line.
void print_centroids(const Vector3D *centroids, const int k) {
  //printf("Centroids:\n");
  for (int i = 0; i<k; i++) 
  {
    printf("%+013d ", centroids[i].x);
    printf("%+013d ", centroids[i].y);
    printf("%+013d ", centroids[i].z);
    printf("\n");
  }
}

void reading_vectors3D(FILE* file_stream, Vector3D *all_vectors, int nr_vectors)
{
  char line[256];
  int li = 0;
  while (fgets(line, 256, file_stream) && li < nr_vectors)
  {
    char* tok;
    int32_t var[3];
    //for (tok = strtok(line, ";"); tok && *tok; tok = strtok(NULL, " ;\n"))
    //we now how the file is formated
    tok = strtok(line, " ");
    for (int i = 0; i < 3; i++)
    {
      int itok = atoi(tok);
      //printf("processing tok %d\n",itok);
      var[i] = itok;
      tok = strtok(NULL, " ");
    }
    //printf("read line [%04d]: %+013d %+013d %+013d\n", li, var[0], var[1], var[2]);
    all_vectors[li] = (Vector3D) { .x = var[0], .y = var[1], .z = var[2] };
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
  Vector3D *vectors;
  vectors = (Vector3D*) malloc(vectors_per_proc * sizeof(Vector3D));
  if(vectors == NULL)
  {
    perror("malloc");
    exit(1);
  }

  // The sum of vectors assigned to each cluster by this process.
  // k vectors of d elements.
  Vector3D sums[MAX_CENTROIDS];

  // The number of vectors assigned to each cluster by this process. k integers.
  int counts[MAX_CENTROIDS];

  // The current centroids against which vectors are being compared.
  // These are shipped to the process by the root process.
  Vector3D centroids[MAX_CENTROIDS];

  // The cluster assignments for each site.
  int labels[vectors_per_proc];

  //
  // Data structures maintained only in root process.
  //
  // All the sites for all the processes.
  // site_per_proc * nprocs vectors of d floats.
  Vector3D *all_vectors = NULL;
  // Sum of sites assigned to each cluster by all processes.
  Vector3D *grand_sums = NULL;
  // Number of sites assigned to each cluster by all processes.
  int32_t *grand_counts = NULL;
  // Result of program: a cluster label for each site.
  int *all_labels;

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

    all_vectors = (Vector3D*) malloc(nr_vectors * sizeof(Vector3D));
    if(all_vectors == NULL)
    {
      perror("malloc");
      exit(1);
    }

    reading_vectors3D(file_stream, all_vectors, nr_vectors);
    fclose(file_stream);

    // Take evenly distributed vectors as the initial cluster centroids.
    int step = nr_vectors/k;
    for (int i = 0; i < k; i++)
    {
      centroids[i] = all_vectors[i*step];
    }
    printf("finished reading file...\n");
#ifdef PRINTALL
    print_centroids(centroids, k);
#endif
    grand_sums = (Vector3D*) malloc(k * sizeof(Vector3D));
    if(grand_sums == NULL)
    {
      perror("malloc");
      exit(1);
    }

    grand_counts = (int32_t*) malloc(k * sizeof(int32_t));
    if(grand_counts == NULL)
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
  MPI_Scatter((int32_t*) all_vectors, VDIM*vectors_per_proc, MPI_INTEGER, (int32_t*) vectors, VDIM*vectors_per_proc, MPI_INTEGER, 0, MPI_COMM_WORLD);


  uint32_t norm = INT_MAX;  // Will tell us if centroids have moved.
  int iteration = 0;

  while(norm > NORM_LOWER_LIMIT && iteration < MAX_ITERATIONS)
  { // While they've moved...

    // Broadcast the current cluster centroids to all processes.
    MPI_Bcast((int32_t*) centroids, k*VDIM, MPI_INTEGER, 0, MPI_COMM_WORLD);

    // Each process reinitializes its cluster accumulators.
    for (int i = 0; i < k; i++)
    {
      sums[i] = (Vector3D) {.x = 0, .y = 0, .z = 0};
    }
    for (int i = 0; i < k; i++)
    {
      counts[i] = 0;
    }

    // Find the closest centroid to each site and assign to cluster.
    for (int i = 0; i < vectors_per_proc; i++)
    {
      int cluster = assign_vector(&vectors[i], centroids, k);
      // Record the assignment of the site to the cluster.
      counts[cluster]++;
      add_vector(&vectors[i], &sums[cluster]);
    }


    // Gather and sum at root all cluster sums for individual processes.
    MPI_Reduce((int32_t*) sums, (int32_t*) grand_sums, k * VDIM, MPI_INTEGER, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce((int32_t*) counts, (int32_t*) grand_counts, k, MPI_INTEGER, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
#ifdef PRINTALL
      printf("all sums:\n");
      print_centroids(grand_sums, k);
      printf("all counts:\n");
      for(int i = 0; i<k; i++)
      {
        printf("   %03d:   %d\n", i, grand_counts[i]);
      }
#endif
      // Root process computes new centroids by dividing sums per cluster
      // by count per cluster.
      for (int i = 0; i<k; i++) 
      {
        div_vector(&grand_sums[i], grand_counts[i]);
      }
      printf("after div\n");
      // Have the centroids changed much?
      //norm = distance2(grand_sums, centroids, vector_dim*k);
      norm = 0;
      for (int i = 0; i<k; i++) 
      {
        norm += my_distance2(&grand_sums[i], &centroids[i]);
      }
      printf("[%04d] norm: %u\n", iteration, norm);
      // Copy new centroids from grand_sums into centroids.
      for (int i = 0; i<k; i++)
      {
        centroids[i] = grand_sums[i];
      }
#ifdef PRINTALL
      printf("current centroids: \n");
      print_centroids(centroids,k);
#endif
    }
    // Broadcast the norm.  All processes will use this in the loop test.
    MPI_Bcast(&norm, 1, MPI_INTEGER, 0, MPI_COMM_WORLD);
    iteration++;
  }

  // Now centroids are fixed, so compute a final label for each site.
  for (int i = 0; i < vectors_per_proc; i++) 
  {
    labels[i] = assign_vector(&vectors[i], centroids, k);
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
      printf("   %03d:   %d\n", i, grand_counts[i]);
    }
    //printf("Final centroids:\n");
    //print_centroids(centroids,k);
#ifdef DEBUG 
    t1 = get_timestamp();
    double secs = (t1 - t0) / 1000000.0L;
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>\tMPI C runtime: %lfs\n", secs);
#endif

  free(all_vectors);
  free(grand_sums);
  free(grand_counts);
  free(all_labels);
  }

  free(vectors);
  MPI_Finalize();

}


