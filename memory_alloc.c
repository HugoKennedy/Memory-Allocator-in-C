#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <setjmp.h>
#include "cmocka.h" 
#include "memory_alloc.h"


/* Starting from b, skip inc blocks of size block_size
 * retrun the address of the resulting block
 */
// calcul l'addresse du bloc situé plus loin en mémoire
static struct memory_block* memalloc_get_address(struct memory_block* b, int inc, size_t block_size) {
  uintptr_t base = (uintptr_t) b;
  uintptr_t result = base + (inc * block_size);
  return (struct memory_block*) result;
}

/* compute the address of index th block of the allocator */
// renvoie l'adresse du bloc d'indice index dans le tableau de blocs préalloués
static struct memory_block* memalloc_get(struct memory_alloc* m, int index) {
  assert(index >= 0 && index < m->nb_prealloc_blocks);
  return memalloc_get_address(m->prealloc_blocks, index, m->block_size);
}

/* compute the address of next block */
// renvoie l'adresse du bloc situé juste après le bloc pointé par current
static struct memory_block* memalloc_get_next(struct memory_alloc* m, struct memory_block* current) {
  return memalloc_get_address(current, 1, m->block_size);
}

// Question 1.c

void memalloc_init(struct memory_alloc* m, int nb_blocks, size_t block_size) {

  /* 
  initialiser struct m
  préallouer les blocs nb_blocks de block_size 
  nb_prealloc_blocks / block_size / prealloc_blocks / available_blocks / first_block (next) / errno
  */

// on check que m n'est pas NULL, sinon on ne peut pas l'initialiser
  if (m==NULL) {
    return;
  }

// on initialise les champs de m à des valeurs par défaut
  m->nb_prealloc_blocks = 0;
  m->block_size = 0;
  m->prealloc_blocks = NULL;
  m->available_blocks = 0;
  m->first_block = NULL;
  m->errno = E_SUCCESS;

// on vérifie que le nombre de blocs et la taille des blocs sont valides
// il faut au moins 1 bloc et la taille doit être au moins celle d'un struct memory_block pour pouvoir stocker le pointeur vers le bloc suivant (adresse)
// par ex : erreur avec block de 2o < 8o
  if (nb_blocks <= 0 || block_size < sizeof(struct memory_block)) {
      m->errno = E_NOMEM; 
      return;
  }

// malloc renvoie l'adresse du début du bloc de mémoire préalloué de size nb_blocks * block_size, ou NULL en cas d'erreur
// malloc attend une taille d'ou le résultat de nb_blocks * block_size doit être converti en size_t
// on fait malloc avant de remplir la structure

  m->prealloc_blocks = malloc((size_t)nb_blocks * block_size); // pointe vers le début du bloc mémoire

// on traite le cas où malloc échoue et renvoie NULL, auquel cas on met à jour errno et on retourne

  if (m->prealloc_blocks == NULL) {
      m->errno = E_NOMEM;
      return;
  }

// si on arrive ici, c'est que malloc a réussi, on peut remplir les champs de m

  m->nb_prealloc_blocks = (size_t)nb_blocks;
  m->block_size = block_size;
  m->available_blocks = (size_t)nb_blocks;
  m->first_block = (struct memory_block*) m->prealloc_blocks;

// on initialise les blocs préalloués en les chaînant entre eux, c'est à dire que chaque bloc pointe vers le bloc suivant, sauf le dernier qui pointe vers NULL

  for (int i = 0; i < nb_blocks; i++) {

      // on récupère l'adresse du bloc d'indice i (current)
      struct memory_block* current = memalloc_get(m, i);

       if (i == nb_blocks - 1) {
           // Le tout dernier bloc n'a pas de suivant
           current->next = NULL;
       } else {
           // Les autres pointent vers le bloc d'à côté
           current->next = memalloc_get(m, i + 1);
       }
  }

  // si on arrive ici, c'est que tout s'est bien passé, on met à jour errno pour indiquer le succès
  m->errno = E_SUCCESS;
}


// Question 1.d

// on rappelle free(ptr)
/* Finalize the memory_alloc structure, and free all the memory that was malloc'd */
void memalloc_finalize(struct memory_alloc* m) {

  // On vérifie que m n'est pas NULL, sinon on ne peut pas le finaliser

   if (m == NULL) {
      return;
   }

  //  Libération de la mémoire allouée dynamiquement
  // On vérifie d'abord que le pointeur n'est pas déjà NULL

  if (m->prealloc_blocks != NULL) { // pas nécessaire
      free(m->prealloc_blocks); // On rend la mémoire au système
      m->prealloc_blocks = NULL; // on coupe définitivement le lien (pour éviter de futures erreurs)
  }

  // Remise à zéro de tous les autres champs de la structure

  m->nb_prealloc_blocks = 0;
  m->block_size = 0;
  m->available_blocks = 0;
  m->first_block = NULL;
    
  // On peut remettre errno à E_SUCCESS car la finalisation s'est bien passée
  m->errno = E_SUCCESS;

}

// Question 1.f

/* return the number of consecutive blocks starting from first */
int memalloc_nb_consecutive_blocks(struct memory_alloc* m, struct memory_block* first) {

// suivant en mémoire physique VS suivant dans la liste chaînée
// On utilise memalloc_get_next pour vérifier que les blocs sont consécutifs en mémoire physique, et pas seulement dans la liste chaînée
  if (m == NULL || first == NULL) {
      return 0;
  }

  int count = 1;
  struct memory_block* current = first;
  
  while (current->next != NULL) {
      // On calcule l'adresse théorique du voisin physique (juste à côté en mémoire)
      struct memory_block* next_physical = memalloc_get_next(m, current);

      // Est-ce que le bloc suivant dans la liste (current->next) 
      // est bien le voisin physique attendu ?
      if (current->next == next_physical) {
          count++; // Oui, on a un bloc consécutif de plus
          current = current->next; // On avance pour vérifier le suivant
      } else {
          // Non, le bloc d'après dans la liste est "loin" en mémoire.
          // La chaîne consécutive s'arrête ici.
          break; 
      }
  }


  return count;
}

//Question 2.a
// on ne peut pas déplacer les blocs physiques mais que les liens 

void memalloc_reorder(struct memory_alloc* m) {

  // Sécurité : Si l'allocateur n'est pas initialisé, ou s'il y a 0 ou 1 seul bloc libre, 
  // il n'y a rien à trier ! On s'arrête tout de suite.

  if (m == NULL || m->first_block == NULL || m->first_block->next == NULL) {
      return;
  }

  int swapped; // Un booléen pour savoir si on a fait un échange pendant notre parcours
  // il vaut 1 si au moins 1 lien a été changé
  // 0 sinon (d'où booléen)

  struct memory_block* current;
  struct memory_block* prev;
  struct memory_block* next_node;

  // Le do...while permet de faire des passages successifs jusqu'à ce que la liste soit triée (swapped vaut 0)

  do {
      swapped = 0; // Au début du parcours, on n'a encore rien échangé
      current = m->first_block; // On commence au début de la liste
      prev = NULL; // Au tout début, il n'y a pas de bloc précédent

      // On parcourt la liste tant qu'on a un bloc actuel ET un bloc suivant à comparer
      while (current != NULL && current->next != NULL) {
          next_node = current->next; // On regarde le voisin (le bloc B)

          // Est-ce que l'adresse de current (A) est plus grande que next_node (B) ?
          // Si oui, l'ordre logique ne respecte pas l'ordre physique, il faut échanger !
          if (current > next_node) {
                
              // Raccorder le bloc précédent 
              if (prev == NULL) {
                  // Cas particulier : on échange le tout premier bloc.
                  // Le nouveau premier bloc devient B (next_node)
                  m->first_block = next_node;
              } else {
                  // Cas normal : le bloc précédent pointe vers B au lieu de A
                  prev->next = next_node;
              }

              // Croiser les flèches de A et B 
              current->next = next_node->next; // A pointe vers ce qu'il y avait après B
              next_node->next = current;       // B pointe vers A

              // Préparer l'itération suivante 
              // Physiquement sur notre fil, l'ordre est maintenant : prev -> next_node -> current
              // Donc pour le prochain tour de boucle, le "précédent" sera next_node.
              // Et "current" reste current, car il a reculé d'une case dans le fil logique !
              prev = next_node;
                
              swapped = 1; // On retient qu'on a fait au moins un échange
          } else {
              // Si les blocs sont déjà dans le bon ordre, on avance simplement nos curseurs
              prev = current;
              current = current->next;
          }
      }
  // Si swapped vaut 1, ça veut dire que c'était encore le bazar, on refait un tour complet !
  // Si swapped vaut 0, c'est trié, la boucle s'arrête.
  } while (swapped); 
}


/* Initialize an allocated buffer with zeros */
// initialise un bloc de mémoire alloué en le remplissant de 0
static void initialize_buffer(struct memory_block* block,
			      size_t size) {
  char* ptr = (char*)block;
  for(int i=0; i<size; i++) {
    ptr[i]=0;
  }
}

// Question 1.g (!!! difficile)

// Question 1.g modifiée par la Question 2.b

void* memalloc_allocate(struct memory_alloc* m, size_t size) {

    if (m == NULL || size == 0) {
        return NULL;
    }

    int blocks_needed = size / m->block_size;
    if (size % m->block_size != 0) {
        blocks_needed++; // On ajoute 1 bloc si la division n'est pas "pile poil"
    }

    // NOUVEAUTÉ 2.b : On s'autorise au maximum 2 passages.
    // pass = 0 : On cherche normalement
    // pass = 1 : On vient de trier la liste, on tente une dernière fois
    for (int pass = 0; pass < 2; pass++) {

        struct memory_block* current = m->first_block;
        struct memory_block* prev = NULL;

        // DÉBUT DE LA RECHERCHE 
        while (current != NULL) {
            
            int consecutive = memalloc_nb_consecutive_blocks(m, current);

            if (consecutive >= blocks_needed) {
                // Trouver le dernier bloc de la zone à allouer
                struct memory_block* last_of_sequence = current;
                for (int i = 1; i < blocks_needed; i++) {
                    last_of_sequence = last_of_sequence->next;
                }
                
                // Retirer la zone de la liste des blocs libres
                if (prev == NULL) {
                    m->first_block = last_of_sequence->next;
                } else {
                    prev->next = last_of_sequence->next; 
                }

                // Mise à jour
                m->available_blocks -= blocks_needed;
                m->errno = E_SUCCESS;

                // Initialisation (remplir de 0)
                initialize_buffer(current, size);

                // On renvoie l'adresse
                return (void*) current;
            }

            prev = current;
            current = current->next;
        }
        // FIN DE LA RECHERCHE

        // Si on arrive ici, c'est que la recherche a échoué.
        
        // A-t-on au moins assez de blocs libres au total dans l'allocateur ?
        if (m->available_blocks < blocks_needed) {
            // Non, on n'a physiquement pas la place, c'est mort.
            m->errno = E_NOMEM;
            return NULL; 
        }

        // Si on a la place au total et qu'on est au premier passage (pass == 0),
        // on appelle à l'aide notre fonction de tri pour démêler tout ça !
        if (pass == 0) {
            memalloc_reorder(m); // On trie la liste
            // La boucle 'for' va recommencer pour le pass = 1 et refaire une recherche !
        }
    }

    // Si on sort de la grande boucle 'for', ça veut dire qu'on a cherché (pass 0), 
    // on a trié, on a cherché encore (pass 1), et on n'a toujours rien trouvé !
    // Cela signifie que la mémoire est trop fragmentée physiquement.
    m->errno = E_SHOULD_PACK;
    return NULL;
}

// Question 1.e
// on suppose que l'utilisateur ne se trompe pas

void memalloc_free(struct memory_alloc* m, void* address, size_t size) {

  if (m == NULL || address == NULL || size == 0) {
      return;
  }

  int blocks_to_free = size / m->block_size;
  if (size % m->block_size != 0) {
      blocks_to_free++;
  }

  struct memory_block* first_freed = (struct memory_block*) address;
  struct memory_block* current = first_freed;

  // on reconstruit la chaîne des blocs qu'on rend, en s'assurant que les blocs sont bien consécutifs en mémoire physique

  for (int i = 0; i < blocks_to_free - 1; i++) {
      // Le voisin physique suivant
      struct memory_block* next_physical = memalloc_get_next(m, current);
        
      // On recrée le lien logique (la liste chaînée)
      current->next = next_physical;
        
      // On avance notre curseur
      current = next_physical;
  }

  // Rattacher notre zone libérée au reste de la liste des blocs libres
  // À ce stade, "current" pointe sur le TOUT DERNIER bloc de la zone qu'on rend.
  // On lui dit de pointer vers l'ancien début de la liste libre.

  // on colle notre zone rendue devant l'ancien début de la liste libre, pour éviter de devoir parcourir la liste pour trouver où insérer notre zone rendue
  // d'ou complexité et problème dans l'exercice 2
  
  current->next = m->first_block;

  // Et le nouveau début de la liste libre devient le début de notre zone rendue
  m->first_block = first_freed;

  // Mise à jour de la tour de contrôle
  m->available_blocks += blocks_to_free;
  m->errno = E_SUCCESS;


}

// Question 1.e

void memalloc_print(struct memory_alloc* m) {

// %zu pour size_t
// %p pour les pointeurs

  printf("---------------------------------\n");

  if (m == NULL) {
      printf("Allocator is not initialized.\n");
      printf("---------------------------------\n");
      return;
  }

  printf("  Block size: %zu\n", m->block_size);
  printf("  Available blocks: %zu\n", m->available_blocks);

// printf s'attend à recevoir un pointeur de type void* pour afficher une adresse mémoire, on doit donc caster first_block en void* pour éviter un warning de compilation
  printf("  First free: %p\n", (void*)m->first_block); // car sinon c'est un pointeur vers struct memory_block

  printf("  Status: ");
  memalloc_error_print(m->errno); // defini juste en dessous

  printf("  Content: ");

  struct memory_block* current = m->first_block;

  while (current != NULL) {

    printf("[%p] -> ", (void*)current);
    current = current->next;
  }

  printf("NULL\n");

  printf("---------------------------------\n");
}


void memalloc_error_print(enum memory_errno error_number) {
  switch(error_number) {
  case E_SUCCESS:
    printf("Success\n");
    break;
  case E_NOMEM:
    printf("Not enough memory\n");
    break;
  case  E_SHOULD_PACK:
    printf("Not enough contiguous blocks\n");
    break;
  default:
    printf("Unknown\n");
    break;
  }
}

// Exercice 3

// dans l'exercice 1, memalloc_free a besoin de la taille alors que free(ptr) non
// exemple du box avec la clef sous le paillasson
// octets demandés par l'utilisateur + octets étiquette
// on renvoie l'adresse qui commence juste après l'étiquette
// l'utilisateur écrit les données sans écraser étiquette

void* memalloc_lifelike_malloc(struct memory_alloc *m, size_t size) {

  // on dmd size + 8 octets
  // memalloc_allocate global
  // onrenvoie l'adresse décalée

  if (m == NULL || size == 0) {
      return NULL;
  }

  // On calcule la taille totale nécessaire : la demande de l'utilisateur + notre étiquette (8 octets)
  size_t size_with_header = size + sizeof(void*); // size + taille d'un pointeur

  // On demande cette place à notre allocateur de l'exercice 2
  void* raw_ptr = memalloc_allocate(m, size_with_header);
  if (raw_ptr == NULL) {
      return NULL; // Plus de place
  }

  // On calcule la taille REELLE qui a été réservée (nombre de blocs * taille d'un bloc)
  // Car l'exercice nous dit que c'est cette valeur (ex: 24) qu'on doit cacher.
  int blocks_needed = size_with_header / m->block_size;
  if (size_with_header % m->block_size != 0) {
      blocks_needed++;
  }

  size_t actual_allocated_size = blocks_needed * m->block_size;

  // On cache notre étiquette tout au début !
  // On convertit raw_ptr en (size_t*) pour dire "écris un entier ici"
  *((size_t*)raw_ptr) = actual_allocated_size;

  // On décale le pointeur de 8 octets vers la droite pour cacher l'étiquette à l'utilisateur
  // (On le convertit en char* pour avancer octet par octet)
  void* user_ptr = (void*)((char*)raw_ptr + sizeof(void*)); // char nous permet d'avancer précisément sur quelques octets

  return user_ptr;
}

// on prend l'adresse de l'utilisateur
// on recule de 8 ocets
// on retompe sur l'etiquette
// on donne l'adresse a memalloc_free

void memalloc_lifelike_free(struct memory_alloc *m, void* addr) {
  
  if (m == NULL || addr == NULL) {
        return;
    }

  // On prend l'adresse de l'utilisateur et on recule de 8 octets (vers la gauche)
  // On tombe pile poil sur notre étiquette secrète !
  void* raw_ptr = (void*)((char*)addr - sizeof(void*));

  // On lit la valeur écrite sur l'étiquette
  size_t actual_allocated_size = *((size_t*)raw_ptr);

  // On rend toute la zone à notre parking de l'exercice 1
  memalloc_free(m, raw_ptr, actual_allocated_size);

}

// on demande d'agrandir / rétrécir une zone
// nouvelle zone avec la nouvelle taille avec nouveau malloc
// on recopie les anciennes données 
// on détruit l'ancienne zone

void* memalloc_lifelike_realloc(struct memory_alloc *m, void* addr, size_t size){
  
  // issu du man realloc

  if (addr == NULL) {
      // Si l'adresse est NULL, realloc se comporte exactement comme malloc
      return memalloc_lifelike_malloc(m, size);
  }
  if (size == 0) {
      // Si la taille est 0, realloc se comporte exactement comme free
      memalloc_lifelike_free(m, addr);
      return NULL;
  }

  // On alloue un tout nouveau box de la nouvelle taille
  void* new_addr = memalloc_lifelike_malloc(m, size);
  if (new_addr == NULL) {
      return NULL; // Echec de l'agrandissement, l'ancienne zone reste intacte
  }

  // On va lire la taille de l'ANCIEN box pour savoir combien d'octets on doit déménager
  void* raw_old_ptr = (void*)((char*)addr - sizeof(void*));
  size_t old_allocated_size = *((size_t*)raw_old_ptr);
    
  // L'ancienne taille utilisable, c'est la taille totale moins les 8 octets de l'étiquette
  size_t old_user_size = old_allocated_size - sizeof(void*);

  // On calcule combien d'octets on doit copier. 
  // (On prend le minimum entre l'ancienne taille et la nouvelle)
  size_t size_to_copy;

  if (old_user_size < size) {
      size_to_copy = old_user_size;
  } else {
      size_to_copy = size;
  }

  // On fait le déménagement des données de l'ancienne zone vers la nouvelle
  memcpy(new_addr, addr, size_to_copy);

  // On détruit l'ancien box
  memalloc_lifelike_free(m, addr);

  return new_addr;
}

// Exercice 4 : Canari

// On définit notre mot de passe secret (le Canari)
#define MAGIC_CANARY 0xDEADBEEF

void* memalloc_lifelike_malloc_canary(struct memory_alloc *m, size_t size) {
    if (m == NULL || size == 0) {
        return NULL;
    }

    // EXERCICE 4 : L'étiquette fait maintenant 16 octets (2 * sizeof(size_t))
    // 8 octets pour la taille + 8 octets pour le canari
    size_t header_size = 2 * sizeof(size_t); 
    size_t size_with_header = size + header_size;

    void* raw_ptr = memalloc_allocate(m, size_with_header);
    if (raw_ptr == NULL) {
        return NULL;
    }

    int blocks_needed = size_with_header / m->block_size;
    if (size_with_header % m->block_size != 0) {
        blocks_needed++;
    }
    size_t actual_allocated_size = blocks_needed * m->block_size;

    // On cache la TAILLE tout au début
    *((size_t*)raw_ptr) = actual_allocated_size;

    // On cache le CANARI juste après la taille (on avance de 8 octets)
    size_t* canary_ptr = (size_t*)((char*)raw_ptr + sizeof(size_t));
    *canary_ptr = MAGIC_CANARY;

    // On décale le pointeur pour l'utilisateur après notre gros en-tête de 16 octets
    void* user_ptr = (void*)((char*)raw_ptr + header_size);

    return user_ptr;
}

void memalloc_lifelike_free_canary(struct memory_alloc *m, void* addr) {
    if (m == NULL || addr == NULL) {
        return;
    }

    // On recule de 8 octets pour trouver le canari
    size_t* canary_ptr = (size_t*)((char*)addr - sizeof(size_t));

    // VÉRIFICATION DE SÉCURITÉ : Est-ce que le canari est mort ?
    if (*canary_ptr != MAGIC_CANARY) {
        printf("Le Canari est mort !\n");
        printf("Un debordement de tampon a ecrase la memoire juste avant l'adresse %p.\n", addr);

        // on tue le programme ici
        //exit(EXIT_FAILURE);
    }

    // On recule encore de 8 octets pour trouver la taille de la zone
    void* raw_ptr = (void*)((char*)addr - 2 * sizeof(size_t));
    size_t actual_allocated_size = *((size_t*)raw_ptr);

    // On rend la zone au parking
    memalloc_free(m, raw_ptr, actual_allocated_size);
}

void* memalloc_lifelike_realloc_canary(struct memory_alloc *m, void* addr, size_t size){
    if (addr == NULL) {
        return memalloc_lifelike_malloc_canary(m, size);
    }
    if (size == 0) {
        memalloc_lifelike_free_canary(m, addr);
        return NULL;
    }

    void* new_addr = memalloc_lifelike_malloc_canary(m, size);
    if (new_addr == NULL) {
        return NULL; 
    }

    // On doit reculer de 16 octets (2 * sizeof(size_t)) pour lire l'ancienne taille
    void* raw_old_ptr = (void*)((char*)addr - 2 * sizeof(size_t));
    size_t old_allocated_size = *((size_t*)raw_old_ptr);
    size_t old_user_size = old_allocated_size - 2 * sizeof(size_t);

    size_t size_to_copy;
    if (old_user_size < size) {
        size_to_copy = old_user_size;
    } else {
        size_to_copy = size;
    }

    memcpy(new_addr, addr, size_to_copy);
    memalloc_lifelike_free_canary(m, addr);

    return new_addr;
}
