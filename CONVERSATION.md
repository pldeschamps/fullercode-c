# Création du projet fullercode-c — fil de discussion (2026-06-05)

---

## Demande initiale (utilisateur)

> dans D:\repos\fullercode-c créer un nouveau projet en C qui geocode et décode les fullercode :
> - une routine (je suggère fullergeocoding) qui à partir de coordonnées lat-lon double et un ushortint i crée un fuller code de i caractères
> - une routine (je suggère fullergeodecoding) qui à partir d'un fullercode créer des coordonnées lat-lon en degrés décimales (deux double)

---

## Exploration du projet TypeScript de référence (d:\repos\fullercode-ts)

Avant d'écrire le C, Claude a exploré les sources TypeScript existantes pour comprendre l'algorithme.

### Fichiers lus

| Fichier | Contenu clé |
|---|---|
| `src/fullercode.ts` | `findClosestFace`, `findSubtriangle3D`, `projectToTriangle`, `findSubtriangle2D` |
| `src/Subtriangles.ts` | Calcul des 15 points de subdivision et des 16 sous-triangles |
| `src/FaceGeoPositions.ts` | Structure de données d'un triangle |
| `src/fuller.ts` | Chargement de l'icosaèdre depuis `icosahedron.json` |
| `src/state.ts` | `RADIUS = 6371010.0`, `MAX_FULLERCODE_LEN = 12` |
| `src/cesium.ts` | Fonction `findEnclosingTriangle` — boucle principale d'encodage |
| `src/UnitSphereCartesian.ts` | Conversion lat/lon → coordonnées cartésiennes unité |
| `icosahedron.json` | 12 sommets + 20 faces de l'icosaèdre avec leurs `subtrianglesIds` |

### Constantes extraites

```
RADIUS           = 6371010.0 m
MAX_FULLERCODE_LEN = 12 caractères
transition3D2D   = 11  (niveaux 0-10 : tests 3D ; niveaux 11+ : projection 2D)
```

### Alphabet fullercode

- **1er caractère** (face initiale, 20 possibilités) : `C M 3 F A 2 H 5 P X 9 V 8 T R 7 N S J K`
- **Caractères suivants** (subdivision, 16 possibilités) : `C M 3 F A 2 H 5 P X 9 V 8 T R 7`

---

## Algorithme

### Données de l'icosaèdre (`icosahedron.json`)

12 sommets (coordonnées sur la sphère unité, à multiplier par RADIUS) :

```
id  x                      y                      z
 1  -0.414682220851542      0.655962408702304      0.630675807431286
 2   0.420152428828912      0.078145249043253      0.904082549660778
 3   0.5188367348275258     0.8354203781459196     0.1813318349657353
 4   0.9950094390590583    -0.09134780014578021    0.04014717414595104
 5   0.3557813991100285    -0.8435800034654891     0.40223422753474947
 6  -0.5154559603719798    -0.3817168942575905     0.7672009942351089
 7   0.414682220851542     -0.655962408702304     -0.630675807431286
 8  -0.420152428828912     -0.078145249043253     -0.904082549660778
 9  -0.5188367348275258    -0.8354203781459196    -0.1813318349657353
10  -0.9950094390590583     0.09134780014578021   -0.04014717414595104
11  -0.3557813991100285     0.8435800034654891    -0.40223422753474947
12   0.5154559603719798     0.3817168942575905    -0.7672009942351089
```

20 faces (id, sommets, subtrianglesIds) :

```
P  [2,5,6]   "C5PX9V8TR7M3FA2H"
M  [2,4,5]   "CA2H5PX9V8TR7M3F"
X  [2,3,4]   "CM3FA2H5PX9V8TR7"
C  [2,1,3]   "CTR7M3FA2H5PX9V8"
N  [2,6,1]   "C9V8TR7M3FA2H5PX"
V  [12,4,3]  "C2AF3M7RT8V9XP5H"
5  [4,12,7]  "CA2H5PX9V8TR7M3F"
F  [7,5,4]   "CP5H2AF3M7RT8V9X"
S  [8,7,12]  "CP5H2AF3M7RT8V9X"
A  [5,7,9]   "C5PX9V8TR7M3FA2H"
J  [1,10,11] "CTR7M3FA2H5PX9V8"
9  [6,9,10]  "C9V8TR7M3FA2H5PX"
H  [9,6,5]   "CV9XP5H2AF3M7RT8"
R  [8,9,7]   "CV9XP5H2AF3M7RT8"
3  [3,11,12] "CM3FA2H5PX9V8TR7"
T  [11,3,1]  "C3M7RT8V9XP5H2AF"
K  [8,11,10] "C3M7RT8V9XP5H2AF"
7  [10,1,6]  "CRT8V9XP5H2AF3M7"
8  [8,10,9]  "CRT8V9XP5H2AF3M7"
2  [8,12,11] "C2AF3M7RT8V9XP5H"
```

### Encodage (fullergeocoding)

```
1. Convertir (lat, lon) → coordonnées cartésiennes sur la sphère × RADIUS
       cos_lat = cos(lat_rad)
       q = (cos_lat·cos(lon_rad)·R,  cos_lat·sin(lon_rad)·R,  sin(lat_rad)·R)

2. Trouver la face initiale la plus proche (distance euclid. au centroïde)
       → code[0] = face.id

3. Pour i = 0 .. len-2 :
       a. Calculer les 15 points de subdivision de la face courante
       b. Si i < 11 : findSubtriangle3D  (produits vectoriels)
          Si i == 11 : projeter en 2D (projectToTriangle) puis findSubtriangle2D
          Si i > 11  : continuer en 2D
       c. code[i+1] = ids[index_trouvé]
       d. avancer dans le sous-triangle trouvé
```

### Points de subdivision (noms issus de Subtriangles.ts)

Pour un triangle (a, b, c) — midpoints projetés sur la sphère :

```
sp[0]  = a
sp[1]  = b
sp[2]  = c
sp[3]  = ab   = sph_mid(a, b)
sp[4]  = bc   = sph_mid(b, c)
sp[5]  = ac   = sph_mid(a, c)
sp[6]  = a_ab = sph_mid(a, ab)
sp[7]  = ac_ab= sph_mid(ac, ab)
sp[8]  = ac_a = sph_mid(ac, a)
sp[9]  = b_bc = sph_mid(b, bc)
sp[10] = ab_bc= sph_mid(ab, bc)
sp[11] = ab_b = sph_mid(ab, b)
sp[12] = c_ac = sph_mid(c, ac)
sp[13] = bc_ac= sph_mid(bc, ac)
sp[14] = bc_c = sph_mid(bc, c)
```

### 16 sous-triangles (sommets dans sp[], orientation)

```
idx  sp[a] sp[b] sp[c]   orient_same
  0    7    10    13        1
  1    0     6     8        1
  2    7     8     6        0  (orienté inversé)
  3    6     3     7        1
  4   10     7     3        0
  5    3    11    10        1
  6   11     1     9        1
  7    9    10    11        0
  8   10     9     4        1
  9    4    13    10        0
 10   13     4    14        1
 11   12    14     2        1
 12   14    12    13        0
 13    5    13    12        1
 14   13     5     7        0
 15    8     7     5        1
```

### Attribution des caractères aux sous-triangles

- Par défaut (face initiale OU `parentOrientation == true`) :  
  `ids[i] = stids[i]`

- Si `depth > 0` ET `parentOrientation == false` :  
  `ids[i] = stids[pBox[i]]` avec `pBox = [0,2,1,8,9,10,7,6,13,14,15,12,11,3,4,5]`

### Décodage (fullergeodecoding)

```
1. code[0] → trouver la face initiale dans les 20 faces
2. Pour chaque caractère suivant :
       a. Calculer les ids[] de la face courante
       b. Trouver k tel que ids[k] == code[i]
       c. Avancer dans le sous-triangle k
3. Centroïde du triangle final → lat/lon
       centroïde = sph_center(v0, v1, v2)
       lat = asin(z/R)  ×  180/π
       lon = atan2(y,x) ×  180/π
```

---

## Fichiers créés

```
D:\repos\fullercode-c\
├── fullercode.h      API publique
├── fullercode.c      Implémentation (~270 lignes, C99)
├── main.c            Programme de test/démo
├── Makefile          build : make  (gcc MSYS2)
├── CMakeLists.txt    build : cmake (si disponible)
└── CONVERSATION.md   ce fichier
```

### API publique (`fullercode.h`)

```c
#include <stdint.h>

/* lat/lon (degrés décimaux) + longueur souhaitée → fullercode
 * out doit pointer sur un buffer d'au moins (len+1) octets.
 * Retourne 0 si succès, -1 si paramètres invalides. */
int fullergeocoding(double lat_deg, double lon_deg, uint16_t len, char *out);

/* fullercode → lat/lon (degrés décimaux)
 * Retourne 0 si succès, -1 si code invalide. */
int fullergeodecoding(const char *code, double *lat_deg, double *lon_deg);
```

---

## Build et tests

### Compilation

```
gcc -O2 -std=c99 -Wall -Wextra -o fullercode_demo fullercode.c main.c -lm
```

### Résultats des tests (sortie de fullercode_demo)

```
=== fullercode geocoding tests ===

Paris (6 chars)                 code=MHAA8V          decoded=(48.830723, 2.357478)  err=2903.2 m
Paris (10 chars)                code=MHAA8V97TC      decoded=(48.856624, 2.352038)  err=12.2 m
Paris (12 chars)                code=MHAA8V97TC7X    decoded=(48.856605, 2.352198)  err=0.6 m
New York (8 chars)              code=PT7R92MC        decoded=(40.711294, -74.008494)  err=268.8 m
Tokyo (8 chars)                 code=7P8FPV8M        decoded=(35.689080, 139.691304)  err=58.9 m
Sydney (8 chars)                code=KCXH5FRX        decoded=(-33.868915, 151.209015)  err=29.3 m
North Pole (6 chars)            code=N8A8VH          decoded=(89.978916, -90.714619)  err=2344.4 m
South Pole (6 chars)            code=SXMX53          decoded=(-89.978916, 89.285381)  err=2344.4 m
Null Island (6 chars)           code=V7RRM8          decoded=(0.004603, -0.014642)  err=1706.7 m
Antimeridian E (6 c)            code=92HH3T          decoded=(-0.004603, 179.985358)  err=1706.7 m
Antimeridian W (6 c)            code=92HH3T          decoded=(-0.004603, 179.985358)  err=1706.7 m

=== decode only ===
  decode("C")      -> (46.041894, 71.527903)
  decode("CM")     -> (55.718120, 78.579444)
  decode("CM3")    -> (53.338850, 85.596297)
  decode("CM3F")   -> (53.414010, 83.347176)
  decode("CM3FA2") -> (53.328817, 83.908516)

=== error cases ===
  encode len=0:  -1 (expected -1)
  encode lat>90: -1 (expected -1)
  decode "":     -1 (expected -1)
  decode "?":    -1 (expected -1)
```

### Précision par longueur de code (Paris)

| Longueur | Erreur type |
|---|---|
| 6 caractères | ~3 km |
| 8 caractères | ~270 m |
| 10 caractères | ~12 m |
| 12 caractères | ~0.6 m |

La précision quadruple à chaque caractère supplémentaire (chaque niveau subdivise en 16 sous-triangles).
