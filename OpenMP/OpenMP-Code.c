#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#define DEFAULT_WIDTH 20
#define DEFAULT_HEIGTH 20
#define DEFAULT_ITERATIONS 20
#define DEFAULT_THREADS 2

//computes the difference between two times, in milliseconds
double compute_time_interval(struct timeval start_time, struct timeval end_time)
{
	return (double)((end_time.tv_sec - start_time.tv_sec)*1000000 + (end_time.tv_usec - start_time.tv_usec))) / 1000;
}

//counts the number of active cells in the grid
int count_active_cells(unsigned int **grid, int width, int heigth)
{
	int active_cells = 0, i, j;

	for (i = 0; i < heigth; i++)
		for (j = 0; j < width; j++)
			active_cells += grid[i][j];

	return active_cells;
}

//saves the results passed as a parameter in a result file
void write_to_file(int width, int heigth, int threads, double total_time)
{
	FILE * f;
	f = fopen("OpenMP-Results.csv", "a");

	fprintf(f, "%d,%d,%d,%f\n", width, heigth, threads, total_time);
	fflush(f);
	fclose(f);
}

//allocates the memory for a matrix given its dimensions
unsigned int **create_matrix(int heigth, int width)
{
	int i;
	unsigned int *grid = (unsigned int *) malloc(heigth * width * sizeof(unsigned int));
	unsigned int **matrix = (unsigned int **) malloc(heigth * sizeof(unsigned int *));

	for (i = 0; i < heigth; i++)
		matrix[i] = &(grid[width *i]);

	return matrix;
}

//swaps the content of a matrix with the content of another one (by swapping their pointers)
void swap_matrix(unsigned int ***old, unsigned int ***new)
{
	unsigned int **temp = *old;
	*old = *new;
	*new = temp;
}

//deletes a matrix
void delete_matrix(unsigned int **matrix)
{
	free(matrix[0]);
	free(matrix);
}

//main auxiliary function, initializes the grid situation and monitors its evolution
void play(int width, int heigth, int threads)
{
	int i, j, k;
	unsigned int **grid = create_matrix(heigth, width);
	double total_time = 0.0;

	//initialization
	for (i = 0; i < heigth; i++)
		for (j = 0; j < width; j++)
			grid[i][j] = rand() % 10 >= 7 ? 1 : 0;

	printf("Number of active cells at the beginning of the execution: %d\n", count_active_cells(grid, width, heigth));

	//evolution
	for (i = 0; i < DEFAULT_ITERATIONS; i++)
	{
		struct timeval start_time, end_time;
		unsigned int **new_grid = create_matrix(heigth, width);
		gettimeofday(&start_time, NULL);

		#pragma omp parallel
		for private(j, k)
		for (j = 0; j < heigth; j++)
		{
			for (k = 0; k < width; k++)
			{
				int active_neighbours = 0, j_neighbour, k_neighbour;

				//check of the condition of all the cells surrounding the current one
				for (j_neighbour = j - 1; j_neighbour <= j + 1; j_neighbour++)
					for (k_neighbour = k - 1; k_neighbour <= k + 1; k_neighbour++)
						if (!(j == j_neighbour && k == k_neighbour))
							active_neighbours += grid[(j_neighbour + heigth) % heigth][(k_neighbour + width) % width];

				//update of the value of the cell according to the rules of the game
				if (grid[j][k] == 1 && (active_neighbours < 2 || active_neighbours > 3))
					new_grid[j][k] = 0;
				else if ((grid[j][k] == 1 && (active_neighbours == 2 || active_neighbours == 3)) || (grid[j][k] == 0 && active_neighbours == 3))
					new_grid[j][k] = 1;
				else
					new_grid[j][k] = grid[j][k];
			}
		}

		gettimeofday(&end_time, NULL);
		swap_matrix(&grid, &new_grid);
		delete_matrix(new_grid);
		total_time += compute_time_interval(start_time, end_time);
	}

	write_to_file(width, heigth, threads, total_time);
	printf("Number of active cells at the end of the execution: %d\n", count_active_cells(grid, width, heigth));
	delete_matrix(grid);
}

int main(int argc, char **argv)
{
	//width, heigth and number of threads are initialized with their default values
	int width = DEFAULT_WIDTH,
		heigth = DEFAULT_HEIGTH,
		threads = DEFAULT_THREADS;

	if (argc != 4)
	{
		printf("Invalid number of parameters");
		return 0;
	}

	//if the program was launched with correct custom parameters, the default values are replaced with them
	if (atoi(argv[1]) > DEFAULT_WIDTH)
		width = atoi(argv[1]);

	if (atoi(argv[2]) > DEFAULT_HEIGTH)
		heigth = atoi(argv[2]);

	if (atoi(argv[3]) > DEFAULT_THREADS)
		threads = atoi(argv[3]);

	omp_set_num_threads(threads);
	play(width, heigth, threads);
}