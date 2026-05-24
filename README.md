# Memory Allocator

Petit projet en C qui implémente un allocateur mémoire simple basé sur une zone mémoire préallouée découpée en blocs de taille fixe.

## Contenu du projet

```text
.
├── memory_alloc.h          # Déclarations des structures et fonctions
├── memory_alloc.c          # Implémentation de l'allocateur
├── memory_alloc_main.c     # Programme principal de démonstration
├── test_memory_alloc.c     # Tests unitaires avec CMocka
├── Makefile                # Compilation du projet
├── install_cmocka.sh       # Script d'installation locale de CMocka
└── cmocka/                 # Bibliothèque CMocka installée localement
```

## Fonctionnalités

Le projet permet de :

- initialiser une zone mémoire préallouée ;
- découper cette zone en blocs de taille fixe ;
- allouer un ou plusieurs blocs consécutifs ;
- libérer une zone mémoire ;
- afficher l'état courant de l'allocateur ;
- réordonner la liste des blocs libres pour limiter la fragmentation ;
- simuler des fonctions proches de `malloc`, `free` et `realloc` ;
- utiliser un mécanisme de canari pour détecter certains débordements mémoire.

## Compilation

Pour compiler le programme principal :

```bash
make
```

Cela génère l'exécutable :

```text
memory_alloc
```

## Exécution

Pour lancer le programme principal :

```bash
./memory_alloc
```

## Tests unitaires

Le projet utilise la bibliothèque **CMocka** pour les tests.

Si CMocka n'est pas encore installé dans le dossier du projet, lancer :

```bash
chmod +x install_cmocka.sh
./install_cmocka.sh
```

Puis lancer les tests avec :

```bash
make test
```

## Nettoyage

Pour supprimer les exécutables générés :

```bash
make clean
```

## Fichiers principaux

### `memory_alloc.h`

Contient les structures principales du projet :

- `struct memory_block` : représente un bloc libre dans la liste chaînée ;
- `struct memory_alloc` : représente l'allocateur mémoire ;
- `enum memory_errno` : indique l'état de la dernière opération mémoire.

### `memory_alloc.c`

Contient l'implémentation des fonctions principales :

- `memalloc_init`
- `memalloc_finalize`
- `memalloc_allocate`
- `memalloc_free`
- `memalloc_reorder`
- `memalloc_lifelike_malloc`
- `memalloc_lifelike_free`
- `memalloc_lifelike_realloc`

### `test_memory_alloc.c`

Contient les tests unitaires vérifiant notamment :

- l'initialisation de l'allocateur ;
- l'allocation et la libération de blocs ;
- le comptage des blocs consécutifs ;
- le réordonnancement des blocs libres ;
- les fonctions proches de `malloc`, `free` et `realloc` ;
- le mécanisme de canari.

## Exemple d'utilisation

```c
struct memory_alloc m;

memalloc_init(&m, 8, 16);

char* buffer = memalloc_allocate(&m, 64);

if (buffer != NULL) {
    buffer[0] = 'a';
    memalloc_free(&m, buffer, 64);
}

memalloc_finalize(&m);
```

## Auteur

Projet réalisé dans le cadre d'un exercice de programmation en C sur la gestion mémoire.
