#pragma once
#ifndef CELLFUNCTIONS_H
#define CELLFUNCTIONS_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>

struct TimeIntervals {
	unsigned long int OverallStart_usec;
	unsigned long int OverallEnd_usec;

	unsigned long int TimeDisplaying_usec;
	unsigned long int TimeCommunicating_usec;
};

struct TimeIntervals TotalTimeInterval;

int GetCellState(int ColIndex, int RowIndex, struct TimeIntervals *timer);
void SendCellState(int *CellArray, int Rank, struct TimeIntervals *timer);
int DetermineState(int LeftNeighbor, int RightNeighbor, int CurrentCellColIndex, int CurrentCellRowIndex, int CellsPerRow, int CellsPerProcess, int ProcessCount, int TotalRows, int Rank, struct TimeIntervals *timer);
void SimulateOneRotation(int *CellArray, int processes, int rank, int RowsOwned, int CellsPerRow, struct TimeIntervals *timer);
void PrintCellArray(int *CellArray, int rank, int ProcessCount, int CellsPerRow, int CellCount);

int GetCellStateNT(int ColIndex, int RowIndex);
void SendCellStateNT(int *CellArray, int Rank);
int DetermineStateNT(int LeftNeighbor, int RightNeighbor, int CurrentCellColIndex, int CurrentCellRowIndex, int CellsPerRow, int CellsPerProcess, int ProcessCount, int TotalRows, int Rank);
void SimulateOneRotationNT(int *CellArray, int processes, int rank, int RowsOwned, int CellsPerRow);


#endif // !CELLFUNCTIONS_H
