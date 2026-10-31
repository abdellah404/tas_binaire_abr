#ifndef TAS_PRIORITY_QUEUE_H
#define TAS_PRIORITY_QUEUE_H

#include <queue>
#include <stdexcept>

// Max-Heap de la bibliotheque standard C++.
// priority_queue utilise par defaut un vector comme conteneur interne.
class TasPriorityQueue {
private:
    std::priority_queue<int> valeurs;

public:
    void inserer(int valeur) {
        valeurs.push(valeur);
    }

    int maximum() const {
        if (valeurs.empty()) throw std::runtime_error("Priority queue vide");
        return valeurs.top();
    }

    int extraireMaximum() {
        if (valeurs.empty()) throw std::runtime_error("Priority queue vide");
        int valeur = valeurs.top();
        valeurs.pop();
        return valeur;
    }

    int taille() const {
        return static_cast<int>(valeurs.size());
    }

    // Pas de rechercher(valeur) ni supprimer(valeur) dans std::priority_queue.
};

#endif
