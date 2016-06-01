#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define TRUE 1
#define FALSE 0
#define PRZED_LEKARZEM 1
#define W_KOLEJCE_DO_LEKARZA 2
#define W_KOLEJCE_DO_SALONU 3
#define W_SALONIE 4
#define ZAKONCZONY 5

#define CHCE_DO_LEKARZA 10
#define U_LEKARZA 11

#define ZAPYTANIE_O_STAN_KOLEJKI 100
#define INFORMACJA_O_STANIE_KOLEJKI 101
#define WYJSCIE_OD_LEKARZA 102

#define WYSLANE_ROWNO 0 
#define WYSLANE_WCZESNIEJ -1
#define WYSLANE_POZNIEJ 1
struct message
{
	int state;
	int modelek;
};
void zeruj_zegar(int *zegar_logiczny, int size) {
	int i=0;
	for(i=0; i<size; i++) {
		zegar_logiczny = 0;
	}
}
void przed_lekarzem(int *stan) {
	// sleep albo cos tu mozna dodac
	*stan = CHCE_DO_LEKARZA;
}
void chce_do_lekarza(int *stan, int rank, int size, int lekarz_id, int *zegar_logiczny) {
	int i=0;
	printf("SIZE: %i\n", size);
	for(i=0; i<size; i++) { // size cos nie tak
		if(rank == i) continue;
		else {
			printf("Proces: %i wysyla zapytanie o stan kolejki do wskazanego lekarza %i do procesu %i\n", rank, lekarz_id, i);
			MPI_Send_Clock(&lekarz_id, i, ZAPYTANIE_O_STAN_KOLEJKI, zegar_logiczny, rank);
		}	
	}
}
void u_lekarza(int rank, int *liczba_modelek) {
	int i=0;
	int modelki = *liczba_modelek;
	for(i=0; i<modelki; i++) {
		//sleep(100);
		int czy_swinia = rand() % 10;
		if(czy_swinia == 1) {
			*liczba_modelek--;		
		}
	}
	printf("%i: pozostala liczba modelek po wyjsciu od lekarza to %i\n",rank, *liczba_modelek);
}
void wyjscie_od_lekarza(int rank, int size, int *zegar_logiczny) {
	int i=0;
	for(i=0; i<size; i++) {
		if(rank == i) continue;
		else {
			MPI_Send_Clock(&i, i, WYJSCIE_OD_LEKARZA, zegar_logiczny, rank);		
		}
	}
}
void czekajac_na_odpowiedzi(int *stan, int *zegar_logiczny, int lekarz_id, int rank, int size) {
	int *buf = malloc(sizeof(int));
	int lekarz_odebrane = 0;
	int lekarz_kolejka = 0;
	MPI_Status status;
	int czy_petla = TRUE;
	while(czy_petla) {
		int zegar_info = MPI_Recv_Clock(buf,MPI_ANY_SOURCE, MPI_ANY_TAG, &status, zegar_logiczny, rank);
		printf("Odebrano wiadoMosc od: %i -", status.MPI_SOURCE);
		switch(status.MPI_TAG) {
			case ZAPYTANIE_O_STAN_KOLEJKI:
				printf("Interesuje go stan kolejki do lekarza %i\n", *buf);
				if(*stan == CHCE_DO_LEKARZA) {
					MPI_Send_Clock(&lekarz_id, status.MPI_SOURCE, INFORMACJA_O_STANIE_KOLEJKI, zegar_logiczny, rank);				
				}
				else {
					int inny_stan = -1;
					MPI_Send_Clock(&inny_stan, status.MPI_SOURCE, INFORMACJA_O_STANIE_KOLEJKI, zegar_logiczny, rank);					
				}
				break;
			case INFORMACJA_O_STANIE_KOLEJKI:
				lekarz_odebrane++;
				printf("%i Odeslal naM wynik do zapytania o stan kolejki do lekarza %i\n",status.MPI_SOURCE, *buf);
				if(*buf == -1) {
					printf("Proces nie jest w kolejce do lekarza");				
				}
				else {
					if(lekarz_id != *buf) {
						printf("Proces nie jest zainteresowany naszyM lekarzeM\n");
					}
					else {
						printf("Proces czeka w kolejce z naMi\n");
						/*if(zegar_info == WYSLANE_ROWNO) {
							if(rank > status.MPI_SOURCE) lekarz_kolejka++;
						}
						if(zegar_info == WYSLANE_WCZESNIEJ) {
							lekarz_kolejka++;
						}*/
						lekarz_kolejka++;
					}
				}
				if(lekarz_odebrane == size-1) { // !!! NA SIZE-1 !!!!
					printf("%i: DostaleM wszystkie odpowiedzi\n", rank);
					if(lekarz_kolejka == 0) {
						printf("%i: Wchodze do lekarza!\n", rank);
						*stan = U_LEKARZA;
						czy_petla=FALSE;			
					}
				}
				break;
			case WYJSCIE_OD_LEKARZA:
				printf(" Wyjscie od lekarza\n");				
				break;
		}
	}

	free(buf);
}
//nakladka na send - zegar
int MPI_Send_Clock(const void *buf, int dest, int tag, int *zegar_logiczny, int rank) {
	zegar_logiczny[rank]++;
	int *new_buff = malloc(2*sizeof(int));
	memcpy(new_buff, buf, sizeof(int));
	new_buff[1] = zegar_logiczny[dest];
	int return_value = MPI_Send(new_buff, 2, MPI_INTEGER, dest, tag, MPI_COMM_WORLD); // 2 INTY
	free(new_buff);
}
// nakladka na recv - zegar
int MPI_Recv_Clock(void *buf, int source, int tag, MPI_Status *status, int *zegar_logiczny, int rank) {
	int *new_buff = malloc(2*sizeof(int));
	MPI_Recv(new_buff, 2, MPI_INTEGER, source, tag,  MPI_COMM_WORLD, status);
	int result;
	// zegar
	if(zegar_logiczny[rank] < new_buff[1]) {
		result= WYSLANE_POZNIEJ;
		zegar_logiczny[rank]=new_buff[1];
	}
	if(zegar_logiczny[rank] == new_buff[1]) {
		result = WYSLANE_ROWNO;
	}
	if(zegar_logiczny[rank] > new_buff[1]) {
		result = WYSLANE_WCZESNIEJ; 	
	}
	////////
	memcpy(buf, new_buff, sizeof(int));
	return result;
}
int main(int argc, char **argv)
{
	int rank, size, dane,liczba_modelek;
	char processor_name[64];

	MPI_Init( &argc, &argv );

	MPI_Comm_size( MPI_COMM_WORLD, &size );
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	MPI_Get_processor_name( processor_name, &size);
	int *zegar_logiczny = malloc(sizeof(int)*size);
	zeruj_zegar(zegar_logiczny, size);
	// int seed = time(&rank);
	srand(time(NULL)+rank);
	if(argc > 2)
		liczba_modelek = rand()% (int) strtol(argv[2],NULL,10);
	else
		liczba_modelek = 1;

	printf(" %d at %s \n", rank, processor_name );
	
	//MPI_Bcast( &dane, 1, MPI_INT, 0, MPI_COMM_WORLD );
	
	printf("liczba modelek: %d\n", liczba_modelek );
	int stan = PRZED_LEKARZEM;
	int wybrany_lekarz = rand () % (int)strtol(argv[1],NULL,10);
	int czy_czekamy_na_odpowiedz = FALSE;
	while(TRUE) {
		if(czy_czekamy_na_odpowiedz==FALSE) {
			switch(stan) {
				case PRZED_LEKARZEM:
					przed_lekarzem(&stan);			
					break;
				case CHCE_DO_LEKARZA:
					chce_do_lekarza(&stan, rank, 4, wybrany_lekarz, zegar_logiczny);
					czy_czekamy_na_odpowiedz=TRUE;
					break;
				case U_LEKARZA:
					u_lekarza(rank, &liczba_modelek);
					wyjscie_od_lekarza(rank, 4, zegar_logiczny);
					// TODO
					stan = 999;
					if(liczba_modelek > 0) {
					}
					else {
						//wyslanie info o koncu i tyle					
					}
					break;
			}
		}
		else {
			czekajac_na_odpowiedzi(&stan, zegar_logiczny, wybrany_lekarz, rank, 4);
			czy_czekamy_na_odpowiedz = FALSE;			
		}
	}
	/*int lekarz_id = rand () % (int)strtol(argv[1],NULL,10);
	czekaj_na_lekarza(lekarz_id);*/
	MPI_Finalize();
}
