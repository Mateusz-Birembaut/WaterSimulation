#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <Magnum/GL/Texture.h>

class ShallowWater{
private:
    //Dimensions de la simulation
    int nx, ny; //nombre de cellules sur chaque axe
    float dx; //l'écart entre les cellules
    float dt; //le pas de temps
    float gravity = 9.81f; //la gravité

    //Textures des quantités
    //h calculé a t + dt/2, stocké au centre des cellules
    std::vector<float> h; //calculé a partir de la divergence des q, représente la hauteur de l'eau uniquement (hauteur total = h + terrain)

    //q calculé a t + dt, stockés sur les parois des cellules !
    std::vector<float> qx; //q = h * u
    std::vector<float> qy;
    std::vector<float> terrain; // la hauteur du terrain

    //secondaire, temporaire
    std::vector<float> ux; //velocité sur l'axe x calculé a partir de qx
    std::vector<float> uy; //velocité sur l'axe y


    //stabilité
    float dryEps = 1e-3f; //valeur de h a partir de laquelle une cellule est considéré comme sec
    float CFLCoef; //coefficient CFL

    //coeur de la simu
    void computeVelocities(); //calcul de u et v a partir de q et h
    void computeVelocitiesDelta(); // calcul des du/dt
    void updateFluxes(); //mise a jour des q 
    void updateWaterHeight(); //calcul des h a partir de la divergence des q

    void applyBarrier(); //gère les quantités en bordure de la simulation (paroies réflechissantes ou ouvertes)

    void enforceCFL(); //Pour garder la simulation stable il ne faut pas que la velocité soit très grande selon le pas de temps

    //helper func
    float upwinded_h_x(int i, int j,float qxij); // récupère la hauteur sur les faces
    float upwinded_h_y(int i, int j,float qyij);


public:

    ShallowWater(){}

    ShallowWater(size_t nx_, size_t ny_, float dx_, float dt_){
        nx = nx_; ny = ny_; dx = dx_; dt = dt_;

        h.assign( nx * ny, 0.0f);
        terrain.assign(nx*ny, 0.0f);
        
        qx.assign((nx+1) * ny, 0.0f);
        qy.assign(nx*(ny+1), 0.0f);

        ux.assign((nx+1) * ny, 0.0f);
        uy.assign(nx*(ny+1), 0.0f);
        

    }

    void step();


    //helper functions

    inline int idc(int i, int j) const {return j * nx + i;} //retourne l'index pour les grilles centrés (comme la hauteur)
    inline int idsx(int i, int j) const { return j * (nx+1) + i;} //index side x, retourne l'index pour les valeurs de qx et ux
    inline int idsy(int i, int j) const { return j * nx + i;} //index side y, retourne l'index pour les valeurs de qy et uy

    int getnx() const {return nx;}
    int getny() const {return ny;}
    void updateHeightTexture(Magnum::GL::Texture2D* texture) const;

     //initialisation
    void initBump();

    void loadTerrainHeightMap();
};