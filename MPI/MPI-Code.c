#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#define DEFAULT_WIDTH 20
#define DEFAULT_HEIGTH 20
#define DEFAULT_ITERATIONS 20

//data structure for defining the chunk of data given to a process
struct chunk
{
	int rows;
	int columns;
	int precedent_process;
	int subsequent_process;
	int mpi_rank;
	int mpi_size;
	unsigned int **matrix;
};

//computes the difference between two times, in milliseconds
double compute_time_interval(struct timeval start_time, struct timeval end_time)
{
	return (double)((end_time.tv_sec - start_time.tv_sec)*1000000 + (end_time.tv_usec - start_time.tv_usec)) / 1000;
}

//saves the results passed as a parameter in a result file
void write_to_file(int width, int heigth, int processes, int nodes, double total_time)
{
	FILE * f;
	f = fopen("MPI-Results.csv", "a");

	fprintf(f, "%d,%d,%d,%d,%f\n", width, heigth, processes, nodes, total_time);
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
		matrix[i] = &(grid[width * i]);

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

//initializes the values in an instance of the chunk structure
void init_chunk(struct chunk *local_chunk, int rows, int columns, int precedent_process, int subsequent_process, int mpi_rank, int mpi_size)
{
	local_chunk->rows = rows;
	local_chunk->columns = columns;
	local_chunk->precedent_process = precedent_process;
	local_chunk->subsequent_process = subsequent_process;
	local_chunk->mpi_rank = mpi_rank;
	local_chunk->mpi_size = mpi_size;
	local_chunk->matrix = create_matrix(rows, columns);

	int i, j;
	for (i = 1; i < local_chunk->rows - 1; i++)
		for (j = 0; j < local_chunk->columns; j++)
			local_chunk->matrix[i][j] = rand() % 10 >= 7 ? 1 : 0;
}

//computes a single round of the game
void one_round(struct chunk *local_chunk, unsigned int **next_matrix, MPI_Datatype row_datatype)
{
	int i, j;
	MPI_Status status;

	//sending the first and the last row of the current chunk respectively to the precedent and the subsequent process
	MPI_Send(&local_chunk->matrix[1][0], 1, row_datatype, local_chunk->precedent_process, 0, MPI_COMM_WORLD);
	MPI_Send(&local_chunk->matrix[local_chunk->rows - 2][0], 1, row_datatype, local_chunk->subsequent_process, 0, MPI_COMM_WORLD);

	//receiving the immediately upper and the immediately lower row to the current chunk respectively from the precedent and the subsequent process
	MPI_Recv(&local_chunk->matrix[0][0], local_chunk->columns, MPI_INT, local_chunk->precedent_process, 0, MPI_COMM_WORLD, &status);
	MPI_Recv(&local_chunk->matrix[local_chunk->rows - 1][0], local_chunk->columns, MPI_INT, local_chunk->subsequent_process, 0, MPI_COMM_WORLD, &status);

	for (i = 1; i < (local_chunk->rows - 1); i++)
	{
		for (j = 0; j < local_chunk->columns; j++)
		{
			int active_neighbours = 0, i_neighbour, j_neighbour;

			//check of the condition of all the cells surrounding the current one
			for (i_neighbour = i - 1; i_neighbour <= i + 1; i_neighbour++)
				for (j_neighbour = j - 1; j_neighbour <= j + 1; j_neighbour++)
					if (!(i == i_neighbour && j == j_neighbour))
						active_neighbours += local_chunk->matrix[i_neighbour][(j_neighbour + local_chunk->columns) % local_chunk->columns];

			//update of the value of the cell according to the rules of the game
			if (local_chunk->matrix[i][j] == 1 && (active_neighbours < 2 || active_neighbours > 3))
				next_matrix[i][j] = 0;
			else if ((local_chunk->matrix[i][j] == 1 && (active_neighbours == 2 || active_neighbours == 3)) || (local_chunk->matrix[i][j] == 0 && active_neighbours == 3))
				next_matrix[i][j] = 1;
			else
				next_matrix[i][j] = local_chunk->matrix[i][j];
		}
	}
}

//main auxiliary function, initializes the grid situation and monitors its evolution
void play(struct chunk *local_chunk, int heigth, int width, int nodes)
{
	int i;
	double total_time = 0.0;

	MPI_Datatype row_datatype;
	MPI_Type_contiguous(local_chunk->columns, MPI_UNSIGNED, &row_datatype);
	MPI_Type_commit(&row_datatype);

	//evolution
	for (i = 0; i < DEFAULT_ITERATIONS; i++)
	{
		struct timeval start_time, end_time;
		unsigned int **next_matrix = create_matrix(local_chunk->rows, local_chunk->columns);

		if (local_chunk->mpi_rank == 0)
		{
			gettimeofday(&start_time, NULL);
			one_round(local_chunk, next_matrix, row_datatype);
			gettimeofday(&end_time, NULL);

			total_time += compute_time_interval(start_time, end_time);
		}
		else
			one_round(local_chunk, next_matrix, row_datatype);

		swap_matrix(&local_chunk->matrix, &next_matrix);
		delete_matrix(next_matrix);
	}

	if (local_chunk->mpi_rank == 0)
		write_to_file(width, heigth, local_chunk->mpi_size, nodes, total_time);

	MPI_Type_free(&row_datatype);
	delete_matrix(local_chunk->matrix);
}

int main(int argc, char **argv)
{
	//width and heigth are initialized with their default values
	int width = DEFAULT_WIDTH,
		heigth = DEFAULT_HEIGTH,
		nodes;

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

	nodes = atoi(argv[3]);

	int mpi_rank, mpi_size;
	
	//initization of the MPI execution environment
	MPI_Init(&argc, &argv);

	//the root process (the one with rank=0) broadcasts the heigth and width parameters to each worker process
	MPI_Bcast(&heigth, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
	
	//each process gets its own rank and size
	MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
	
	//each process computes the size of its chunk of data, leftovers are added to the last process
	//the "+2" is necessary because each process will receive also one row from its upper neighbour and
	//one row from its lower neighbour in order to correctly calculate the new state of its cells
	int process_rows = (heigth / mpi_size) + 2;
	if (mpi_rank == mpi_size - 1)
		process_rows += heigth % mpi_size;

	struct chunk local_chunk;
	int precedent_process = (mpi_rank == 0) ? mpi_size - 1 : mpi_rank - 1;
	int subsequent_process = (mpi_rank == mpi_size - 1) ? 0 : mpi_rank + 1;

	init_chunk(&local_chunk, process_rows, width, precedent_process, subsequent_process, mpi_rank, mpi_size);
	play(&local_chunk, heigth, width, nodes);

	return MPI_Finalize();
}