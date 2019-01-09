#pragma once
#ifndef GOLDRIVER_H
#define GOLDRIVER_H

#include <stddef.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>
#include <stdio.h>

#include "CellFunctions.h"

int _PROCESSRANK;
int _TOTALCELLCOUNT;
int _TOTALROWSOWNED;
int _TOTALPROCESSCOUNT;





void GOLDriver_init(int TotalCells, int RowsOwned, int ProcessCount);
void GOL_Start(int generations, int Rank);




#endif // !GOLDRIVER_H