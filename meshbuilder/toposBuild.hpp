#pragma once
#include "stdafx.hpp"

#include <map>
#include "FixedSizeMeshContainer.hpp"
#include "VariableSizeMeshContainer.hpp"
#include <omp.h>

namespace topos
{
	
    //coords
    template<typename T>
    void build_coord(FixedSizeMeshContainer<T>& C, int Lx, int Ly, int Nx, int Ny){
        vector<T> temp;

        for (int i = 0; i < Ny; i++)
        {
            for (int j = 0; j < Nx; j++)
            {
                temp.push_back(static_cast<T>((Lx / (Nx - 1)) * j));
                temp.push_back(static_cast<T>((Ly / (Ny - 1)) * j));
            }
        }
		C.add(temp);
    }

    int computeMeshFiguresNumberLeft(int figCount1, int figCount2, int skippedElemsCount, int curMeshFigureStructure)//defines how many triangles and squares left in mesh after skipping elements
    {
        int elemsLeft = skippedElemsCount > curMeshFigureStructure ? (figCount1 + figCount2) - ((skippedElemsCount - curMeshFigureStructure) % (figCount1 + figCount2)) : (figCount1 + figCount2) - curMeshFigureStructure;

        //curMeshFigureStructure - left figures to put on mesh till current time 
        //consider curMeshFigureStructure is ALWAYS <= figCount1 + figCount2

        if (skippedElemsCount > curMeshFigureStructure)
        {
            elemsLeft = (figCount1 + figCount2) - ((skippedElemsCount - curMeshFigureStructure) % (figCount1 + figCount2));
        }
        else if (skippedElemsCount == curMeshFigureStructure)
        {
            elemsLeft = (figCount1 + figCount2) - (skippedElemsCount - curMeshFigureStructure);
        }
        else
        {
            elemsLeft = curMeshFigureStructure - skippedElemsCount;
        }

        return elemsLeft;
    }


    void GlobalIndexes(map<int,int>& G2L, vector<int>& global)
    {
        global.clear();

        for(map<int,int>::iterator map_iter = G2L.begin(); map_iter != G2L.end(); ++map_iter)//forming global
        {
            global.push_back(map_iter->first);
        }
    }


    void getLocalIndexes(map<int,int>& G2L, vector<int>& local)
    {
        local.clear();

        for(map<int,int>::iterator map_iter = G2L.begin(); map_iter != G2L.end(); ++map_iter)//forming local
        {
            local.push_back(map_iter->second);
        }
    }

    VariableSizeMeshContainer<int> toLocalIndexesTopoEN(VariableSizeMeshContainer<int>& originEN, map<int,int>& G2L)
    {
        
        vector<int> BlockSize;
        vector<int> temp;
        VariableSizeMeshContainer<int> local(temp,BlockSize);

        for(size_t i = 0; i < originEN.getBlockNumber(); ++i)
        {
            size_t blockSize = originEN.getBlockSize(i);

            for(size_t j = 0; j < blockSize; ++j)
            {
                temp.push_back(G2L[originEN[i][j]]);
            }
            BlockSize.push_back(blockSize);
        }

        local.add(temp,BlockSize);

        return local;
    }


    //topoEN
    //also generates G2L
    VariableSizeMeshContainer<int> build_topoEN(int Nx, int Ny, int k3, int k4, int nE, int beg_i, int end_i, int beg_j, int end_j, map<int,int>& G2L){
        
        vector<int> BlockSize;
        vector<int> temp;
        VariableSizeMeshContainer<int> topoEN(temp, BlockSize);

        if ((beg_i > Nx) || (end_i >= Nx) || (beg_j > Ny) || (end_j >= Ny) || (end_i <= 0) || (end_i <= 0) || (end_i <= 0) || (end_i <= 0))
        {
            cerr << "Wrong submesh parameters!" << endl;
            return topoEN;
        }

        int elementsSkipped = beg_j == 0 ? Nx * beg_j + beg_i : Nx * beg_j + beg_i - 1;//total number of elements is k3 + k4. So two triangles is one element itself

        int meshFigureStructureCur = computeMeshFiguresNumberLeft(k3,k4,elementsSkipped,0);
       
        int local_i = 0;
        int cur_i = beg_i;
        int cur_j = beg_j;
        
        G2L.clear();

        while(cur_j < end_j)
        {
            while(cur_i < end_i)
            {
                G2L.insert(pair<int,int>(Nx * cur_j + cur_i, local_i));

                if (meshFigureStructureCur > k4)//triangle
                {
                    temp.push_back(Nx * cur_j + cur_i);
                    temp.push_back(Nx * cur_j + cur_i + 1);
                    temp.push_back(Nx * (cur_j + 1) + cur_i);

                    BlockSize.push_back(3);

                    temp.push_back(Nx * cur_j + cur_i + 1);
                    temp.push_back(Nx * (cur_j + 1) + cur_i + 1);
                    temp.push_back(Nx * (cur_j + 1) + cur_i);
                    BlockSize.push_back(3);                    
                }
                else if (meshFigureStructureCur <= k4)//square
                {
                    temp.push_back(Nx * cur_j + cur_i);
                    temp.push_back(Nx * cur_j + cur_i + 1);
                    temp.push_back(Nx * (cur_j + 1) + cur_i + 1);
                    temp.push_back(Nx * (cur_j + 1) + cur_i);

                    BlockSize.push_back(4);
                }

                meshFigureStructureCur--; 
                if (meshFigureStructureCur == 0)
                {
                    meshFigureStructureCur = k3 + k4;
                } 

                cur_i++;
                local_i++;
            }

            elementsSkipped = (Nx - (cur_i + 1)) + beg_i;//'+1' as we turn index into number
            meshFigureStructureCur = computeMeshFiguresNumberLeft(k3, k4, elementsSkipped,meshFigureStructureCur);
                
            //changing y coord
            G2L.insert(pair<int,int>(Nx * cur_j + cur_i, local_i));

            local_i++;
            cur_i = beg_i;
            cur_j++;

        }

        for(cur_i = beg_i; cur_i <= end_i; ++cur_i,++local_i)//last y, we haven't visited it yet, but have to put in map
        {
            G2L.insert(pair<int,int>(Nx * end_j + cur_i, local_i));
        }

        topoEN.add(temp, BlockSize);
        return topoEN;
    }



    //topoSN
    VariableSizeMeshContainer<int> build_topoSN(int Nx, int Ny, int k3, int k4){
        vector<int> BlockSize;
        vector<int> temp;
        int k = 0;
        VariableSizeMeshContainer<int> topoSN(temp, BlockSize);    

        for(int i = 0; i < (Nx * Ny); ++i) {
            if (i % Nx != Nx - 1) {
                temp.push_back(i);
                temp.push_back(i + 1);
                BlockSize.push_back(2);
            }

            if (i >= Nx) {
                temp.push_back(i);
                temp.push_back(i - Nx);
                BlockSize.push_back(2);
            }

            if ((i % Nx != Nx - 1) && (i >= Nx)){
                if (k < k3) {
                    temp.push_back(i);
                    temp.push_back(i - Nx + 1);
                    BlockSize.push_back(2);
                }

                ++k;

                if (k == k3 + k4)
                    k = 0;  
            }
        }
		topoSN.add(temp, BlockSize);

        return topoSN;
    } 

    //reversed topology
    template<typename T>
    VariableSizeMeshContainer<T> build_reverse_topo(const VariableSizeMeshContainer<T>& topo)
    {
        vector<int> BlockSize;
        vector<T> temp;
        vector<int> count_i_mas;
        vector<int> count_mass;

        VariableSizeMeshContainer<T> reverse_topo(temp, BlockSize);
        size_t nE = topo.getBlockNumber();
        
        int nN = 0;
        for (size_t i = 0; i < nE; i++){
            for (size_t j = 0; j < topo.getBlockSize(i); j++)
                if (topo[i][j]>nN)
                    nN = topo[i][j];
        }
        nN++;
		
        count_i_mas.reserve(nN);
        count_mass.reserve(nN);
    
		
                
        for (int i = 0; i < nN; i++){
            count_mass.push_back(0);
            count_i_mas.push_back(0);
		}


        for (size_t i = 0; i < nE; i++){
            for (size_t j = 0; j < topo.getBlockSize(i); j++) {
                count_mass[topo[i][j]]++;
                count_i_mas[topo[i][j]]++;
            }
        }
        for (int i = 0; i < nN; i++){
            BlockSize.push_back(count_mass[i]);
            for (int j = 0; j < count_mass[i]; j++)
                temp.push_back(0);
        }
         

        reverse_topo.add(temp,BlockSize);

        for (size_t i = 0; i < nE; i++){
            for (size_t j = 0; j < topo.getBlockSize(i); j++){
                reverse_topo[topo[i][j]][count_mass[topo[i][j]] - count_i_mas[topo[i][j]]] = i;
                count_i_mas[topo[i][j]]--;
            }
        }

        return reverse_topo;
    }

    // topoBSN
    VariableSizeMeshContainer<int> build_topoBSN(int Nx, int Ny){
        vector<int> BlockSize;
        vector<int> temp;
        VariableSizeMeshContainer<int> topoBSN(temp, BlockSize);

        for(int i = 0; i < Nx - 1; ++i) {
            temp.push_back(0);
            temp.push_back(i);
            temp.push_back(i + 1);
            BlockSize.push_back(3);

            topoBSN.add(temp, BlockSize);

            temp.clear();
            BlockSize.clear();
        }

        for(int i = 0; i < Ny - 1; ++i) {
            temp.push_back(1);
            temp.push_back(Nx - 1 + i);
            temp.push_back(Nx - 1 + i + 1);
            BlockSize.push_back(3);
        }

        for(int i = 0; i < Nx - 1; ++i) {
            temp.push_back(2);
            temp.push_back(Nx + Ny - 2 + i);
            temp.push_back(Nx + Ny - 2 + i + 1);
            BlockSize.push_back(3);
        }

        for(int i = 0; i < Ny - 1; ++i) {
            temp.push_back(3);
            temp.push_back(Nx + Nx + Ny - 3 + i);

            if (i + 1 == Ny - 1)
                temp.push_back(0);
            else
                temp.push_back(Nx + Nx + Ny - 3 + i + 1);
            BlockSize.push_back(3);
        }
        topoBSN.add(temp, BlockSize);

        return topoBSN;
    }

    // topoBNS
    template<typename T>
    VariableSizeMeshContainer<T> build_topoBNS(const VariableSizeMeshContainer<T>& topo){
        vector<int> BlockSize;
        vector<T> temp;
        int Nx, Ny;
        VariableSizeMeshContainer<T> topoBNS(temp, BlockSize);       
        for(Nx = 1; topo[Nx - 1][0] == 0; ++Nx) {}

        for(Ny = 1; topo[Nx + Ny - 2][0] == 1; ++Ny) {}


        for(int i = 0; i < (Nx + Ny - 2) * 2; ++i) {
            if (i == 0) 
                temp.push_back(2*(Nx + Ny - 2) - 1);
            else 
                temp.push_back(i - 1);  

            temp.push_back(i);
            BlockSize.push_back(2);           
        }

        topoBNS.add(temp, BlockSize);

        return topoBNS;
    }


    //topoNN
    template<typename T>
    VariableSizeMeshContainer<T> build_topoNN(const VariableSizeMeshContainer<T>& topoSN){
        vector<int> BlockSize;
        vector<T> temp;
        vector<int> count_i_mas;
        vector<int> count_mass;
        int k;

        VariableSizeMeshContainer<T> topoNN(temp, BlockSize);
        int nS = topoSN.getBlockNumber();
        
        int nN = 0;
        for (int i = 0; i < nS; i++){
            for (size_t j = 0; j < topoSN.getBlockSize(i); j++)
                if (topoSN[i][j]>nN)
                    nN = topoSN[i][j];
        }
        nN++;
       
        count_i_mas.reserve(nN);
        count_mass.reserve(nN);

        for (int i = 0; i < nN; i++){
            count_mass.push_back(0);
            count_i_mas.push_back(0);
        }

        for (int i = 0; i < nS; i++){
            for (size_t j = 0; j < topoSN.getBlockSize(i); j++) {
                count_mass[topoSN[i][j]]++;
                count_i_mas[topoSN[i][j]]++;
            }
        }

        
        for (int i = 0; i < nN; i++){
            BlockSize.push_back(count_mass[i]);
            for (int j = 0; j < count_mass[i]; j++)
                temp.push_back(0);
        }
         
        topoNN.add(temp,BlockSize);

        for (int i = 0; i < nS; i++){
            for (size_t j = 0; j < topoSN.getBlockSize(i); j++){
                k = count_mass[topoSN[i][j]] - count_i_mas[topoSN[i][j]];
                topoNN[topoSN[i][j]][k] = j ? topoSN[i][0] : topoSN[i][1];
                count_i_mas[topoSN[i][j]]--;
            }
        }
        

        return topoNN;
    }


    // Mesh decomposing(only decart meshes are supported yet)
    vector<int> decomposeMesh(int Nx, int Ny, int Px, int Py, int px, int py){

        vector<int> coord(4);// ibeg, iend; jbeg, jend indexes

        if ((px >= Px) || (py >= Py) || (px < 0) || (py < 0))
        {
            cerr << "Decompose error: Wrong \'px\' or \'py\' values!" << endl;
            return coord;
        }
        else if ((px >= Px) || (py >= Py))
        {
            cerr << "Decompose error: Part index is more than part number!" << endl;
            return coord;
        }
        else if ((Px > Nx - 1) || (Py > Ny - 1))
        {
            cerr << "Decompose error: Number of parts is more than elements themselves!" << endl;
            return coord;   
        }
        else
        {
            int xPartSize = Nx / Px;
            int yPartSize = Ny / Py;

            int xLeft = Nx % Px;
            int yLeft = Ny % Py;

            if ((px < xLeft) && (py < yLeft))//add left element to coord[x] and coord[y]
            {
                coord[0] = xPartSize * px + px; 
                coord[1] = xPartSize * (px + 1) + px + 1;
                coord[2] = yPartSize * py + py;
                coord[3] = yPartSize * (py + 1) + py + 1;
            }
            
            else if (py < yLeft)//add left element to coord[y]
            {
                coord[0] = xPartSize * px + xLeft; 
                coord[1] = xPartSize * (px + 1) + xLeft;
                coord[2] = yPartSize * py + py;
                coord[3] = yPartSize * (py + 1) + py + 1;
            }
            else if (px < xLeft)//add left element to coord[x]
            {
                coord[0] = xPartSize * px + px; 
                coord[1] = xPartSize * (px + 1) + px + 1;
                coord[2] = yPartSize * py + yLeft;
                coord[3] = yPartSize * (py + 1) + yLeft;
            }
            else//no extra element added
            {
                coord[0] = xPartSize * px + xLeft; 
                coord[1] = xPartSize * (px + 1) + xLeft;
                coord[2] = yPartSize * py + yLeft;
                coord[3] = yPartSize * (py + 1) + yLeft;
            }

            return coord;

        }
    }

    // Mesh decomposing(only decart meshes are supported yet). Get all "ibeg, iend; jbeg, jend" for all indexes [0,Px - 1],[0, Py - 1]
    FixedSizeMeshContainer<int> decomposeMesh(int Nx, int Ny, int Px, int Py){

        FixedSizeMeshContainer<int> coords(4);
        coords.reserve(4 * Px * Py);

        for(int px = 0; px < Px; ++px)
        {
            for(int py = 0; py < Py; ++py)
            {
                coords.add(decomposeMesh(Nx, Ny, Px, Py, px, py));
            }
        }
        coords.printContainer();

        return coords;
    }


}