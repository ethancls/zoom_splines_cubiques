#include "lodepng.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* COMMANDES DE COMPILATION :

    gcc zoom.c lodepng.c -o zoom -lm
    ./zoom entree.png sortie.png

*/

/* CONSTANTES */

/* STRUCTURES DE DONNÉES */

typedef struct myimage_t
{
    unsigned hauteur;
    unsigned largeur;
    unsigned char *rouge;
    unsigned char *vert;
    unsigned char *bleu;
    unsigned char *alpha;

} myimage;

typedef struct polynome_t
{
    double a;
    double b;
    double c;
    double d;
} polynome;

typedef struct maspline_t
{
    unsigned n;
    double *x;
    polynome **p;
} maspline;

/* DÉCLARATIONS DES FONCTIONS */

// Initialiser la structure
void init_myimage(myimage *image);

void afficherBarreChargement(int progress, int total);

// Lire une image et la stocker dans une structure myimage
myimage lireImage(const char *filename);

// Écrire l'image à partir des composantes rouge, vert, bleu et alpha
void ecrireImage(myimage img, const char *filename);

// Conversion Noir et Blanc
void NoirEtBlanc(myimage *img);

void calcul_une_spline(double x, double y, double x1, double y1, double k1, double k2, double *a, double *b, double *c, double *d);

void calcul_spline(double *tab_x, double *tab_y, unsigned n, maspline *spline);

myimage sized(myimage *img, unsigned hauteur, unsigned largeur);

/* DÉFINITIONS DES FONCTIONS */

void init_myimage(myimage *image)
{
    image->hauteur = 0;
    image->largeur = 0;
    image->rouge = NULL;
    image->vert = NULL;
    image->bleu = NULL;
    image->alpha = NULL;
}

myimage lireImage(const char *filename)
{
    myimage img;
    init_myimage(&img);
    unsigned erreur;
    unsigned char *temp;

    erreur = lodepng_decode32_file(&temp, &img.largeur, &img.hauteur, filename);
    if (erreur)
        printf("Erreur %u : %s\n", erreur, lodepng_error_text(erreur));

    printf("\n -- Chargement de l'image : ");
    printf("@> Données image lue: %u x %u pixels | %u MP\n", img.largeur, img.hauteur, (img.largeur * img.hauteur) / 1000000);

    img.rouge = (unsigned char *)malloc(img.largeur * img.hauteur);
    img.vert = (unsigned char *)malloc(img.largeur * img.hauteur);
    img.bleu = (unsigned char *)malloc(img.largeur * img.hauteur);
    img.alpha = (unsigned char *)malloc(img.largeur * img.hauteur);

    for (unsigned int i = 0; i < img.largeur * img.hauteur; i++)
    {
        // afficherBarreChargement(i, img.largeur * img.hauteur);
        img.rouge[i] = temp[4 * i];
        img.vert[i] = temp[4 * i + 1];
        img.bleu[i] = temp[4 * i + 2];
        img.alpha[i] = temp[4 * i + 3];
    }

    free(temp);
    return img;
}

void ecrireImage(myimage img, const char *filename)
{
    printf("\n -- Écriture de l'image : ");
    printf("@> Données image écrite : %u x %u pixels | %u MP\n\n", img.largeur, img.hauteur, (img.largeur * img.hauteur) / 1000000);

    unsigned erreur;
    unsigned char *temp = (unsigned char *)malloc((4 * img.largeur * img.hauteur));
    int j = 0;
    for (unsigned int i = 0; i < 4 * img.largeur * img.hauteur; i += 4)
    {
        // afficherBarreChargement(i, 4 * img.largeur * img.hauteur);
        temp[i] = img.rouge[j];
        temp[i + 1] = img.vert[j];
        temp[i + 2] = img.bleu[j];
        temp[i + 3] = img.alpha[j];
        j++;
    }

    erreur = lodepng_encode32_file(filename, temp, img.largeur, img.hauteur);
    if (erreur)
        printf("Erreur %u : %s\n", erreur, lodepng_error_text(erreur));
    free(temp);
}

void calcul_une_spline(double x, double y, double x1, double y1, double k1, double k2, double *a, double *b, double *c, double *d)
{
    double e = k1 + k2 + 2 * y - 2 * y1;
    double f = 3 * y1 - k2 - 2 * k1 - 3 * y;
    double g = k1;
    double h = y;
    *a = e / pow(x1 - x, 3);
    *b = (f / pow(x1 - x, 2)) - (3 * x * e / pow(x1 - x, 3));
    *c = (-2 * f * x / pow(x1 - x, 2)) + (3 * x * x * e / pow(x1 - x, 3)) + (g / (x1 - x));
    *d = (-e * x * x * x / pow(x1 - x, 3)) + (f * x * x / pow(x1 - x, 2)) - (g * x / (x1 - x)) + h; // c'est un peu moche pardon
}

void calcul_spline(double *tab_x, double *tab_y, unsigned n, maspline *spline)
{
    spline->n = n - 1;
    spline->x = tab_x;
    spline->p = (polynome **)calloc(spline->n, sizeof(polynome *));

    for (unsigned i = 0; i < spline->n; i++)
    {
        spline->p[i] = (polynome *)calloc(1, sizeof(polynome));

        double k1, k2;
        if (i == 0)
        {
            k1 = (tab_y[i + 1] - tab_y[i]) / (tab_x[i + 1] - tab_x[i]);
            k2 = (tab_y[i + 2] - tab_y[i + 1]) / (tab_x[i + 2] - tab_x[i + 1]);
        }
        else if (i == n - 2)
        {
            k1 = (tab_y[i] - tab_y[i - 1]) / (tab_x[i] - tab_x[i - 1]);
            k2 = (tab_y[i + 1] - tab_y[i]) / (tab_x[i + 1] - tab_x[i]);
        }
        else
        {
            k1 = (tab_y[i] - tab_y[i - 1]) / (tab_x[i] - tab_x[i - 1]);
            k2 = (tab_y[i + 2] - tab_y[i + 1]) / (tab_x[i + 2] - tab_x[i + 1]);
        }

        calcul_une_spline(tab_x[i], tab_y[i], tab_x[i + 1], tab_y[i + 1], k1, k2, &(spline->p[i]->a), &(spline->p[i]->b), &(spline->p[i]->c), &(spline->p[i]->d));
    }
}

void afficherBarreChargement(int progress, int total)
{
    int barWidth = 70;
    float pourcentage = (float)progress / total;
    int pos = (int)(barWidth * pourcentage);

    printf("\033[0;33m["); // Change la couleur du texte en jaune
    for (int i = 0; i < barWidth; i++)
    {
        if (i <= pos)
            printf("#");
        else
            printf(" ");
    }

    printf("\033[0m] \033[0;35m%.0f%%\r", pourcentage * 100); // Change la couleur du texte en violet
    fflush(stdout);
}

myimage sized(myimage *img, unsigned largeur, unsigned hauteur)
{
    myimage sized;
    init_myimage(&sized);
    sized.hauteur = hauteur;
    sized.largeur = largeur;

    // Affectation des matrices aux tableaux de l'image sized
    sized.rouge = (unsigned char *)calloc(sized.largeur * sized.hauteur, sizeof(unsigned char));
    sized.vert = (unsigned char *)calloc(sized.largeur * sized.hauteur, sizeof(unsigned char));
    sized.bleu = (unsigned char *)calloc(sized.largeur * sized.hauteur, sizeof(unsigned char));
    sized.alpha = (unsigned char *)calloc(sized.largeur * sized.hauteur, sizeof(unsigned char));

    double tab_x[2];
    double tab1_r_y[2];
    double tab2_r_y[2];
    double tab1_v_y[2];
    double tab2_v_y[2];
    double tab1_b_y[2];
    double tab2_b_y[2];
    double tab1_a_y[2];
    double tab2_a_y[2];

    maspline spline1_r;
    maspline spline2_r;
    maspline spline1_v;
    maspline spline2_v;
    maspline spline1_b;
    maspline spline2_b;
    maspline spline1_a;
    maspline spline2_a;

    double eta_r, psi_r, eta_v, psi_v, eta_b, psi_b, eta_a, psi_a;
    double tab_x_vert[2];
    double tab_r_y[2];
    double tab_v_y[2];
    double tab_b_y[2];
    double tab_a_y[2];

    maspline spline_r_vert;
    maspline spline_v_vert;
    maspline spline_b_vert;
    maspline spline_a_vert;

    unsigned char r, v, b, a;

    double x, y;
    int projection;
    int ligne = 0, colonne = 0;
    for (int i = 0; i < (int)sized.hauteur * (int)sized.largeur; i++)
    {
        afficherBarreChargement(i, (int)sized.largeur * (int)sized.hauteur);
        if (colonne >= (int)sized.largeur)
        {
            ligne++;
            colonne = 0;
        }
        x = (double)((double)img->largeur / (double)sized.largeur) * (double)colonne;
        y = (double)((double)img->hauteur / (double)sized.hauteur) * (double)ligne;

        // Calculer les coordonnées du pixel de la nouvelle image par rapport à l'image d'entrée (fonction linéaire)
        projection = (int)(floor(x) + floor(y) * img->largeur);
        tab_x[0] = floor(x);
        tab_x[1] = floor(x) + 1;

        tab1_r_y[0] = img->rouge[projection];
        tab1_r_y[1] = img->rouge[projection + 1];
        tab2_r_y[0] = img->rouge[projection + img->largeur];
        tab2_r_y[1] = img->rouge[projection + img->largeur + 1];

        tab1_v_y[0] = img->vert[projection];
        tab1_v_y[1] = img->vert[projection + 1];
        tab2_v_y[0] = img->vert[projection + img->largeur];
        tab2_v_y[1] = img->vert[projection + img->largeur + 1];

        tab1_b_y[0] = img->bleu[projection];
        tab1_b_y[1] = img->bleu[projection + 1];
        tab2_b_y[0] = img->bleu[projection + img->largeur];
        tab2_b_y[1] = img->bleu[projection + img->largeur + 1];

        tab1_a_y[0] = img->alpha[projection];
        tab1_a_y[1] = img->alpha[projection + 1];
        tab2_a_y[0] = img->alpha[projection + img->largeur];
        tab2_a_y[1] = img->alpha[projection + img->largeur + 1];

        calcul_spline(tab_x, tab1_r_y, 2, &spline1_r);
        calcul_spline(tab_x, tab2_r_y, 2, &spline2_r);

        calcul_spline(tab_x, tab1_v_y, 2, &spline1_v);
        calcul_spline(tab_x, tab2_v_y, 2, &spline2_v);

        calcul_spline(tab_x, tab1_b_y, 2, &spline1_b);
        calcul_spline(tab_x, tab2_b_y, 2, &spline2_b);

        calcul_spline(tab_x, tab1_a_y, 2, &spline1_a);
        calcul_spline(tab_x, tab2_a_y, 2, &spline2_a);

        eta_r = (double)(spline1_r.p[0]->a * pow(x, 3) + spline1_r.p[0]->b * pow(x, 2) + spline1_r.p[0]->c * x + spline1_r.p[0]->d);
        psi_r = (double)(spline2_r.p[0]->a * pow(x, 3) + spline2_r.p[0]->b * pow(x, 2) + spline2_r.p[0]->c * x + spline2_r.p[0]->d);

        eta_v = (double)(spline1_v.p[0]->a * pow(x, 3) + spline1_v.p[0]->b * pow(x, 2) + spline1_v.p[0]->c * x + spline1_v.p[0]->d);
        psi_v = (double)(spline2_v.p[0]->a * pow(x, 3) + spline2_v.p[0]->b * pow(x, 2) + spline2_v.p[0]->c * x + spline2_v.p[0]->d);

        eta_b = (double)(spline1_b.p[0]->a * pow(x, 3) + spline1_b.p[0]->b * pow(x, 2) + spline1_b.p[0]->c * x + spline1_b.p[0]->d);
        psi_b = (double)(spline2_b.p[0]->a * pow(x, 3) + spline2_b.p[0]->b * pow(x, 2) + spline2_b.p[0]->c * x + spline2_b.p[0]->d);

        eta_a = (double)(spline1_a.p[0]->a * pow(x, 3) + spline1_a.p[0]->b * pow(x, 2) + spline1_a.p[0]->c * x + spline1_a.p[0]->d);
        psi_a = (double)(spline2_a.p[0]->a * pow(x, 3) + spline2_a.p[0]->b * pow(x, 2) + spline2_a.p[0]->c * x + spline2_a.p[0]->d);

        tab_x_vert[0] = floor(y);
        tab_x_vert[1] = floor(y) + 1;
        tab_r_y[0] = eta_r;
        tab_r_y[1] = psi_r;

        tab_x_vert[0] = floor(y);
        tab_x_vert[1] = floor(y) + 1;
        tab_v_y[0] = eta_v;
        tab_v_y[1] = psi_v;

        tab_x_vert[0] = floor(y);
        tab_x_vert[1] = floor(y) + 1;
        tab_b_y[0] = eta_b;
        tab_b_y[1] = psi_b;

        tab_x_vert[0] = floor(y);
        tab_x_vert[1] = floor(y) + 1;
        tab_a_y[0] = eta_a;
        tab_a_y[1] = psi_a;

        calcul_spline(tab_x_vert, tab_r_y, 2, &spline_r_vert);
        calcul_spline(tab_x_vert, tab_v_y, 2, &spline_v_vert);
        calcul_spline(tab_x_vert, tab_b_y, 2, &spline_b_vert);
        calcul_spline(tab_x_vert, tab_a_y, 2, &spline_a_vert);

        r = (unsigned char)(spline_r_vert.p[0]->a * pow(y, 3) + spline_r_vert.p[0]->b * pow(y, 2) + spline_r_vert.p[0]->c * y + spline_r_vert.p[0]->d);
        v = (unsigned char)(spline_v_vert.p[0]->a * pow(y, 3) + spline_v_vert.p[0]->b * pow(y, 2) + spline_v_vert.p[0]->c * y + spline_v_vert.p[0]->d);
        b = (unsigned char)(spline_b_vert.p[0]->a * pow(y, 3) + spline_b_vert.p[0]->b * pow(y, 2) + spline_b_vert.p[0]->c * y + spline_b_vert.p[0]->d);
        a = (unsigned char)(spline_a_vert.p[0]->a * pow(y, 3) + spline_a_vert.p[0]->b * pow(y, 2) + spline_a_vert.p[0]->c * y + spline_a_vert.p[0]->d);

        sized.rouge[i] = r;
        sized.vert[i] = v;
        sized.bleu[i] = b;
        sized.alpha[i] = a;

        colonne++;
        
    }

  
    return sized;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Paramètres incorrects, réessayez !\n");
        return EXIT_FAILURE;
    }
    const char *in = argv[1];
    const char *out = argv[2];
    if (in == NULL)
    {
        printf("Erreur d'ouverture de fichier !\n");
        return EXIT_FAILURE;
    }
    myimage img;
    init_myimage(&img);
    img = lireImage(in);
    unsigned largeur = img.largeur * 2.5;
    unsigned hauteur = img.hauteur * 2.5;
    img = sized(&img, largeur, hauteur);
    ecrireImage(img, out);
    free(img.rouge);
    free(img.vert);
    free(img.bleu);
    free(img.alpha);
    return EXIT_SUCCESS;
}
