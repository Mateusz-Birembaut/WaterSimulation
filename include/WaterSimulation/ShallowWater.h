#pragma once

#include <vector>

class ShallowWater{
private:
    //Dimensions de la simulation
    int nx, ny; //nombre de cellules sur chaque axe
    float dx; //l'écart entre les cellules
    float dt; //le pas de temps
    float g; //la gravité

    //Carte des hauteurs
    //h calculé a t + dt/2, stocké au centre des cellules
    std::vector<float> h; //calculé a partir de la divergence des q, représente la hauteur de l'eau uniquement (hauteur total = h + terrain)

    //q calculé a t + dt, stockés sur les parois des cellules !!
    std::vector<float> qx; //q = h * u
    std::vector<float> qy;
    std::vector<float> terrain; // la hauteur du terrain

    //secondaire, temporaire
    std::vector<float> ux; //velocité sur l'axe x calculé a partir de qx
    std::vector<float> uy; //velocité sur l'axe y

    //stabilité
    float dryEps; //valeur de h a partir de laquelle une cellule est considéré comme sec
    float CFLCoef; //stabilité CFL

    void computeVelocities(); //calcul de u et v a partir de q et h
    void computeVelocitiesDelta(); // calcul des du/dt
    void updateFluxes(); //mise a jour des q 
    void updateWaterHeight(); //calcul des h a partir de la divergence des q

    void applyBarrier(); //gère les quantités en bordure de la simulation (paroies réflechissantes ou ouvertes)

    void enforceCFL(); //Pour garder la simulation stable il ne faut pas que la velocité soit très grande selon le pas de temps

}