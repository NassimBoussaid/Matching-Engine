# Matching-Engine-C++

![C++](https://img.shields.io/badge/C%2B%2B-17%2B-blue)
![License](https://img.shields.io/badge/Licence-MIT-green)

## Présentation

Ce dépôt contient un moteur de matching haute performance développé en C++ moderne (C++17), destiné à traiter et à apparier des ordres financiers selon des règles strictes de priorité (prix-temps).  
Le moteur simule la logique de base d’un système de négociation électronique, capable de lire des ordres à partir d’un fichier CSV, de les exécuter conformément aux conventions de marché, et de produire une piste d’audit complète au format CSV.

---

## Fonctionnalités principales

- **Traitement des ordres** :
  - Prend en charge les actions d’ordre `NEW`, `MODIFY` et `CANCEL`.
  - Supporte les types d’ordre `LIMIT` et `MARKET`.
  - Validation complète des ordres en entrée.

- **Gestion du carnet d’ordres** :
  - Maintient un carnet d’ordres dédié par instrument financier.
  - Priorité d’exécution : *meilleur prix d’abord*, puis *ancienneté (timestamp)*.
  - Gère tout le cycle de vie des ordres : en attente, exécutés, partiellement exécutés, annulés, rejetés.

- **Entrée/Sortie CSV** :
  - Lecture des ordres depuis un fichier CSV configurable.
  - Écriture des rapports détaillés d’exécution au format CSV.

- **Suite de tests robuste** :
  - Framework de tests complet inclus.
  - Tests unitaires, tests d’intégration, gestion des erreurs, benchmarks de performance.

---

## Structure du projet

```
Matching-Engine/
├── matching_engine
│   ├── main.cpp                  # Programme principal
│   ├── test_matching_engine.cpp  # Suite de tests unitaires
│   ├── Order.h                   # Définition de la structure Order
│   ├── OrderBook.h               # Gestion du carnet d'ordres
│   ├── MatchingEngine.h          # Moteur de matching multi-instruments
│   ├── CSVParser.h               # Lecture/écriture des fichiers CSV
│   ├── Validator.h               # Validation des champs d'ordres
│   ├── Makefile                  # Fichier de compilation
└── README.md                     # Documentation du projet
```

## Installation

Cloner le dépôt :

```bash
git clone https://github.com/NassimBoussaid/Matching-Engine
cd Matching-Engine
cd matching_engine
```

Compiler le projet :

```bash
make
```

---

## Utilisation

### Exécuter le moteur de matching

Lancer le moteur sur un fichier CSV d'entrée :

```bash
g++ -std=c++17 -o matching_engine main.cpp -O2
./matching_engine input.csv output.csv
```

Le moteur va :
- Lire le fichier `input.csv`
- Valider les champs des ordres
- Appliquer la logique de matching (priorité prix-temps)
- Gérer les actions `NEW`, `MODIFY`, `CANCEL`
- Générer un fichier `output.csv` détaillant le statut de chaque ordre.

---

### Tests unitaires

Vérifier le bon fonctionnement du moteur en exécutant la suite de tests :

```bash
make run_tests
```

Les tests couvrent :
- Parsing et validation des ordres
- Logique de matching
- Gestion des `MODIFY` et `CANCEL`
- Support multi-instrument
- Tests de performance (benchmark)

---

### Générer un fichier de test d'exemple

```bash
make sample_input
make run_sample
```

---

### Générer un fichier de test de validation (ordres erronés)

```bash
make validation_test
make run_validation
```

---

## Fonctionnalités supportées

| Ordres/Actions | Description |
|----------------|-------------|
| Ordres LIMIT   | Ordres avec limite de prix (prix minimum pour SELL, prix maximum pour BUY) |
| Ordres MARKET  | Ordres au marché exécutés au meilleur prix disponible |
| Actions NEW    | Insertion d'un nouvel ordre dans le carnet |
| Actions MODIFY | Modification d'un ordre existant avec conservation de l'historique d'exécution |
| Actions CANCEL | Annulation d'un ordre présent dans le carnet |

---

## Auteurs

- Nassim BOUSSAID
- Nicolas COUTURAUD
- Karthy MOUROUGAYA
- Hugo SOULIER

Superivisé par M. Kevin Rodrigues

---

## Licence

Ce projet est sous licence MIT.
