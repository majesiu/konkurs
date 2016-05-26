#include <mpi.h>
int main(int argc, char **argv)
{
	int rank, size, dane;
	char processor_name[ 64];
	
	MPI_Init( &argc, &argv );

	MPI_Comm_size( MPI_COMM_WORLD, &size );
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	MPI_Get_processor_name( processor_name, &size);
	printf(" %d at %s \n", rank, processor_name );

	dane = 555;
	if ( !rank )
	{
		dane = 666;
	}
	MPI_Bcast( &dane, 1, MPI_INT, 0, MPI_COMM_WORLD );
	printf("dane %d\n", dane );
	MPI_Finalize();
}
