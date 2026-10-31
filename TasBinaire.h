#ifndef TAS_BINAIRE_H
#define TAS_BINAIRE_H

#include <vector>
#include <stdexcept>
#include <utility>

// Max-Heap implemente manuellement dans un tableau (vector).
// Les tests utilisent uniquement des valeurs distinctes.
class TasBinaire {
private:
    std::vector<int> valeurs;

    void remonter(int i) {
        while (i > 0) {
            int parent = (i - 1) / 2;
            if (valeurs[parent] >= valeurs[i]) return;
            std::swap(valeurs[parent], valeurs[i]);
            i = parent;
        }
    }

    void descendre(int i) {
        int n = static_cast<int>(valeurs.size());
        while (true) {
            int gauche = 2 * i + 1;
            int droite = 2 * i + 2;
            int grand = i;

            if (gauche < n && valeurs[gauche] > valeurs[grand])
                grand = gauche;
            if (droite < n && valeurs[droite] > valeurs[grand])
                grand = droite;
            if (grand == i) return;

            std::swap(valeurs[i], valeurs[grand]);
            i = grand;
        }
    }

    // Retire l'element a l'indice i et repare le tas.
    void supprimerIndice(int i) {
        int dernier = static_cast<int>(valeurs.size()) - 1;
        valeurs[i] = valeurs[dernier];
        valeurs.pop_back();

        if (i >= static_cast<int>(valeurs.size())) return;
        if (i > 0 && valeurs[i] > valeurs[(i - 1) / 2])
            remonter(i);
        else
            descendre(i);
    }

public:
    void reserver(int n) { valeurs.reserve(n); }

    void inserer(int valeur) {
        // Les valeurs fournies par les benchmarks sont toutes distinctes.
        valeurs.push_back(valeur);
        remonter(static_cast<int>(valeurs.size()) - 1);
    }

    // Le tas ne peut pas rechercher arbitrairement en O(log n).
    bool rechercher(int valeur) const {
        for (int element : valeurs)
            if (element == valeur) return true;
        return false;
    }

    bool supprimer(int valeur) {
        for (int i = 0; i < static_cast<int>(valeurs.size()); i++) {
            if (valeurs[i] == valeur) {
                supprimerIndice(i);
                return true;
            }
        }
        return false;
    }

    int maximum() const {
        if (valeurs.empty()) throw std::runtime_error("Tas vide");
        return valeurs[0];
    }

    int extraireMaximum() {
        if (valeurs.empty()) throw std::runtime_error("Tas vide");
        int valeur = valeurs[0];
        supprimerIndice(0);
        return valeur;
    }

    int taille() const { return static_cast<int>(valeurs.size()); }
};

#endif
