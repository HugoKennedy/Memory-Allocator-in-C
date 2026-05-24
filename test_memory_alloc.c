#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <setjmp.h>
#include "cmocka.h" // nécessaire pour utiliser les fonctions de tests
#include "memory_alloc.h"

// pour les tests on reprend les questions du sujet

/* Test memory_init() */
// on teste que memalloc_init remplit correctement la structure
// on teste que memalloc_finalize remet correctement à zero
// questions 1;c et 1.d

void test_memory_init(void** state){ //void** imposé par cmocka
  size_t block_size = 8;
  int nb_blocks = 100;

  struct memory_alloc m;
  memalloc_init(&m, nb_blocks, block_size);

  assert_int_equal(m.nb_prealloc_blocks, nb_blocks);
  assert_int_equal(m.block_size, block_size);
  assert_int_equal(m.available_blocks, nb_blocks);
  assert_int_equal(m.errno, E_SUCCESS);

  memalloc_finalize(&m); 
  assert_int_equal(m.errno, E_SUCCESS);
  assert_int_equal(m.nb_prealloc_blocks, 0);
  assert_int_equal(m.available_blocks, 0);
}

// on teste ici le comptage des blocs
// on teste si le code mesure bien le nb de places libres
// Question 1.f

void test_memory_nb_consecutive_blocks(void** state) {
  struct memory_alloc m;
  
  // On crée une petite arène de 5 blocs de 16 octets
  memalloc_init(&m, 5, 16);

  // Au tout début, les 5 blocs sont libres et consécutifs
  assert_int_equal(memalloc_nb_consecutive_blocks(&m, m.first_block), 5); // code précédant dans memory_alloc.c

  // On demande à allouer 1 bloc (16 octets). Cela retire le 1er bloc de la liste des libres.
  void* p1 = memalloc_allocate(&m, 16);
  
  // Il ne doit rester plus que 4 blocs consécutifs libres
  assert_int_equal(memalloc_nb_consecutive_blocks(&m, m.first_block), 4);

  // Nettoyage de fin de test
  memalloc_free(&m, p1, 16);
  memalloc_finalize(&m);
}

// On teste l'allocation / la libération
// questions 1.g et 1.h

void test_memory_alloc(void** state){
  size_t block_size = 64;
  int nb_blocks = 10;
  void* pointers[nb_blocks]; //tableau de 10 pointeurs pour memoriser les adresses de memalloc_allocate

  struct memory_alloc m;
  memalloc_init(&m, nb_blocks, block_size);
  assert_int_equal(m.nb_prealloc_blocks, nb_blocks); //meme logique que test précédent
  assert_int_equal(m.block_size, block_size);
  assert_int_equal(m.available_blocks, nb_blocks);
  assert_int_equal(m.errno, E_SUCCESS);


  /* allocating too much memory should fail */
  pointers[0] = memalloc_allocate(&m, block_size * nb_blocks + 1); // on dmd plus que ce que l'allocateur possède
  assert_null(pointers[0]); // renvoie null
  assert_int_equal(m.errno, E_NOMEM);

  /* allocate a few buffers */
  for(int i = 0; i<nb_blocks; i++) {
    pointers[i] = memalloc_allocate(&m, block_size); // on demande exactement 1 bloc par tour
    assert_non_null(pointers[i]);
    assert_int_equal(m.available_blocks, nb_blocks - (i+1)); // car i cmmence à 0
    assert_int_equal(m.errno, E_SUCCESS);

    /* write something to the buffer */
    memset(pointers[i], 7, block_size); // remplit la zone memoire avec 7 (adresse ptr[i] sur 64 octets on écrit 7)
  }

  /* free the allocated buffers */
  for(int i = 0; i<nb_blocks; i++) {
    memalloc_free(&m, pointers[nb_blocks-(i+1)], block_size);
    assert_int_equal(m.available_blocks, i+1);
    assert_int_equal(m.errno, E_SUCCESS);
  }

  memalloc_finalize(&m); 
  assert_int_equal(m.errno, E_SUCCESS);
  assert_int_equal(m.nb_prealloc_blocks, 0);
  assert_int_equal(m.available_blocks, 0);
}

// Question 1.h
// on teste que le bloc libéré est bien placé en première position 

void test_memory_free_behavior(void** state) {
  struct memory_alloc m;
  memalloc_init(&m, 5, 16); // 5 blocs

  // On alloue une zone de 2 blocs (32 octets)
  void* zone = memalloc_allocate(&m, 32);
  
  // Il doit rester 3 blocs disponibles
  assert_int_equal(m.available_blocks, 3);

  // On libère la zone qu'on vient de prendre
  memalloc_free(&m, zone, 32);
  
  // On vérifie que les compteurs sont revenus à 5
  assert_int_equal(m.available_blocks, 5);

  // important
  // "ajouter les blocs de la zone à libérer au début de la liste"
  // Donc le premier bloc libre (m.first_block) doit correspondre à l'adresse de "zone" rendue 
  assert_ptr_equal(m.first_block, zone);

  memalloc_finalize(&m);
}

// test défragmentation exercice 2

void test_memory_reorder_and_allocate(void** state) {
    struct memory_alloc m;
    memalloc_init(&m, 5, 16); // Parking de 5 places de 16 octets
    
    // On gare 3 petites voitures
    void* b0 = memalloc_allocate(&m, 16); // Place 0

    void* b1 = memalloc_allocate(&m, 16); // Place 1 (voiture garée que l'on utilise pas)
    // on aurait juste pu mettre : memalloc_allocate(&m, 16);
    //memalloc_allocate(&m, 16);

    void* b2 = memalloc_allocate(&m, 16); // Place 2
    
    // On fait partir la voiture 0 puis la voiture 2.
    // L'ordre de la liste libre devient : Place 2 -> Place 0 -> Place 3 -> Place 4
    memalloc_free(&m, b0, 16);
    memalloc_free(&m, b2, 16);
    
    // On a 4 places libres au total, mais la liste est emmêlée.
    // On demande un gros espace de 3 places (48 octets).
    // Grâce à l'exercice 2, l'allocateur va trier la liste en : 0 -> 2 -> 3 -> 4
    // Il va trouver que 2, 3 et 4 sont consécutifs et réussir l'allocation !
    void* big_chunk = memalloc_allocate(&m, 48); 
    
    // Vérifications :
    assert_non_null(big_chunk); // L'allocation ne doit pas renvoyer NULL (malloc qui echoue)
    assert_ptr_equal(big_chunk, b2); // Il a dû utiliser les places 2, 3 et 4 car ce sont les seules places libres consécutives.
    assert_int_equal(m.errno, E_SUCCESS); // Pas d'erreur
    
    memalloc_finalize(&m);
}

// TEST LIFELIKE Exercice 3

void test_memory_lifelike(void** state) {
    struct memory_alloc m;
    // On crée 10 blocs de 16 octets (160 octets au total)
    memalloc_init(&m, 10, 16);

    // Test du lifelike_malloc 
    // On demande 20 octets pour l'utilisateur.
    // L'allocateur doit rajouter 8 octets pour l'étiquette = 28 octets.
    // 28 octets nécessitent 2 blocs de 16 octets (32 octets au total).
    void* ptr1 = memalloc_lifelike_malloc(&m, 20);
    
    assert_non_null(ptr1);
    // On a consommé 2 blocs, il doit en rester 8.
    assert_int_equal(m.available_blocks, 8); 

    // Vérification magique : on recule de 8 octets pour lire l'étiquette cachée !
    size_t* etiquette = (size_t*)((char*)ptr1 - sizeof(void*));
    // L'étiquette doit indiquer qu'on a physiquement réservé 32 octets
    assert_int_equal(*etiquette, 32); 

    // On écrit dedans pour prouver que la zone est accessible
    memset(ptr1, 42, 20); // a partir de ptr1, les 20 premiers octets vont recevoir la valeur 42

    // Test du lifelike_realloc 
    // On demande à agrandir notre zone à 40 octets.
    // 40 octets + 8 (étiquette) = 48 octets. Cela rentre pile poil dans 3 blocs de 16 !
    void* ptr2 = memalloc_lifelike_realloc(&m, ptr1, 40);
    
    assert_non_null(ptr2);
    // ptr1 a été libéré (rendant 2 blocs) et ptr2 a été alloué (prenant 3 blocs)
    // Bilan : 10 - 3 = 7 blocs disponibles restants.
    assert_int_equal(m.available_blocks, 7);

    // Test du lifelike_free 
    // On libère la nouvelle zone sans donner sa taille !
    memalloc_lifelike_free(&m, ptr2);
    
    // Le parking doit être revenu à 10 places libres
    assert_int_equal(m.available_blocks, 10);

    memalloc_finalize(&m);
}

// exercice 4 : canari 

void test_memory_canary(void** state) {
    struct memory_alloc m;
    memalloc_init(&m, 5, 16);

    // On demande un tableau pour stocker 10 caractères avec la fonction _canary
    char* texte = (char*) memalloc_lifelike_malloc_canary(&m, 10);
    
    // Le développeur se trompe et écrit dans la case "moins 1" (AVANT le tableau).
    // Cela va écraser les données de notre pauvre Canari...
    texte[-1] = 'X'; 

    // Au moment de libérer, la fonction va inspecter le canari et afficher l'alerte !
    memalloc_lifelike_free_canary(&m, texte);

    memalloc_finalize(&m);
}

// on test

int main(int argc, char**argv) {
  const struct CMUnitTest tests[] = {
    /* a few tests for memory allocator.
     *
     * If you implemented correctly the functions, all these tests should be successfull
     * Of course this test suite may not cover all the tricky cases, and you should add
     * your own tests.
     */
    cmocka_unit_test(test_memory_init),
    cmocka_unit_test(test_memory_nb_consecutive_blocks),
    cmocka_unit_test(test_memory_alloc),
    cmocka_unit_test(test_memory_free_behavior),
    cmocka_unit_test(test_memory_reorder_and_allocate),
    cmocka_unit_test(test_memory_lifelike),
    cmocka_unit_test(test_memory_canary)
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
