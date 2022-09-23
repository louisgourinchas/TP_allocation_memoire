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
	struct mem_fit_function_t function;
} mem_state_t;

//typedef struct mem_state mem_state_t;

static mem_state_t gbl_state;

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
	mem_set_fit_handler(mem_first_fit);
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
	size_t realsize = size;
	if(size%8 != 0){
		realsize = 8*((size/8)+1); //+1 pour avoir le multiple de 8 supp
	}
	realsize+=sizeof(struct mem_free_block_s);

	struct mem_free_block_s * emplacement = gbl_function(gbl_state.first, realsize);

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

	if (block_courant -> size == realsize){ //on alloue le block courant en entier.

		if(block_precedent != NULL)
		{
			block_precedent->next = block_courant->next;
		}
		else
		{
			gbl_state.first = block_courant->next;
		}
		

	} else { //il reste de la mémoire libre dans block_courant

		struct mem_free_block_s *block_reste = (void*)block_courant+realsize;
		block_reste->size = block_courant->size - realsize;
		block_reste->next = block_courant->next;

		if (block_precedent != NULL)
		{
			block_precedent->next = block_reste;
		}
		else
		{
			gbl_state.first = block_reste;
		}	
	}
	block_alloue = (void *) block_courant;
	block_alloue->size = realsize;

	return block_alloue + 1;
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

	if(gbl_state.first == NULL)
	{
		struct mem_free_block_s* block_zone_lible = (void *)block_zone;
		block_zone_lible->size = block_zone->size;
		block_zone_lible->next = NULL;
		gbl_state.first=block_zone_lible;
	}

	struct mem_free_block_s* block_prec = gbl_state.first;
	struct mem_free_block_s* block_suiv = block_prec->next;

	//cas où la zone est placée avant le premier block (gbl_state.first OU block_prec ici).
	if ((void *)block_zone < (void *)block_prec){

		if ((void * )(block_zone + block_zone->size) == (void *)block_prec){ //la zone est "collée" au premier block.

			size_t sauvegarde_taille = block_prec->size;
			block_prec = (void *)block_zone;
			block_prec->size = block_zone->size + sauvegarde_taille;
			block_prec->next = block_suiv;
			gbl_state.first = block_prec;

		} else { //il existe un ou plusieurs block(s) alloués entre la zone et block_prec.
			struct mem_free_block_s* block_zone_libere = (void *) block_zone;
			block_zone_libere->size = block_zone->size;
			block_zone_libere->next = gbl_state.first; //Plus propre/logique d'utiliser block_prec ?
			gbl_state.first = block_zone_libere;
		}
		return; //pas super elegant, safe, à enlever si on est sur de ne pas rentrer dans les autres conditionelles.
	}
	
	//On cherche à se placer tel que block_prec < block_zone < block_suiv.
	//on n'accompte pas pour la possibilité que l'utilisateur appelle free sur une zone libre.
	while(block_suiv != NULL && (void *)block_suiv < (void *)block_zone){
		block_prec = block_suiv;
		block_suiv = block_prec->next;
	} 

	if((void *)block_prec < (void *)block_zone && (void *)block_zone < (void *)block_suiv){
		// il y a block allouee entre block zone et block prec  ET  entre block zone et block suiv
		if ((void *)(block_prec + block_prec->size) < (void *)block_zone && (void *)(block_zone + block_zone->size) < (void *)block_suiv)
		{
			struct mem_free_block_s* block_zone_lible = (void *)block_zone;
			block_prec->next = block_zone_lible;
			block_zone_lible->next = block_suiv;
			block_zone_lible->size = block_zone->size;
		}
		// il y a block allouee entre block zone et block suiv  ET  block zone est justement colle apres block prec 
		else if ((void *)(block_prec + block_prec->size) == (void *)block_zone && (void *)(block_zone + block_zone->size) < (void *)block_suiv)
		{
			block_prec->size += block_zone->size;
		}
		// il y a block allouee entre block zone et block prec  ET  block zone est justement colle avant block suiv
		else if ((void *)(block_prec + block_prec->size) < (void *)block_zone && (void *)(block_zone + block_zone->size) == (void *)block_suiv)
		{
			struct mem_free_block_s* sauvegarde_suivant = block_suiv;
			block_suiv = (void *)block_zone;
			block_suiv->next = sauvegarde_suivant->next;
			block_prec->next = block_suiv;
			block_suiv->size = block_zone->size + sauvegarde_suivant->size;
		}
		// block zone colle apres block prec et avant block suiv
		else if ((void *)(block_prec + block_prec->size) == (void *)block_zone && (void *)(block_zone + block_zone->size) == (void *)block_suiv)
		{
			block_prec->size += block_zone->size + block_suiv->size;
			block_prec->next = block_suiv->next;
		}	
	}
	// zone est placé après le dernier block libre
	else
	{
		// il y a block allouee entre block zone et block prec
		if ((void *)(block_prec + block_prec->size) < (void *)block_zone)
		{
			struct mem_free_block_s* block_zone_lible = (void *)block_zone;
			block_zone_lible->size = block_zone->size;
			block_prec->next = block_zone_lible;
			block_zone_lible->next = NULL;
		}
		//block zone colle apres block prec
		else
		{
			block_prec->size += block_zone->size;
		}
		
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
	while (adresse_test <= gbl_state.adresse + gbl_state.size){

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
				block_courant_alloue = (void *) block_courant_libre + block_courant_libre->size;
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
	gbl_state.function = mff;
}

//-------------------------------------------------------------
// Stratégies d'allocation
//-------------------------------------------------------------
mem_free_block_t *mem_first_fit(mem_free_block_t *first_free_block, size_t wanted_size) {
    
	//on essaie d'allouer un block plus grand que la mémoire
	if(wanted_size > gbl_state.size){
		return NULL;
	}

	//on trouve un block de taille suffisante
	struct mem_free_block_s *block_courant = first_free_block; 	
	struct mem_free_block_s *block_precedent = NULL;
	struct mem_alloue_block_s *block_alloue;

	//parcours des blocks vides jusqu'a arriver à la fin OU trouver un block courant de taille suffisante.
	while(block_courant != NULL && block_courant->size < wanted_size){
		block_precedent = block_courant;
		block_courant = block_courant->next;
	}

	//pas de block mémoire de taille suffisante.
	if (block_courant == NULL){
		return NULL;
	}else {
		return block_courant;
	}

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
