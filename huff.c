#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct noeud_h{
    unsigned int freq;
    int car;
    struct noeud_h *g;
    struct noeud_h *d;
} noeud_h;

typedef struct cellule_h{
    unsigned int freq;
    int car;
    noeud_h *noeud;
    struct cellule_h *s;
} cellule_h;

noeud_h* creerNoeud(int car, unsigned int freq, noeud_h *g, noeud_h *d);
cellule_h* creerCellule(noeud_h *noeud);
cellule_h* insererTrie(cellule_h *tete, cellule_h *nouvelle);
noeud_h* construireArbre(cellule_h *tete);
void libererArbre(noeud_h *a);
void compression(FILE *fichier, FILE *fichierSortie, char **codes);
long int tailleFichier(FILE *fichier);
long int tailleTableau(char *chaine, int ajout);
void genererCodes(noeud_h *racine, char *codeActuel, char **codes);

noeud_h* creerNoeud(int car, unsigned int freq, noeud_h *g, noeud_h *d){

    noeud_h *noeud = (noeud_h *)malloc(sizeof *noeud);
    if(noeud == NULL)
        exit(1);

    noeud->car = car;
    noeud->freq = freq;
    noeud->g = g;
    noeud->d = d;
    return noeud;
}

//créer une cellule de liste chaînée
cellule_h* creerCellule(noeud_h *noeud){
    cellule_h *cellule = (cellule_h *)malloc(sizeof *cellule);
    if(cellule == NULL)
        exit(1);

    cellule->freq = noeud->freq;
    cellule->car = noeud->car;
    cellule->noeud = noeud;
    cellule->s = NULL;
    return cellule;
}

//insérer une cellule dans la liste triée par fréquence
cellule_h* insererTrie(cellule_h *tete, cellule_h *nouvelle){
    if (tete == NULL || nouvelle->freq < tete->freq){
        nouvelle->s = tete;
        return nouvelle;
    }
    cellule_h *courant = tete;
    while (courant->s && courant->s->freq <= nouvelle->freq){
        courant = courant->s;
    }
    nouvelle->s = courant->s;
    courant->s = nouvelle;
    return tete;
}


//construire de l'arbre de huffman
noeud_h* construireArbre(cellule_h *tete){
    while (tete && tete->s) {
        //on prend les 2 cellules avec le moins de fréquences
        cellule_h *gauche = tete;
        cellule_h *droite = tete->s;
        tete = droite->s;

        //fusion des 2 noeuds
        noeud_h *parent = creerNoeud(-1, gauche->freq + droite->freq, gauche->noeud, droite->noeud);

        //insere le noeud fusionné dans la liste
        cellule_h *nouvelle = creerCellule(parent);
        tete = insererTrie(t, nouvelle);

        free(gauche);
        free(droite);
    }
    return tete ? tete->noeud : NULL;
}

void libererArbre(noeud_h *a){
    if (a == NULL)
        return;
    libererArbre(a->g);
    libererArbre(a->d);
    free(a);
}

//compression -> former en paquet d'octet les codes huffman
void compression(FILE *fichier, FILE *fichierSortie, char **codes){
    int c;
    unsigned char buffer = 0;
    int cmp = 0;

    while ((c = fgetc(fichier)) != EOF){
        char *code = codes[(unsigned char)c];
        if (code != NULL) {
            for (int i = 0; code[i] != '\0'; i++){
                if (code[i] == '1') {
                    buffer += (1 << (7 - cmp));
                }

                cmp++;

                if (cmp == 8){
                    fputc(buffer, fichierSortie);
                    buffer = 0;
                    cmp = 0;
                }
            }
        }
    }

    //cas où bit restant 
    if (cmp > 0) {
        fputc(buffer, fichierSortie);
    }
}

/*source: https://www.geeksforgeeks.org/c-program-find-size-file/*/
long int tailleFichier(FILE *fichier){ 
    if (fichier == NULL){ 
        printf("Erreur lors de l'ouverture du fichier\n"); 
        exit(1); 
    } 
  
    fseek(fichier, 0L, SEEK_END); 
    long int res = ftell(fichier); 

    return res; 
} 

long int tailleTableau(char *chaine, int ajout){
    int longueur = 0;
    while (chaine[longueur] != '\0'){
        longueur++;
    }
    return longueur + ajout + 1;
}

//genere les codes huffman
void genererCodes(noeud_h *racine, char *codeActuel, char **codes){
    if (racine == NULL)
        return;

    if (racine->g == NULL && racine->d == NULL){
        //allouer suffisamment de mémoire pour le code actuel
        int taille = tailleTableau(codeActuel,0);
        codes[racine->car] = malloc(taille);

        if(codes[racine->car] == NULL){
            exit(1);
        }

        if (codes[racine->car] != NULL){
            strcpy(codes[racine->car], codeActuel);
        }
        return;
    }

    char *gaucheCode = malloc(tailleTableau(codeActuel, 1));
    if (gaucheCode == NULL){
        exit(1);
    }
    strcpy(gaucheCode, codeActuel);
    strcat(gaucheCode, "0");
    genererCodes(racine->g, gaucheCode, codes);
    free(gaucheCode);

    char *droitCode = malloc(tailleTableau(codeActuel, 1));
    if (droitCode == NULL){
        exit(1);
    }
    strcpy(droitCode, codeActuel);
    strcat(droitCode, "1");
    genererCodes(racine->d, droitCode, codes);
    free(droitCode);
}




int main(int argc, char *argv[]){
    
    if (argc < 2){
        printf("Veuillez indiquer un fichier\n");
        exit(1);
    }

    FILE *fichier = fopen(argv[1], "r");
    if (fichier == NULL){
        printf("Erreur lors de l'ouverture du fichier\n");
        exit(1);
    }

    unsigned int frequences[256] = {0};
    int c;
    while ((c = fgetc(fichier)) != EOF){
        frequences[(unsigned char)c]++;
    }
    fclose(fichier);

    cellule_h *tete = NULL;
    for (int i = 0; i < 256; i++){
        if (frequences[i] > 0){
            noeud_h *noeud = creerNoeud(i,frequences[i],NULL,NULL);
            cellule_h *cellule = creerCellule(noeud);
            tete = insererTrie(tete, cellule);
        }
    }

    noeud_h *racine = construireArbre(tete);

    char *codes[256] = {0};
    char codeActuel[256] = "";
    genererCodes(racine, codeActuel, codes);

    //compression du fichier
    fichier = fopen(argv[1], "r");
    FILE *fichierSortie = fopen("compression.bin","wb");

    if (fichier == NULL || fichierSortie == NULL){
        printf("Erreur lors de l'ouverture des fichiers\n");
        exit(1);
    }

    compression(fichier, fichierSortie, codes);
    printf("Compression réussie!\n");

    long int res = tailleFichier(fichier); 
    if (res != -1) 
        printf("Taille initiale du fichier: %ld octets\n",res); 
    long int res1 = tailleFichier(fichierSortie); 
    if (res1 != -1) 
        printf("Taille après compression: %ld octets\n",res1); 

    fclose(fichier);
    fclose(fichierSortie);

    libererArbre(racine);
    for (int i = 0; i < 256; i++){
        free(codes[i]);
    }

    return 0;
}
