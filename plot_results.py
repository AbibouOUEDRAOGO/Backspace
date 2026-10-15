"""
Génération des figures pour le rapport Pagerank Backspace
=========================================================
"""

import os
import sys
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec

# ---------------------------------------------------------------
# Configuration matplotlib
# ---------------------------------------------------------------
plt.rcParams.update({
    'font.family':      'serif',
    'font.size':        11,
    'axes.titlesize':   12,
    'axes.labelsize':   11,
    'legend.fontsize':  10,
    'xtick.labelsize':  10,
    'ytick.labelsize':  10,
    'figure.dpi':       150,
    'lines.linewidth':  2.0,
    'lines.markersize': 5,
    'axes.grid':        True,
    'grid.alpha':       0.3,
    'grid.linestyle':   '--',
})

COLOR_GOOGLE    = '#3A56A5'
COLOR_BACKSPACE = '#C0392B'
COLOR_SPEARMAN  = '#2980B9'
COLOR_KENDALL   = '#E67E22'
COLOR_ITER      = '#27AE60'

OUT_DIR = '.'


# ---------------------------------------------------------------
# Chargement des CSV
# ---------------------------------------------------------------
def load_csv(filename):
    if not os.path.exists(filename):
        print(f"[ERREUR] Fichier introuvable : {filename}")
        print("  Lancez d'abord ./backspace")
        sys.exit(1)
    return np.genfromtxt(filename, delimiter=',', names=True)


# ---------------------------------------------------------------
# Figure 1 : Spearman + Kendall vs alpha (côte à côte)
# ---------------------------------------------------------------
def plot_spearman_kendall_alpha(data, save=True):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 4.5))

    ax1.plot(data['alpha'], data['spearman'],
             'o-', color=COLOR_SPEARMAN, label='Spearman ρ')
    ax1.axvline(0.85, color='black', lw=1, ls='--', alpha=0.5, label='α = 0.85')
    ax1.set_xlabel("Facteur d'amortissement α")
    ax1.set_ylabel('ρ (Spearman)')
    ax1.set_title('Corrélation de Spearman\nGoogle vs Backspace')
    ax1.set_xlim(0, 1); ax1.set_ylim(None, 1.05)
    ax1.legend()

    ax2.plot(data['alpha'], data['kendall'],
             's-', color=COLOR_KENDALL, label='Kendall τ')
    ax2.axvline(0.85, color='black', lw=1, ls='--', alpha=0.5, label='α = 0.85')
    ax2.set_xlabel("Facteur d'amortissement α")
    ax2.set_ylabel('τ (Kendall)')
    ax2.set_title('Distance de Kendall\nGoogle vs Backspace')
    ax2.set_xlim(0, 1); ax2.set_ylim(-0.02, None)
    ax2.legend()

    fig.suptitle('Écart entre les rankings Google et Backspace en fonction de α',
                 fontsize=13, fontweight='bold')
    fig.tight_layout()
    if save:
        path = os.path.join(OUT_DIR, 'spearman_kendall_alpha.png')
        fig.savefig(path, bbox_inches='tight')
        print(f"  Sauvegardé : {path}")
    return fig


# ---------------------------------------------------------------
# Figure 2 : Nombre d'itérations vs alpha
# ---------------------------------------------------------------
def plot_iterations_alpha(data, save=True):
    fig, ax = plt.subplots(figsize=(7, 4.5))
    ax.plot(data['alpha'], data['iter_google'],
            'o-', color=COLOR_ITER, label="Itérations (Google)")
    ax.axvline(0.85, color='black', lw=1, ls='--', alpha=0.5, label='α = 0.85')
    ax.set_xlabel("Facteur d'amortissement α")
    ax.set_ylabel("Nombre d'itérations")
    ax.set_title("Nombre d'itérations jusqu'à convergence en fonction de α")
    ax.set_xlim(0, 1)
    ax.legend()
    fig.tight_layout()
    if save:
        path = os.path.join(OUT_DIR, 'iterations_alpha.png')
        fig.savefig(path, bbox_inches='tight')
        print(f"  Sauvegardé : {path}")
    return fig


# ---------------------------------------------------------------
# Figure 3 : Spearman + Kendall vs taux de dangling nodes
# ---------------------------------------------------------------
def plot_metriques_taux(data, save=True):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 4.5))

    x = data['pct_dangling']

    ax1.plot(x, data['spearman'], 'o-', color=COLOR_SPEARMAN, label='Spearman ρ')
    ax1.fill_between(x, data['spearman'], alpha=0.1, color=COLOR_SPEARMAN)
    ax1.set_xlabel('% de pages dangling')
    ax1.set_ylabel('ρ (Spearman)')
    ax1.set_title('Corrélation de Spearman\nvs proportion de dangling nodes')
    ax1.set_ylim(None, 1.05)
    ax1.legend()

    ax2.plot(x, data['kendall'], 's-', color=COLOR_KENDALL, label='Kendall τ')
    ax2.fill_between(x, data['kendall'], alpha=0.1, color=COLOR_KENDALL)
    ax2.set_xlabel('% de pages dangling')
    ax2.set_ylabel('τ (Kendall)')
    ax2.set_title('Distance de Kendall\nvs proportion de dangling nodes')
    ax2.set_ylim(-0.02, None)
    ax2.legend()

    fig.suptitle("Impact du nombre de dangling nodes sur l'écart des rankings (α = 0.85)",
                 fontsize=13, fontweight='bold')
    fig.tight_layout()
    if save:
        path = os.path.join(OUT_DIR, 'metriques_taux.png')
        fig.savefig(path, bbox_inches='tight')
        print(f"  Sauvegardé : {path}")
    return fig


# ---------------------------------------------------------------
# Figure 4 : Courbe de convergence (alpha = 0.85)
# ---------------------------------------------------------------
def plot_convergence(save=True):
    data_g = load_csv('convergence_google_085.csv')
    data_b = load_csv('convergence_backspace_085.csv')

    fig, ax = plt.subplots(figsize=(8, 5))

    ax.semilogy(data_g['iteration'], data_g['diff_L1'],
                'o-', color=COLOR_GOOGLE, markersize=3,
                label='Google (graphe original)')
    ax.semilogy(data_b['iteration'], data_b['diff_L1'],
                's--', color=COLOR_BACKSPACE, markersize=3,
                label='Backspace (graphe étendu)')

    # Pente théorique alpha^k calée sur la première valeur
    k_max = max(len(data_g['iteration']), len(data_b['iteration']))
    k_th  = np.arange(1, k_max + 1)
    y0    = data_g['diff_L1'][0]
    ax.semilogy(k_th, y0 * (0.85 ** k_th), ':',
                color='gray', lw=1.5, label='Pente théorique α^k (α=0.85)')

    ax.set_xlabel('Itération k')
    ax.set_ylabel(r'$\|\mathbf{x}^{(k)} - \mathbf{x}^{(k-1)}\|_1$')
    ax.set_title('Convergence des deux méthodes (α = 0.85)')
    ax.legend()
    fig.tight_layout()
    if save:
        path = os.path.join(OUT_DIR, 'convergence_085.png')
        fig.savefig(path, bbox_inches='tight')
        print(f"  Sauvegardé : {path}")
    return fig


# ---------------------------------------------------------------
# Figure 5 : Dashboard résumé (toutes expériences)
# ---------------------------------------------------------------
def plot_dashboard(data_alpha, data_taux, save=True):
    fig = plt.figure(figsize=(14, 10))
    fig.suptitle(
        'PageRank Backspace vs Google — Résumé des expériences\n'
        'Graphe : EPA.mtx',
        fontsize=14, fontweight='bold'
    )
    gs = gridspec.GridSpec(2, 3, figure=fig, hspace=0.45, wspace=0.38)

    # (0,0) Spearman vs alpha
    ax1 = fig.add_subplot(gs[0, 0])
    ax1.plot(data_alpha['alpha'], data_alpha['spearman'],
             'o-', color=COLOR_SPEARMAN)
    ax1.axvline(0.85, color='black', lw=1, ls='--', alpha=0.4)
    ax1.set_xlabel('α'); ax1.set_ylabel('ρ')
    ax1.set_title('Spearman ρ vs α')
    ax1.set_xlim(0, 1); ax1.set_ylim(None, 1.05)

    # (0,1) Kendall vs alpha
    ax2 = fig.add_subplot(gs[0, 1])
    ax2.plot(data_alpha['alpha'], data_alpha['kendall'],
             's-', color=COLOR_KENDALL)
    ax2.axvline(0.85, color='black', lw=1, ls='--', alpha=0.4)
    ax2.set_xlabel('α'); ax2.set_ylabel('τ')
    ax2.set_title('Kendall τ vs α')
    ax2.set_xlim(0, 1); ax2.set_ylim(-0.02, None)

    # (0,2) Itérations vs alpha
    ax3 = fig.add_subplot(gs[0, 2])
    ax3.plot(data_alpha['alpha'], data_alpha['iter_google'],
             'o-', color=COLOR_ITER)
    ax3.axvline(0.85, color='black', lw=1, ls='--', alpha=0.4)
    ax3.set_xlabel('α'); ax3.set_ylabel('Itérations')
    ax3.set_title('Convergence vs α')
    ax3.set_xlim(0, 1)

    # (1,0) Spearman vs % dangling
    ax4 = fig.add_subplot(gs[1, 0])
    ax4.plot(data_taux['pct_dangling'], data_taux['spearman'],
             'o-', color=COLOR_SPEARMAN)
    ax4.set_xlabel('% dangling'); ax4.set_ylabel('ρ')
    ax4.set_title('Spearman ρ vs dangling\n(α = 0.85)')
    ax4.set_ylim(None, 1.05)

    # (1,1) Kendall vs % dangling
    ax5 = fig.add_subplot(gs[1, 1])
    ax5.plot(data_taux['pct_dangling'], data_taux['kendall'],
             's-', color=COLOR_KENDALL)
    ax5.set_xlabel('% dangling'); ax5.set_ylabel('τ')
    ax5.set_title('Kendall τ vs dangling\n(α = 0.85)')
    ax5.set_ylim(-0.02, None)

    # (1,2) Convergence alpha=0.85
    ax6 = fig.add_subplot(gs[1, 2])
    try:
        dg = load_csv('convergence_google_085.csv')
        db = load_csv('convergence_backspace_085.csv')
        ax6.semilogy(dg['iteration'], dg['diff_L1'],
                     '-', color=COLOR_GOOGLE, label='Google')
        ax6.semilogy(db['iteration'], db['diff_L1'],
                     '--', color=COLOR_BACKSPACE, label='Backspace')
        ax6.set_xlabel('Itération k')
        ax6.set_ylabel('||Δx||₁')
        ax6.set_title('Convergence (α = 0.85)')
        ax6.legend(fontsize=8)
    except SystemExit:
        ax6.text(0.5, 0.5, 'convergence_*.csv\nmanquant',
                 ha='center', va='center', transform=ax6.transAxes, color='gray')

    if save:
        path = os.path.join(OUT_DIR, 'dashboard.png')
        fig.savefig(path, bbox_inches='tight')
        print(f"  Sauvegardé : {path}")
    return fig


# ---------------------------------------------------------------
# Point d'entrée
# ---------------------------------------------------------------
if __name__ == '__main__':
    print("=" * 55)
    print("Génération des figures — Pagerank Backspace")
    print("=" * 55)

    print("\nChargement des données...")
    data_alpha = load_csv('resultats_alpha.csv')
    data_taux  = load_csv('resultats_taux.csv')

    print(f"  Alpha : {len(data_alpha)} points "
          f"({data_alpha['alpha'].min():.2f} → {data_alpha['alpha'].max():.2f})")
    print(f"  Taux  : {len(data_taux)} points")

    print("\nGénération des figures...")
    plot_spearman_kendall_alpha(data_alpha)
    plot_iterations_alpha(data_alpha)
    plot_metriques_taux(data_taux)

    if (os.path.exists('convergence_google_085.csv') and
            os.path.exists('convergence_backspace_085.csv')):
        plot_convergence()
    else:
        print("  [INFO] Fichiers de convergence absents — figure ignorée.")
        print("  Relancez ./backspace pour les générer.")

    plot_dashboard(data_alpha, data_taux)

    print("\nToutes les figures ont été générées.")
    