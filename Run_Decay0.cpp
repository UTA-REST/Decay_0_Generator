#include <iostream>
//#include <string>
//#include <vector>
//#include <sstream>
//#include <fstream>
#include <gsl/gsl_integration.h>
#include "decay0.h"
#include "Random.h"

//g++ -std=c++17 Random.cpp decay0.cpp Run_Decay0.cpp -o Run_Decay0 -lgsl

int main () 
{
    // changing the seed for the random numbergenerator 
    constexpr std::uint64_t Seed = 777;
    Random_Set_Seed(Seed);

    // Decay0 interface for BB decays ... (BB0nu: DecayMode 1), (BB2nu: DecayMode 4)  
    int _Xe136DecayMode = 4;
    int _Ba136FinalState = 0;

    decay0 *_decay0;
    _decay0 = 0;

    const std::string XeName("Xe136");

    _decay0 = new decay0(XeName, _Ba136FinalState, _Xe136DecayMode);

    std::vector<decay0Part> theParts;
    _decay0->decay0DoIt(theParts);

    std::cout << "\n" << "PDG" << " " << "Energy" << " " << "time" << " " << "Mom_x?" << " " << "Mom_y?" << " " << "Mom_z?" << std::endl;
    for(std::vector<decay0Part>::const_iterator itp = theParts.begin(); itp != theParts.end(); itp++) 
    {
       std::cout << itp->_pdgCode << " " << itp->_energy << " " << itp->_time << " " << itp->_pmom[0] \
       << " " << itp->_pmom[1] << " " << itp->_pmom[2] << std::endl;
    }

    // this prints the presets in the generator
    // seems kinda useless
    //_decay0->printDecayModeList();


    //used this to make the output file. this thing is kinda slow...
    // took nearly an hour for 100,000
    /* double Total_energy=0.0;
    std::ofstream myfile ("bb2nu_energy.txt");
    for(int i=1; i<=1000000; i++)
    {
        std::cout<< i << std::endl;
        _decay0 = new decay0(XeName, _Ba136FinalState, _Xe136DecayMode);
        std::vector<decay0Part> theParts;
        _decay0->decay0DoIt(theParts);

        for(std::vector<decay0Part>::const_iterator itp = theParts.begin(); itp != theParts.end(); itp++) 
        {
        //std::cout << itp->_pdgCode << " " << itp->_energy << " " << itp->_time << " " << itp->_pmom[0] \
        << " " << itp->_pmom[1] << " " << itp->_pmom[2] << std::endl;
            Total_energy+=itp->_energy;
        }
        myfile << Total_energy;
        myfile << "\n";
        Total_energy = 0.0;
    }
    myfile.close(); */

    return 0;
}