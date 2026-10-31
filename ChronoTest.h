#ifndef CHRONO_TEST_H
#define CHRONO_TEST_H

#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <new>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef __APPLE__
#include <mach/mach.h>
#endif

#include "ArbreBinaireRecherche.h"
#include "TasBinaire.h"
#include "TasPriorityQueue.h"

// Benchmark : ABR manuel, tas vector manuel et std::priority_queue.
class ChronoTest {
private:
    using Horloge = std::chrono::steady_clock;

    // Changer uniquement cette limite pour autoriser davantage de temps.
    const double LIMITE_TEMPS_MS = 20000.0;
    const int NB_EXTRACTIONS = 100000;
    const long long TRAVAIL_MAX_RECHERCHE = 600000000LL;

    Horloge::time_point debutProgramme = Horloge::now();

    struct TempsDepasse {};

    struct ResultatsGeneraux {
        double insertion = 0;
        double recherchePresente = 0;
        double rechercheAbsente = 0;
        double suppression = 0;
        int trouves = 0;
        int absents = 0;
        int supprimes = 0;
        int tailleFinale = 0;
        bool ok = false;
    };

    struct ResultatsPriorite {
        double insertion = 0;
        double lecture = 0;
        double extraction = 0;
        long long sommeLecture = 0;
        long long sommeExtraction = 0;
        int tailleFinale = 0;
        bool ok = false;
    };

    double ms(Horloge::time_point debut, Horloge::time_point fin) const {
        return std::chrono::duration<double, std::milli>(fin - debut).count();
    }

    double tempsEcoule() const {
        return ms(debutProgramme, Horloge::now());
    }

    void verifierTemps() const {
        if (tempsEcoule() >= LIMITE_TEMPS_MS)
            throw TempsDepasse{};
    }

    // Lecture de la RAM disponible sur Linux (dont limite Docker/cgroup).
    long long memoireDisponible() const {
        long long disponible = 0;
        std::ifstream meminfo("/proc/meminfo");
        std::string nom, unite;
        long long ko = 0;
        while (meminfo >> nom >> ko >> unite) {
            if (nom == "MemAvailable:") {
                disponible = ko * 1024;
                break;
            }
        }

        std::ifstream limiteF("/sys/fs/cgroup/memory.max");
        std::ifstream utiliseF("/sys/fs/cgroup/memory.current");
        std::string limiteTexte;
        long long utilise = 0;
        if (limiteF >> limiteTexte && utiliseF >> utilise && limiteTexte != "max") {
            try {
                long long libre = std::stoll(limiteTexte) - utilise;
                if (libre < 0) libre = 0;
                if (disponible == 0 || libre < disponible)
                    disponible = libre;
            } catch (const std::exception&) {
                // Le format cgroup peut varier selon la machine.
            }
        }

#ifdef __APPLE__
        // RAM libre et inactive, recuperable sur macOS.
        vm_statistics64_data_t stats;
        mach_msg_type_number_t compteur = HOST_VM_INFO64_COUNT;
        if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                             reinterpret_cast<host_info64_t>(&stats),
                             &compteur) == KERN_SUCCESS) {
            vm_size_t page = 0;
            host_page_size(mach_host_self(), &page);
            disponible = (static_cast<long long>(stats.free_count) +
                          static_cast<long long>(stats.inactive_count)) * page;
        }
#endif
        // Sans information RAM, budget volontairement prudent : 1 Gio.
        return disponible > 0 ? disponible : 1024LL * 1024 * 1024;
    }

    bool assezDeMemoire(long long besoin) const {
        // Maximum 60 % de la RAM actuellement disponible.
        return besoin < static_cast<long long>(memoireDisponible() * 0.60);
    }

    // Estimations prudentes (allocateur de noeuds, capacite du vector, etc.).
    bool memoireDonneesOK(int n) const {
        return assezDeMemoire(4LL * n + 256LL * 1024 * 1024);
    }

    bool memoireStructureOK(int n, bool abr) const {
        return assezDeMemoire((abr ? 48LL : 12LL) * n + 128LL * 1024 * 1024);
    }

    std::vector<int> creerNombres(int n, bool aleatoire) const {
        std::vector<int> nombres;
        nombres.reserve(n);
        for (int i = 0; i < n; i++)
            nombres.push_back(2 * i + 1); // Impairs uniques, sans doublons.

        if (aleatoire) {
            std::mt19937 generateur(42); // Meme ordre a chaque execution.
            std::shuffle(nombres.begin(), nombres.end(), generateur);
        }
        return nombres;
    }

    int nombreQuestions(int n) const {
        if (n == 20000) return 2000; // Meme test trie que dans le TP precedent.
        int q = static_cast<int>(TRAVAIL_MAX_RECHERCHE / n);
        if (q < 5) q = 5;
        if (q > 500) q = 500;
        return q;
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
        const int n = static_cast<int>(nombres.size());
        const int q = static_cast<int>(questions.size());

        auto debut = Horloge::now();
        for (int i = 0; i < n; i++) {
            structure.inserer(nombres[i]);
            if ((i & 65535) == 0) verifierTemps();
        }
        r.insertion = ms(debut, Horloge::now());

        debut = Horloge::now();
        for (int valeur : questions) {
            if (structure.rechercher(valeur)) r.trouves++;
            verifierTemps();
        }
        r.recherchePresente = ms(debut, Horloge::now());

        debut = Horloge::now();
        for (int valeur : questions) {
            if (!structure.rechercher(valeur + 1)) r.absents++;
            verifierTemps();
        }
        r.rechercheAbsente = ms(debut, Horloge::now());

        debut = Horloge::now();
        for (int valeur : questions) {
            if (structure.supprimer(valeur)) r.supprimes++;
            verifierTemps();
        }
        r.suppression = ms(debut, Horloge::now());
        r.tailleFinale = structure.taille();

        r.ok = r.trouves == q && r.absents == q && r.supprimes == q &&
               r.tailleFinale == n - q;
        for (int i = 0; i < 5 && i < q; i++) {
            if (structure.rechercher(questions[i])) r.ok = false;
        }
        return r;
    }

    // std::priority_queue ne propose pas de recherche ni de suppression
    // d'une valeur arbitraire. Seule son insertion est comparable ici.
    struct ResultatsInsertion {
        double insertion = 0;
        int tailleFinale = 0;
        bool ok = false;
    };

    ResultatsInsertion mesurerInsertionPriorityQueue(const std::vector<int>& nombres) const {
        TasPriorityQueue structure;
        ResultatsInsertion r;
        int n = static_cast<int>(nombres.size());
        auto debut = Horloge::now();
        for (int i = 0; i < n; i++) {
            structure.inserer(nombres[i]);
            if ((i & 65535) == 0) verifierTemps();
        }
        r.insertion = ms(debut, Horloge::now());
        r.tailleFinale = structure.taille();
        r.ok = r.tailleFinale == n && structure.maximum() == 2 * n - 1;
        return r;
    }

    template<class Structure>
    ResultatsPriorite mesurerPriorite(const std::vector<int>& nombres, int k) const {
        Structure structure;
        ResultatsPriorite r;
        const int n = static_cast<int>(nombres.size());

        auto debut = Horloge::now();
        for (int i = 0; i < n; i++) {
            structure.inserer(nombres[i]);
            if ((i & 65535) == 0) verifierTemps();
        }
        r.insertion = ms(debut, Horloge::now());

        debut = Horloge::now();
        for (int i = 0; i < k; i++) {
            // La somme empeche d'ignorer les lectures dans le benchmark.
            std::atomic_signal_fence(std::memory_order_seq_cst);
            r.sommeLecture += structure.maximum();
            if ((i & 4095) == 0) verifierTemps();
        }
        r.lecture = ms(debut, Horloge::now());

        debut = Horloge::now();
        for (int i = 0; i < k; i++) {
            r.sommeExtraction += structure.extraireMaximum();
            if ((i & 4095) == 0) verifierTemps();
        }
        r.extraction = ms(debut, Horloge::now());
        r.tailleFinale = structure.taille();

        const long long maximum = 2LL * n - 1;
        const long long sommeAttendue = 1LL * k * (2LL * n - k);
        r.ok = r.sommeLecture == 1LL * k * maximum &&
               r.sommeExtraction == sommeAttendue &&
               r.tailleFinale == n - k;
        return r;
    }

    template<class Structure>
    bool testLogique() const {
        Structure s;
        for (int x : {50, 30, 70, 20, 40, 60, 80}) s.inserer(x);
        if (s.taille() != 7 || !s.rechercher(40) || s.rechercher(100)) return false;
        if (!s.supprimer(30) || s.rechercher(30)) return false;
        if (s.maximum() != 80 || s.extraireMaximum() != 80) return false;
        if (s.rechercher(80) || s.taille() != 5) return false;
        return true;
    }

    void afficherGeneral(const std::string& nom, const ResultatsGeneraux& r) const {
        std::cout << std::left << std::setw(10) << nom << std::right
                  << std::setw(14) << r.insertion
                  << std::setw(15) << r.recherchePresente
                  << std::setw(15) << r.rechercheAbsente
                  << std::setw(15) << r.suppression
                  << std::setw(16) << r.tailleFinale
                  << std::setw(12) << (r.ok ? "OK" : "ERREUR") << '\n';
    }

    void afficherInsertionPriorityQueue(const ResultatsInsertion& r) const {
        std::cout << std::left << std::setw(10) << "PQUEUE" << std::right
                  << std::setw(14) << r.insertion
                  << std::setw(15) << "N/A"
                  << std::setw(15) << "N/A"
                  << std::setw(15) << "N/A"
                  << std::setw(16) << r.tailleFinale
                  << std::setw(12) << (r.ok ? "OK" : "ERREUR") << '\n';
    }

    void afficherPriorite(const std::string& nom, const ResultatsPriorite& r) const {
        std::cout << std::left << std::setw(10) << nom << std::right
                  << std::setw(14) << r.insertion
                  << std::setw(15) << r.lecture
                  << std::setw(15) << r.extraction
                  << std::setw(19) << r.sommeExtraction
                  << std::setw(16) << r.tailleFinale
                  << std::setw(12) << (r.ok ? "OK" : "ERREUR") << '\n';
    }

    void ligneIgnoree(const std::string& nom, const std::string& raison) const {
        std::cout << "  " << nom << " : NON TESTE (" << raison << ")\n";
    }

    template<class Structure>
    void effectuerGeneral(const std::string& nom, const std::vector<int>& nombres,
                          const std::vector<int>& questions, bool abr) {
        int n = static_cast<int>(nombres.size());
        if (tempsEcoule() >= LIMITE_TEMPS_MS) {
            ligneIgnoree(nom, "limite de 20 secondes atteinte");
            return;
        }
        if (!memoireStructureOK(n, abr)) {
            ligneIgnoree(nom, "RAM insuffisante pour ce volume");
            return;
        }
        try {
            auto resultat = mesurerGeneral<Structure>(nombres, questions);
            afficherGeneral(nom, resultat);
        } catch (const TempsDepasse&) {
            ligneIgnoree(nom, "interrompu par la limite de temps");
        } catch (const std::bad_alloc&) {
            ligneIgnoree(nom, "allocation memoire refusee");
        }
    }

    void effectuerInsertionPriorityQueue(const std::vector<int>& nombres) {
        int n = static_cast<int>(nombres.size());
        if (tempsEcoule() >= LIMITE_TEMPS_MS) {
            ligneIgnoree("PQUEUE", "limite de 20 secondes atteinte");
            return;
        }
        if (!memoireStructureOK(n, false)) {
            ligneIgnoree("PQUEUE", "RAM insuffisante pour ce volume");
            return;
        }
        try {
            afficherInsertionPriorityQueue(mesurerInsertionPriorityQueue(nombres));
        } catch (const TempsDepasse&) {
            ligneIgnoree("PQUEUE", "interrompu par la limite de temps");
        } catch (const std::bad_alloc&) {
            ligneIgnoree("PQUEUE", "allocation memoire refusee");
        }
    }

    template<class Structure>
    void effectuerPriorite(const std::string& nom, const std::vector<int>& nombres,
                           int k, bool abr) {
        int n = static_cast<int>(nombres.size());
        if (tempsEcoule() >= LIMITE_TEMPS_MS) {
            ligneIgnoree(nom, "limite de 20 secondes atteinte");
            return;
        }
        if (!memoireStructureOK(n, abr)) {
            ligneIgnoree(nom, "RAM insuffisante pour ce volume");
            return;
        }
        try {
            auto resultat = mesurerPriorite<Structure>(nombres, k);
            afficherPriorite(nom, resultat);
        } catch (const TempsDepasse&) {
            ligneIgnoree(nom, "interrompu par la limite de temps");
        } catch (const std::bad_alloc&) {
            ligneIgnoree(nom, "allocation memoire refusee");
        }
    }

    void afficherEnteteGeneral(int n, int q, const std::string& titre) const {
        std::cout << "\n========== " << titre << " : N=" << n << " VALEURS ==========\n";
        std::cout << "INSERTION=" << n << " valeurs ; RECHERCHE PRESENTE=" << q
                  << " appels ; RECHERCHE ABSENTE=" << q
                  << " appels ; SUPPRESSION=" << q << " appels.\n";
        std::cout << std::left << std::setw(10) << "Structure" << std::right
                  << std::setw(14) << "Inserer ms"
                  << std::setw(15) << "Present ms"
                  << std::setw(15) << "Absent ms"
                  << std::setw(15) << "Supprimer ms"
                  << std::setw(16) << "Taille finale"
                  << std::setw(12) << "Controle" << '\n';
    }

    void afficherEntetePriorite(int n, int k, const std::string& titre) const {
        std::cout << "\n========== PRIORITE " << titre << " : N=" << n << " VALEURS ==========\n";
        std::cout << "INSERTION=" << n << " valeurs ; LIRE MAXIMUM=" << k
                  << " appels (sans retrait) ; EXTRAIRE MAXIMUM=" << k
                  << " appels (retrait de " << k << " valeurs).\n";
        std::cout << std::left << std::setw(10) << "Structure" << std::right
                  << std::setw(14) << "Inserer ms"
                  << std::setw(15) << "Lire max ms"
                  << std::setw(15) << "Extraire ms"
                  << std::setw(19) << "Somme extraite"
                  << std::setw(16) << "Taille finale"
                  << std::setw(12) << "Controle" << '\n';
    }

    void scenario(int n, bool aleatoire, bool abrAutorise) {
        int q = nombreQuestions(n);
        int k = std::min(NB_EXTRACTIONS, n);
        const std::string titre = aleatoire ? "ALEATOIRE" : "TRIE CROISSANT";

        if (tempsEcoule() >= LIMITE_TEMPS_MS) {
            std::cout << "\nSCENARIO " << titre << " N=" << n
                      << " : NON TESTE (limite de temps atteinte)\n";
            return;
        }
        if (!memoireDonneesOK(n)) {
            std::cout << "\nSCENARIO " << titre << " N=" << n
                      << " : NON TESTE (RAM insuffisante pour les nombres)\n";
            return;
        }

        try {
            std::vector<int> nombres = creerNombres(n, aleatoire);
            if (tempsEcoule() >= LIMITE_TEMPS_MS) {
                std::cout << "\nSCENARIO " << titre << " N=" << n
                          << " : NON TESTE (limite apres preparation)\n";
                return;
            }
            std::vector<int> questions = creerQuestions(nombres, q);

            afficherEnteteGeneral(n, q, titre);
            if (abrAutorise)
                effectuerGeneral<ArbreBinaireRecherche>("ABR", nombres, questions, true);
            else
                ligneIgnoree("ABR", "ordre croissant : cout quadratique");
            effectuerGeneral<TasBinaire>("TAS-VECT", nombres, questions, false);
            effectuerInsertionPriorityQueue(nombres);

            afficherEntetePriorite(n, k, titre);
            if (abrAutorise)
                effectuerPriorite<ArbreBinaireRecherche>("ABR", nombres, k, true);
            else
                ligneIgnoree("ABR", "ordre croissant : cout quadratique");
            effectuerPriorite<TasBinaire>("TAS-VECT", nombres, k, false);
            effectuerPriorite<TasPriorityQueue>("PQUEUE", nombres, k, false);
        } catch (const std::bad_alloc&) {
            std::cout << "SCENARIO " << titre << " N=" << n
                      << " : NON TESTE (memoire indisponible)\n";
        }
    }

public:
    ChronoTest() {
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "BENCHMARK ABR vs TAS-VECT vs PQUEUE -- C++17 -- temps en millisecondes (ms)\n";
        std::cout << "N = nombre de VALEURS inserees. Q = nombre d'APPELS de chaque operation generale.\n";
        std::cout << "K = jusqu'a 100000 APPELS de lecture du max et autant d'extractions (K=min(N,100000)).\n";
        std::cout << "Lire maximum NE supprime rien ; extraire maximum supprime K valeurs.\n";
        std::cout << "Echantillons identiques pour ABR, TAS-VECT et PQUEUE, valeurs impaires uniques.\n";
        std::cout << "Temps hors preparation et destruction.\n";
        std::cout << "SECURITE : 20 s maximum visees et 60 % de RAM libre max par allocation estimee.\n";
        std::cout << "PQUEUE = std::priority_queue ; TAS-VECT = tas vector programme a la main.\n";
        std::cout << "Tests generaux PQUEUE : insertion de N valeurs seulement, taille finale N ; N/A = operation absente.\n";
        std::cout << "NON TESTE = aucune mesure ; jamais une valeur de temps egale a zero.\n";
    }

private:
    bool testLogiquePriorityQueue() const {
        TasPriorityQueue s;
        for (int x : {50, 30, 70, 20, 40, 60, 80}) s.inserer(x);
        if (s.taille() != 7 || s.maximum() != 80) return false;
        if (s.extraireMaximum() != 80 || s.maximum() != 70 || s.taille() != 6)
            return false;
        for (int x : {70, 60, 50, 40, 30, 20}) {
            if (s.extraireMaximum() != x) return false;
        }
        return s.taille() == 0;
    }

public:
    void testerLogique() const {
        bool abr = testLogique<ArbreBinaireRecherche>();
        bool tas = testLogique<TasBinaire>();
        bool pq = testLogiquePriorityQueue();
        std::cout << "\nTESTS LOGIQUES : ABR=" << (abr ? "OK" : "ERREUR")
                  << ", TAS-VECT=" << (tas ? "OK" : "ERREUR")
                  << ", PQUEUE=" << (pq ? "OK" : "ERREUR") << '\n';
        if (!abr || !tas || !pq) throw std::runtime_error("Tests logiques incorrects");
    }

    void testerTailles() {
        int tailles[] = {1000000, 2000000, 10000000, 20000000, 50000000, 100000000};
        // Ce petit test montre le pire cas ABR avant le budget des grands tests.
        scenario(20000, false, true);

        for (int n : tailles)
            scenario(n, true, true);

        // A 1M et au-dela, les tests croissants concernent les deux tas.
        for (int n : tailles)
            scenario(n, false, false);

        std::cout << "\n====== COMMENT LIRE LES TABLEAUX ======\n";
        std::cout << "N = taille initiale ; Q = repetitions de CHAQUE recherche et suppression ; ";
        std::cout << "K = repetitions de CHAQUE consultation et extraction.\n";
        std::cout << "Tests generaux : ABR et TAS-VECT taille finale=N-Q ; PQUEUE taille finale=N (insertion seule).\n";
        std::cout << "Tests priorite : taille finale attendue = N-K.\n";
        std::cout << "Plus le temps (ms) est petit, plus cette operation est rapide.\n";
        std::cout << "ABR aleatoire : O(log N) en moyenne ; ABR trie : O(N) par operation au pire.\n";
        std::cout << "TAS-VECT : insertion/extraction O(log N), lecture max O(1), recherche arbitraire O(N).\n";
        std::cout << "PQUEUE : insertion/extraction O(log N), top O(1), pas de recherche/suppression arbitraire.\n";
        std::cout << "N/A = non disponible dans std::priority_queue (pas un temps mesure).\n";
        std::cout << "FIN DES TESTS : temps total ecoule=" << tempsEcoule() << " ms\n";
    }
};

#endif
