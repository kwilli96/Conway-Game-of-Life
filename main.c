#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "GOLDriver.h"



int main(int argc, char *argv[])
{
	int rank, p;
	int i;
	struct timeval t1, t2;
	struct timespec ts1, ts2;

	int multiplier = 128;


	int CellData[2] = { 0,0 };

	srand(time(0));

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &p);



	int Rows = p * multiplier;

	int CellsPerRow = Rows / p;

	if (rank == 0)
	{
		CellData[0] = Rows;
		CellData[1] = CellsPerRow;
	};


	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Bcast(CellData, 2, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);


	int buf;

	if (rank == 0)
	{
		for (i = 1; i < p; i++)
		{
			buf = rand();
			MPI_Send(&buf, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
		}
	}
	else
	{
		MPI_Recv(&buf, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}


	srand(buf);
	MPI_Barrier(MPI_COMM_WORLD);

	GOLDriver_init(CellData[1] * CellData[0] / p, CellData[0] / p, p);

	MPI_Barrier(MPI_COMM_WORLD);
	GOL_Start(5, rank);





	MPI_Finalize();
}