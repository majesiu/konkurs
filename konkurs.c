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
#define ZACZAC_KONKURS 14

#define ZAPYTANIE_O_STAN_KOLEJKI 100
#define INFORMACJA_O_STANIE_KOLEJKI 101
#define WYJSCIE_OD_LEKARZA 102
#define WYJSCIE_Z_SALONU 103
#define ZAPYTANIE_O_STAN_SALONU 104
#define INFORMACJA_O_STANIE_SALONU 105
#define INFORMACJA_O_ZAKONCZENIU 106

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
		zegar_logiczny[i] = 0;
	}
}
void przed_lekarzem(int *stan) {
	// sleep albo cos tu mozna dodac
	*stan = CHCE_DO_LEKARZA;
}
int chce_do_lekarza(int *stan, int rank, int size, int lekarz_id, int *zegar_logiczny) {
	int i=0;
	zegar_logiczny[rank]++;
	printf("Proces: %i o zegarze %i o statusie %i wysyla zapytanie o stan kolejki do wskazanego lekarza %i do wszystkich procesów\n", rank, zegar_logiczny[rank], *stan, lekarz_id);
	for(i=0; i<size; i++) { // size cos nie tak
		if(rank == i) continue;
		else {;
			MPI_Send_Clock_All(&lekarz_id, i, ZAPYTANIE_O_STAN_KOLEJKI, zegar_logiczny, rank);
		}	
	}
	return zegar_logiczny[rank];
}
void u_lekarza(int rank, int *liczba_modelek) {
	int i=0;
	int modelki = *liczba_modelek;
	for(i=0; i<modelki; i++) {
		//sleep(100);
		int czy_swinia = rand() % 10;
		if(czy_swinia == 1) {
		//	*liczba_modelek--;		
		}
	}
}
void wyjscie_od_lekarza(int id_lekarza, int rank, int size, int *zegar_logiczny) {
	int i=0;
	zegar_logiczny[rank]++;
	for(i=0; i<size; i++) {
		if(rank == i) continue;
		else {
			MPI_Send_Clock_All(&id_lekarza, i, WYJSCIE_OD_LEKARZA, zegar_logiczny, rank);		
		}
	}
}

void w_salonie(int rank,int liczba_modelek){
	//sleep(50*liczba_modelek);
	//printf("Proces: %i Liczba modelek obsłużonych w salonie: %i\n",rank, liczba_modelek);
}

int chce_do_salonu(int *stan, int rank, int size, int modelek, int *zegar_logiczny) {
	int i=0;
	//printf("SIZE: %i\n", size);
	zegar_logiczny[rank]++;
	printf("Proces: %i o zegarze %i o statusie %i wysyla zapytanie o stan kolejki do wszystkich salonów, ma on %i modelek\n", rank, zegar_logiczny[rank], *stan, modelek);
	for(i=0; i<size; i++) { // size cos nie tak
		if(rank == i) continue;
		else {
			MPI_Send_Clock_All(&modelek, i, ZAPYTANIE_O_STAN_SALONU, zegar_logiczny, rank);
		}	
	}
	return zegar_logiczny[rank];
}

void wyjscie_z_salonu(int rank, int size, int *zegar_logiczny, int liczba_modelek) {
	int i=0;
	zegar_logiczny[rank]++;
	for(i=0; i<size; i++) {
		if(rank == i) continue;
		else {
			MPI_Send_Clock_All(&liczba_modelek, i, WYJSCIE_Z_SALONU, zegar_logiczny, rank);		
		}
	}
}
void zakonczenie(int rank, int size, int *zegar_logiczny) {
	int i=0;
	zegar_logiczny[rank]++;
	for(i=0; i<size; i++) {
		if(rank == i) continue;
		else {
			MPI_Send_Clock_All(&i, i, INFORMACJA_O_ZAKONCZENIU, zegar_logiczny, rank);			
		}	
	}
}
void czekajac_na_odpowiedzi(int *stan, int *zegar_logiczny, int lekarz_id, int rank, int size, int modelek, int miejsca, int reset, int kiedy_do_lekarza, int kiedy_do_salonu, int *w_kolejce_do_salonu) {
	int *buf = malloc(sizeof(int));
	static int lekarz_odebrane = 0;
	static int lekarz_kolejka = 0;
	static int salon_odebrane = 0;
	static int ile_w_salonie = 0;
	MPI_Status status;
	int czy_petla = TRUE;
	static int ile_zakonczonych = 0;
	int *w_kolejce = malloc(size*sizeof(int));
	//static int w_kolejce_do_salonu[size];// = malloc(size*sizeof(int));
	int i;
	for (i=0;i<size;i++) w_kolejce[i]=0;
	if(reset==TRUE) {
#ifdef DEBUG
		printf("RESET\n");
#endif
		lekarz_odebrane = 0;
		lekarz_kolejka = 0;
		salon_odebrane = 0;
		ile_w_salonie = 0;	
	}
	if(ile_zakonczonych == (size-1) && *stan==ZAKONCZONY) {
					printf("Proces: %i o zegarze %i o statusie %i WYCHODZI NA TO ZE WSZYSCY SA GOTOWI DO KONKURSU!\n", rank,zegar_logiczny[rank],*stan);
					*stan = ZACZAC_KONKURS;			
					czy_petla=FALSE;	
	}
	while(czy_petla) {  
		int zegar_info = MPI_Recv_Clock(buf,MPI_ANY_SOURCE, MPI_ANY_TAG, &status, zegar_logiczny, rank);
		//printf("Proces: %i Odebrano wiadoMosc od: %i \n", rank, status.MPI_SOURCE);
		switch(status.MPI_TAG) {
			case ZAPYTANIE_O_STAN_KOLEJKI:
				printf("Proces: %i o zegarze %i o statusie %i Otrzymal zapytanie od procesu %i o stan kolejki do lekarza %i\n", rank, zegar_logiczny[rank],*stan, status.MPI_SOURCE, *buf);
				if(zegar_info < kiedy_do_lekarza && *buf == lekarz_id){
					w_kolejce[status.MPI_SOURCE]=1;
				}
				if(zegar_info == kiedy_do_lekarza && rank > status.MPI_SOURCE  && *buf == lekarz_id){
					w_kolejce[status.MPI_SOURCE]=1;
				}
				if(*stan == CHCE_DO_LEKARZA) {
					MPI_Send_Clock(&lekarz_id, status.MPI_SOURCE, INFORMACJA_O_STANIE_KOLEJKI, zegar_logiczny, rank);	
					printf("Proces: %i o zegarze %i o statusie %i Odeslal informacje o stanie kolejki do procesu %i o tresci %i\n", rank, zegar_logiczny[rank],*stan, status.MPI_SOURCE, lekarz_id);			
				}
				else {
					int inny_stan = -1;
					MPI_Send_Clock(&inny_stan, status.MPI_SOURCE, INFORMACJA_O_STANIE_KOLEJKI, zegar_logiczny, rank);
					printf("Proces: %i o zegarze %i o statusie %i Odeslal informacje o stanie kolejki do procesu %i o tresci %i\n", rank, zegar_logiczny[rank],*stan, status.MPI_SOURCE, inny_stan);					
				}
				break;
			case INFORMACJA_O_STANIE_KOLEJKI:
				lekarz_odebrane++;
				printf("Proces: %i o zegarze %i o statusie %i Proces %i Odeslal na wynik do zapytania o stan kolejki do lekarza: %i, liczba otrzymanych odp to: %i\n",rank, zegar_logiczny[rank],*stan, status.MPI_SOURCE, *buf,lekarz_odebrane);
				if(*buf == -1) {
					if(w_kolejce[status.MPI_SOURCE] == 1) {
						w_kolejce[status.MPI_SOURCE] = 0;
					}
#ifdef DEBUG
					printf("Proces nie jest w kolejce do lekarza");		
#endif		
				}
				else {
					if(lekarz_id != *buf) {
#ifdef DEBUG
						printf("Proces nie jest zainteresowany naszyM lekarzeM\n");
#endif
					}
					else {
#ifdef DEBUG
						printf("Proces czeka w kolejce z naMi\n");
#endif
					}
				}
				if(lekarz_odebrane == size-1 && *stan == CHCE_DO_LEKARZA) { // !!! NA SIZE-1 !!!!
					int ile_w_kolejce = 0;
					for(i=0; i<size; i++) {
						ile_w_kolejce+=w_kolejce[i];
					}
					printf("Proces: %i o zegarze %i o statusie %i Dostalem wszystkie odpowiedzi co do lekarza, kolejka wynosi %i\n", rank, zegar_logiczny[rank],*stan, ile_w_kolejce);
					if(ile_w_kolejce == 0 && lekarz_odebrane==size-1) {
						printf("Proces: %i o zegarze %i o statusie %i Wchodze do lekarza!\n", rank, zegar_logiczny[rank],*stan);
						*stan = U_LEKARZA;
						czy_petla=FALSE;
					}
				}
				break;
			case WYJSCIE_OD_LEKARZA:
				printf("Proces: %i o zegarze %i o statusie %i Dostal info ze proces %i wychodzi od lekarza %i, liczba otzymanych odp to: %i \n",rank, zegar_logiczny[rank],*stan ,status.MPI_SOURCE, *buf,lekarz_odebrane);
				if(*buf == lekarz_id && *stan == CHCE_DO_LEKARZA){
					w_kolejce[status.MPI_SOURCE] = 0;
					/////// sprawdzic ile jeszcze w kolejce
					int ile_w_kolejce = 0;
					for(i=0; i<size; i++) {
						ile_w_kolejce+=w_kolejce[i];
					}
					if(ile_w_kolejce == 0 && lekarz_odebrane==size-1) {
						printf("Proces: %i o zegarze %i o statusie %i Wchodze do lekarza!\n", rank, zegar_logiczny[rank],*stan);
						*stan = U_LEKARZA;
						czy_petla=FALSE;
					}
				}
/*				if(lekarz_odebrane == size-1) { // !!! NA SIZE-1 !!!!
#ifdef DEBUG
					printf("%i: DostaleM wszystkie odpowiedzi\n", rank);
#endif
					if((lekarz_kolejka <= 0) && (*stan == CHCE_DO_LEKARZA)) {
						printf("Proces: %i: Wchodze do lekarza!\n", rank);
						*stan = U_LEKARZA;
						czy_petla=FALSE;			
					}
				}	*/			
				break;
			case ZAPYTANIE_O_STAN_SALONU:
				if(zegar_info < kiedy_do_salonu){
					w_kolejce_do_salonu[status.MPI_SOURCE]=*buf;
				}
				if(zegar_info == kiedy_do_salonu && rank > status.MPI_SOURCE){
					w_kolejce_do_salonu[status.MPI_SOURCE]=*buf;
				}
				printf("Proces: %i o zegarze %i o statusie %i Otrzymal zapytania o stan salonu od %i, który ma : %i modelek\n",rank, zegar_logiczny[rank],*stan, status.MPI_SOURCE, *buf);
				if(*stan == CHCE_DO_SALONU) {
					MPI_Send_Clock(&modelek, status.MPI_SOURCE, INFORMACJA_O_STANIE_SALONU, zegar_logiczny, rank);			
					printf("Proces: %i o zegarze %i o statusie %i Odeslal zapytanie o stan salonu do %i, który ma : %i modelek o tresci: %i \n",rank, zegar_logiczny[rank],*stan, status.MPI_SOURCE, *buf,modelek);	
				}
				else {
					int inny_stan = -1;
					MPI_Send_Clock(&inny_stan, status.MPI_SOURCE, INFORMACJA_O_STANIE_SALONU, zegar_logiczny, rank);	
					printf("Proces: %i o zegarze %i o statusie %i Odeslal zapytanie o stan salonu do %i, który ma : %i modelek o tresci: %i\n",rank, zegar_logiczny[rank],*stan, status.MPI_SOURCE, *buf,inny_stan);				
				}
				break;
			case INFORMACJA_O_STANIE_SALONU:
				salon_odebrane++;
				printf("Proces: %i o zegarze %i o statusie %i Proces %i Odeslal nam wynik do zapytania o stan salonu, ma on: %i modelek\n",rank, zegar_logiczny[rank], *stan, status.MPI_SOURCE, *buf);
				if(*buf == -1) {
					if(w_kolejce_do_salonu[status.MPI_SOURCE] > 0) {
						w_kolejce_do_salonu[status.MPI_SOURCE] = 0;
					}
				}
				if(salon_odebrane == size-1) { // !!! NA SIZE-1 !!!!
					int suma_w_salonie = 0;	
					int i=0;
					for(i=0; i<size; i++) {
						suma_w_salonie+=w_kolejce_do_salonu[i];
					}
					printf("Proces: %i o zegarze %i o statusie %i Dostalem wszystkie odpowiedzi co do salonu, w salonie bądź w kolejce do niego jest %i modelek\n", rank, zegar_logiczny[rank],*stan, suma_w_salonie);

					if((suma_w_salonie + modelek <=  miejsca) && (*stan == CHCE_DO_SALONU)) {
						printf("Proces: %i o zegarze %i o statusie %i  Wchodze do salonu!\n", rank,zegar_logiczny[rank],*stan);
						*stan = W_SALONIE;
						czy_petla=FALSE;			
					}
				}
				break;
			case WYJSCIE_Z_SALONU:
			printf("Proces: %i o zegarze %i o statusie %i Dostal info ze proces %i wychodzi z salonu %i, liczba otzymanych odp to: %i \n",rank, zegar_logiczny[rank],*stan ,status.MPI_SOURCE, *buf,salon_odebrane);
				w_kolejce_do_salonu[status.MPI_SOURCE] = 0;
				if(salon_odebrane == size-1) { // !!! NA SIZE-1 !!!!
					int suma_w_salonie = 0;	
					int i=0;
					for(i=0; i<size; i++) {
						suma_w_salonie+=w_kolejce_do_salonu[i];
					}
					printf("Proces: %i o zegarze %i o statusie %i Dostalem wszystkie odpowiedzi co do salonu, w salonie bądź w kolejce do niego jest %i modelek\n", rank, zegar_logiczny[rank],*stan, suma_w_salonie);

					if((suma_w_salonie + modelek <=  miejsca) && (*stan == CHCE_DO_SALONU)) {
						printf("Proces: %i o zegarze %i o statusie %i  Wchodze do salonu!\n", rank,zegar_logiczny[rank],*stan);
						*stan = W_SALONIE;
						czy_petla=FALSE;			
					}
				}
				break;
			case INFORMACJA_O_ZAKONCZENIU:
				ile_zakonczonych++;
				printf("Proces: %i o zegarze %i o statusie %i Dostałem informacje o zakonczeniu działania procesu %i\n", rank,  zegar_logiczny[rank],*stan, status.MPI_SOURCE);
				if(ile_zakonczonych == (size-1) && *stan==ZAKONCZONY) {
					printf("Proces: %i o zegarze %i o statusie %i WYCHODZI NA TO ZE WSZYSCY SA GOTOWI DO KONKURSU!\n", rank,zegar_logiczny[rank],*stan);
					*stan = ZACZAC_KONKURS;			
					czy_petla=FALSE;	
				}
				break;
		}
	}
	free(w_kolejce);
	free(buf);
}
//nakladka na send - zegar
int MPI_Send_Clock(const void *buf, int dest, int tag, int *zegar_logiczny, int rank) {
	zegar_logiczny[rank]++;
	int *new_buff = malloc(2*sizeof(int));
	memcpy(new_buff, buf, sizeof(int));
	new_buff[1] = zegar_logiczny[rank]; // ?
	int return_value = MPI_Send(new_buff, 2, MPI_INTEGER, dest, tag, MPI_COMM_WORLD); // 2 INTY
	free(new_buff);
}
int MPI_Send_Clock_All(const void *buf, int dest, int tag, int *zegar_logiczny, int rank) {
	int *new_buff = malloc(2*sizeof(int));
	memcpy(new_buff, buf, sizeof(int));
	new_buff[1] = zegar_logiczny[rank]; // ?
	int return_value = MPI_Send(new_buff, 2, MPI_INTEGER, dest, tag, MPI_COMM_WORLD); // 2 INTY
	free(new_buff);
}
// nakladka na recv - zegar
int MPI_Recv_Clock(void *buf, int source, int tag, MPI_Status *status, int *zegar_logiczny, int rank) {
	int *new_buff = malloc(2*sizeof(int));
	MPI_Recv(new_buff, 2, MPI_INTEGER, source, tag,  MPI_COMM_WORLD, status);
	int result;
	// zegar
	//printf("Proces %i : odebral od %i, zegar %i vs %i", rank, source, zegar_logiczny[rank], new_buff[1]);
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
	zegar_logiczny[rank]++;
	return new_buff[1];
}

int main(int argc, char **argv)
{
	int rank, size, dane,liczba_modelek;
	//char processor_name[64];

	MPI_Init( &argc, &argv );

	MPI_Comm_size( MPI_COMM_WORLD, &size );
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );

	//SMPI_Get_processor_name( processor_name, &size);
	int *zegar_logiczny = malloc(sizeof(int)*size);
	zeruj_zegar(zegar_logiczny, size);
	srand(time(NULL)+rank);
	int miejsca =(int) strtol(argv[2],NULL,10); 
	if(argc > 2)
		liczba_modelek = rand()%( (int) strtol(argv[2],NULL,10) - 1) + 1;
	else
		liczba_modelek = 1;

	//printf(" %d at %s \n", rank, processor_name );
	int stan = PRZED_LEKARZEM;
	printf("Proces: %i o zegarze %i o statusie %i  liczba modelek: %d o size = %i \n", rank, zegar_logiczny[rank],stan, liczba_modelek,size);

	int wybrany_lekarz = rand () % (int)strtol(argv[1],NULL,10);
	int czy_czekamy_na_odpowiedz = FALSE;
	int czy_reset = FALSE;
	int kiedy_do_lekarza,kiedy_do_salonu;
	int *w_kolejce_do_salonu = malloc(sizeof(int)*size);
	int i;
	for (i = 0; i < size; ++i)
	{
		w_kolejce_do_salonu[i] = 0;
	}
	while(TRUE) {
		if(czy_czekamy_na_odpowiedz==FALSE) {
			switch(stan) {
				case PRZED_LEKARZEM:
					przed_lekarzem(&stan);
					czy_reset=TRUE;			
					break;
				case CHCE_DO_LEKARZA:
					kiedy_do_lekarza = chce_do_lekarza(&stan, rank, size, wybrany_lekarz, zegar_logiczny);
					czy_czekamy_na_odpowiedz=TRUE;
					break;
				case U_LEKARZA:
					u_lekarza(rank, &liczba_modelek);
					printf("Proces: %i o zegarze %i o statusie %i pozostala liczba modelek po wyjsciu od lekarza to %i \n",rank, zegar_logiczny[rank], stan, liczba_modelek);
					wyjscie_od_lekarza(wybrany_lekarz,rank, size, zegar_logiczny);
					printf("Proces: %i o zegarze %i o statusie %i wyslal info do wszystkich procesow o wyjsciu od lekarza od id %i\n", rank, zegar_logiczny[rank], stan, wybrany_lekarz);
					// TODO
					if(liczba_modelek > 0) {
						stan = CHCE_DO_SALONU;
					}
					else {
						stan = ZAKONCZONY;
					//	zakonczenie(rank, 4, zegar_logiczny);
					//	czy_czekamy_na_odpowiedz=TRUE;				
					}
					break;
				case CHCE_DO_SALONU:
					kiedy_do_salonu = chce_do_salonu(&stan, rank, size, liczba_modelek, zegar_logiczny);
					czy_czekamy_na_odpowiedz=TRUE;
					break;
				case W_SALONIE:
					w_salonie(rank,liczba_modelek);
					printf("Proces: %i o zegarze %i o statusie %i Liczba modelek obsłużonych w salonie: %i\n",rank, zegar_logiczny[rank],stan, liczba_modelek);
					wyjscie_z_salonu(rank,size,zegar_logiczny,liczba_modelek);
					printf("Proces: %i o zegarze %i o statusie %i wyslal info do wszystkich procesow o wyjsciu z salonu, ma on %i modelek \n", rank, zegar_logiczny[rank], stan, liczba_modelek);
					stan = ZAKONCZONY;
					printf ("Proces: %i o zegarze %i o statusie %i z %d modelkami udaje się na konkurs \n",rank,zegar_logiczny[rank],stan,liczba_modelek);
					break;
				case ZAKONCZONY:
					zakonczenie(rank, size, zegar_logiczny);
					printf("Proces: %i o zegarze %i o statusie %i wyslal info do wszystkich procesow o zakonczeniu dzialania\n", rank, zegar_logiczny[rank], stan);
					czy_czekamy_na_odpowiedz=TRUE;
					break;
				case ZACZAC_KONKURS:
					//exit(0); // czy petla?
					break;

			}
		}
		else {
			czekajac_na_odpowiedzi(&stan, zegar_logiczny, wybrany_lekarz, rank, size, liczba_modelek, miejsca, czy_reset ? TRUE : FALSE,kiedy_do_lekarza,kiedy_do_salonu, w_kolejce_do_salonu);
			czy_reset=FALSE;
			czy_czekamy_na_odpowiedz = FALSE;			
		}
	}
	/*int lekarz_id = rand () % (int)strtol(argv[1],NULL,10);
	czekaj_na_lekarza(lekarz_id);*/
	MPI_Finalize();
}
