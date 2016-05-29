#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PRZED_LEKARZEM 1
#define W_KOLEJCE_DO_LEKARZA 2
#define W_KOLEJCE_DO_SALONU 3
#define W_SALONIE 4
#define ZAKONCZONY 5 


struct message
{
	int state;
	int modelek;
};


void salon()
{
	int a;
}

//argumenty -liczba lekarzy, pojemmność salonu
//do dolosowania - ilość reprezentowanych modelek < pojemność salonu
int main(int argc, char **argv)
{
	int rank, size, dane,liczba_modelek;
	char processor_name[64];

	MPI_Init( &argc, &argv );;

	MPI_Comm_size( MPI_COMM_WORLD, &size );
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	MPI_Get_processor_name( processor_name, &size);



	// int seed = time(&rank);
	srand(time(NULL)+rank);
	if(argc > 2)
		liczba_modelek = rand()% (int) strtol(argv[2],NULL,10);
	else
		liczba_modelek = 1;

	printf(" %d at %s \n", rank, processor_name );

	//MPI_Bcast( &dane, 1, MPI_INT, 0, MPI_COMM_WORLD );
	printf("liczba modelek: %d\n", liczba_modelek );
	MPI_Finalize();
}
