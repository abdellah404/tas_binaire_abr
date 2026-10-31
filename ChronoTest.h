#ifndef CHRONO_TEST_H
#define CHRONO_TEST_H

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "ArbreBinaireRecherche.h"
#include "TasBinaire.h"
#include "TasPriorityQueue.h"

// Tous les temps sont des durees TOTALES, en millisecondes.
class ChronoTest {
private:
    using Horloge = std::chrono::steady_clock;

    struct ResultatsGeneraux {
        double insertion = 0;
        double presente = 0;
        double absente = 0;
        double suppression = 0;
        int tailleFinale = 0;
        bool ok = false;
    };

    struct ResultatsPriorite {
        double insertion = 0;
        double lecture = 0;
        double extraction = 0;
        long long somme = 0;
        int tailleFinale = 0;
        bool ok = false;
    };

    double duree(Horloge::time_point debut) const {
        return std::chrono::duration<double, std::milli>(Horloge::now() - debut).count();
    }

    std::vector<int> creerNombres(int n, bool aleatoire) const {
        std::vector<int> nombres;
        nombres.reserve(n);
        for (int i = 0; i < n; i++)
            nombres.push_back(2 * i + 1);

        if (aleatoire) {
            std::mt19937 generateur(42);
            std::shuffle(nombres.begin(), nombres.end(), generateur);
        }
        return nombres;
    }

    std::vector<int> creerQuestions(const std::vector<int>& nombres, int q) const {
        std::vector<int> questions;
        questions.reserve(q);
        int pas = static_cast<int>(nombres.size()) / q;
        for (int i = 0; i < q; i++)
            questions.push_back(nombres[i * pas]);
        return questions;
    }

    template<class Structure>
    ResultatsGeneraux mesurerGeneral(const std::vector<int>& nombres,
                                     const std::vector<int>& questions) const {
        Structure structure;
        ResultatsGeneraux r;
        int n = static_cast<int>(nombres.size());
        int q = static_cast<int>(questions.size());
        int trouvees = 0, absentes = 0, supprimees = 0;

        auto debut = Horloge::now();
        for (int valeur : nombres) structure.inserer(valeur);
        r.insertion = duree(debut);

        debut = Horloge::now();
        for (int valeur : questions)
            if (structure.rechercher(valeur)) trouvees++;
        r.presente = duree(debut);

        debut = Horloge::now();
        for (int valeur : questions)
            if (!structure.rechercher(valeur + 1)) absentes++;
        r.absente = duree(debut);

        debut = Horloge::now();
        for (int valeur : questions)
            if (structure.supprimer(valeur)) supprimees++;
        r.suppression = duree(debut);

        r.tailleFinale = structure.taille();
        r.ok = (trouvees == q && absentes == q && supprimees == q &&
                r.tailleFinale == n - q);
        return r;
    }

    // priority_queue ne propose pas rechercher() ni supprimer(valeur).
    double mesurerInsertionPriorityQueue(const std::vector<int>& nombres, bool& ok) const {
        TasPriorityQueue structure;
        auto debut = Horloge::now();
        for (int valeur : nombres) structure.inserer(valeur);
        double temps = duree(debut);
        ok = structure.taille() == static_cast<int>(nombres.size()) &&
             structure.maximum() == 2 * static_cast<int>(nombres.size()) - 1;
        return temps;
    }

    template<class Structure>
    ResultatsPriorite mesurerPriorite(const std::vector<int>& nombres, int k) const {
        Structure structure;
        ResultatsPriorite r;
        int n = static_cast<int>(nombres.size());

        auto debut = Horloge::now();
        for (int valeur : nombres) structure.inserer(valeur);
        r.insertion = duree(debut);

        // Lecture du maximum : aucun element n'est retire.
        // La somme est verifiee pour que les resultats soient utilises.
        long long sommeLecture = 0;
        debut = Horloge::now();
        for (int i = 0; i < k; i++) sommeLecture += structure.maximum();
        r.lecture = duree(debut);

        // Extraction = lecture ET suppression du maximum (K fois).
        debut = Horloge::now();
        for (int i = 0; i < k; i++) r.somme += structure.extraireMaximum();
        r.extraction = duree(debut);

        r.tailleFinale = structure.taille();
        long long sommeAttendue = 1LL * k * (2LL * n - k);
        r.ok = (sommeLecture == 1LL * k * (2LL * n - 1) &&
                r.somme == sommeAttendue && r.tailleFinale == n - k);
        return r;
    }

    template<class Structure>
    bool verifierStructure() const {
        Structure structure;
        for (int valeur : {50, 30, 70, 20, 40, 60, 80})
            structure.inserer(valeur);
        if (structure.taille() != 7 || !structure.rechercher(40) ||
            structure.rechercher(100)) return false;
        if (!structure.supprimer(30) || structure.rechercher(30)) return false;
        return structure.maximum() == 80 &&
               structure.extraireMaximum() == 80 && structure.taille() == 5;
    }

    bool verifierPriorityQueue() const {
        TasPriorityQueue structure;
        for (int valeur : {50, 30, 70, 20, 40, 60, 80})
            structure.inserer(valeur);
        if (structure.taille() != 7 || structure.maximum() != 80) return false;
        for (int attendu : {80, 70, 60, 50, 40, 30, 20})
            if (structure.extraireMaximum() != attendu) return false;
        return structure.taille() == 0;
    }

    void afficherGeneral(const std::string& nom, const ResultatsGeneraux& r) const {
        std::cout << std::left << std::setw(12) << nom << std::right
                  << std::setw(15) << r.insertion
                  << std::setw(15) << r.presente
                  << std::setw(15) << r.absente
                  << std::setw(15) << r.suppression
                  << std::setw(15) << r.tailleFinale
                  << std::setw(10) << (r.ok ? "OK" : "ERREUR") << '\n';
        if (!r.ok) throw std::runtime_error("Verification generale echouee : " + nom);
    }

    void afficherPriorite(const std::string& nom, const ResultatsPriorite& r) const {
        std::cout << std::left << std::setw(12) << nom << std::right
                  << std::setw(15) << r.insertion
                  << std::setw(15) << r.lecture
                  << std::setw(15) << r.extraction
                  << std::setw(19) << r.somme
                  << std::setw(15) << r.tailleFinale
                  << std::setw(10) << (r.ok ? "OK" : "ERREUR") << '\n';
        if (!r.ok) throw std::runtime_error("Verification priorite echouee : " + nom);
    }

    void effectuerScenario(int n, int q, bool aleatoire) const {
        int k = std::min(100000, n);
        std::vector<int> nombres = creerNombres(n, aleatoire);
        std::vector<int> questions = creerQuestions(nombres, q);
        std::string type = aleatoire ? "ALEATOIRE" : "TRIE CROISSANT";

        std::cout << "\n====== " << type << " : N=" << n << " VALEURS ======\n";
        std::cout << "Operations generales : N=" << n << " insertions, Q=" << q
                  << " recherches presentes, Q=" << q << " absentes, Q=" << q
                  << " suppressions (total par colonne en ms).\n";
        std::cout << std::left << std::setw(12) << "Structure" << std::right
                  << std::setw(15) << "Insertion ms"
                  << std::setw(15) << "Presente ms"
                  << std::setw(15) << "Absente ms"
                  << std::setw(15) << "Suppr. ms"
                  << std::setw(15) << "Taille finale"
                  << std::setw(10) << "Controle" << '\n';

        afficherGeneral("ABR", mesurerGeneral<ArbreBinaireRecherche>(nombres, questions));
        afficherGeneral("TAS-VECT", mesurerGeneral<TasBinaire>(nombres, questions));
        bool ok = false;
        double insertionPQ = mesurerInsertionPriorityQueue(nombres, ok);
        std::cout << std::left << std::setw(12) << "PQUEUE" << std::right
                  << std::setw(15) << insertionPQ
                  << std::setw(15) << "N/A" << std::setw(15) << "N/A"
                  << std::setw(15) << "N/A" << std::setw(15) << n
                  << std::setw(10) << (ok ? "OK" : "ERREUR") << '\n';
        if (!ok) throw std::runtime_error("Verification insertion PQUEUE echouee");

        std::cout << "\nPriorite : N=" << n << " insertions, K=" << k
                  << " lectures du maximum SANS retrait, puis K=" << k
                  << " extractions (suppressions du maximum).\n";
        std::cout << std::left << std::setw(12) << "Structure" << std::right
                  << std::setw(15) << "Insertion ms"
                  << std::setw(15) << "Lecture ms"
                  << std::setw(15) << "Extraire ms"
                  << std::setw(19) << "Somme extraite"
                  << std::setw(15) << "Taille finale"
                  << std::setw(10) << "Controle" << '\n';

        afficherPriorite("ABR", mesurerPriorite<ArbreBinaireRecherche>(nombres, k));
        afficherPriorite("TAS-VECT", mesurerPriorite<TasBinaire>(nombres, k));
        afficherPriorite("PQUEUE", mesurerPriorite<TasPriorityQueue>(nombres, k));
    }

public:
    ChronoTest() {
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "TP4 - ABR vs TAS-VECT vs std::priority_queue (C++17)\n";
        std::cout << "Temps totaux en millisecondes (ms).\n";
        std::cout << "N = valeurs inserees ; Q = appels de chaque operation generale ; "
                  << "K = appels de lecture/extraction du maximum.\n";
        std::cout << "Meme jeu de valeurs impaires distinctes pour les 3 structures.\n";
        std::cout << "Temps hors preparation des donnees et destruction.\n";
    }

    void testerLogique() const {
        bool abr = verifierStructure<ArbreBinaireRecherche>();
        bool tas = verifierStructure<TasBinaire>();
        bool pq = verifierPriorityQueue();
        std::cout << "Tests logiques : ABR=" << (abr ? "OK" : "ERREUR")
                  << ", TAS-VECT=" << (tas ? "OK" : "ERREUR")
                  << ", PQUEUE=" << (pq ? "OK" : "ERREUR") << '\n';
        if (!abr || !tas || !pq) throw std::runtime_error("Echec des tests logiques");
    }

    void testerTrieCroissant() const {
        effectuerScenario(20000, 2000, false);
    }

    void testerAleatoire(int n, int q) const {
        effectuerScenario(n, q, true);
    }
};

#endif
