#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include "GrilleSDL.h"
#include "Ressources.h"
#include "ClientTetris.h"

// Dimensions de la grille de jeu
#define NB_LIGNES   14
#define NB_COLONNES 20

// Nombre de cases maximum par piece
#define NB_CASES    4

// Macros utlisees dans le tableau tab
#define VIDE        0

int tab[NB_LIGNES][NB_COLONNES]
={ {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
  
typedef struct
{
	int ligne;
	int colonne;
} CASE;

typedef struct
{
	CASE cases[NB_CASES];
	int  nbCases;
	int  image;
} PIECE;



PIECE pieces[7] = { 0,0,0,1,1,0,1,1,4,TWILIGHT,       // carre
                    0,0,1,0,2,0,2,1,4,RARITY,    // L
                    0,1,1,1,2,0,2,1,4,APPLEJACK,      // J
                    0,0,0,1,1,1,1,2,4,DERPY,       // Z
                    0,1,0,2,1,0,1,1,4,PINKIEPIE,       // S
                    0,0,0,1,0,2,1,1,4,FLUTTERSHY,      // T
                    0,0,0,1,0,2,0,3,4,RAINBOWDASH };     // I


///////////////////////////////////////////////////////////////////////////////////////////////////

struct struct_message
{
	char* string;
	int length;
	int reset;
};
struct struct_message message;
int tailleMessage;
PIECE pieceInseree;
PIECE pieceEnCours;
void* thread_piece(void*);
void* thread_message(void*);
void* thread_event(void*);
void set_message(const char* string);


pthread_mutex_t mutex_message, mutex_pieceEnCours, mutex_casesInserees, mutex_tab;
pthread_cond_t cond_casesInserees;
pthread_t hthread_message, hthread_piece, hthread_event;

int main(int argc,char* argv[])
{
	EVENT_GRILLE_SDL event;
	char buffer[80];
	char ok;
	
	srand((unsigned)time(NULL));

	// Ouverture de la grille de jeu (SDL)
	printf("(THREAD MAIN) Ouverture de la grille de jeu\n");
	fflush(stdout);
	sprintf(buffer,"!!! TETRIS ZERO GRAVITY !!!");
	if (OuvrirGrilleSDL(NB_LIGNES,NB_COLONNES,40,buffer) < 0)
	{
		printf("Erreur de OuvrirGrilleSDL\n");
		fflush(stdout);
		exit(1);
	}

	// Chargement des sprites et de l'image de fond
	ChargementImages();
	DessineSprite(12,11,VOYANT_VERT);

	ok = 0;
	
	set_message("message super long");


	pieceInseree.image;
	pieceInseree.nbCases = 0;

	pthread_mutex_init(&mutex_message, NULL);
	pthread_mutex_init(&mutex_pieceEnCours, NULL);
	pthread_mutex_init(&mutex_casesInserees, NULL);
	pthread_mutex_init(&mutex_tab, NULL);
	pthread_cond_init(&cond_casesInserees, NULL);


	pthread_create(&hthread_message, NULL, thread_message, NULL);
	pthread_create(&hthread_piece, NULL, thread_piece, NULL);
	pthread_create(&hthread_event, NULL, thread_event, NULL);

	pthread_join(hthread_event, NULL);

	exit(0);



}

void* thread_event(void* a)
{
	int ok = 0;
	int i;
	EVENT_GRILLE_SDL event;
	while(!ok)
		{
			event = ReadEvent();
			if (event.type == CROIX) ok = 1;
			else if (event.type == CLIC_DROIT)
			{
				pthread_mutex_lock(&mutex_casesInserees);
				pthread_mutex_lock(&mutex_tab);

				for (i = 0; i < pieceInseree.nbCases; ++i)
				{
					EffaceCarre(pieceInseree.cases[i].ligne, pieceInseree.cases[i].colonne);
					tab[pieceInseree.cases[i].ligne][ pieceInseree.cases[i].colonne] = VIDE;
				}
				pieceInseree.nbCases = 0;

				pthread_mutex_unlock(&mutex_tab);
				pthread_mutex_unlock(&mutex_casesInserees);
			}
			else if (event.type == CLIC_GAUCHE && tab[event.ligne][event.colonne] == VIDE
					&& event.colonne < 10)
			{
				pthread_mutex_lock(&mutex_casesInserees);
				//if (pieceInseree.nbCases < pieceEnCours.nbCases)
				{
					pieceInseree.cases[pieceInseree.nbCases].ligne = event.ligne;
					pieceInseree.cases[pieceInseree.nbCases].colonne = event.colonne;
					pieceInseree.nbCases++;
					pthread_cond_signal(&cond_casesInserees);
				}
				pthread_mutex_unlock(&mutex_casesInserees);

				pthread_mutex_lock(&mutex_pieceEnCours);
				pthread_mutex_lock(&mutex_tab);
				DessineSprite(event.ligne,event.colonne,pieceEnCours.image);
				tab[event.ligne][event.colonne] = pieceEnCours.image;
				pthread_mutex_unlock(&mutex_pieceEnCours);
				pthread_mutex_unlock(&mutex_tab);
			}
		}

		// Fermeture de la grille de jeu (SDL)
		printf("(THREAD MAIN) Fermeture de la grille..."); fflush(stdout);
		FermerGrilleSDL();

		pthread_mutex_destroy(&mutex_casesInserees);
		pthread_mutex_destroy(&mutex_message);
		pthread_mutex_destroy(&mutex_pieceEnCours);
		pthread_mutex_destroy(&mutex_tab);

		pthread_cond_destroy(&cond_casesInserees);

		printf("OK\n"); fflush(stdout);


		pthread_exit(NULL);
		return NULL;
}

void set_score(int score)
{
	char cscore[5];
	int i;
	sprintf(cscore, "%04d", score);
	for (i = 0; i < 4; ++i)
		DessineLettre(1, 15+i, cscore[i]);
}

void set_message(const char* string)
{
	int i;
	pthread_mutex_lock(&mutex_message);
	free(message.string);
	message.length = strlen(string);
	if (message.length > 8)
		message.length += 3;
	message.string = (char*) malloc(message.length+1);
	message.reset = 1;
	strcpy(message.string, string);
	if (message.length > 8)
	{
		for (i = strlen(string); i < message.length-1; ++i)
			message.string[i] = ' ';
		message.string[i] = 0;
	}
	pthread_mutex_unlock(&mutex_message);
}

void* thread_message(void* a)
{
	int i, j;
	struct timespec sleeptime;
	sleeptime.tv_sec = 0;
	sleeptime.tv_nsec = 500000000;
	while (1)
	{
		pthread_mutex_lock(&mutex_message);
		message.reset = 0;
		if (message.length)
		for (i = 0; i < message.length && !message.reset; i++)
		{
			for (j = 0; j < 8 && j < message.length; j++)
				DessineLettre(10, 11+j, message.string[(i+j) % message.length]);
			for (; j < 8; j++)
				DessineLettre(10, 11+j, ' ');

			if (message.length <= 8)
				i--;
			pthread_mutex_unlock(&mutex_message);
			nanosleep(&sleeptime, NULL);
			pthread_mutex_lock(&mutex_message);
		}
		pthread_mutex_unlock(&mutex_message);
	}
	return NULL;
}

void trier_cases_piece (PIECE* piece)
{
	int i, j;
	CASE ctemp;
	for (i = 0; i < piece->nbCases; ++i)
	{
		for (j = i+1; j < piece->nbCases; ++j)
		{
			if (piece->cases[j].ligne < piece->cases[i].ligne
					|| (piece->cases[j].ligne == piece->cases[i].ligne
						&& piece->cases[j].colonne < piece->cases[i].colonne))
			{
				ctemp = piece->cases[i];
				piece->cases[i] = piece->cases[j];
				piece->cases[j] = ctemp;
			}
		}
	}
}

void rotation_piece (PIECE* ppiece)
{
	int i, j, temp;
	int lmin, cmin;
	CASE ctemp;

	// EFFECTUER ROTATION
	for(j = rand() % 4; j >= 0; --j)
	{
		lmin = INT32_MAX, cmin = INT32_MAX;
		for (i = 0; i < ppiece->nbCases; ++i) {
			temp = ppiece->cases[i].ligne;
			ppiece->cases[i].ligne = -ppiece->cases[i].colonne;
			ppiece->cases[i].colonne = temp;

			if (ppiece->cases[i].ligne < lmin)
				lmin = ppiece->cases[i].ligne;
			if (ppiece->cases[i].colonne < cmin)
				cmin = ppiece->cases[i].colonne;

		}

		for (i = 0; i < ppiece->nbCases; ++i) {
			ppiece->cases[i].ligne -= lmin;
			ppiece->cases[i].colonne -= cmin;
		}
	}


	// REORDONNER CASES
	trier_cases_piece(ppiece);

}

void* thread_piece(void* a)
{
	int i, j, cmin, lmin, cmax, lmax, c, l;
	CASE ctemp;
	PIECE ptemp;
	int score = 0;
	char scoreString[8];

	set_score(score);

	pthread_mutex_lock(&mutex_pieceEnCours);
	pieceEnCours = pieces[0];
	pthread_mutex_unlock(&mutex_pieceEnCours);

	while (1)
	{
		pthread_mutex_lock(&mutex_pieceEnCours);

		pthread_mutex_lock(&mutex_tab);
		for (i = 0; i < pieceEnCours.nbCases; ++i) {
			//tab[pieceEnCours.cases[i].ligne + l][ pieceEnCours.cases[i].colonne + c] = VIDE;
			EffaceCarre(pieceEnCours.cases[i].ligne + l, pieceEnCours.cases[i].colonne + c);
		}
		pthread_mutex_unlock(&mutex_tab);

		pieceEnCours = pieces[rand() % 7];

		// EFFECTUER ROTATION
		rotation_piece(&pieceEnCours);


		// DETERMINER COORDONNES D'AFFICHAGE

		cmin = 0; lmin = 0; cmax = 0; lmax = 0;
		for (i = 0; i < pieceEnCours.nbCases; ++i) {
			if (pieceEnCours.cases[i].ligne < lmin)
				lmin = pieceEnCours.cases[i].ligne;
			if (pieceEnCours.cases[i].ligne > lmax)
				lmax = pieceEnCours.cases[i].ligne;


			if (pieceEnCours.cases[i].colonne < cmin)
				cmin = pieceEnCours.cases[i].colonne;
			if (pieceEnCours.cases[i].colonne > cmax)
				cmax = pieceEnCours.cases[i].colonne;
		}
		c = (cmax - cmin < 3)? 16 : 15;
		l = (lmax-lmin < 2)? 4 : 3;


		// AFFICHER PIECE
		for (i = 0; i < pieceEnCours.nbCases; ++i) {
			DessineSprite(pieceEnCours.cases[i].ligne + l, pieceEnCours.cases[i].colonne + c, pieceEnCours.image);
		}



		while (1)
		{

			pthread_mutex_lock(&mutex_casesInserees);
			while (pieceInseree.nbCases < pieceEnCours.nbCases)
			{
				pthread_mutex_unlock(&mutex_pieceEnCours);
				pthread_cond_wait(&cond_casesInserees, &mutex_casesInserees);
				pthread_mutex_lock(&mutex_pieceEnCours);
			}
			pthread_mutex_unlock(&mutex_pieceEnCours);

			// REORDONNER CASES INSEREES
			trier_cases_piece(&pieceInseree);

			ptemp = pieceInseree;

			pthread_mutex_lock(&mutex_tab);
			for(i = 0; i < ptemp.nbCases; ++i)
			{
				tab[ptemp.cases[i].ligne][ ptemp.cases[i].colonne] = VIDE;
				EffaceCarre(ptemp.cases[i].ligne, ptemp.cases[i].colonne);
			}
			pthread_mutex_unlock(&mutex_tab);


			lmin = INT32_MAX; cmin = INT32_MAX;
			for (i = 0; i < NB_CASES; ++i)
			{
				if (pieceInseree.cases[i].ligne < lmin)
					lmin = pieceInseree.cases[i].ligne;
				if (pieceInseree.cases[i].colonne < cmin)
					cmin = pieceInseree.cases[i].colonne;
			}
			for (i = 0; i < NB_CASES; ++i)
			{
				pieceInseree.cases[i].ligne -= lmin;
				pieceInseree.cases[i].colonne -= cmin;
			}

			for (i = 0; i < pieceEnCours.nbCases; ++i)
			{
				if (pieceEnCours.cases[i].colonne != pieceInseree.cases[i].colonne
						|| pieceEnCours.cases[i].ligne != pieceInseree.cases[i].ligne)
					break;
			}

			pieceInseree.nbCases = 0;
			pthread_mutex_unlock(&mutex_casesInserees);

			if (i == ptemp.nbCases)
			{
				int j;
				set_score(++score);

				pthread_mutex_lock(&mutex_tab);
				for (j = 0; j < ptemp.nbCases; ++j)
				{
					tab[ptemp.cases[j].ligne][ ptemp.cases[j].colonne] =  BRIQUE;
					DessineSprite(ptemp.cases[j].ligne, ptemp.cases[j].colonne, BRIQUE);
				}
				pthread_mutex_unlock(&mutex_tab);
				break;
			}

		}


	}
	return NULL;
}






