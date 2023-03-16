#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#define DEFAULT_WIDTH 20
#define DEFAULT_HEIGTH 20
#define DEFAULT_ITERATIONS 20
#define DEFAULT_THREADS 32

//computes the difference between two times, in milliseconds
double compute_time_interval(struct timeval start_time, struct timeval end_time)
{
	return (double)((end_time.tv_sec - start_time.tv_sec)*1000000 + (end_time.tv_usec - start_time.tv_usec)) / 1000;
}

//saves the results passed as a parameter in a result file
void write_to_file(int width, int heigth, int threads, double total_time)
{
	FILE * f;
	f = fopen("CUDA-Results.csv", "a");

	fprintf(f, "%d,%d,%d,%f\n", width, heigth, threads, total_time);
	fflush(f);
	fclose(f);
}

//swaps the content of a matrix with the content of another one (by swapping their pointers)
void swap_matrix(unsigned int **old, unsigned int **new_)
{
	unsigned int *temp = *old;
	*old = *new_;
	*new_ = temp;
}

//computes a single round of the game
__global__ void one_round(unsigned int *grid, unsigned int *new_grid, int heigth, int width)
{
	const int grid_size = heigth * width;
	const int position = blockIdx.x * blockDim.x + threadIdx.x;

	if (position >= grid_size)
		return;

	//computing position of the current cell
	int column_index = position % width;
	int row_index = position - column_index;

	//computing positions of the cells surrounding the current one
	int left = (column_index + width - 1) % width;
	int right = (column_index + 1) % width;
	int top = (row_index + grid_size - width) % grid_size;
	int bottom = (row_index + width) % grid_size;

	//the computed indexes are combined to locate all of the cells surrounding the current one, their values are summed up in order to find the number of active neighbours
	int active_neighbours = grid[left + top] + grid[column_index + top] + grid[right + top] + grid[left + row_index] + grid[right + row_index] + grid[left + bottom] + grid[column_index + bottom] + grid[right + bottom];

	//update of the value of the cell according to the rules of the game
	if (grid[column_index + row_index] == 1 && (active_neighbours < 2 || active_neighbours > 3))
		new_grid[column_index + row_index] = 0;
	else if ((grid[column_index + row_index] == 1 && (active_neighbours == 2 || active_neighbours == 3)) || (grid[column_index + row_index] == 0 && active_neighbours == 3))
		new_grid[column_index + row_index] = 1;
	else
		new_grid[column_index + row_index] = grid[column_index + row_index];
}

//main auxiliary function, initializes the grid situation and monitors its evolution
void play(int width, int heigth, int num_threads)
{
	int i, j;
	struct timeval start_time, end_time;
	double total_time = 0.0;
	unsigned int *grid = (unsigned int *) malloc(heigth *width* sizeof(unsigned int));

	//initialization
	for (i = 0; i < heigth; i++)
		for (j = 0; j < width; j++)
			grid[i *width + j] = rand() % 10 >= 7 ? 1 : 0;

	size_t grid_size = heigth *width* sizeof(unsigned int);
	unsigned int *cuda_grid;
	unsigned int *cuda_new_grid;
	cudaMalloc((void **) &cuda_grid, grid_size);
	cudaMalloc((void **) &cuda_new_grid, grid_size);
	cudaMemcpy(cuda_grid, grid, grid_size, cudaMemcpyHostToDevice);
	dim3 threads(num_threads);
	dim3 chunks((int)(heigth *width + threads.x - 1) / threads.x);

	for (i = 0; i < DEFAULT_ITERATIONS; i++)
	{
		gettimeofday(&start_time, NULL);

		//computation of the game evolution in this iteration
		one_round <<<chunks, threads>>> (cuda_grid, cuda_new_grid, heigth, width);

		cudaDeviceSynchronize();
		gettimeofday(&end_time, NULL);

		swap_matrix(&cuda_grid, &cuda_new_grid);
		total_time += (double) compute_time_interval(start_time, end_time);
	}

	write_to_file(width, heigth, num_threads, total_time);
	cudaFree(cuda_grid);
	cudaFree(cuda_new_grid);
	free(grid);
}

int main(int argc, char **argv)
{
	//width, heigth and chunk size are initialized with their default values
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

	play(width, heigth, threads);
}