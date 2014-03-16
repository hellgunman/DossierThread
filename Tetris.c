
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
char majScore = 0;
int score = 0;
int nbAnalyses = 0;
int colonnesCompletes[4];
int nbColonnesCompletes = 0;
int lignesCompletes[4];
int nbLignesCompletes = 0;
int quit = 0;
char traitementEnCours = 0;

void* thread_piece(void*);
void* thread_message(void*);
void* thread_event(void*);
void* thread_score(void*);
void* thread_gravite(void*);
void set_message(const char* string);

/***** Thread Case *****/
void *thread_case(void* a);
void handlerSIGUSR1(int sig);

/***** Fin Partie *****/
void *thread_fin(void*);
void handlerSIGUSR2(int sig);

/***** TOP SCORE ****/
void* thread_top_score(void*);
void handlerSIGQUIT(int);


/***** Joueur Connecté *****/
void *thread_joueursconnectes(void *);
key_t ser_key = 0;
char pseudo[20] = {"Anonyme"};
void handlerSIGHUP(int sig);

pthread_key_t key;

void LiberonsLesKeys(void *p) {free(pthread_getspecific(key));}

pthread_mutex_t mutex_message, mutex_pieceEnCours, mutex_casesInserees,
	mutex_tab, mutex_threadcase, mutex_analyse, mutex_score, mutex_traitement;
pthread_cond_t cond_casesInserees, cond_analyse, cond_score;
pthread_t hthread_message, hthread_piece, hthread_event, hthread_gravite, hthread_score,
	tabthreadcase[14][10], hthread_fin, hthread_top_score, hthread_joueursconnectes;

int main(int argc,char* argv[])
{
	EVENT_GRILLE_SDL event;
	char buffer[80];
	char ok;
	int i, j;
	CASE c;
	struct sigaction sigact;
	
	pthread_key_create(&key, LiberonsLesKeys);

	srand((unsigned)time(NULL));
	
    sigset_t mask, oldmask;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigemptyset(&mask);
	
	/// SIGUSR1
	sigact.sa_handler = handlerSIGUSR1;
    sigact.sa_flags = 0;
    sigaction(SIGUSR1, &sigact, NULL);

	/// SIGUSR2
	sigact.sa_handler=handlerSIGUSR2;
    sigaction(SIGUSR2, &sigact, NULL);

    // SIGQUIT
	sigact.sa_handler=handlerSIGQUIT;
    sigaction(SIGQUIT, &sigact, NULL);

    // SIGHUP
    	sigact.sa_handler=handlerSIGHUP;
        sigaction(SIGHUP, &sigact, NULL);

    // MASQUAGE DES SIGNAUX

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
	


	pieceInseree.image;
	pieceInseree.nbCases = 0;

	pthread_mutex_init(&mutex_message, NULL);
	pthread_mutex_init(&mutex_pieceEnCours, NULL);
	pthread_mutex_init(&mutex_casesInserees, NULL);
	pthread_mutex_init(&mutex_tab, NULL);
	pthread_mutex_init(&mutex_score, NULL);
	pthread_mutex_init(&mutex_analyse, NULL);
	pthread_mutex_init(&mutex_traitement, NULL);
	pthread_cond_init(&cond_casesInserees, NULL);
	pthread_cond_init(&cond_score, NULL);
	pthread_cond_init(&cond_analyse, NULL);


	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);
	sigaddset(&mask, SIGHUP);
	sigaddset(&mask, SIGQUIT);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);


	if(argc >= 2)
	{
		ser_key = atoi(argv[1]);
                printf("(THREAD MAIN) ser_key = %d\n", ser_key);

		//if(argc == 3)
		strcpy(pseudo, argv[2]);

		/// SIGHUP
		sigact.sa_handler = handlerSIGHUP;
		sigaction(SIGHUP, &sigact, NULL);


		pthread_create(&hthread_joueursconnectes, NULL, thread_joueursconnectes, NULL);
		pthread_create(&hthread_top_score, NULL, thread_top_score, NULL);
		//pthread_kill(hthread_joueursconnectes, SIGHUP);
		handlerSIGQUIT(0);
    }
    else
    {
        DessineLettre(8, 15, ' ');
        DessineLettre(8, 16, ' ');
        DessineLettre(8, 17, ' ');
        DessineLettre(8, 18, ' ');
    	set_message("hors ligne");
        DessineLettre(12, 17, ' ');
        DessineLettre(12, 18, ' ');


	}



	pthread_create(&hthread_score, NULL,thread_score, NULL);
	pthread_create(&hthread_message, NULL, thread_message, NULL);
        
        pthread_mutex_lock(&mutex_pieceEnCours);
        
	pthread_create(&hthread_piece, NULL, thread_piece, NULL);
	pthread_create(&hthread_event, NULL, thread_event, NULL);
        pthread_mutex_lock(&mutex_pieceEnCours);
        pthread_mutex_unlock(&mutex_pieceEnCours);
	pthread_create(&hthread_gravite, NULL, thread_gravite, NULL);
	pthread_create(&hthread_fin, NULL, thread_fin, NULL);
	
	/*
	sigaddset(&oldmask, SIGUSR1);
	sigaddset(&oldmask, SIGUSR2);
	sigaddset(&oldmask, SIGHUP);
	sigaddset(&oldmask, SIGQUIT);
	sigprocmask(SIG_SETMASK, &oldmask, NULL);
*/
	
	
	/**********************************/
	pthread_mutex_lock(&mutex_threadcase);
    for(i=0;i<14;i++)
	{
        for(j=0;j<10;j++)
		{
            c.ligne = i;
            c.colonne = j;

			pthread_create(&tabthreadcase[i][j], NULL, thread_case, &c);
            pthread_mutex_lock(&mutex_threadcase);
        }
    }
    pthread_mutex_unlock(&mutex_threadcase);
	
	pthread_join(hthread_fin, NULL);

	if (!quit)
		pthread_cancel(hthread_event);
	for (i = 0; i < 14; ++i)
		for (j = 0; j < 10; ++j)
			pthread_cancel(tabthreadcase[i][j]);
	for (i = 0; i < 14; ++i)
		for (j = 0; j < 10; ++j)
			pthread_join(tabthreadcase[i][j], NULL);

	pthread_cancel(hthread_score);
	pthread_join(hthread_score, NULL);
        
	if (argc >= 2)
	{
		pthread_cancel(hthread_joueursconnectes);
		pthread_join(hthread_joueursconnectes, NULL);
		pthread_cancel(hthread_top_score);
		pthread_join(hthread_top_score, NULL);
	}


	printf("(THREAD MAIN) ATTENTE CROIX\n");

	while (!quit)
	{
		event = ReadEvent();
		if (event.type == CROIX)
			quit = 1;
	}

	pthread_cancel(hthread_message);


	// Fermeture de la grille de jeu (SDL)
	printf("(THREAD MAIN) Fermeture de la grille...\n"); fflush(stdout);
	FermerGrilleSDL();


	pthread_mutex_destroy(&mutex_casesInserees);
	pthread_mutex_destroy(&mutex_message);
	pthread_mutex_destroy(&mutex_pieceEnCours);
	pthread_mutex_destroy(&mutex_tab);
	pthread_mutex_destroy(&mutex_score);
	pthread_mutex_destroy(&mutex_analyse);

	pthread_cond_destroy(&cond_casesInserees);
	pthread_cond_destroy(&cond_score);
	pthread_cond_destroy(&cond_analyse);

	exit(0);



}

void* thread_event(void* a)
{
	int ok = 0;
	int i, j;
	EVENT_GRILLE_SDL event;
	struct timespec tempo;
	tempo.tv_sec = 0;
	tempo.tv_nsec = 250000000;
	while(!ok)
		{
			event = ReadEvent();
			if (event.type == CROIX) ok = 1;
			else if (event.type == CLIC_DROIT  && traitementEnCours == 0)
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
					&& event.colonne < 10 && traitementEnCours == 0)
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

				if(pieceInseree.nbCases == pieceEnCours.nbCases)
				{
					pthread_mutex_lock(&mutex_traitement);
					traitementEnCours = 1;
					pthread_mutex_unlock(&mutex_traitement);
					DessineSprite(12, 11, VOYANT_BLEU);
				}

				pthread_mutex_unlock(&mutex_pieceEnCours);
				pthread_mutex_unlock(&mutex_tab);
			}
			else if (event.type == CLIC_GAUCHE && tab[event.ligne][event.colonne] != VIDE
					&& event.colonne < 10)
			{
				pthread_mutex_lock(&mutex_tab);
				DessineSprite(12, 11, VOYANT_ROUGE);

				nanosleep(&tempo, NULL);

				if(traitementEnCours != 1)
					DessineSprite(12, 11,VOYANT_VERT);
				pthread_mutex_unlock(&mutex_tab);
			}
			
		}

		quit = 1;
		pthread_cancel(hthread_fin);


		printf("OK\n"); fflush(stdout);


		pthread_exit(NULL);
		return NULL;
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
void message_cleanup(void*)
{
	pthread_mutex_lock(&mutex_message);
	free(message.string);
	message.length = 0;
	pthread_mutex_unlock(&mutex_message);
}

void* thread_message(void* a)
{
	int i, j;
	struct timespec sleeptime;
	sleeptime.tv_sec = 0;
	sleeptime.tv_nsec = 500000000;

	pthread_cleanup_push(message_cleanup, NULL);

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
	pthread_cleanup_pop(1);
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
	char scoreString[8];


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
//pieceEnCours = pieces[0]; /// TRIIIIIIIIIIIIIIIIIIIICHE
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
				pthread_mutex_lock(&mutex_score);
				score++;
				majScore = 1;
				pthread_cond_signal(&cond_score);
				pthread_mutex_unlock(&mutex_score);


				pthread_mutex_lock(&mutex_tab);
				for (j = 0; j < ptemp.nbCases; ++j)
				{
					tab[ptemp.cases[j].ligne][ ptemp.cases[j].colonne] =  BRIQUE;
					DessineSprite(ptemp.cases[j].ligne, ptemp.cases[j].colonne, BRIQUE);
				}
				pthread_mutex_unlock(&mutex_tab);

				//////////////////////////////////////////////
				pthread_mutex_lock(&mutex_analyse);
				nbColonnesCompletes = 0;
				nbLignesCompletes = 0;
				nbAnalyses = 0; /******************* verif*/
				pthread_mutex_unlock(&mutex_analyse);

				i = 0;
				while(i < pieceEnCours.nbCases)
				{
					pthread_kill(tabthreadcase[ptemp.cases[i].ligne][ptemp.cases[i].colonne], SIGUSR1);
					++i;
				}
				break;
			}
			else
			{
				pthread_mutex_lock(&mutex_traitement);
				traitementEnCours = 0;
				pthread_mutex_unlock(&mutex_traitement);
				DessineSprite(12, 11, VOYANT_VERT);
			}

		}


	}
	return NULL;
}

void score_cleanup(void*)
{
	printf("(THREAD SCORE) GAME OVER\n");
        if (ser_key)
        {
            int ret = EnvoiScore(ser_key, score);
            if( ret == -1)
            {
                printf("(THREAD SCORE) ERREUR D'ENVOI DU SCORE\n");
            }
            else if (ret == 1)
            {
                printf("(THREAD SCORE) MEILLEUR SCORE\n");
                set_message("Game Over mais vous avez battu le meilleur score");
            }
            else if (ret == 0)
            {
                printf("(THREAD SCORE) MOINS BON\n");
                set_message("Game Over");
            }
            else                
                printf("(THREAD SCORE) OUATE DE PHOQUE V2 %d!\n", ret);
        }
	else
            printf("(THREAD SCORE) OUATE DE PHOQUE %d\n", ser_key);
}

void *thread_score(void *a) {
    printf("(THREAD SCORE) Lancement du thread Score\n");
    char cscore[5];
    int i;
    pthread_cleanup_push(score_cleanup, NULL);
    sprintf(cscore, "   0");
	for (i = 0; i < 4; ++i)
		DessineLettre(1, 15+i, cscore[i]);
    pthread_mutex_lock(&mutex_score);
    while(!majScore) {
        printf("(THREAD SCORE) MAJ du score : '%4d'\n", score);
        pthread_cond_wait(&cond_score, &mutex_score);

        sprintf(cscore, "%4d", score);
    	for (i = 0; i < 4; ++i)
    		DessineLettre(1, 15+i, cscore[i]);
        majScore = 0;
    }
    pthread_mutex_unlock(&mutex_score);
    pthread_cleanup_pop(1);
    return NULL;
}

/***************************** THREAD CASE ***************************************/
void *thread_case(void* a)
{
    CASE *c = (CASE*)malloc(sizeof(CASE));
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
	
    if(!c)
        fprintf(stderr, "Erreur malloc thread CASE\n");

    pthread_setspecific(key, c);

    *c = *(CASE*) a;
    pthread_mutex_unlock(&mutex_threadcase);

    while(1)
        pause();
    return NULL;
}

void handlerSIGUSR1(int sig)
{
	int i;
	CASE *c = (CASE*)pthread_getspecific(key);
        
        printf("(THREAD CASE) SIGUSR1 RECEIVED\n");

	i = 0;
	while(i < 10)
	{
        pthread_mutex_lock(&mutex_tab);

        if(tab[c->ligne][i] == 0)
		{
            pthread_mutex_unlock(&mutex_tab);
            break;
        }

        pthread_mutex_unlock(&mutex_tab);
		i++;
    }

    if(i == 10)
	{
        pthread_mutex_lock(&mutex_analyse);

        i = 0;
        while(i < nbLignesCompletes)
		{
            if(lignesCompletes[i] == c->ligne)
                break;
            i++;
        }

        if(i == nbLignesCompletes)
		{
            lignesCompletes[nbLignesCompletes] = c->ligne;
            nbLignesCompletes++;

			i = 0;
			while(i < 10)
            {
                DessineSprite(c->ligne,i,FUSION);
				i++;
            }
        }

        pthread_mutex_unlock(&mutex_analyse);
    }

	i = 0;
	while(i < 14)
	{
        pthread_mutex_lock(&mutex_tab);

        if(tab[i][c->colonne] == 0)
		{
            pthread_mutex_unlock(&mutex_tab);
            break;
        }

        pthread_mutex_unlock(&mutex_tab);
		i++;
    }

    if(i == 14)
	{
        pthread_mutex_lock(&mutex_analyse);

		i = 0;
        while(i < nbColonnesCompletes)
		{
            if(colonnesCompletes[i] == c->colonne)
                break;
            i++;
        }

        if(i == nbColonnesCompletes)
		{
			colonnesCompletes[nbColonnesCompletes] = c->colonne;
            nbColonnesCompletes++;

            i = 0;
			while(i < 14)
			{
                DessineSprite(i,c->colonne,FUSION);
				i++;
            }
        }

        pthread_mutex_unlock(&mutex_analyse);
    }

    pthread_mutex_lock(&mutex_analyse);
    nbAnalyses++;
    pthread_cond_signal(&cond_analyse);
    pthread_mutex_unlock(&mutex_analyse);
}
/*********************************************************************************/

void* thread_gravite(void*)
{
	struct timespec nano, nano2;
	nano.tv_sec = 2;
	nano.tv_nsec = 0;
	nano2.tv_sec = 0;
	nano2.tv_nsec = 500000000;
	int i, j, k;
	int temp;
	printf("(THREAD GRAVITE) STARTED\n");
	while (1)
	{
		// ATTENTE DE LA CONDITION
		pthread_mutex_lock(&mutex_analyse);
		pthread_mutex_lock(&mutex_pieceEnCours);
		while (nbAnalyses < pieceEnCours.nbCases)
		{
			pthread_mutex_unlock(&mutex_pieceEnCours);
			pthread_cond_wait(&cond_analyse, &mutex_analyse);

			pthread_mutex_lock(&mutex_pieceEnCours);
		}
		pthread_mutex_unlock(&mutex_pieceEnCours);
		pthread_mutex_unlock(&mutex_analyse);

                printf("(THREAD GRAVITE) CALCULER GRAVITE %d %d\n", nbAnalyses, pieceEnCours.nbCases);

		if (nbColonnesCompletes == 0 && nbLignesCompletes == 0)
		{

			/// VERIFIER SI POSABLE
			pthread_kill(hthread_fin, SIGUSR2);
			nbAnalyses = 0;
			continue;
		}
		printf("(THREAD GRAVITE) COND RECEIVED\n");
		
		pthread_mutex_lock(&mutex_traitement);
		traitementEnCours = 1;
		pthread_mutex_unlock(&mutex_traitement);
		DessineSprite(12, 11, VOYANT_BLEU);

		for (i = 0; i < 4; i++)
		{
			for (j = i+1; j < 4; j++)
			{
				if (j < nbColonnesCompletes && colonnesCompletes[j] < colonnesCompletes[i])
				{
					temp = colonnesCompletes[j];
					colonnesCompletes[j] = colonnesCompletes[i];
					colonnesCompletes[i] = temp;
				}
				if (j < nbLignesCompletes && lignesCompletes[j] < lignesCompletes[i])
				{
					temp = lignesCompletes[j];
					lignesCompletes[j] = lignesCompletes[i];
					lignesCompletes[i] = temp;
				}
			}
		}

		// REMPLACE PAR FUSION
		for (i = 0; i < nbColonnesCompletes; i++)
		{
			pthread_mutex_lock(&mutex_tab);
			for (j = 0; j < NB_LIGNES; j++)
			{
				tab[j][colonnesCompletes[i]] = FUSION;
				DessineSprite(j, colonnesCompletes[i], FUSION);
			}
			pthread_mutex_unlock(&mutex_tab);
		}
		for (i = 0; i < nbLignesCompletes; i++)
		{

			pthread_mutex_lock(&mutex_tab);
			for (j = 0; j < 10; j++)
			{
				tab[lignesCompletes[i]][j] = FUSION;
				DessineSprite(lignesCompletes[i],j, FUSION);
			}
			pthread_mutex_unlock(&mutex_tab);
		}

		pthread_mutex_lock(&mutex_score);
		score += 5;
		majScore = 1;
		pthread_cond_signal(&cond_score);
		pthread_mutex_unlock(&mutex_score);

		nanosleep(&nano, NULL);

		// VIDER CASES
		for (i = 0; i < nbColonnesCompletes; i++)
		{
			pthread_mutex_lock(&mutex_tab);
			for (j = 0; j < NB_LIGNES; j++)
			{
				tab[j][colonnesCompletes[i]] = VIDE;
				EffaceCarre(j, colonnesCompletes[i]);
			}
			pthread_mutex_unlock(&mutex_tab);
		}
		for (i = 0; i < nbLignesCompletes; i++)
		{

			pthread_mutex_lock(&mutex_tab);
			for (j = 0; j < 10; j++)
			{
				tab[lignesCompletes[i]][j] = VIDE;
				EffaceCarre(lignesCompletes[i],j);
			}
			pthread_mutex_unlock(&mutex_tab);
		}

		nanosleep(&nano2, NULL);

		// DECALER CASES

		// DE GAUCHE A DROITE
		for (i = 0; i < nbColonnesCompletes && colonnesCompletes[i] < 5; i++)
		{
			pthread_mutex_lock(&mutex_tab);
			for (k = colonnesCompletes[i]; k >= 0; k--)
			{
				for (j = 0; j < NB_LIGNES; j++)
				{
					if (k == 0 || tab[j][k-1] == VIDE)
					{
						tab[j][k] = VIDE;
						EffaceCarre(j, k);
					}
					else
					{
						tab[j][k] = tab[j][k-1];
						DessineSprite(j, k, tab[j][k-1]);
					}
				}
			}
			pthread_mutex_unlock(&mutex_tab);
		}

		// DE DROITE A GAUCHE
		for (i = nbColonnesCompletes -1 ; i >= 0 && colonnesCompletes[i] >= 5; i--)
		{
			pthread_mutex_lock(&mutex_tab);
			for (k = colonnesCompletes[i]; k < 10; k++)
			{
				for (j = 0; j < NB_LIGNES; j++)
				{
					if (k == 9 || tab[j][k+1] == VIDE)
					{
						tab[j][k] = VIDE;
						EffaceCarre(j, k);
					}
					else
					{
						tab[j][k] = tab[j][k+1];
						DessineSprite(j, k, tab[j][k+1]);
					}
				}
			}
			pthread_mutex_unlock(&mutex_tab);
		}

		// DE HAUT EN BAS
		for (i = 0; i < nbLignesCompletes && lignesCompletes[i] < 7; i++)
		{
			pthread_mutex_lock(&mutex_tab);
			for (k = lignesCompletes[i]; k >= 0; k--)
			{
				for (j = 0; j < 10; j++)
				{
					if (k == 0 || tab[k-1][j] == VIDE)
					{
						tab[k][j] = VIDE;
						EffaceCarre(k, j);
					}
					else
					{
						tab[k][j] = tab[k-1][j];
						DessineSprite(k, j, tab[k-1][j]);
					}
				}
			}
			pthread_mutex_unlock(&mutex_tab);
		}

		// DE BAS EN HAUT
		for (i = nbLignesCompletes - 1; i >= 0 && lignesCompletes[i] >= 7; i--)
		{
			pthread_mutex_lock(&mutex_tab);
			for (k = lignesCompletes[i]; k < NB_LIGNES; k++)
			{
				for (j = 0; j < 10; j++)
				{
					if (k == NB_LIGNES - 1 || tab[k+1][j] == VIDE)
					{
						tab[k][j] = VIDE;
						EffaceCarre(k, j);
					}
					else
					{
						tab[k][j] = tab[k+1][j];
						DessineSprite(k, j, tab[k+1][j]);
					}
				}
			}
			pthread_mutex_unlock(&mutex_tab);
		}

		nbAnalyses = 0;
		
		pthread_mutex_lock(&mutex_traitement);
		traitementEnCours = 0;
		pthread_mutex_unlock(&mutex_traitement);
		DessineSprite(12, 11, VOYANT_VERT);


		/// VERIFIER SI POSABLE
		pthread_kill(hthread_fin, SIGUSR2);
	}


	pthread_exit(NULL);
	return NULL;
}

void *thread_fin(void *)
{
    sigset_t mask;
    
	sigemptyset(&mask);
    sigaddset(&mask, SIGUSR2);
    pthread_sigmask(SIG_UNBLOCK, &mask, NULL);

    while(1)
    {
        pause();
        pthread_mutex_lock(&mutex_traitement);
        traitementEnCours = 0;
        pthread_mutex_unlock(&mutex_traitement);
        DessineSprite(12, 11, VOYANT_VERT);
    }
}

void handlerSIGUSR2(int sig)
{
    int i = 0;
	int j = 0;
	int c = 0;
        
        printf("(THREAD FIN) SIGUSR2 RECEIVED\n");

    for(i=0;i<14;i++)
    {
        for(j=0;j<10;j++)
        {
            pthread_mutex_lock(&mutex_pieceEnCours);
            pthread_mutex_lock(&mutex_tab);

            for(c=0;c<4;c++)
			{
                if(i + pieceEnCours.cases[c].ligne < 14 && j + pieceEnCours.cases[c].colonne < 10)
				{
					if(tab[i + pieceEnCours.cases[c].ligne][j + pieceEnCours.cases[c].colonne] > 0)
						break;
				}
				else
					break;
            }

            pthread_mutex_unlock(&mutex_tab);
            pthread_mutex_unlock(&mutex_pieceEnCours);

            if(c == 4)
                return;
                //break;
        }
    }
	
	pthread_mutex_lock(&mutex_traitement);
	traitementEnCours = 1;
	pthread_mutex_unlock(&mutex_traitement);
	DessineSprite(12, 11, VOYANT_ROUGE);
	printf("(THREAD PARTIE) FIN PARTIE\n");
    pthread_exit(NULL);
}


void* thread_top_score(void*)
{
	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGQUIT);
	pthread_sigmask(SIG_UNBLOCK, &mask, NULL);

	while(1)
		pause();

	pthread_exit(NULL);
	return NULL;
}

void handlerSIGQUIT(int)
{
	TOPSCORE top;
	if (ser_key && GetTopScore(ser_key, &top) == 0)
	{
		char buff[100];
		int i;

		sprintf(buff, "%s %s", top.login, top.pseudo);
		set_message(buff);
		sprintf(buff, "%4d", top.score);
		for(i = 0; i < 4; i++)
			DessineLettre(8, 15+i, buff[i]);
	}
}

void joueurs_cleanup(void*)
{
	DeconnectionServeur(ser_key);
}

/***** Joueur Connecté *****/
void *thread_joueursconnectes(void *)
{

	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGHUP);
	pthread_sigmask(SIG_UNBLOCK, &mask, NULL);

    pthread_cleanup_push(joueurs_cleanup, NULL);
    if(ConnectionServeur(ser_key, pseudo) != 0)
        printf("La connexion au serveur a echouer\n");
    while(1)
        pause();

    pthread_cleanup_pop(1);
}

void handlerSIGHUP(int sig)
{
	int connectes = GetNbJoueursConnectes(ser_key);
	printf("nb connectés: %d", connectes);

	if(connectes > 98)
	{
		DessineSprite(12, 17, CHIFFRE_9);
        DessineSprite(12, 18, CHIFFRE_9);
	}
	else
	{
		DessineSprite(12, 17, CHIFFRE_0 + (connectes-connectes%10)/10);
        DessineSprite(12, 18, CHIFFRE_0 + connectes%10);
    }
}

