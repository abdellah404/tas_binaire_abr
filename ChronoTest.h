#ifndef CHRONO_TEST_H
#define CHRONO_TEST_H

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>
#include <new>

#include "TasMakeHeap.h"
#include "TasBinaire.h"
#include "TasPriorityQueue.h"

// Compare trois MAX-HEAPS sur exactement les memes nombres.
// Seules la construction et la suppression du maximum sont mesurees.
class ChronoTest {
private:
    using Horloge = std::chrono::steady_clock;
    const int K = 100000; // Nombre de maximums effectivement supprimes.

    struct Resultat {
        double construction = 0;
        double extraction = 0;
        int tailleFinale = 0;
        bool correct = false;
    };

    double millisecondes(Horloge::time_point debut) const {
        return std::chrono::duration<double, std::milli>(Horloge::now() - debut).count();
    }

    // Estimation prudente de la memoire disponible (Linux et cgroup).
    long long ramLibre() const {
        long long ram = 0;
        std::ifstream fichier("/proc/meminfo");
        std::string cle, unite;
        long long ko = 0;
        while (fichier >> cle >> ko >> unite) {
            if (cle == "MemAvailable:") { ram = ko * 1024; break; }
        }
        std::ifstream limite("/sys/fs/cgroup/memory.max");
        std::ifstream utilise("/sys/fs/cgroup/memory.current");
        std::string maximum;
        long long consommation = 0;
        if (limite >> maximum && utilise >> consommation && maximum != "max") {
            try {
                long long libre = std::stoll(maximum) - consommation;
                if (libre < 0) libre = 0;
                if (ram == 0 || libre < ram) ram = libre;
            } catch (...) {}
        }
        return ram;
    }

    bool memoireSuffisante(int n) const {
        // 1 vecteur de donnees + 1 tas + allocations temporaires + marge.
        // 2 x N entiers = 8*N octets, marge x2 + 256 Mio.
        long long besoin = 16LL * n + 256LL * 1024 * 1024;
        long long libre = ramLibre();
        // Sur les systemes sans estimation disponible, eviter le gros volume.
        if (libre <= 0) return n <= 10000000;
        return besoin <= libre * 60 / 100;
    }

    std::vector<int> creerNombres(int n, bool aleatoire) const {
        std::vector<int> nombres;
        nombres.reserve(n);
        for (int i = 0; i < n; ++i) nombres.push_back(2 * i + 1);
        if (aleatoire) {
            std::mt19937 generateur(42);
            std::shuffle(nombres.begin(), nombres.end(), generateur);
        }
        return nombres;
    }

    template<class Tas>
    void construire(Tas& tas, const std::vector<int>& nombres) const {
        tas.reserver(static_cast<int>(nombres.size()));
        for (int valeur : nombres) tas.inserer(valeur);
    }

    // std::make_heap : vraie construction en bloc O(N).
    void construire(TasMakeHeap& tas, const std::vector<int>& nombres) const {
        tas.construire(nombres);
    }

    template<class Tas>
    Resultat mesurer(const std::vector<int>& nombres) const {
        Tas tas;
        Resultat r;
        int n = static_cast<int>(nombres.size());
        int k = std::min(K, n);

        auto debut = Horloge::now();
        construire(tas, nombres);
        r.construction = millisecondes(debut);

        // La somme verifie que l'on a supprime les K plus grands impairs.
        long long somme = 0;
        bool decroissant = true;
        int precedent = std::numeric_limits<int>::max();
        debut = Horloge::now();
        for (int i = 0; i < k; ++i) {
            int valeur = tas.extraireMaximum();
            if (valeur > precedent) decroissant = false;
            precedent = valeur;
            somme += valeur;
        }
        r.extraction = millisecondes(debut);

        r.tailleFinale = tas.taille();
        long long attendu = 1LL * k * (2LL * n - k);
        r.correct = decroissant && somme == attendu && r.tailleFinale == n - k;
        return r;
    }

    void afficher(const std::string& nom, const Resultat& r) const {
        std::cout << std::left << std::setw(18) << nom
                  << std::right << std::setw(18) << r.construction
                  << std::setw(20) << r.extraction
                  << std::setw(17) << r.tailleFinale
                  << std::setw(12) << (r.correct ? "OK" : "ERREUR") << '\n';
    }

    void scenario(int n, bool aleatoire) const {
        std::cout << "\nN=" << n << " ; donnees "
                  << (aleatoire ? "ALEATOIRES" : "TRIEES CROISSANTES") << '\n';
        if (!memoireSuffisante(n)) {
            std::cout << "NON TESTE : memoire libre insuffisante pour ce volume.\n";
            return;
        }
        try {
            const std::vector<int> nombres = creerNombres(n, aleatoire);
            // Chaque tas est detruit avant de construire le suivant.
            afficher("std::make_heap", mesurer<TasMakeHeap>(nombres));
            afficher("Tas vector manuel", mesurer<TasBinaire>(nombres));
            afficher("priority_queue", mesurer<TasPriorityQueue>(nombres));
        } catch (const std::bad_alloc&) {
            std::cout << "NON TESTE : memoire insuffisante pendant l'execution.\n";
        }
    }

public:
    void testerLogique() const {
        const std::vector<int> nombres = {7, 1, 13, 3, 11, 5, 9};
        bool ok1 = mesurer<TasMakeHeap>(nombres).correct;
        bool ok2 = mesurer<TasBinaire>(nombres).correct;
        bool ok3 = mesurer<TasPriorityQueue>(nombres).correct;
        std::cout << "Tests logiques : make_heap=" << (ok1 ? "OK" : "ERREUR")
                  << " ; vector=" << (ok2 ? "OK" : "ERREUR")
                  << " ; priority_queue=" << (ok3 ? "OK" : "ERREUR") << '\n';
        if (!ok1 || !ok2 || !ok3) throw std::runtime_error("Echec du test logique");
    }

    void testerGrandesTailles() const {
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "Comparaison de trois tas (C++17, temps en ms)\n";
        std::cout << "N = valeurs construites ; K = 100000 maximums SUPPRIMES par tas.\n";
        std::cout << "Construction = make_heap en bloc ou insertions individuelles.\n";
        std::cout << "Les donnees sont identiques ; les tas sont testes un par un.\n";
        std::cout << "Valeurs impaires distinctes. Preparation et destruction non chronometrees.\n";
        std::cout << std::left << std::setw(18) << "Structure"
                  << std::right << std::setw(18) << "Construction ms"
                  << std::setw(20) << "100k retraits ms"
                  << std::setw(17) << "Taille finale"
                  << std::setw(12) << "Controle" << '\n';
        for (int n : {100000, 500000, 1000000, 10000000}) {
            scenario(n, true);
            scenario(n, false);
        }
    }
};

#endif
