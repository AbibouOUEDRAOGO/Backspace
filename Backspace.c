#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define CAPACITE_INIT   8       // taille initiale de chaque liste de voisins
#define EPSILON         1e-10
#define MAX_ITER        1000
#define NB_ALPHAS       19

// Structure ListeVoisins dynamique
typedef struct {
    int taille;    // nombre de voisins actuels
    int capacite;  // taille du tableau alloué
    int *liste;    // tableau alloué dynamiquement
} ListeVoisins;

// Initialise une liste vide
void lv_init(ListeVoisins *lv)
{
    lv->taille   = 0;
    lv->capacite = CAPACITE_INIT;
    lv->liste    = (int *)malloc(CAPACITE_INIT * sizeof(int));
    if (!lv->liste) { fprintf(stderr, "malloc échoué\n"); exit(1); }
}

// Ajoute un élément agrandit si nécessaire (x2)
void lv_ajouter(ListeVoisins *lv, int val)
{
    if (lv->taille >= lv->capacite)
    {
        lv->capacite *= 2;
        lv->liste = (int *)realloc(lv->liste, lv->capacite * sizeof(int));
        if (!lv->liste) { fprintf(stderr, "realloc échoué\n"); exit(1); }
    }
    lv->liste[lv->taille++] = val;
}

// Copie profonde d'une ListeVoisins
void lv_copier(ListeVoisins *dst, const ListeVoisins *src)
{
    dst->taille   = src->taille;
    dst->capacite = src->capacite;
    dst->liste    = (int *)malloc(src->capacite * sizeof(int));
    if (!dst->liste) { fprintf(stderr, "malloc échoué\n"); exit(1); }
    memcpy(dst->liste, src->liste, src->taille * sizeof(int));
}

// Libère la mémoire d'une liste
void lv_liberer(ListeVoisins *lv)
{
    free(lv->liste);
    lv->liste    = NULL;
    lv->taille   = 0;
    lv->capacite = 0;
}

// Remet une liste à vide sans libérer (pour réutilisation)
void lv_vider(ListeVoisins *lv)
{
    lv->taille = 0;
}

// Retire un élément par valeur (filtre en place)
void lv_retirer(ListeVoisins *lv, int val)
{
    int nouvelle_taille = 0;
    for (int m = 0; m < lv->taille; m++)
        if (lv->liste[m] != val)
            lv->liste[nouvelle_taille++] = lv->liste[m];
    lv->taille = nouvelle_taille;
}

// Variables globales — graphe original
ListeVoisins *voisin_sortant = NULL;
ListeVoisins *voisin_entrant = NULL;
int *d_plus  = NULL;
int *d_moins = NULL;
int N = 0;

// Sauvegarde du graphe original
ListeVoisins *voisin_sortant_sauve = NULL;
ListeVoisins *voisin_entrant_sauve = NULL;
int *d_plus_sauve  = NULL;
int *d_moins_sauve = NULL;

// Graphe transformé (Backspace)
ListeVoisins *voisin_sortant_trans = NULL;
ListeVoisins *voisin_entrant_trans = NULL;
int *d_plus_trans  = NULL;
int *d_moins_trans = NULL;
int *origine       = NULL;
int N_trans = 0;

// Vecteurs PageRank
double *r     = NULL;
double *r_new = NULL;

// Historique de convergence
double historique_diff[MAX_ITER];
int    historique_nb_iter = 0;

// Tableaux globaux pour comparer_rankings
int *diff_abs_global = NULL;
int *pages_global    = NULL;

// Fonction de tri factoriée (qsort)
static double *scores_tri;
int comparateur_scores_desc(const void *a, const void *b)
{
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    if (scores_tri[ib] > scores_tri[ia]) return  1;
    if (scores_tri[ib] < scores_tri[ia]) return -1;
    return 0;
}

int comparateur_diff_abs(const void *a, const void *b)
{
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    if (diff_abs_global[ib] > diff_abs_global[ia]) return  1;
    if (diff_abs_global[ib] < diff_abs_global[ia]) return -1;
    return 0;
}

void calculer_rangs(double *scores, int n, int *rang_tri, int *position)
{
    for (int i = 0; i < n; i++) rang_tri[i] = i;
    scores_tri = scores;
    qsort(rang_tri, n, sizeof(int), comparateur_scores_desc);
    for (int i = 0; i < n; i++)
        position[rang_tri[i]] = i;
}

// ---------------------------------------------------------------
// Allocation / libération des tableaux du graphe original
// ---------------------------------------------------------------
void allouer_graphe(int n)
{
    voisin_sortant = (ListeVoisins *)malloc(n * sizeof(ListeVoisins));
    voisin_entrant = (ListeVoisins *)malloc(n * sizeof(ListeVoisins));
    d_plus         = (int *)calloc(n, sizeof(int));
    d_moins        = (int *)calloc(n, sizeof(int));
    if (!voisin_sortant || !voisin_entrant || !d_plus || !d_moins)
    { fprintf(stderr, "Erreur allocation graphe\n"); exit(1); }
    for (int i = 0; i < n; i++)
    {
        lv_init(&voisin_sortant[i]);
        lv_init(&voisin_entrant[i]);
    }
}

void liberer_graphe_courant()
{
    if (!voisin_sortant) return;
    for (int i = 0; i < N; i++)
    {
        lv_liberer(&voisin_sortant[i]);
        lv_liberer(&voisin_entrant[i]);
    }
    free(voisin_sortant); voisin_sortant = NULL;
    free(voisin_entrant); voisin_entrant = NULL;
    free(d_plus);         d_plus  = NULL;
    free(d_moins);        d_moins = NULL;
}

void liberer_sauvegarde()
{
    if (!voisin_sortant_sauve) return;
    for (int i = 0; i < N; i++)
    {
        lv_liberer(&voisin_sortant_sauve[i]);
        lv_liberer(&voisin_entrant_sauve[i]);
    }
    free(voisin_sortant_sauve); voisin_sortant_sauve = NULL;
    free(voisin_entrant_sauve); voisin_entrant_sauve = NULL;
    free(d_plus_sauve);         d_plus_sauve  = NULL;
    free(d_moins_sauve);        d_moins_sauve = NULL;
}

void liberer_graphe_trans()
{
    if (!voisin_sortant_trans) return;
    for (int i = 0; i < N_trans; i++)
    {
        lv_liberer(&voisin_sortant_trans[i]);
        lv_liberer(&voisin_entrant_trans[i]);
    }
    free(voisin_sortant_trans); voisin_sortant_trans = NULL;
    free(voisin_entrant_trans); voisin_entrant_trans = NULL;
    free(d_plus_trans);         d_plus_trans  = NULL;
    free(d_moins_trans);        d_moins_trans = NULL;
    free(origine);              origine       = NULL;
    N_trans = 0;
}

// ---------------------------------------------------------------
// Lecture du graphe
// Compatible MTX (2 colonnes, base-1) et graph.txt (3 colonnes, base-0)
// ---------------------------------------------------------------
void lire_graphe(const char *nom_fichier)
{
    FILE *f = fopen(nom_fichier, "r");
    if (f == NULL) { printf("Erreur ouverture fichier\n"); exit(1); }

    char ligne[1024];
    while (fgets(ligne, sizeof(ligne), f))
    {
        if (ligne[0] != '%')
        {
            int M, nb_arcs;
            sscanf(ligne, "%d %d %d", &N, &M, &nb_arcs);
            break;
        }
    }

    allouer_graphe(N);

    
    r               = (double *)malloc(N * sizeof(double));
    r_new           = (double *)malloc(N * sizeof(double));
    diff_abs_global = (int *)malloc(N * sizeof(int));
    pages_global    = (int *)malloc(N * sizeof(int));

    char arc_ligne[256];
    int i, j, val;
    while (fgets(arc_ligne, sizeof(arc_ligne), f))
    {
        if (arc_ligne[0] == '%' || arc_ligne[0] == '\n') continue;
        int lu = sscanf(arc_ligne, "%d %d %d", &i, &j, &val);
        if (lu < 2) continue;
        if (lu == 2) { i--; j--; }
        if (i < 0 || j < 0 || i >= N || j >= N) continue;
        lv_ajouter(&voisin_sortant[i], j);
        lv_ajouter(&voisin_entrant[j], i);
        d_plus[i]++;
        d_moins[j]++;
    }
    fclose(f);

    // Diagnostic max de voisins
    int max_s = 0, max_e = 0, page_max_s = 0, page_max_e = 0;
    for (int k = 0; k < N; k++)
    {
        if (voisin_sortant[k].taille > max_s)
        { max_s = voisin_sortant[k].taille; page_max_s = k; }
        if (voisin_entrant[k].taille > max_e)
        { max_e = voisin_entrant[k].taille; page_max_e = k; }
    }
    printf("Max voisins sortants : %d (page %d)\n", max_s, page_max_s);
    printf("Max voisins entrants : %d (page %d)\n", max_e, page_max_e);
}

// Détection des pages sans liens sortants
void detecter_pages_sans_sortie()
{
    int compteur = 0;
    printf("\nPages sans lien sortant :\n");
    for (int i = 0; i < N; i++)
    {
        if (d_plus[i] == 0)
        {
            printf("  Page %d (d_moins = %d)\n", i, d_moins[i]);
            compteur++;
        }
    }
    if (compteur == 0) printf("  Aucune page sans sortie trouvee.\n");
}

// Sauvegarde et restauration du graphe original
void sauvegarder_graphe()
{
    liberer_sauvegarde();
    voisin_sortant_sauve = (ListeVoisins *)malloc(N * sizeof(ListeVoisins));
    voisin_entrant_sauve = (ListeVoisins *)malloc(N * sizeof(ListeVoisins));
    d_plus_sauve         = (int *)malloc(N * sizeof(int));
    d_moins_sauve        = (int *)malloc(N * sizeof(int));
    for (int i = 0; i < N; i++)
    {
        lv_copier(&voisin_sortant_sauve[i], &voisin_sortant[i]);
        lv_copier(&voisin_entrant_sauve[i], &voisin_entrant[i]);
        d_plus_sauve[i]  = d_plus[i];
        d_moins_sauve[i] = d_moins[i];
    }
}

void restaurer_graphe()
{
    for (int i = 0; i < N; i++)
    {
        lv_liberer(&voisin_sortant[i]);
        lv_liberer(&voisin_entrant[i]);
        lv_copier(&voisin_sortant[i], &voisin_sortant_sauve[i]);
        lv_copier(&voisin_entrant[i], &voisin_entrant_sauve[i]);
        d_plus[i]  = d_plus_sauve[i];
        d_moins[i] = d_moins_sauve[i];
    }
}

// ---------------------------------------------------------------
// Destruction aléatoire de liens sortants
// ---------------------------------------------------------------
void detruire_liens_sortants(double taux, int silencieux)
{
    int nb_detruites = 0;
    for (int i = 0; i < N; i++)
    {
        if (d_plus[i] == 0) continue;
        double tirage = (double)rand() / RAND_MAX;
        if (tirage < taux)
        {
            for (int k = 0; k < voisin_sortant[i].taille; k++)
            {
                int jj = voisin_sortant[i].liste[k];
                lv_retirer(&voisin_entrant[jj], i);
                d_moins[jj]--;
            }
            lv_vider(&voisin_sortant[i]);
            d_plus[i] = 0;
            nb_detruites++;
        }
    }
    if (!silencieux)
        printf("  Pages detruites : %d (taux reel : %.1f%%)\n",
               nb_detruites, 100.0 * nb_detruites / N);
}

// ---------------------------------------------------------------
// Transformation Backspace
// ---------------------------------------------------------------
void transformer_backspace(int verbeux)
{
    liberer_graphe_trans(); // on supprime toute ancienne transformation, on repart proprement

    int *correspondance = (int *)malloc(N * sizeof(int));
    for (int i = 0; i < N; i++) correspondance[i] = -1;

    // Première passe : compter pour allouer exactement
    int n_normaux = 0, n_clones = 0;
    for (int i = 0; i < N; i++)
        if (d_plus[i] > 0) n_normaux++;
    for (int p = 0; p < N; p++)
        if (d_plus[p] == 0 && d_moins[p] > 0)
            for (int k = 0; k < voisin_entrant[p].taille; k++)
            {
                int parent = voisin_entrant[p].liste[k];
                if (d_plus[parent] > 0) n_clones++;
            }
    int n_total = n_normaux + n_clones;

    voisin_sortant_trans = (ListeVoisins *)malloc(n_total * sizeof(ListeVoisins));
    voisin_entrant_trans = (ListeVoisins *)malloc(n_total * sizeof(ListeVoisins));
    d_plus_trans         = (int *)calloc(n_total, sizeof(int));
    d_moins_trans        = (int *)calloc(n_total, sizeof(int));
    origine              = (int *)malloc(n_total * sizeof(int));
    for (int i = 0; i < n_total; i++)
    {
        lv_init(&voisin_sortant_trans[i]);
        lv_init(&voisin_entrant_trans[i]);
    }

    // Réallouer r et r_new pour n_total (deux blocs indépendants)
    free(r);
    free(r_new);
    r     = (double *)malloc(n_total * sizeof(double));
    r_new = (double *)malloc(n_total * sizeof(double));

    N_trans = 0;

    // Étape 1 : pages normales
    for (int i = 0; i < N; i++)
    {
        if (d_plus[i] > 0)
        {
            correspondance[i] = N_trans;
            origine[N_trans]  = i;
            N_trans++;
        }
    }

    // Étape 2 : clones des dangling nodes
    for (int p = 0; p < N; p++)
    {
        if (d_plus[p] == 0 && d_moins[p] > 0)
        {
            for (int k = 0; k < voisin_entrant[p].taille; k++)
            {
                int parent = voisin_entrant[p].liste[k];
                if (correspondance[parent] == -1)
                {
                    fprintf(stderr, "AVERTISSEMENT : parent %d de dangling %d est aussi dangling, ignore\n", parent, p);
                    continue;
                }
                int id_copie   = N_trans;
                int idx_parent = correspondance[parent];
                origine[id_copie] = p;
                lv_ajouter(&voisin_sortant_trans[id_copie], idx_parent);   // copie → parent (backspace)
                lv_ajouter(&voisin_sortant_trans[idx_parent], id_copie);   // parent → copie
                N_trans++;
            }
        }
    }

    // Étape 3 : arcs entre pages normales
    for (int i = 0; i < N; i++)
    {
        if (d_plus[i] == 0) continue;
        int idx_i = correspondance[i];
        for (int k = 0; k < voisin_sortant[i].taille; k++)
        {
            int jj = voisin_sortant[i].liste[k];
            if (d_plus[jj] > 0)
                lv_ajouter(&voisin_sortant_trans[idx_i], correspondance[jj]);
        }
    }

    // Étape 4 : recalculer d_plus_trans, d_moins_trans, voisin_entrant_trans
    for (int i = 0; i < N_trans; i++)
        d_plus_trans[i] = voisin_sortant_trans[i].taille;
    for (int i = 0; i < N_trans; i++)
        for (int k = 0; k < voisin_sortant_trans[i].taille; k++)
        {
            int dest = voisin_sortant_trans[i].liste[k];
            lv_ajouter(&voisin_entrant_trans[dest], i);
            d_moins_trans[dest]++;
        }

    free(correspondance);

    if (verbeux)
    {
        printf("\n--- Detail graphe transforme (%d pages) ---\n", N_trans);
        for (int i = 0; i < N_trans; i++)
        {
            printf("  Page %d (origine=%d) -> ", i, origine[i]);
            for (int k = 0; k < voisin_sortant_trans[i].taille; k++)
                printf("%d ", voisin_sortant_trans[i].liste[k]);
            printf("\n");
        }
    }
}

// ---------------------------------------------------------------
// PageRank générique
// ---------------------------------------------------------------
int pagerank_sur(ListeVoisins *ve, int *dp, int n, double alpha, int silencieux)
{
    for (int i = 0; i < n; i++) r[i] = 1.0 / n;
    historique_nb_iter = 0;

    for (int iter = 0; iter < MAX_ITER; iter++)
    {
        double correction = 0.0;
        for (int j = 0; j < n; j++)
            if (dp[j] == 0) correction += r[j] / n;

        for (int i = 0; i < n; i++)
        {
            r_new[i]  = (1.0 - alpha) / n;
            r_new[i] += alpha * correction;
            for (int k = 0; k < ve[i].taille; k++)
            {
                int j = ve[i].liste[k];
                r_new[i] += alpha * r[j] / dp[j];
            }
        }

        double diff = 0.0;
        for (int i = 0; i < n; i++) diff += fabs(r_new[i] - r[i]);
        for (int i = 0; i < n; i++) r[i] = r_new[i];

        historique_diff[iter] = diff;
        historique_nb_iter    = iter + 1;

        if (diff < EPSILON)
        {
            if (!silencieux)
                printf("  Convergence en %d iterations (diff = %e)\n", iter + 1, diff);
            return iter + 1;
        }
    }
    if (!silencieux)
        printf("  Attention : pas de convergence apres %d iterations\n", MAX_ITER);
    return MAX_ITER;
}

// ---------------------------------------------------------------
// Affichage du ranking
// ---------------------------------------------------------------
void afficher_ranking(int n, int *org, int top_n)
{
    int *ordre    = (int *)malloc(n * sizeof(int));
    int *position = (int *)malloc(n * sizeof(int));
    calculer_rangs(r, n, ordre, position);
    int affiche = (top_n < n) ? top_n : n;
    for (int i = 0; i < affiche; i++)
    {
        if (org != NULL)
            printf("  Rang %d : Page %d (origine=%d)  score=%.8f\n",
                   i+1, ordre[i], org[ordre[i]], r[ordre[i]]);
        else
            printf("  Rang %d : Page %d  score=%.8f\n",
                   i+1, ordre[i], r[ordre[i]]);
    }
    free(ordre); free(position);
}

// ---------------------------------------------------------------
// Agrégation des scores
// ---------------------------------------------------------------
void agreger_scores_backspace(double *score_backspace)
{
    for (int i = 0; i < N; i++) score_backspace[i] = 0.0;
    for (int i = 0; i < N_trans; i++)
        score_backspace[origine[i]] += r[i];
}

// ---------------------------------------------------------------
// Métriques de comparaison
// ---------------------------------------------------------------
double calculer_spearman(double *score_google, double *score_backspace)
{
    int *rang_google     = (int *)malloc(N * sizeof(int));
    int *position_google = (int *)malloc(N * sizeof(int));
    int *rang_back       = (int *)malloc(N * sizeof(int));
    int *position_back   = (int *)malloc(N * sizeof(int));
    calculer_rangs(score_google,    N, rang_google,    position_google);
    calculer_rangs(score_backspace, N, rang_back,      position_back);
    double somme_d2 = 0.0;
    for (int p = 0; p < N; p++)
    {
        double d = (double)(position_google[p] - position_back[p]);
        somme_d2 += d * d;
    }
    double rho = 1.0 - (6.0 * somme_d2) / ((double)N * ((double)N * (double)N - 1.0));
    free(rang_google); free(position_google);
    free(rang_back);   free(position_back);
    return rho;
}

double calculer_kendall(double *score_google, double *score_backspace)
{
    int *rang_google     = (int *)malloc(N * sizeof(int));
    int *position_google = (int *)malloc(N * sizeof(int));
    int *rang_back       = (int *)malloc(N * sizeof(int));
    int *position_back   = (int *)malloc(N * sizeof(int));
    calculer_rangs(score_google,    N, rang_google,    position_google);
    calculer_rangs(score_backspace, N, rang_back,      position_back);
    long long discordants = 0;
    for (int i = 0; i < N; i++)
        for (int j = i + 1; j < N; j++)
        {
            int sg = (position_google[i] < position_google[j]) ? 1 : -1;
            int sb = (position_back[i]   < position_back[j])   ? 1 : -1;
            if (sg != sb) discordants++;
        }
    long long total = (long long)N * (N - 1) / 2;
    free(rang_google); free(position_google);
    free(rang_back);   free(position_back);
    return (double)discordants / (double)total;
}

double calculer_kendall_approx(double *score_google, double *score_backspace,
                                int nb_echantillons)
{
    int *rang_google     = (int *)malloc(N * sizeof(int));
    int *position_google = (int *)malloc(N * sizeof(int));
    int *rang_back       = (int *)malloc(N * sizeof(int));
    int *position_back   = (int *)malloc(N * sizeof(int));

    if (!rang_google || !position_google || !rang_back || !position_back)
    {
        fprintf(stderr, "Erreur allocation Kendall approx\n");
        exit(1);
    }

    calculer_rangs(score_google,    N, rang_google,    position_google);
    calculer_rangs(score_backspace, N, rang_back,      position_back);

    long long discordants = 0;
    long long testes = 0;

    for (int s = 0; s < nb_echantillons; s++)
    {
        int i = rand() % N;
        int j = rand() % N;

        if (i == j)
        {
            s--;
            continue;
        }

        int sg = (position_google[i] < position_google[j]) ? 1 : -1;
        int sb = (position_back[i]   < position_back[j])   ? 1 : -1;

        if (sg != sb) discordants++;
        testes++;
    }

    free(rang_google); free(position_google);
    free(rang_back);   free(position_back);

    if (testes == 0) return 0.0;
    return (double)discordants / (double)testes;
}

void comparer_rankings(double *score_google, double *score_backspace, int nb_affiche)
{
    int *rang_google     = (int *)malloc(N * sizeof(int));
    int *position_google = (int *)malloc(N * sizeof(int));
    int *rang_back       = (int *)malloc(N * sizeof(int));
    int *position_back   = (int *)malloc(N * sizeof(int));
    calculer_rangs(score_google,    N, rang_google,    position_google);
    calculer_rangs(score_backspace, N, rang_back,      position_back);
    for (int p = 0; p < N; p++)
    {
        diff_abs_global[p] = position_google[p] - position_back[p];
        if (diff_abs_global[p] < 0) diff_abs_global[p] = -diff_abs_global[p];
        pages_global[p] = p;
    }
    qsort(pages_global, N, sizeof(int), comparateur_diff_abs);
    double rho = calculer_spearman(score_google, score_backspace);
    double tau = calculer_kendall(score_google, score_backspace);
    printf("  Correlation de Spearman : rho = %.6f\n", rho);
    printf("  Distance de Kendall tau : tau = %.6f\n", tau);
    printf("  (Spearman : 1=identiques, 0=aucun lien, -1=inverses)\n");
    printf("  (Kendall  : 0=identiques, 1=totalement inverses)\n\n");
    int top = (nb_affiche < N) ? nb_affiche : N;
    printf("  Page | ScoreGoogle | ScoreBackspace | RangGoogle | RangBack | Difference\n");
    printf("  -----|-------------|----------------|------------|----------|----------\n");
    for (int k = 0; k < top; k++)
    {
        int p    = pages_global[k];
        int diff = position_google[p] - position_back[p];
        printf("  %4d | %11.10f | %14.10f | %10d | %8d | %+d\n",
               p, score_google[p], score_backspace[p],
               position_google[p] + 1, position_back[p] + 1, diff);
    }
    free(rang_google); free(position_google);
    free(rang_back);   free(position_back);
}

// ---------------------------------------------------------------
// Exports CSV
// ---------------------------------------------------------------
void export_csv(const char *nom_fichier, const char *entete,
                double *col1, double *col2, double *col3, double *col4, int n)
{
    remove(nom_fichier);
    FILE *f = fopen(nom_fichier, "w");
    if (f == NULL) { printf("Erreur creation fichier CSV : %s\n", nom_fichier); return; }
    fprintf(f, "%s\n", entete);
    for (int i = 0; i < n; i++)
        fprintf(f, "%.6f,%.8f,%.8f,%.8f\n", col1[i], col2[i], col3[i], col4[i]);
    fclose(f);
    printf("  CSV exporte : %s\n", nom_fichier);
}

void export_convergence_csv(const char *nom_fichier)
{
    remove(nom_fichier);
    FILE *f = fopen(nom_fichier, "w");
    if (f == NULL) { printf("Erreur creation fichier CSV : %s\n", nom_fichier); return; }
    fprintf(f, "iteration,diff_L1\n");
    for (int i = 0; i < historique_nb_iter; i++)
        fprintf(f, "%d,%.12f\n", i + 1, historique_diff[i]);
    fclose(f);
    printf("  CSV exporte : %s\n", nom_fichier);
}

// ---------------------------------------------------------------
// Programme principal
// Usage : ./backspace        → mode résumé  (grands graphes)
//         ./backspace -v     → mode verbeux (graph.txt)
// ---------------------------------------------------------------
int main(int argc, char *argv[])
{
    
    setvbuf(stdout, NULL, _IONBF, 0);

    int verbeux = (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'v');

    srand((unsigned int)time(NULL));

    const char *fichier = verbeux ? "graph.txt" : "California.mtx";
    lire_graphe(fichier);

    printf("========================================\n");
    printf("Graphe : %s\n", fichier);
    printf("Pages  : %d\n", N);
    int nb_sans_sortie_original = 0;
    for (int i = 0; i < N; i++)
        if (d_plus[i] == 0) nb_sans_sortie_original++;
    printf("Pages sans sortie : %d\n", nb_sans_sortie_original);
    int nb_arcs = 0;
    for (int i = 0; i < N; i++) nb_arcs += d_plus[i];
    printf("Arcs : %d\n", nb_arcs);
    printf("========================================\n");

    if (verbeux)
    {
        printf("\nListe des voisins sortants :\n");
        for (int i = 0; i < N; i++)
        {
            printf("  Page %d -> ", i);
            for (int j = 0; j < voisin_sortant[i].taille; j++)
                printf("%d ", voisin_sortant[i].liste[j]);
            printf("\n");
        }
        printf("\nDegres sortants (d_plus) :\n");
        for (int i = 0; i < N; i++)
            printf("  Page %d : %d\n", i, d_plus[i]);
        printf("\nDegres entrants (d_moins) :\n");
        for (int i = 0; i < N; i++)
            printf("  Page %d : %d\n", i, d_moins[i]);
        detecter_pages_sans_sortie();
    }

    sauvegarder_graphe();

    double *score_google    = (double *)malloc(N * sizeof(double));
    double *score_backspace = (double *)malloc(N * sizeof(double));

    int top_affiche = verbeux ? N : 10;
    int top_compare = verbeux ? N : 20;

    // ================================================================
    // PARTIE 1 : Étude de alpha
    // ================================================================
    printf("\n########################################\n");
    printf("PARTIE 1 : Etude de alpha\n");
    printf("(graphe original, taux destruction = 0%%)\n");
    printf("########################################\n");

    restaurer_graphe();
    transformer_backspace(verbeux);
    printf("Graphe transforme : %d pages\n", N_trans);

    double alphas[NB_ALPHAS];
    for (int a = 0; a < NB_ALPHAS; a++)
        alphas[a] = 0.05 + a * 0.05;

    double csv_alpha[NB_ALPHAS], csv_spearman[NB_ALPHAS];
    double csv_kendall[NB_ALPHAS], csv_iter[NB_ALPHAS];

    for (int a = 0; a < NB_ALPHAS; a++)
    {
        double alpha = alphas[a];
        printf("\n========================================\n");
        printf("alpha = %.2f\n", alpha);
        printf("========================================\n");

        printf("\n=== PageRank Google ===\n");
        int iter_google = pagerank_sur(voisin_entrant, d_plus, N, alpha, 0);
        for (int i = 0; i < N; i++) score_google[i] = r[i];
        afficher_ranking(N, NULL, top_affiche);

        if (fabs(alpha - 0.85) < 1e-9)
            export_convergence_csv("convergence_google_085.csv");

        printf("\n=== PageRank Backspace ===\n");
        pagerank_sur(voisin_entrant_trans, d_plus_trans, N_trans, alpha, 0);
        agreger_scores_backspace(score_backspace);

        if (fabs(alpha - 0.85) < 1e-9)
            export_convergence_csv("convergence_backspace_085.csv");

        if (verbeux)
        {
            afficher_ranking(N_trans, origine, N_trans);
            printf("\n  Scores agreges par page originale :\n");
            for (int i = 0; i < N; i++)
                printf("  Page %d : %.8f\n", i, score_backspace[i]);
        }
        else
        {
            int *ordre    = (int *)malloc(N * sizeof(int));
            int *position = (int *)malloc(N * sizeof(int));
            calculer_rangs(score_backspace, N, ordre, position);
            int top = (N < top_affiche) ? N : top_affiche;
            printf("  Top %d (apres agregation) :\n", top);
            for (int i = 0; i < top; i++)
                printf("  Rang %2d : Page %5d  score=%.8f\n",
                       i+1, ordre[i], score_backspace[ordre[i]]);
            free(ordre); free(position);
        }

        printf("\n=== Comparaison");
        if (!verbeux) printf(" (top %d plus grands changements)", top_compare);
        printf(" ===\n");
        comparer_rankings(score_google, score_backspace, top_compare);

        csv_alpha[a]    = alpha;
        csv_spearman[a] = calculer_spearman(score_google, score_backspace);
        csv_kendall[a]  = calculer_kendall(score_google, score_backspace);
        csv_iter[a]     = (double)iter_google;
    }

    export_csv("resultats_alpha.csv",
               "alpha,spearman,kendall,iter_google",
               csv_alpha, csv_spearman, csv_kendall, csv_iter, NB_ALPHAS);

    // ================================================================
    // PARTIE 2 : Étude du taux de destruction
    // ================================================================
    printf("\n########################################\n");
    printf("PARTIE 2 : Impact du taux de destruction\n");
    printf("(alpha fixe a 0.85)\n");
    printf("########################################\n");
    printf("  Taux | Pages sans sortie       | Spearman | Kendall\n");
    printf("  -----|-------------------------|----------|--------\n");

    double taux[] = {0.0, 0.05, 0.10, 0.15, 0.20, 0.25, 0.30, 0.40, 0.50};
    int nb_taux   = 9;

    
    FILE *csv_taux_f = fopen("resultats_taux.csv", "w");
    if (csv_taux_f == NULL)
    {
        printf("Erreur creation fichier CSV : resultats_taux.csv\n");
    }
    else
    {
        fprintf(csv_taux_f, "taux_demande,pct_dangling,spearman,kendall,n_trans\n");
        fflush(csv_taux_f);
    }

    for (int t = 0; t < nb_taux; t++)
    {
        printf("\n[Partie 2] Debut taux %.0f%%\n", taux[t] * 100);
        restaurer_graphe();

        int nb_dangling = 0;
        if (taux[t] == 0.0)
        {
            for (int i = 0; i < N; i++)
                if (d_plus[i] == 0) nb_dangling++;
        }
        else
        {
            printf("  - Destruction des liens...\n");
            detruire_liens_sortants(taux[t], 1);
            for (int i = 0; i < N; i++)
                if (d_plus[i] == 0) nb_dangling++;
        }

        printf("  - Transformation Backspace...\n");
        transformer_backspace(0);
        printf("  - Graphe transforme : %d pages\n", N_trans);

        printf("  - PageRank Google...\n");
        pagerank_sur(voisin_entrant, d_plus, N, 0.85, 1);
        for (int i = 0; i < N; i++) score_google[i] = r[i];

        printf("  - PageRank Backspace...\n");
        pagerank_sur(voisin_entrant_trans, d_plus_trans, N_trans, 0.85, 1);
        agreger_scores_backspace(score_backspace);

        printf("  - Spearman...\n");
        double rho = calculer_spearman(score_google, score_backspace);

       
        printf("  - Kendall approx...\n");
        double tau = calculer_kendall_approx(score_google, score_backspace, 100000);

        printf("  %3.0f%% | %5d / %d (%.1f%%) | %.6f | %.6f\n",
               taux[t] * 100, nb_dangling, N,
               100.0 * nb_dangling / N, rho, tau);

        if (csv_taux_f != NULL)
        {
            fprintf(csv_taux_f, "%.6f,%.8f,%.8f,%.8f,%d\n",
                    taux[t] * 100,
                    100.0 * nb_dangling / N,
                    rho, tau, N_trans);
            fflush(csv_taux_f);
        }
    }

    if (csv_taux_f != NULL)
    {
        fclose(csv_taux_f);
        printf("  CSV exporte : resultats_taux.csv\n");
    }

    // Libération finale
    free(score_google);
    free(score_backspace);
    liberer_graphe_courant();
    liberer_sauvegarde();
    liberer_graphe_trans();
    free(r);
    free(r_new);
    free(diff_abs_global);
    free(pages_global);

    return 0;
}
