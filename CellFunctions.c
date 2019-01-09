#include "CellFunctions.h"


// return: 1 is alive
// return: 0 is dead
int GetCellState(int ColIndex, int RowIndex, struct TimeIntervals *timer)
{
	struct timeval t1, t2;

	int CellState[1];
	MPI_Status status;

	//printf("(RECV) rank: %d, Index: %d", RowIndex, ColIndex);


	gettimeofday(&t1, NULL);
	MPI_Send(&ColIndex, 1, MPI_INT, RowIndex, 123, MPI_COMM_WORLD);
	MPI_Recv(CellState, 1, MPI_INT, RowIndex, 123, MPI_COMM_WORLD, &status);
	gettimeofday(&t2, NULL);

	timer->TimeCommunicating_usec += (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);

	return CellState[0];
}

void SendCellState(int *CellArray, int Rank, struct TimeIntervals *timer)
{
	struct timeval t1, t2;

	int CellCol;
	MPI_Status status;


	gettimeofday(&t1, NULL);
	MPI_Recv(&CellCol, 1, MPI_INT, Rank, 123, MPI_COMM_WORLD, &status);

	//printf("(SEND) rank: %d, Index: %d", Rank, CellCol);

	MPI_Send(&CellArray[CellCol], 1, MPI_INT, Rank, 123, MPI_COMM_WORLD);
	gettimeofday(&t2, NULL);

	timer->TimeCommunicating_usec += (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);
}


int DetermineState(int LeftNeighbor, int RightNeighbor, int CurrentCellColIndex, int CurrentCellRowIndex, int CellsPerRow, int CellsPerProcess, int ProcessCount, int TotalRows, int Rank, struct TimeIntervals *timer)
{

	int LivingNeighbors = LeftNeighbor + RightNeighbor;
	int NeighborCol, NeighborRow = 0;


	// Sets row to row above current row
	if (CurrentCellRowIndex - 1 < 0)
	{
		NeighborRow = ProcessCount - 1;
	}
	else
	{
		NeighborRow = CurrentCellRowIndex - 1;
	}



	// Gets NorthWest Cell
	if (CurrentCellColIndex % CellsPerRow - 1 < 0)
	{
		NeighborCol = CurrentCellColIndex + CellsPerRow;
	}
	else
	{
		NeighborCol = CurrentCellColIndex - 1;
	}
	LivingNeighbors += GetCellState(NeighborCol, NeighborRow, timer);


	// Gets North Cell
	NeighborCol = CurrentCellColIndex;
	LivingNeighbors += GetCellState(NeighborCol, NeighborRow, timer);



	// Gets NorthEast Cell
	if (CurrentCellColIndex % CellsPerRow + 1 == CellsPerRow)
	{
		NeighborCol = CurrentCellColIndex - CellsPerRow;
	}
	else
	{
		NeighborCol = CurrentCellColIndex + 1;
	}
	LivingNeighbors += GetCellState(NeighborCol, NeighborRow, timer);

	

	// Sets row to row below current row
	if (CurrentCellRowIndex + 1 == ProcessCount)
	{
		NeighborRow = 0;
	}
	else
	{
		NeighborRow = CurrentCellRowIndex + 1;
	}

	//Gets SouthWest Cell
	if (CurrentCellColIndex % CellsPerRow - 1 < 0)
	{
		NeighborCol = CurrentCellColIndex + CellsPerRow;
	}
	else
	{
		NeighborCol = CurrentCellColIndex - 1;
	}
	LivingNeighbors += GetCellState(NeighborCol, NeighborRow, timer);


	//Gets South Cell
	NeighborCol = CurrentCellColIndex;
	LivingNeighbors += GetCellState(NeighborCol, NeighborRow, timer);


	//Gets SouthEast Cell
	if (CurrentCellColIndex % CellsPerRow + 1 == CellsPerRow)
	{
		NeighborCol = CurrentCellColIndex - CellsPerRow;
	}
	else
	{
		NeighborCol = CurrentCellColIndex + 1;
	}
	LivingNeighbors += GetCellState(NeighborCol, NeighborRow, timer);



	if (LivingNeighbors < 3)
	{
		return 0;
	}
	else if (LivingNeighbors > 5)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

void SimulateOneRotation(int *CellArray, int processes, int rank, int RowsOwned, int CellsPerRow, struct TimeIntervals *timer)
{
	int *DeadCellCount;
	int i;
	int row, GlobalRow;
	int LeftNeighbor, RightNeighbor;
	int RowAboveIndex, RowBelowIndex;

	struct timeval t1, t2;

	timer->OverallEnd_usec = (unsigned long)0;
	timer->OverallStart_usec = (unsigned long)0;
	timer->TimeCommunicating_usec = (unsigned long)0;
	timer->TimeDisplaying_usec = (unsigned long)0;

	gettimeofday(&t1, NULL);
	timer->OverallStart_usec = (t1.tv_sec) * 1000000 + (t1.tv_usec);

	int CellsPerProcess = RowsOwned * CellsPerRow;
	size_t n = sizeof(CellArray) / sizeof(int);
	int CellCount = CellsPerRow;

	DeadCellCount = (int *)calloc(RowsOwned * CellsPerRow, sizeof(int));


	//printf("n = %d, CellCount = %d, RowsOwned = %d, ArraySize = %d\n", (int)n, (int)CellCount, RowsOwned, (int)sizeof(CellArray));


	//ranks are split into odd and even so that they can properly overlap their mpi calls
	//Even Figures out their Cells first and Odd listens
	//They then switch rolls with Odd asking about cell status and Even Listening
	if (rank % 2 == 0)
	{
		//printf("rank: %d\n", rank);
		for (row = 0; row < RowsOwned; row++)
		{
			//printf("rank: %d, RowCount = %d, CellCount: %d, RowNum = %d\n", rank, RowsOwned, CellCount, row);
			

			//GlobalRow = processes * row + rank;

			//i is location of cell in CellArray
			for (i = row * CellCount; i < (row + 1) * CellCount; i++)
			{
				//printf("rank: %d, CellIndex: %d\n", rank, i);
				//printf("rank: %d, %d < %d\n", rank, i, (row + 1) * CellCount);
				if (i == row * CellCount)
				{
					LeftNeighbor = CellArray[(row + 1) * CellCount - 1];
					RightNeighbor = CellArray[i + 1];
				}
				else if (i == (row + 1) * CellCount - 1)
				{
					LeftNeighbor = CellArray[i - 1];
					RightNeighbor = CellArray[row * CellCount];
				}
				else
				{
					LeftNeighbor = CellArray[i - 1];
					RightNeighbor = CellArray[i + 1];
				}

				
				DeadCellCount[i] = DetermineState(LeftNeighbor, RightNeighbor, i, rank, CellCount, CellsPerProcess, processes, RowsOwned*processes, rank, timer);
			}
		}
		// Sets row to row above current row
		if (rank - 1 < 0)
		{
			RowAboveIndex = processes - 1;
		}
		else
		{
			RowAboveIndex = rank - 1;
		}

		// Sets row to row below current row
		if (rank + 1 == processes)
		{
			RowBelowIndex = 0;
		}
		else
		{
			RowBelowIndex = rank + 1;
		}

		for (i = 0; i < CellsPerProcess; i++)
		{
			SendCellState(CellArray, RowBelowIndex, timer);
			SendCellState(CellArray, RowBelowIndex, timer);
			SendCellState(CellArray, RowBelowIndex, timer);

			SendCellState(CellArray, RowAboveIndex, timer);
			SendCellState(CellArray, RowAboveIndex, timer);
			SendCellState(CellArray, RowAboveIndex, timer);
		}
	}
	else
	{
		//printf("rank: %d\n", rank);
		// Sets row to row above current row
		if (rank - 1 < 0)
		{
			RowAboveIndex = processes - 1;
		}
		else
		{
			RowAboveIndex = rank - 1;
		}

		// Sets row to row below current row
		if (rank + 1 == processes)
		{
			RowBelowIndex = 0;
		}
		else
		{
			RowBelowIndex = rank + 1;
		}

		for (i = 0; i < CellsPerProcess; i++)
		{
			SendCellState(CellArray, RowBelowIndex, timer);
			SendCellState(CellArray, RowBelowIndex, timer);
			SendCellState(CellArray, RowBelowIndex, timer);

			SendCellState(CellArray, RowAboveIndex, timer);
			SendCellState(CellArray, RowAboveIndex, timer);
			SendCellState(CellArray, RowAboveIndex, timer);
		}

		//GlobalRow is the row in the whole matrix that the Simulator is currently working on
		for (row = 0; row < RowsOwned; row++)
		{
			//GlobalRow = processes * row + rank;

			//printf("rank: %d, RowCount = %d, CellCount: %d, RowNum = %d\n", rank, RowsOwned, CellCount, row);
			//printf("rank: %d, %d < %d\n", rank, row * CellCount, (row + 1) * CellCount);

			//i is location of cell in CellArray
			for (i = row * CellCount; i < (row + 1) * CellCount; i++)
			{
				//printf("rank: %d, CellIndex: %d\n", rank, i);
				//printf("rank: %d, %d < %d\n", rank, i, (row + 1) * CellCount);
				if (i == row * CellCount)
				{
					LeftNeighbor = CellArray[(row + 1) * CellCount - 1];
					RightNeighbor = CellArray[i + 1];
				}
				else if (i == (row + 1) * CellCount - 1)
				{
					LeftNeighbor = CellArray[i - 1];
					RightNeighbor = CellArray[row * CellCount];
				}
				else
				{
					LeftNeighbor = CellArray[i - 1];
					RightNeighbor = CellArray[i + 1];
				}


				DeadCellCount[i] = DetermineState(LeftNeighbor, RightNeighbor, i, rank, CellCount, CellsPerProcess, processes, RowsOwned*processes, rank, timer);
			}
			//printf("rank: %d, RowCount = %d, RowNum = %d\n", rank, RowsOwned, row);
		}

	}

	for (i = 0; i < CellsPerProcess; i++)
	{
		CellArray[i] = DeadCellCount[i];
	}

	//free(DeadCellCount);

	gettimeofday(&t2, NULL);
	timer->OverallEnd_usec = (t2.tv_sec) * 1000000 + (t2.tv_usec);
}

void PrintCellArray(int *CellArray, int rank, int ProcessCount, int CellsPerRow, int CellCount)
{
	int CurrentRow = rank;
	int CellIndexDelta;

	int StartStatus = 1;
	MPI_Status status;

	int i = 0;
	int CurrentIndex;
	int PreviousIndex = 0;
	int CurrentPrintRowIndex;
	int CurrentPrintColIndex;
	/*
	for (i = 0; i < CellCount; i++)
	{
		printf("rank: %d, index: %d, CellContents: %d\n", rank, i, CellArray[i]);
	}
	*/

	//printf("rank: %d, CellCount: %d, ProcessCount: %d\n", rank, CellCount, ProcessCount);

	int *recvbuf = malloc(sizeof(int) * CellCount * ProcessCount);

	//printf("rank: %d, %d < %d", rank, rank, ProcessCount * (CellCount / CellsPerRow));

	//printf("rank: %d, 1 Value: %d, 2 Value: %d, 3 Value: %d, 4 Value: %d\n", rank, CellArray[0], CellArray[1], CellArray[2], CellArray[CellCount - 1]);

	
	if (rank == 0)
	{
		for (i = 0; i < ProcessCount * CellCount / CellsPerRow; i++)
		{
			if (i % ProcessCount == 0)
			{
				for (CurrentIndex = 0; CurrentIndex < CellsPerRow; CurrentIndex++)
				{
					printf("%d | ", CellArray[CurrentIndex + PreviousIndex]);
				}
				PreviousIndex = CurrentIndex;
				printf("\n");
			}
			else
			{
				MPI_Recv(&recvbuf[i * CellsPerRow], CellsPerRow, MPI_INT, i % ProcessCount, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			}
		}
	}
	else
	{
		for (i = 0; i < CellCount / CellsPerRow; i++)
		{
			MPI_Send(&CellArray[i * CellsPerRow], CellsPerRow, MPI_INT, 0, 1, MPI_COMM_WORLD);
		}
	}

}


int GetCellStateNT(int ColIndex, int RowIndex)
{
	struct timeval t1, t2;

	int CellState[1];
	MPI_Status status;

	//printf("(RECV) rank: %d, Index: %d", RowIndex, ColIndex);


	gettimeofday(&t1, NULL);
	MPI_Send(&ColIndex, 1, MPI_INT, RowIndex, 123, MPI_COMM_WORLD);
	MPI_Recv(CellState, 1, MPI_INT, RowIndex, 123, MPI_COMM_WORLD, &status);
	gettimeofday(&t2, NULL);

	//timer->TimeCommunicating_usec += (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);

	return CellState[0];
}

void SendCellStateNT(int *CellArray, int Rank)
{
	struct timeval t1, t2;

	int CellCol;
	MPI_Status status;


	gettimeofday(&t1, NULL);
	MPI_Recv(&CellCol, 1, MPI_INT, Rank, 123, MPI_COMM_WORLD, &status);

	//printf("(SEND) rank: %d, Index: %d", Rank, CellCol);

	MPI_Send(&CellArray[CellCol], 1, MPI_INT, Rank, 123, MPI_COMM_WORLD);
	gettimeofday(&t2, NULL);

	//timer->TimeCommunicating_usec += (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);
}


int DetermineStateNT(int LeftNeighbor, int RightNeighbor, int CurrentCellColIndex, int CurrentCellRowIndex, int CellsPerRow, int CellsPerProcess, int ProcessCount, int TotalRows, int Rank)
{

	int LivingNeighbors = LeftNeighbor + RightNeighbor;
	int NeighborCol, NeighborRow = 0;


	// Sets row to row above current row
	if (CurrentCellRowIndex - 1 < 0)
	{
		NeighborRow = ProcessCount - 1;
	}
	else
	{
		NeighborRow = CurrentCellRowIndex - 1;
	}



	// Gets NorthWest Cell
	if (CurrentCellColIndex % CellsPerRow - 1 < 0)
	{
		NeighborCol = CurrentCellColIndex + CellsPerRow;
	}
	else
	{
		NeighborCol = CurrentCellColIndex - 1;
	}
	LivingNeighbors += GetCellStateNT(NeighborCol, NeighborRow);


	// Gets North Cell
	NeighborCol = CurrentCellColIndex;
	LivingNeighbors += GetCellStateNT(NeighborCol, NeighborRow);



	// Gets NorthEast Cell
	if (CurrentCellColIndex % CellsPerRow + 1 == CellsPerRow)
	{
		NeighborCol = CurrentCellColIndex - CellsPerRow;
	}
	else
	{
		NeighborCol = CurrentCellColIndex + 1;
	}
	LivingNeighbors += GetCellStateNT(NeighborCol, NeighborRow);



	// Sets row to row below current row
	if (CurrentCellRowIndex + 1 == ProcessCount)
	{
		NeighborRow = 0;
	}
	else
	{
		NeighborRow = CurrentCellRowIndex + 1;
	}

	//Gets SouthWest Cell
	if (CurrentCellColIndex % CellsPerRow - 1 < 0)
	{
		NeighborCol = CurrentCellColIndex + CellsPerRow;
	}
	else
	{
		NeighborCol = CurrentCellColIndex - 1;
	}
	LivingNeighbors += GetCellStateNT(NeighborCol, NeighborRow);


	//Gets South Cell
	NeighborCol = CurrentCellColIndex;
	LivingNeighbors += GetCellStateNT(NeighborCol, NeighborRow);


	//Gets SouthEast Cell
	if (CurrentCellColIndex % CellsPerRow + 1 == CellsPerRow)
	{
		NeighborCol = CurrentCellColIndex - CellsPerRow;
	}
	else
	{
		NeighborCol = CurrentCellColIndex + 1;
	}
	LivingNeighbors += GetCellStateNT(NeighborCol, NeighborRow);



	if (LivingNeighbors < 3)
	{
		return 0;
	}
	else if (LivingNeighbors > 5)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

void SimulateOneRotationNT(int *CellArray, int processes, int rank, int RowsOwned, int CellsPerRow)
{
	int *DeadCellCount;
	int i;
	int row, GlobalRow;
	int LeftNeighbor, RightNeighbor;
	int RowAboveIndex, RowBelowIndex;

	struct timeval t1, t2;

	//timer->OverallEnd_usec = (unsigned long)0;
	//timer->OverallStart_usec = (unsigned long)0;
	//timer->TimeCommunicating_usec = (unsigned long)0;
	//timer->TimeDisplaying_usec = (unsigned long)0;

	gettimeofday(&t1, NULL);
	//timer->OverallStart_usec = (t1.tv_sec) * 1000000 + (t1.tv_usec);

	int CellsPerProcess = RowsOwned * CellsPerRow;
	size_t n = sizeof(CellArray) / sizeof(int);
	int CellCount = CellsPerRow;

	DeadCellCount = (int *)calloc(RowsOwned * CellsPerRow, sizeof(int));

	//printf("n = %d, CellCount = %d, RowsOwned = %d, ArraySize = %d\n", (int)n, (int)CellCount, RowsOwned, (int)sizeof(CellArray));


	//ranks are split into odd and even so that they can properly overlap their mpi calls
	//Even Figures out their Cells first and Odd listens
	//They then switch rolls with Odd asking about cell status and Even Listening

	if (rank % 2 == 0)
	{
		//printf("rank: %d\n", rank);
		for (row = 0; row < RowsOwned; row++)
		{
			//printf("rank: %d, RowCount = %d, CellCount: %d, RowNum = %d\n", rank, RowsOwned, CellCount, row);


			//GlobalRow = processes * row + rank;

			//i is location of cell in CellArray
			for (i = row * CellCount; i < (row + 1) * CellCount; i++)
			{
				//printf("rank: %d, CellIndex: %d\n", rank, i);
				//printf("rank: %d, %d < %d\n", rank, i, (row + 1) * CellCount);
				if (i == row * CellCount)
				{
					LeftNeighbor = CellArray[(row + 1) * CellCount - 1];
					RightNeighbor = CellArray[i + 1];
				}
				else if (i == (row + 1) * CellCount - 1)
				{
					LeftNeighbor = CellArray[i - 1];
					RightNeighbor = CellArray[row * CellCount];
				}
				else
				{
					LeftNeighbor = CellArray[i - 1];
					RightNeighbor = CellArray[i + 1];
				}


				DeadCellCount[i] = DetermineStateNT(LeftNeighbor, RightNeighbor, i, rank, CellCount, CellsPerProcess, processes, RowsOwned*processes, rank);
			}
		}
		// Sets row to row above current row

		if (rank - 1 < 0)
		{
			RowAboveIndex = processes - 1;
		}
		else
		{
			RowAboveIndex = rank - 1;
		}

		// Sets row to row below current row
		if (rank + 1 == processes)
		{
			RowBelowIndex = 0;
		}
		else
		{
			RowBelowIndex = rank + 1;
		}

		for (i = 0; i < CellsPerProcess; i++)
		{
			SendCellStateNT(CellArray, RowBelowIndex);
			SendCellStateNT(CellArray, RowBelowIndex);
			SendCellStateNT(CellArray, RowBelowIndex);

			SendCellStateNT(CellArray, RowAboveIndex);
			SendCellStateNT(CellArray, RowAboveIndex);
			SendCellStateNT(CellArray, RowAboveIndex);
		}
		
	}
	else
	{
		//printf("rank: %d\n", rank);
		// Sets row to row above current row
		if (rank - 1 < 0)
		{
			RowAboveIndex = processes - 1;
		}
		else
		{
			RowAboveIndex = rank - 1;
		}

		// Sets row to row below current row
		if (rank + 1 == processes)
		{
			RowBelowIndex = 0;
		}
		else
		{
			RowBelowIndex = rank + 1;
		}

		for (i = 0; i < CellsPerProcess; i++)
		{
			SendCellStateNT(CellArray, RowBelowIndex);
			SendCellStateNT(CellArray, RowBelowIndex);
			SendCellStateNT(CellArray, RowBelowIndex);

			SendCellStateNT(CellArray, RowAboveIndex);
			SendCellStateNT(CellArray, RowAboveIndex);
			SendCellStateNT(CellArray, RowAboveIndex);
		}

		//GlobalRow is the row in the whole matrix that the Simulator is currently working on
		for (row = 0; row < RowsOwned; row++)
		{
			//GlobalRow = processes * row + rank;

			//printf("rank: %d, RowCount = %d, CellCount: %d, RowNum = %d\n", rank, RowsOwned, CellCount, row);
			//printf("rank: %d, %d < %d\n", rank, row * CellCount, (row + 1) * CellCount);

			//i is location of cell in CellArray
			for (i = row * CellCount; i < (row + 1) * CellCount; i++)
			{
				//printf("rank: %d, CellIndex: %d\n", rank, i);
				//printf("rank: %d, %d < %d\n", rank, i, (row + 1) * CellCount);
				if (i == row * CellCount)
				{
					LeftNeighbor = CellArray[(row + 1) * CellCount - 1];
					RightNeighbor = CellArray[i + 1];
				}
				else if (i == (row + 1) * CellCount - 1)
				{
					LeftNeighbor = CellArray[i - 1];
					RightNeighbor = CellArray[row * CellCount];
				}
				else
				{
					LeftNeighbor = CellArray[i - 1];
					RightNeighbor = CellArray[i + 1];
				}


				DeadCellCount[i] = DetermineStateNT(LeftNeighbor, RightNeighbor, i, rank, CellCount, CellsPerProcess, processes, RowsOwned*processes, rank);
			}
			//printf("rank: %d, RowCount = %d, RowNum = %d\n", rank, RowsOwned, row);
		}

	}
	for (i = 0; i < CellsPerProcess; i++)
	{
		CellArray[i] = DeadCellCount[i];
	}

	//free(DeadCellCount);

	gettimeofday(&t2, NULL);
	//timer->OverallEnd_usec = (t2.tv_sec) * 1000000 + (t2.tv_usec);
	
}
