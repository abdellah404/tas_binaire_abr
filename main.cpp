#include "ChronoTest.h"

int main() {
    ChronoTest test;

    test.testerLogique();
    test.testerTrieCroissant();        // 20 000 valeurs triees
    test.testerAleatoire(1000000, 500); // 1 million de valeurs
    test.testerAleatoire(2000000, 300); // 2 millions de valeurs

    return 0;
}
