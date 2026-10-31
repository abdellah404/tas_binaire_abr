#ifndef ARBRE_BINAIRE_RECHERCHE_H
#define ARBRE_BINAIRE_RECHERCHE_H

#include <stdexcept>
#include <vector>

// Arbre binaire de recherche construit manuellement (non equilibre).
class Noeud {
public:
    int valeur;
    Noeud* gauche;
    Noeud* droite;

    Noeud(int v) : valeur(v), gauche(nullptr), droite(nullptr) {}
};

class ArbreBinaireRecherche {
private:
    Noeud* racine;
    int nombreNoeuds;

public:
    ArbreBinaireRecherche() : racine(nullptr), nombreNoeuds(0) {}

    ArbreBinaireRecherche(const ArbreBinaireRecherche&) = delete;
    ArbreBinaireRecherche& operator=(const ArbreBinaireRecherche&) = delete;

    // Insertion d'entiers (sans doublons pour les tests).
    void inserer(int valeur) {
        if (racine == nullptr) {
            racine = new Noeud(valeur);
            nombreNoeuds++;
            return;
        }

        Noeud* courant = racine;
        while (true) {
            if (valeur < courant->valeur) {
                if (courant->gauche == nullptr) {
                    courant->gauche = new Noeud(valeur);
                    nombreNoeuds++;
                    return;
                }
                courant = courant->gauche;
            }
            else if (valeur > courant->valeur) {
                if (courant->droite == nullptr) {
                    courant->droite = new Noeud(valeur);
                    nombreNoeuds++;
                    return;
                }
                courant = courant->droite;
            }
            else {
                return;
            }
        }
    }

    bool rechercher(int valeur) const {
        Noeud* courant = racine;
        while (courant != nullptr) {
            if (valeur == courant->valeur) return true;
            if (valeur < courant->valeur)
                courant = courant->gauche;
            else
                courant = courant->droite;
        }
        return false;
    }

    bool supprimer(int valeur) {
        Noeud* courant = racine;
        Noeud* parent = nullptr;

        while (courant != nullptr && courant->valeur != valeur) {
            parent = courant;
            if (valeur < courant->valeur)
                courant = courant->gauche;
            else
                courant = courant->droite;
        }
        if (courant == nullptr) return false;

        // Deux enfants : remplacer par le maximum du sous-arbre gauche.
        if (courant->gauche != nullptr && courant->droite != nullptr) {
            Noeud* precedent = courant;
            Noeud* maxGauche = courant->gauche;
            while (maxGauche->droite != nullptr) {
                precedent = maxGauche;
                maxGauche = maxGauche->droite;
            }
            courant->valeur = maxGauche->valeur;
            courant = maxGauche;
            parent = precedent;
        }

        // Il reste au plus un enfant.
        Noeud* enfant = courant->gauche;
        if (enfant == nullptr) enfant = courant->droite;

        if (parent == nullptr)
            racine = enfant;
        else if (parent->gauche == courant)
            parent->gauche = enfant;
        else
            parent->droite = enfant;

        delete courant;
        nombreNoeuds--;
        return true;
    }

    int maximum() const {
        if (racine == nullptr) throw std::runtime_error("Arbre vide");
        Noeud* courant = racine;
        while (courant->droite != nullptr) courant = courant->droite;
        return courant->valeur;
    }

    // Retire le plus grand element de l'arbre.
    int extraireMaximum() {
        if (racine == nullptr) throw std::runtime_error("Arbre vide");
        Noeud* courant = racine;
        Noeud* parent = nullptr;
        while (courant->droite != nullptr) {
            parent = courant;
            courant = courant->droite;
        }
        int valeur = courant->valeur;
        if (parent == nullptr)
            racine = courant->gauche;
        else
            parent->droite = courant->gauche;
        delete courant;
        nombreNoeuds--;
        return valeur;
    }

    int taille() const { return nombreNoeuds; }

    ~ArbreBinaireRecherche() {
        // Destruction iterative : pas de depassement de pile sur un arbre long.
        std::vector<Noeud*> pile;
        if (racine != nullptr) pile.push_back(racine);
        while (!pile.empty()) {
            Noeud* courant = pile.back();
            pile.pop_back();
            if (courant->gauche != nullptr) pile.push_back(courant->gauche);
            if (courant->droite != nullptr) pile.push_back(courant->droite);
            delete courant;
        }
    }
};

#endif
