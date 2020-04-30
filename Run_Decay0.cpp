#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <gsl/gsl_integration.h>

#include "decay0.h"

//g++ -std=c++11 *.cpp -o Run_Decay0 -lgsl
int main () 
{
    decay0 *_decay0;
    int _Xe136DecayMode; // See method printDecayModeList  Default is 1
    int _Ba136FinalState;
    double _energyThreshold;

    _decay0 = 0;
    _energyThreshold = 0.;

    _Ba136FinalState = 0;
    // Decay0 interface for BB decays ... (BB0nu: DecayMode 1), (BB2nu: DecayMode 4)  
    _Xe136DecayMode = 1;

    std::cout<< "it lives" << std::endl;
    const std::string XeName("Xe136");
    std::cout<< XeName << std::endl;

    _decay0 = new decay0(XeName, _Ba136FinalState, _Xe136DecayMode);

    std::vector<decay0Part> theParts;
    _decay0->decay0DoIt(theParts);

    std::cout << "PDG" << " " << "Energy" << " " << "time" << " " << "Mom_x?" << " " << "Mom_y?" << " " << "Mom_z?" << std::endl;

    for(std::vector<decay0Part>::const_iterator itp = theParts.begin(); itp != theParts.end(); itp++) 
    {
       std::cout << itp->_pdgCode << " " << itp->_energy << " " << itp->_time << " " << itp->_pmom[0] \
       << " " << itp->_pmom[1] << " " << itp->_pmom[2] << std::endl;
    }


    //_decay0->printDecayModeList();

    /* for(int i=1; i<=10; i++)
    {
        _decay0 = new decay0(XeName, _Ba136FinalState, _Xe136DecayMode);
        std::vector<decay0Part> theParts;
        _decay0->decay0DoIt(theParts);

        for(std::vector<decay0Part>::const_iterator itp = theParts.begin(); itp != theParts.end(); itp++) 
        {
        std::cout << itp->_pdgCode << " " << itp->_energy << " " << itp->_time << " " << itp->_pmom[0] \
        << " " << itp->_pmom[1] << " " << itp->_pmom[2] << std::endl;
        }


    } */

    return 0;
}