//------------------------------------------------------------------------------
// Projet : TP CSE (malloc)
// Cours  : Conception des systèmes d'exploitation et programmation concurrente
// Cursus : Université Grenoble Alpes - UFRIM²AG - Master 1 - Informatique
// Année  : 2022-2023
//------------------------------------------------------------------------------

#include "mem.h"
#include "mem_space.h"
#include "mem_os.h"
#include <assert.h>
#include <stdio.h>

struct mem_free_block_s{
	size_t size;
	struct mem_free_block_s* next;
};

struct mem_alloue_block_s{
	size_t size;
};

typedef struct mem_state{
	void *adresse;
	size_t size;
	struct mem_free_block_s* first;
} mem_state_t;

//typedef struct mem_state mem_state_t;

static mem_state_t gbl_state;
//vide.
//static mem_fit_function_t gbl_function;

//-------------------------------------------------------------
// mem_init
//-------------------------------------------------------------
/**
 * Initialize the memory allocator.
 * If already init it will re-init.
**/
void mem_init() {
	gbl_state.adresse = mem_space_get_addr();
	gbl_state.size=mem_space_get_size();
	gbl_state.first = gbl_state.adresse;
	gbl_state.first->size=gbl_state.size;
	gbl_state.first->next=NULL;
}

//-------------------------------------------------------------
// mem_alloc
//-------------------------------------------------------------
/**
 * Allocate a bloc of the given size.
**/
void *mem_alloc(size_t size) {

	//Possiblement nécessaire de changer la taille pour prendre en compte l'en-tête (enlevé pour simplifier)
	//Problèmesi on n'accompte pas pour l'en-tete: l'user 
	/*
	size_t realsize = size;
	if(size%8 != 0){
		realsize = 8*((size/8)+2); //+1 pour avoir le multiple de 8 supp, +2 pour compter la taille de l'entete
	}*/

	//on essaie d'allouer un block plus grand que la mémoire
	if(size > gbl_state.size){
		return NULL;
	}

	//on trouve un block de taille suffisante
	struct mem_free_block_s *block_courant = gbl_state.first; 	
	struct mem_free_block_s *block_precedent = NULL;
	struct mem_alloue_block_s *block_alloue;

	//parcours des blocks vides jusqu'a arriver à la fin OU trouver un block courant de taille suffisante.
	while(block_courant != NULL && block_courant->size < size){
		block_precedent = block_courant;
		block_courant = block_courant->next;
	}

	//pas de block mémoire de taille suffisante.
	if (block_courant == NULL){
		return NULL;
	}

	if (block_precedent == NULL){ //le premier block libre était de taille suffisante.

		//on a alloué exactement toute la mémoire disponible 
		if (gbl_state.first->size == size){

			gbl_state.first = NULL;

		} else { //il reste de la mémoire libre dans gbl_state.first

			size_t new_size = (gbl_state.first->size)-size;
			struct mem_free_block_s *new_next = gbl_state.first->next;
			gbl_state.first = gbl_state.first+size;
			gbl_state.first->size = new_size;
			gbl_state.first->next = new_next;

		}
		block_alloue = (void *) block_courant;
		block_alloue->size = size;

	} else {	//le premier block n'était pas suffisant, on est sur un block b quelquonque dans la mémoire.

		if (block_courant -> size == size){ //on alloue le block courant en entier.

			block_precedent->next = block_courant->next;

		} else { //il reste de la mémoire libre dans block_courant

			struct mem_free_block_s *residus = block_courant+size;
			residus->size = block_courant->size - size;
			residus->next = block_courant->next;
			block_precedent->next = residus;
		}
		block_alloue = (void *) block_courant;
		block_alloue->size = size;
	}

	return (void *)block_alloue + 8;
}

//-------------------------------------------------------------
// mem_get_size
//-------------------------------------------------------------
size_t mem_get_size(void * zone)
{
    //TODO: test. potentiel problème: la zone est-elle allouée ou non (offset différent)
	struct mem_alloue_block_s* block_zone = zone-8;
    return block_zone->size;
}

//-------------------------------------------------------------
// mem_free
//-------------------------------------------------------------
/**
 * Free an allocaetd bloc.
**/
void mem_free(void *zone) {
    //TODO: implement
	struct mem_alloue_block_s* block_zone = zone-8;
	struct mem_free_block_s* block_prec = gbl_state.first;
	struct mem_free_block_s* block_suiv = block_prec->next;

	//cas où la zone est placée avant le premier block (gbl_state.first OU block_prec ici).
	if (block_zone < block_prec){

		if (block_zone + block_zone->size == block_prec){ //la zone est "collée" au premier block.

			//Appel Fonction mem_fuse avec block_zone et block_prec.

		} else { //il existe un ou plusieurs block(s) alloués entre la zone et block_prec.
			struct mem_free_block_s* block_zone_libere = (void *) block_alloue;
			block_zone_libere.size = block_alloue->size;
			block_zone_libere.next = gbl_state.first; //Plus propre/logique d'utiliser block_prec ?
			gbl_state.first = block_zone_libere;
		}

		return; //pas super elegant, safe, à enlever si on est sur de ne pas rentrer dans les autres conditionelles.
	}

	//On cherche à se placer tel que block_prec < block_courant <blokc_suiv.
	//on n'accompte pas pour la possibilité que l'utilisateur appelle free sur une zone libre.
	While(block_prec < block_zone){
		block_prec = block_suiv;
		block_suiv = block_prec->next;
	}

	//Deux cas: 
	//	-on a trouvé la situation où prec<zone<suiv
	//	-on ne l'a pas trouvé: zone est placé après le dernier block libre.

}

//-------------------------------------------------------------
// Itérateur(parcours) sur le contenu de l'allocateur
// mem_show
//-------------------------------------------------------------
//TODO nettoyer les commentaires. tester. optimiser l'utilisation/affectation des variables courantes pour éviter de la redondance (?).
void mem_show(void (*print)(void *, size_t, int free)) {

	void *adresse_test = gbl_state.adresse; //position actuelle dans la mémoire, sans disctinction libre/alloue
	struct mem_free_block_s *block_courant_libre = gbl_state.first;
	struct mem_alloue_block_s *block_courant_alloue;

	//On continue tant qu'on n'a pas parcouru tout le block mémoire.
	while (adresse_test != gbl_state.adresse + gbl_state.size){

		//soit notre pointeur adresse_test est sur un block alloué soit sur un block libre (ou sur la fin de la mémoire, mais on ne passe pas dans la boucle).
		//donc seulement 2 cas possibles à traiter.
		if(adresse_test == block_courant_libre){	//cas ou le pointeur est sur un block libre.
			
			print(block_courant_libre, block_courant_libre->size, 1); //affichage

			//on doit maintenant passer au prochain block (mettre à jour nos varaibles courantes)
			//le prochain block est un block libre
			if(block_courant_libre + block_courant_libre->size == block_courant_libre->next){
				adresse_test = block_courant_libre->next;
				block_courant_libre = block_courant_libre->next;
			} else {	//le prochain block est un block alloue
				adresse_test = block_courant_libre->next;
				block_courant_alloue = (void *) block_courant_libre->next;
			}
		} else { //le pointeur n'est pas sur un block libre, donc block alloue
			block_courant_alloue = adresse_test;
			print(block_courant_alloue, block_courant_alloue->size, 0);

			//mise à jour des variables courantes pour le prochain block.
			//pas besoin de cas par cas car block_courant_alloue sera set au prochain tour de boucle et block_courant_libre n'est jamais en retard.
			adresse_test = block_courant_alloue + block_courant_alloue->size;

		}
	}
}

//-------------------------------------------------------------
// mem_fit
//-------------------------------------------------------------
void mem_set_fit_handler(mem_fit_function_t *mff) {
	//TODO: implement
	assert(! "NOT IMPLEMENTED !");
}

//-------------------------------------------------------------
// Stratégies d'allocation
//-------------------------------------------------------------
mem_free_block_t *mem_first_fit(mem_free_block_t *first_free_block, size_t wanted_size) {
    //TODO: implement
	assert(! "NOT IMPLEMENTED !");
	return NULL;
}
//-------------------------------------------------------------
mem_free_block_t *mem_best_fit(mem_free_block_t *first_free_block, size_t wanted_size) {
    //TODO: implement
	assert(! "NOT IMPLEMENTED !");
	return NULL;
}

//-------------------------------------------------------------
mem_free_block_t *mem_worst_fit(mem_free_block_t *first_free_block, size_t wanted_size) {
    //TODO: implement
	assert(! "NOT IMPLEMENTED !");
	return NULL;
}
