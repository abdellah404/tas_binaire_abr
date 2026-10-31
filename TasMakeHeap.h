#ifndef TAS_MAKE_HEAP_H
#define TAS_MAKE_HEAP_H

#include <algorithm>
#include <stdexcept>
#include <vector>

// Tas maximum construit avec les algorithmes de la STL.
class TasMakeHeap {
private:
    std::vector<int> valeurs;

public:
    // Construction en une seule fois : O(n).
    void construire(const std::vector<int>& nombres) {
        valeurs = nombres;
        std::make_heap(valeurs.begin(), valeurs.end());
    }

    // Insertion individuelle : O(log n).
    void inserer(int valeur) {
        valeurs.push_back(valeur);
        std::push_heap(valeurs.begin(), valeurs.end());
    }

    bool rechercher(int valeur) const {
        return std::find(valeurs.begin(), valeurs.end(), valeur) != valeurs.end();
    }

    // Suppression d'une valeur quelconque, puis reconstruction du tas.
    bool supprimer(int valeur) {
        auto position = std::find(valeurs.begin(), valeurs.end(), valeur);
        if (position == valeurs.end()) return false;
        *position = valeurs.back();
        valeurs.pop_back();
        std::make_heap(valeurs.begin(), valeurs.end());
        return true;
    }

    int maximum() const {
        if (valeurs.empty()) throw std::runtime_error("Tas vide");
        return valeurs.front();
    }

    int extraireMaximum() {
        if (valeurs.empty()) throw std::runtime_error("Tas vide");
        std::pop_heap(valeurs.begin(), valeurs.end());
        int valeur = valeurs.back();
        valeurs.pop_back();
        return valeur;
    }

    int taille() const { return static_cast<int>(valeurs.size()); }
};

#endif
