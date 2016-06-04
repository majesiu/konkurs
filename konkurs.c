#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TRUE 1
#define FALSE 0

#define ZAKONCZONY 5

#define PRZED_LEKARZEM 9
#define CHCE_DO_LEKARZA 10
#define U_LEKARZA 11
#define CHCE_DO_SALONU 12
#define W_SALONIE 13

#define ZAPYTANIE_O_STAN_KOLEJKI 100
#define INFORMACJA_O_STANIE_KOLEJKI 101
#define WYJSCIE_OD_LEKARZA 102
#define WYJSCIE_Z_SALONU 103
#define ZAPYTANIE_O_STAN_SALONU 104
#define INFORMACJA_O_STANIE_SALONU 105

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

void w_salonie(int rank,int liczba_modelek){
	//sleep(50*liczba_modelek);
	printf("%i: Liczba modelek obsłużonych w salonie: %i\n",rank, liczba_modelek);
}

void chce_do_salonu(int *stan, int rank, int size, int modelek, int *zegar_logiczny) {
	int i=0;
	printf("SIZE: %i\n", size);
	for(i=0; i<size; i++) { // size cos nie tak
		if(rank == i) continue;
		else {
			printf("Proces: %i wysyla zapytanie o stan kolejki do salonu %i do procesu %i\n", rank, modelek, i);
			MPI_Send_Clock(&modelek, i, ZAPYTANIE_O_STAN_SALONU, zegar_logiczny, rank);
		}	
	}
}

void wyjscie_z_salonu(int rank, int size, int *zegar_logiczny, int liczba_modelek) {
	int i=0;
	for(i=0; i<size; i++) {
		if(rank == i) continue;
		else {
			MPI_Send_Clock(&liczba_modelek, i, WYJSCIE_Z_SALONU, zegar_logiczny, rank);		
		}
	}
}

void czekajac_na_odpowiedzi(int *stan, int *zegar_logiczny, int lekarz_id, int rank, int size, int modelek, int miejsca) {
	int *buf = malloc(sizeof(int));
	int lekarz_odebrane = 0;
	int lekarz_kolejka = 0;
	int salon_odebrane = 0;
	int ile_w_salonie = 0;
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
						if(zegar_info == WYSLANE_ROWNO) {
							if(rank > status.MPI_SOURCE) lekarz_kolejka++;
						}
						if(zegar_info == WYSLANE_WCZESNIEJ) {
							lekarz_kolejka++;
						}
						//lekarz_kolejka++;
					}
				}
				if(lekarz_odebrane == size-1) { // !!! NA SIZE-1 !!!!
					printf("%i: DostaleM wszystkie odpowiedzi\n", rank);
					if(lekarz_kolejka == 0 && *stan == CHCE_DO_LEKARZA) {
						printf("%i: Wchodze do lekarza!\n", rank);
						*stan = U_LEKARZA;
						czy_petla=FALSE;			
					}
				}
				break;
			case WYJSCIE_OD_LEKARZA:
				lekarz_kolejka--;
				printf(" Wyjscie od lekarza\n");
				if(lekarz_odebrane == size-1) { // !!! NA SIZE-1 !!!!
					printf("%i: DostaleM wszystkie odpowiedzi\n", rank);
					if(lekarz_kolejka == 0 && *stan == CHCE_DO_LEKARZA) {
						printf("%i: Wchodze do lekarza!\n", rank);
						*stan = U_LEKARZA;
						czy_petla=FALSE;			
					}
				}				
				break;
			case ZAPYTANIE_O_STAN_SALONU:
				printf("Interesuje go stan kolejki do salonu %i\n", *buf);
				if(*stan == CHCE_DO_SALONU) {
					MPI_Send_Clock(modelek, status.MPI_SOURCE, INFORMACJA_O_STANIE_SALONU, zegar_logiczny, rank);				
				}
				else {
					int inny_stan = -1;
					MPI_Send_Clock(&inny_stan, status.MPI_SOURCE, INFORMACJA_O_STANIE_SALONU, zegar_logiczny, rank);					
				}
				break;
			case INFORMACJA_O_STANIE_SALONU:
				salon_odebrane++;
				printf("%i Odeslal nam wynik do zapytania o stan salonu %i\n",status.MPI_SOURCE, *buf);
				if(*buf == -1) {
					printf("Proces nie jest w salonie");				
				}
				else {
					printf("Proces jest w salonie bądź w kolejce do niego \n");
					/*if(zegar_info == WYSLANE_ROWNO) {
						if(rank > status.MPI_SOURCE) lekarz_kolejka++;
					}
					if(zegar_info == WYSLANE_WCZESNIEJ) {
						lekarz_kolejka++;
					}*/
					ile_w_salonie+=*buf;
				}
				if(salon_odebrane == size-1) { // !!! NA SIZE-1 !!!!
					printf("%i: DostaleM wszystkie odpowiedzi do do salonu\n", rank);
					if(ile_w_salonie + modelek <=  miejsca && *stan == CHCE_DO_SALONU) {
						printf("%i: Wchodze do salonu!\n", rank);
						*stan = W_SALONIE;
						czy_petla=FALSE;			
					}
				}
				break;
			case WYJSCIE_Z_SALONU:
				printf(" Wyjscie z salonu\n");		
				ile_w_salonie-=*buf;
				if(salon_odebrane == size-1) { // !!! NA SIZE-1 !!!!
					printf("%i: DostaleM wszystkie odpowiedzi do do salonu\n", rank);
					if(ile_w_salonie + modelek <=  miejsca && *stan == CHCE_DO_SALONU) {
						printf("%i: Wchodze do salonu!\n", rank);
						*stan = W_SALONIE;
						czy_petla=FALSE;			
					}
				}
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
	srand(time(NULL)+rank);
	int miejsca =(int) strtol(argv[2],NULL,10); 
	if(argc > 2)
		liczba_modelek = rand()%( (int) strtol(argv[2],NULL,10) - 1) + 1;
	else
		liczba_modelek = 1;

	printf(" %d at %s \n", rank, processor_name );
	
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
					if(liczba_modelek > 0) {
						stan = CHCE_DO_SALONU;
					}
					else {
						stan = ZAKONCZONY;
						//wyslanie info o koncu i tyle					
					}
					break;
				case CHCE_DO_SALONU:
					chce_do_salonu(&stan, rank, 4, liczba_modelek, zegar_logiczny);
					czy_czekamy_na_odpowiedz=TRUE;
					break;
				case W_SALONIE:
					w_salonie(rank,liczba_modelek);
					wyjscie_z_salonu(rank,4,zegar_logiczny,liczba_modelek);
					stan = ZAKONCZONY;
					printf ("proces %i z %d modelkami udaje się na konkurs",rank,liczba_modelek);
					break;
				case ZAKONCZONY:
					czy_czekamy_na_odpowiedz=TRUE;
					break;

			}
		}
		else {
			czekajac_na_odpowiedzi(&stan, zegar_logiczny, wybrany_lekarz, rank, 4, liczba_modelek, miejsca);
			czy_czekamy_na_odpowiedz = FALSE;			
		}
	}
	/*int lekarz_id = rand () % (int)strtol(argv[1],NULL,10);
	czekaj_na_lekarza(lekarz_id);*/
	MPI_Finalize();
}
