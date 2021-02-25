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
int64_t my_distance2(int32_t *v1_x, int32_t *v1_y, int32_t *v1_z, int32_t *v2_x, int32_t *v2_y, int32_t *v2_z)
{
   int64_t diff = 0;
   int64_t dist = 0;
   int64_t diff2 = 0;

  diff = (int64_t) (*v1_x - *v2_x);
  diff2 = (int64_t) (diff * diff);
  dist += diff2;
  
  diff = (int64_t) (*v1_y - *v2_y);
  diff2 = (int64_t) (diff * diff);
  dist += diff2;
  
  diff = (int64_t) (*v1_z - *v2_z);
  diff2 = (int64_t) (diff * diff);
  dist += diff2;

  //dist = dist >> SCALE_FACTOR;
  return (int64_t) dist;
}

// Assign a vector to the correct cluster by computing its distances to
// each cluster centroid.
int assign_vector(int32_t *vector_x, int32_t *vector_y, int32_t *vector_z, int centroids_x[MAX_CENTROIDS], 
                  int centroids_y[MAX_CENTROIDS], int centroids_z[MAX_CENTROIDS], int k)
{
  int best_cluster = 0;
  int64_t best_dist = LONG_MAX;
  for (int c = 0; c < k; c++)
  {
    int64_t dist = my_distance2(vector_x, vector_y, vector_z, &centroids_x[c],
                                 &centroids_y[c], &centroids_z[c]);
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
void add_vector(int32_t *vector_x, int32_t *vector_y, int32_t *vector_z, int32_t *sum_x, int32_t *sum_y, int32_t *sum_z)
{
  *sum_x += *vector_x;
  *sum_y += *vector_y;
  *sum_z += *vector_z;
}

void div_vector(int32_t *vector_x, int32_t *vector_y, int32_t *vector_z, int32_t dividend)
{
  if(dividend != 0)
  {
    *vector_x /= dividend;
    *vector_y /= dividend;
    *vector_z /= dividend;
  }
}

// Print the centroids one per line.
void print_centroids(int32_t *centroids_x, int32_t *centroids_y, int32_t *centroids_z, int k) {
  //printf("Centroids:\n");
  for (int i = 0; i<k; i++) 
  {
    printf("%+013d ", centroids_x[i]);
    printf("%+013d ", centroids_y[i]);
    printf("%+013d ", centroids_z[i]);
    printf("\n");
  }
}

void reading_vectors3D(FILE* file_stream, int32_t *all_vectors_x, int32_t *all_vectors_y, int32_t *all_vectors_z, int nr_vectors)
{
  char line[256];
  int li = 0;
#ifdef DEBUG
  printf("expecting 3 dimensions per line\n");
#endif
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
    all_vectors_x[li] = var[0];
    all_vectors_y[li] = var[1];
    all_vectors_z[li] = var[2];
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
  int32_t *vectors_x, *vectors_y, *vectors_z, *labels;
  vectors_x = (int32_t*) malloc(vectors_per_proc * sizeof(int32_t));
  if(vectors_x == NULL)
  {
    perror("malloc");
    exit(1);
  }

  vectors_y = (int32_t*) malloc(vectors_per_proc * sizeof(int32_t));
  if(vectors_y == NULL)
  {
    perror("malloc");
    exit(1);
  }
  
  vectors_z = (int32_t*) malloc(vectors_per_proc * sizeof(int32_t));
  if(vectors_z == NULL)
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
  int32_t sums_x[MAX_CENTROIDS];
  int32_t sums_y[MAX_CENTROIDS];
  int32_t sums_z[MAX_CENTROIDS];

  // The number of vectors assigned to each cluster by this process. k integers.
  int counts[MAX_CENTROIDS];

  // The current centroids against which vectors are being compared.
  // These are shipped to the process by the root process.
  int32_t centroids_x[MAX_CENTROIDS];
  int32_t centroids_y[MAX_CENTROIDS];
  int32_t centroids_z[MAX_CENTROIDS];


  //
  // Data structures maintained only in root process.
  //
  // All the sites for all the processes.
  // site_per_proc * nprocs vectors of d floats.
  int32_t *all_vectors_x = NULL;
  int32_t *all_vectors_y = NULL;
  int32_t *all_vectors_z = NULL;
  // Sum of sites assigned to each cluster by all processes.
  int32_t *all_sums_x = NULL;
  int32_t *all_sums_y = NULL;
  int32_t *all_sums_z = NULL;
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

    all_vectors_x = (int32_t*) malloc(nr_vectors * sizeof(int32_t));
    if(all_vectors_x == NULL)
    {
      perror("malloc");
      exit(1);
    }
    
    all_vectors_y = (int32_t*) malloc(nr_vectors * sizeof(int32_t));
    if(all_vectors_y == NULL)
    {
      perror("malloc");
      exit(1);
    }
    
    all_vectors_z = (int32_t*) malloc(nr_vectors * sizeof(int32_t));
    if(all_vectors_z == NULL)
    {
      perror("malloc");
      exit(1);
    }

    reading_vectors3D(file_stream, all_vectors_x, all_vectors_y, all_vectors_z, nr_vectors);
    fclose(file_stream);

    // Take evenly distributed vectors as the initial cluster centroids.
    int step = nr_vectors/k;
    for (int i = 0; i < k; i++)
    {
      centroids_x[i] = all_vectors_x[i*step];
      centroids_y[i] = all_vectors_y[i*step];
      centroids_z[i] = all_vectors_z[i*step];
    }
    printf("finished reading file...\n");
//#ifdef PRINTALL
//    print_centroids(centroids_x, centrodis_y, centroids_z, k);
//#endif
    all_sums_x = (int32_t*) malloc(k * sizeof(int32_t));
    if(all_sums_x == NULL)
    {
      perror("malloc");
      exit(1);
    }

    all_sums_y = (int32_t*) malloc(k * sizeof(int32_t));
    if(all_sums_y == NULL)
    {
      perror("malloc");
      exit(1);
    }
    
    all_sums_z = (int32_t*) malloc(k * sizeof(int32_t));
    if(all_sums_z == NULL)
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
  MPI_Scatter(all_vectors_x, vectors_per_proc, MPI_INTEGER, vectors_x, vectors_per_proc, MPI_INTEGER, 0, MPI_COMM_WORLD);
  MPI_Scatter(all_vectors_y, vectors_per_proc, MPI_INTEGER, vectors_y, vectors_per_proc, MPI_INTEGER, 0, MPI_COMM_WORLD);
  MPI_Scatter(all_vectors_z, vectors_per_proc, MPI_INTEGER, vectors_z, vectors_per_proc, MPI_INTEGER, 0, MPI_COMM_WORLD);


  int norm = INT_MAX;  // Will tell us if centroids have moved.
  int iteration = 0;

  while(norm > NORM_LOWER_LIMIT && iteration < MAX_ITERATIONS)
  { // While they've moved...

    // Each process reinitializes its cluster accumulators.
    for (int i = 0; i < k; i++)
    {
      sums_x[i] = 0;
      sums_y[i] = 0;
      sums_z[i] = 0;
      counts[i] = 0;
    }
    //if(rank != 0)
    //{
    //  for (int i = 0; i < k; i++)
    //  {
    //    centroids_x[i] = 0;
    //    centroids_y[i] = 0;
    //    centroids_z[i] = 0;
    //  }
    //}

    // Broadcast the current cluster centroids to all processes.
    MPI_Bcast(centroids_x, k, MPI_INTEGER, 0, MPI_COMM_WORLD);
    MPI_Bcast(centroids_y, k, MPI_INTEGER, 0, MPI_COMM_WORLD);
    MPI_Bcast(centroids_z, k, MPI_INTEGER, 0, MPI_COMM_WORLD);


    // Find the closest centroid to each site and assign to cluster.
    for (int i = 0; i < vectors_per_proc; i++)
    {
      int cluster = assign_vector(&vectors_x[i], &vectors_y[i], &vectors_z[i],
                                 centroids_x, centroids_y, centroids_z, k);
      // Record the assignment of the site to the cluster.
      counts[cluster]++;
      add_vector(&vectors_x[i], &vectors_y[i], &vectors_z[i], &sums_x[cluster],
                  &sums_y[cluster], &sums_z[cluster]);
    }

#ifdef PRINTALL
    if(rank == 0)
    {
      printf("root sums:\n");
      print_centroids(sums_x, sums_y, sums_z, k);
    }
#endif

    // Gather and sum at root all cluster sums for individual processes.
    MPI_Reduce(sums_x, all_sums_x, k, MPI_INTEGER, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(sums_y, all_sums_y, k, MPI_INTEGER, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(sums_z, all_sums_z, k, MPI_INTEGER, MPI_SUM, 0, MPI_COMM_WORLD);
    
    MPI_Reduce(counts, all_counts, k, MPI_INTEGER, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
#ifdef PRINTALL
      printf("all sums:\n");
      print_centroids(all_sums_x, all_sums_y, all_sums_z, k);
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
        div_vector(&all_sums_x[i], &all_sums_y[i], &all_sums_z[i], all_counts[i]);
      }
      printf("after div\n");
      // Have the centroids changed much?
      //norm = distance2(grand_sums, centroids, vector_dim*k);
      norm = 0;
      int64_t norm_tmp = 0;
      for (int i = 0; i<k; i++)
      {
        norm_tmp += my_distance2(&all_sums_x[i], &all_sums_y[i], &all_sums_z[i], 
                                  &centroids_x[i], &centroids_y[i], &centroids_z[i]);
      }
      if(norm_tmp >= INT_MAX || norm_tmp < 0)
      {
        norm = INT_MAX;
      } else {
        norm = (int32_t) norm_tmp;
      }
      printf("[%04d] norm: %d\n", iteration, norm);
      // Copy new centroids from grand_sums into centroids.
      for (int i = 0; i<k; i++)
      {
        centroids_x[i] = all_sums_x[i];
        centroids_y[i] = all_sums_y[i];
        centroids_z[i] = all_sums_z[i];
      }
#ifdef PRINTALL
      printf("current centroids: \n");
      print_centroids(centroids_x, centroids_y, centroids_z, k);
#endif
    }
    // Broadcast the norm.  All processes will use this in the loop test.
    MPI_Bcast(&norm, 1, MPI_INTEGER, 0, MPI_COMM_WORLD);
    iteration++;
  }

  // Now centroids are fixed, so compute a final label for each site.
  for (int i = 0; i < vectors_per_proc; i++)
  {
    labels[i] = assign_vector(&vectors_x[i], &vectors_y[i], &vectors_z[i],
                                 centroids_x, centroids_y, centroids_z, k);
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

  free(all_vectors_x);
  free(all_vectors_y);
  free(all_vectors_z);
  free(all_sums_x);
  free(all_sums_y);
  free(all_sums_z);
  free(all_counts);
  free(all_labels);
  }

  free(vectors_x);
  free(vectors_y);
  free(vectors_z);
  free(labels);
  MPI_Finalize();

}


