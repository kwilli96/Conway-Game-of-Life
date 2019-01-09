#include "GOLDriver.h"


void GOLDriver_init(int TotalCells, int RowsOwned, int ProcessCount)
{
	_TOTALCELLCOUNT = TotalCells;
	_TOTALROWSOWNED = RowsOwned;
	_TOTALPROCESSCOUNT = ProcessCount;
}

void GOL_Start(int generations, int Rank)
{
	char OutPutName[64];

	int iteration;
	int i;

	int* CellArray;
	int* PreviousGeneration;

	int GenerationDisplaySkip = 4;

	int TotalRunTime_usec = 0;
	int AverageGenerationTime_usec = 0;
	int TotalCommunicationTime_usec = 0;
	int TotalComputationTime = 0;

	int Sendbuf[1];
	int *recvbuf = (int *)malloc(sizeof(int) * _TOTALPROCESSCOUNT);

	struct timeval t1, t2;

	struct TimeIntervals *AllRuns = calloc(generations, sizeof(struct TimeIntervals));
	struct TimeIntervals *SingleRun;

	CellArray = malloc(sizeof(int) * _TOTALCELLCOUNT);




	for (i = 0; i < _TOTALCELLCOUNT; i++)
	{
		CellArray[i] = rand() % 2;
	}


	for (iteration = 0; iteration < generations; iteration++)
	{


		// Simulates one generation of the algorithm
		SimulateOneRotation(CellArray, _TOTALPROCESSCOUNT, Rank, _TOTALROWSOWNED, _TOTALCELLCOUNT / _TOTALROWSOWNED, &AllRuns[iteration]);


		gettimeofday(&t1, NULL);
		MPI_Barrier(MPI_COMM_WORLD);
		gettimeofday(&t2, NULL);

		AllRuns[iteration].TimeCommunicating_usec += (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);


		gettimeofday(&t1, NULL);
		if (iteration % GenerationDisplaySkip == 0)
		{
			if (Rank == 0)
			{
				printf("Generation: %d\n", iteration);
			}
			PrintCellArray(CellArray, Rank, _TOTALPROCESSCOUNT, _TOTALCELLCOUNT / _TOTALROWSOWNED, _TOTALCELLCOUNT);
		}
		gettimeofday(&t2, NULL);


		AllRuns[iteration].TimeDisplaying_usec += (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);

		gettimeofday(&t1, NULL);
		MPI_Barrier(MPI_COMM_WORLD);
		gettimeofday(&t2, NULL);

		AllRuns[iteration].TimeCommunicating_usec += (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);
	}


	for (i = 0; i < generations; i++)
	{
		TotalRunTime_usec += AllRuns[i].OverallEnd_usec - AllRuns[i].OverallStart_usec;
		TotalCommunicationTime_usec += AllRuns[i].TimeCommunicating_usec;
	}
	AverageGenerationTime_usec = TotalRunTime_usec / generations;
	TotalComputationTime = TotalRunTime_usec - TotalCommunicationTime_usec;


	MPI_Gather(&AverageGenerationTime_usec, 1, MPI_INT, recvbuf, 1, MPI_INT, 0, MPI_COMM_WORLD);

	if (_PROCESSRANK == 0)
	{
		for (i = 0; i < _TOTALPROCESSCOUNT; i++)
		{
			if (AverageGenerationTime_usec <= recvbuf[i])
			{
				AverageGenerationTime_usec = recvbuf[i];
			}
		}
		sprintf(OutPutName, "outputP%dN%d.txt", _TOTALPROCESSCOUNT, _TOTALPROCESSCOUNT * _TOTALROWSOWNED);
		FILE *fp = fopen(OutPutName, "w+");

		fprintf(fp, "%d,%d,%d,%d,%d,%d\n", _TOTALPROCESSCOUNT, _TOTALPROCESSCOUNT * _TOTALROWSOWNED, TotalRunTime_usec, AverageGenerationTime_usec, TotalCommunicationTime_usec, TotalComputationTime);
		fclose(fp);
	}
}