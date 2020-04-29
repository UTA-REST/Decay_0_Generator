//
// decay0 implementaiton. 
//
#include <cfloat>
#include <complex>
#include "decay0.h"
#include <G4RandomDirection.hh>
#include <Randomize.hh>
#include <gsl/gsl_integration.h>
#include <gsl/gsl_sf_gamma.h>
#include <gsl/gsl_errno.h>
//
// Global variables used in the function, that are in global space, as they are used in integrations. 
//
int decay0Functions_Zdbb; // the Z of the 
bbFinalState::bbFinalState():
_spin(0),
_key(0),
_levelE(0.), // Ground state, default.. 
_itrans02(0)
{
}
bbFinalState::bbFinalState(int spin, int keyNum, double Elev, int itr2):
_spin(spin),
_key(keyNum),
_levelE(Elev), // Ground state, default.. 
_itrans02(itr2)
{
}

decay0::decay0():
_ready(false),
_emass(0.51099906),
_nuclideName("Xe136"),
_fsNum(0),
_modebb(0),
_modebbOld(0)
{
  _ebb1 = 0.;
  _ebb2 = 4.3; // original code, line 628
  _gwk=0;
  fillInfo();  
}
decay0::decay0(const std::string nuclide, int finalStateNumber, int decayModeNumber, 
                       double eRangeLow, double eRangeHigh):
_ready(false),
_emass(0.51099906),
_nuclideName(nuclide),
_fsNum(finalStateNumber),
_modebb(decayModeNumber),
_modebbOld(decayModeNumber)
{
  _ebb1 = eRangeLow;
  _ebb2 = eRangeHigh; // for mode 4, 2nbbdecay. 
  _gwk=0;
  // Screwy stuff: The original author reorganized his decay mode table:
  // line 617 
  if (decayModeNumber == 6) _modebb = 14;
  if (decayModeNumber == 7) _modebb = 6;
  if (decayModeNumber == 8) _modebb = 13;
  if ((decayModeNumber >= 9) &&  (decayModeNumber <= 14)) _modebb = decayModeNumber-2;  
  fillInfo();  
}
decay0::~decay0() {
  if (_gwk == 0) return;
  gsl_integration_workspace_free (_gwk);
}
void decay0::fillInfo() {
   _ready = false; 
   if (_modebb == 18) {
      std::cerr << " Majoron modes not supported for now in this version.. " << std::endl;
      return;
   }
   // gamma, electron position, neutrino
   _dataMasses[0] = 0.;  _dataMasses[1] = _emass; _dataMasses[2] = _emass; _dataMasses[3] = 0.; 
  _bbNucl._name=_nuclideName;
   if (_nuclideName == std::string("Xe136")) { 
//    _bbNucl._Qbb = 2457.83; // in keV
    _bbNucl._Qbb = 2.45783; // in MeV  Let us use consitent units and avoid dividing by 1000., here and there.... 
    _bbNucl._Zdbb = 56.;
    _bbNucl._Adbb = 136.;  
    _bbNucl._EK = 0.;  
    _bbNucl._allFS.push_back(bbFinalState(0, 0, 0., 0 ));
    _bbNucl._allFS.push_back(bbFinalState(2, 1, 0.819, 2));
    _bbNucl._allFS.push_back(bbFinalState(2, 2, 1.551, 2));
    _bbNucl._allFS.push_back(bbFinalState(0, 1, 1.579, 0));
    _bbNucl._allFS.push_back(bbFinalState(2, 3, 2.080, 2));
    _bbNucl._allFS.push_back(bbFinalState(2, 4, 2.129, 2));
    _bbNucl._allFS.push_back(bbFinalState(0, 2, 2.141, 0));
    _bbNucl._allFS.push_back(bbFinalState(2, 5, 2.223, 2));
    _bbNucl._allFS.push_back(bbFinalState(0, 3, 2.315, 0));
    _bbNucl._allFS.push_back(bbFinalState(2, 6, 2.400, 2));
 } else if (_nuclideName == std::string("Te130")) { // Just put one Nucleide more in.. Irrelevant for NEXT..  
                                                    // just test we can put one more. 
    _bbNucl._Qbb = 2.529;
    _bbNucl._Zdbb = 54.;
    _bbNucl._Adbb = 130.;  
    _bbNucl._EK = 0.;  
    _bbNucl._allFS.push_back(bbFinalState(0, 0, 0., 0));
    _bbNucl._allFS.push_back(bbFinalState(2, 1, 0.536, 2));
    _bbNucl._allFS.push_back(bbFinalState(2, 2, 1.122, 2));
    _bbNucl._allFS.push_back(bbFinalState(0, 1, 1.794, 0));
  } else {   
    std::cerr << " decay0::fillInfo, unknow nuclide " << _nuclideName << "  No decay possible " << std::endl;
    return; 
  }
  if (_fsNum >= _bbNucl._allFS.size()) { 
    std::cerr << " decay0::fillInfo, Unrecognized Final state number  " << _fsNum 
               << "  max number of states is " << _bbNucl._allFS.size() << " No decay possible " << std::endl;
    return;       
  } else {
    _fsFinalState=_bbNucl._allFS[_fsNum];
//  checking the consistency of data: (1) energy
    _levelE = _fsFinalState._levelE;
    const double El=_levelE; // in MeV 
//    std::cerr << " Selected energy level of daughter nuclide " << El << std::endl;
    _e0 = 0.;
    if(_bbNucl._Zdbb >= 0. ) _e0 = _bbNucl._Qbb;
    else  _e0 = _bbNucl._Qbb - 4.*_emass;
    if ((_modebb ==  9) || (_modebb == 10)) _e0 = _bbNucl._Qbb - _bbNucl._EK - 2.*_emass;
    if ((_modebb == 11) || (_modebb == 12)) _e0 = _bbNucl._Qbb - 2.* _bbNucl._EK;
    if(_e0 <= El) {
      std::cerr << " decay0::fillInfo, not enough energy for transition to this level: " << std::endl;
      std::cerr << " full energy release and Elevel = " << _e0 << " " << El << std::endl;
      return;
    }
    //  spin of level and mode of decay
    const int m = _modebb; // for shortness of code.. 
    bool isOKSpin = false;
    if (_fsFinalState._itrans02 == 0) {
      if ((m ==  1) || (m ==  2) || (m ==  3) || (m ==  4) || (m ==  5) || 
          (m ==  6) || (m ==  9) || (m == 10) || (m == 11) || (m == 12) || 
          (m == 13) || (m == 14) || (m == 15) || (m == 17) || (m == 18)) isOKSpin = true; 
    } else if (_fsFinalState._itrans02 == 2) {
     if ((m ==  3) || (m ==  7) || (m ==  8) || (m ==  9) || (m == 10) || 
         (m == 11) || (m == 12) || (m == 16)) isOKSpin = true;
    }
    if (!isOKSpin) {
      std::cerr << " decay mode and spin of daughter nucleus level are inconsistent: " 
                << _modebbOld << " vs " << _fsFinalState._spin << std::endl;
      return;
    }
    if ((_bbNucl._Zdbb > 0) && ((m ==  9) || (m == 10) || (m == 11) || (m == 12))) {
      std::cerr << " decay mode " << _modebbOld << " and nuclide " << _nuclideName << " are inconsistent " << std::endl;
      return;
    }  	
  }
  if ((_modebb == 9) || (_modebb == 11) || (_modebb == 12)) { 
    _ready = true;
    return; 
  }
  if (_modebb == 18) {
      std::cerr << " decay mode " << _modebb << " is not yet translated.." << std::endl;
      return;
  }  	
  this->initSpectrum(); 
  if (_fsNum > 1) {
    std::cerr << " decay0::fillInfo High (> 819 keV) excited stats of Ba136 have not yet been thoroughly checked " << std::endl;
  }
  _ready = true;
}
void decay0::printDecayModeList() {
  std::cout << "  mode of bb-decay: " << std::endl; 
  std::cout << "       1. 0nubb(mn)         0+ -> 0+     {2n} " << std::endl;
  std::cout << "       2. 0nubb(rhc-lambda) 0+ -> 0+     {2n} " << std::endl;
  std::cout << "       3. 0nubb(rhc-lambda) 0+ -> 0+, 2+ {N*} " << std::endl;
  std::cout << "       4. 2nubb             0+ -> 0+     {2n} " << std::endl;
  std::cout << "       5. 0nubbM1           0+ -> 0+     {2n} " << std::endl;
  std::cout << "       6. 0nubbM2           0+ -> 0+     (2n} " << std::endl;
  std::cout << "       7. 0nubbM3           0+ -> 0+     {2n} " << std::endl;
  std::cout << "       8. 0nubbM7           0+ -> 0+     {2n} " << std::endl;
  std::cout << "       9. 0nubb(rhc-lambda) 0+ -> 2+     {2n} " << std::endl;
  std::cout << "      10. 2nubb             0+ -> 2+     {2n}, {N*} " << std::endl;
  std::cout << "      11. 0nuKb+            0+ -> 0+, 2+ " << std::endl;
  std::cout << "      12. 2nuKb+            0+ -> 0+, 2+ " << std::endl;
  std::cout << "      13. 0nu2K             0+ -> 0+, 2+ " << std::endl;
  std::cout << "      14. 2nu2K             0+ -> 0+, 2+ " << std::endl;
  std::cout << "      15. 2nubb             0+ -> 0+ with bosonic neutrinos " << std::endl;
  std::cout << "      16. 2nubb             0+ -> 2+ with bosonic neutrinos " << std::endl;
  std::cout << "      17. 0nubb(rhc-eta)    0+ -> 0+ simplified expression " << std::endl;
  std::cout << "      18. 0nubb(rhc-eta)    0+ -> 0+ with specific NMEs " << std::endl;
  std::cout << "          5-8: Majoron(s) with spectral index SI: " << std::endl;
  std::cout << "             SI=1 - old M of Gelmini-Roncadelli " << std::endl;
  std::cout << "             SI=2 - bulk M of Mohapatra " << std::endl;
  std::cout << "             SI=3 - double M, vector M, charged M " << std::endl;
  std::cout << "             SI=7 " << std::endl;
}
void decay0::initSpectrum() {
  // Correct energy range  for the daughter excited state.. 
  const double Edlevel = _levelE; // already in MeV 
  if  (_bbNucl._Zdbb > 0.) _e0 = _bbNucl._Qbb - Edlevel;
  else                   _e0 = _bbNucl._Qbb - Edlevel - 4.*_emass;
  if ((_modebb == 9) || (_modebb == 10)) _e0 = _bbNucl._Qbb - Edlevel - _bbNucl._EK - 2.*_emass;
  if ((_modebb == 11.) || (_modebb == 12)) _e0 =_bbNucl._Qbb - Edlevel - 2.*_bbNucl._EK;
  std::cerr << "decay0::decay0DoItbb , after edLevel correction e0 "  << _e0 << std::endl;
  // calculate the theoretical energy spectrum of first particle with step
  // of 1 keV and find its maximum. Original code is in GenSubb, intialization 
  std::cout << " decay0::initSpectrum, Wait, please: calculation of theoretical spectrum. e0 is  " << _e0  <<  std::endl;
  if(_ebb1 < 0.) _ebb1 = 0.;
  if(_ebb2 > _e0) _ebb2 =_e0;
  _spmax = -1.;
  const double relerr = 1.0e-4;
  const double errAbs = 0.;  // large error. We go for the relative error 
  double rerrAchieved = 0.;
  int iiMax = static_cast<int>(_e0*1000.); //kEv, as int if I followed correctly... 
  _spthe1.resize(iiMax);
  _spthe2.resize(iiMax);
  std::vector<double> params(10, 0.); // For integration.. oversized 
  params[0] = _emass; // 
  params[1] = _bbNucl._Zdbb;
  params[2] = _e0; 

  gsl_function fe12_modXX;
  fe12_modXX.params = &params[0];
  
  if ((_modebb == 4) ||  (_modebb == 5) || (_modebb == 6) || (_modebb == 8) || 
      (_modebb == 13) ||  (_modebb == 14) || (_modebb == 15) || (_modebb == 16)) {
     const int numWordsScratch = 10000;
     _gwk = gsl_integration_workspace_alloc(numWordsScratch);
     params[0] = _emass; // 
     params[1] = _bbNucl._Zdbb;
     switch(_modebb) {
	  case 4:     
             fe12_modXX.function = &decay0::fe12_mod4;
	    break;				  
//cc     +      gauss(fe12_mod4,1.e-4,e0-e1+1.e-4,relerr)
//c correction for restricted energy sum (ebb1<=e1+e2<=ebb2)
	  case 5:     
             fe12_modXX.function = &decay0::fe12_mod5;
	     break;				            
	  case 6:     
             fe12_modXX.function = &decay0::fe12_mod6;
	     break;
	  case 8:     
             fe12_modXX.function = &decay0::fe12_mod8;
	     break;
	  case 13:     
             fe12_modXX.function = &decay0::fe12_mod13;
	     break;
	  case 14:     
             fe12_modXX.function = &decay0::fe12_mod14;
	     break;
	  case 15:     
             fe12_modXX.function = &decay0::fe12_mod15;
	     break;
	  case 16:     
             fe12_modXX.function = &decay0::fe12_mod16;
	     break;
	 }
   }
   std::cerr << " Filling _spthe1 ,size " << _spthe1.size() << std::endl;
   for(size_t i=0;  i != _spthe1.size(); i++) { 
     _e1=static_cast<double>(1+i)/1000.;
     params[3] = _e1;
     const double e1h=_e1; 
     _spthe1[i]=0.;
     if (_modebb == 1)  _spthe1[i]=fe1_mod1(e1h, &params[0]);
     if (_modebb == 2)  _spthe1[i]=fe1_mod2(e1h, &params[0]);
     if (_modebb == 3)  _spthe1[i]=fe1_mod3(e1h, &params[0]);
     const double elow = std::max(1.e-4, (_ebb1 - _e1 + 1.e-4));
     const double ehigh = std::max(1.e-4,(_ebb2 - _e1 + 1.e-4));
//	   std::cerr << " e1,elow,ehigh= " <<  _e1 << " / " << elow << " / " << ehigh << std::endl;
//          gsl_integration_qag (const gsl_function * f, double a, double b, double epsabs, 
//              double epsrel, size_t limit, int key, gsl_integration_workspace * workspace, double * result, double * abserr)
     if (_e1 < _e0) { 
       if ((_modebb == 4) ||  (_modebb == 5) || (_modebb == 6) || (_modebb == 8) || 
           (_modebb == 13) ||  (_modebb == 14) || (_modebb == 15) || (_modebb == 16)) {
          int intSuccess = gsl_integration_qag(&fe12_modXX, elow, ehigh, errAbs, relerr, 1000, GSL_INTEG_GAUSS41,
	                                  _gwk, &_spthe1[i],  &rerrAchieved );
	  if (intSuccess != GSL_SUCCESS) {
	    std::cerr << " decay0::initSpectrum, failure to integrate fe12_modxx function GSL status " << intSuccess 
	              << std::endl << ".....decay mode " <<  _modebb << " e1 " << _e1 << std::endl;
		      _ready = false;
		      return; 
	  }
       } // (_e1 < _e0) 
       if(_modebb == 7)  _spthe1[i]=fe1_mod7(e1h, &params[0]);
       if(_modebb == 10) _spthe1[i]=fe1_mod10(e1h, &params[0]);
       if(_modebb == 17)  _spthe1[i]=fe1_mod17(e1h, &params[0]);
//       if(_modebb == 18)  _spthe1[i]=fe1_mod18(e1h, &params[0]); // Not implemented yet..
       _spmax= std::max(_spmax, _spthe1[i]);
// 	  std::cerr << " e1 " << _e1 << "  _spthe1=  " << _spthe1[i] << std::endl;
     }
   }
// Not needed, appropriatly sized    
//	do i=int(e0*1000.)+1,4300
//	   spthe1(i)=0.
//	enddo
   std::ofstream fOut("./th-e1-spectrum.dat"); 
   fOut << " i  s " << std::endl;
   for (size_t i=0; i != _spthe1.size(); i++)  
	   fOut << " " << i << " " << _spthe1[i] << std::endl;
   fOut.close();
	
   _toallevents=1.;
	// Using 
	// http://cernlib.sourcearchive.com/documentation/2006.dfsg.2/rgmlt64_8F_source.html
    if ((_modebb == 4) ||  (_modebb == 5) || (_modebb == 6) || (_modebb == 8) || 
           (_modebb == 13) ||  (_modebb == 14)) {
	   params[3] = 0.; // e1, set in dshelp1
	   params[4] = _modebb;
	   params[5] = 0.; //dens;
	   params[6] = _e0; // densf
	   params[7] = 0.;
	   params[8] = _e0; // Confusing... to be checked..
           gsl_function dshelp_modXX;
           dshelp_modXX.params = &params[0];
	   dshelp_modXX.function = &decay0::dshelp1;
	   double r1 = 0.;
           int intSuccess = gsl_integration_qag(&dshelp_modXX, 0., _e0, 0., relerr, 1000, GSL_INTEG_GAUSS41,
	                                  _gwk, &r1,  &rerrAchieved );
	  if (intSuccess != GSL_SUCCESS) {
	    std::cerr << " decay0::initSpectrum, failure to integrate dshelp_modXX, first, function GSL status " << intSuccess 
	              << std::endl << ".....decay mode " <<  _modebb << " e1 " << _e1 << std::endl;
		      _ready = false;
		      return; 
	   }
//	   std::cerr << " integrate dshelp_modXX, r1 " << r1 << std::endl;
	   params[5] = _ebb1; //dens;
	   params[6] = _ebb2; // denf
	   double r2 = 0.;
           intSuccess = gsl_integration_qag(&dshelp_modXX, 0., _e0, 0., relerr, 1000, GSL_INTEG_GAUSS41,
	                                  _gwk, &r2,  &rerrAchieved );
	  if (intSuccess != GSL_SUCCESS) {
	    std::cerr << " decay0::initSpectrum, failure to integrate dshelp_modXX, second  function GSL status " << intSuccess 
	              << std::endl << ".....decay mode " <<  _modebb << " e1 " << _e1 << std::endl;
		      _ready = false;
		      return; 
	   }
//	   std::cerr << " integrate dshelp_modXX, r2 " << r2 << std::endl;
	   _toallevents=r1/r2;
      } else if (_modebb == 10) {
	  double r1=0.;
	  double r2=0.;
	  const double eLowR1 = 1.0e-4;
	  const double eHighR1 = _e0 + 1.0e-4;
          gsl_function fe1_modG10;
          fe1_modG10.params = &params[0];
	  fe1_modG10.function = &decay0::fe1_mod10;
          int intSuccess = gsl_integration_qag(&fe1_modG10, eLowR1, eHighR1, 0., relerr, 1000, GSL_INTEG_GAUSS41,
	                                  _gwk, &r1,  &rerrAchieved );
	  if (intSuccess != GSL_SUCCESS) {
	    std::cerr << " decay0::initSpectrum, failure to integrate fe1_mod10, first function GSL status " << intSuccess 
	              << std::endl << ".....decay mode " <<  _modebb << " e1 " << _e1 << std::endl;
		      _ready = false;
		      return; 
	   }
	  const double eLowR2 = _ebb1 + 1.0e-4;
	  const double eHighR2 = _ebb2 + 1.0e-4;
          intSuccess = gsl_integration_qag(&fe1_modG10, eLowR2, eHighR2, 0., relerr, 1000, GSL_INTEG_GAUSS41,
	                                  _gwk, &r2,  &rerrAchieved );
	  if (intSuccess != GSL_SUCCESS) {
	    std::cerr << " decay0::initSpectrum, failure to integrate fe1_mod10, first function GSL status " << intSuccess 
	              << std::endl << ".....decay mode " <<  _modebb << " e1 " << _e1 << std::endl;
		      _ready = false;
		      return; 
	   }
	   _toallevents = r1/r2;
     }
     std::cout << " .... starting the generation " << std::endl;
}
// 
// Subroutine GENBBsub generates the events of decay of natural
// radioactive nuclides and various modes of double beta decay.
// GENBB units: energy and moment - MeV and MeV/c; time - sec.
// Energy release in double beta decay - in accordance with
// G.Audi and A.H.Wapstra, Nucl. Phys. A 595(1995)409.
// We call this method (top level, decay0doIt) 
//
// A lot of code is initialization.. done above. 

void decay0::decay0DoIt(std::vector<decay0Part> &outPart) const {
  outPart.clear();
  if (!_ready) return;
  decay0DoItbb(outPart);
//  std::cerr << " After bb, numParticles " << outPart.size() << std::endl;
  //
  // Now add the extra gammas..
  //
  if (_nuclideName == std::string("Xe136")) this->Ba136low(outPart);
//  if (_nuclideName == std::string("Te130")) this->Xe130low(outPart);
} 
  
void decay0::decay0DoItbb(std::vector<decay0Part> &outPart) const {
  // 
  // subroutnie bb starts here.. 
  // Full history available in decay0.for, meanwhile, can't resist copy/pasting.. 
  // 
// ***********************************************************************
//                Don't be afraid of perfectness - you'll never reach it.
//                                                          Salvador Dali
// ***********************************************************************
  const double twopi = 2.0*M_PI;
  std::vector<double> params(10, 0.);
  params[0] = _emass; // 
  params[1] = _bbNucl._Zdbb;
  params[2] = _e0;
  void *p = &params[0];

  if (_modebb == 9) {
//  fixed energies of e+ and X-ray; no angular correlation
    this->timedParticle(outPart, 2, _e0, _e0, 0., M_PI, 0., twopi, 0., 0.);
    this->timedParticle(outPart, 1, _bbNucl._EK, _bbNucl._EK, 0., M_PI ,0., twopi, 0., 0.);
    return;
  }
  if (_modebb == 11) {
// one gamma and two X-rays with fixed energies; no angular correlation
   this->timedParticle(outPart, 1, _e0, _e0, 0., M_PI, 0., 2.0*M_PI, 0., 0.);
   this->timedParticle(outPart, 1, _bbNucl._EK, _bbNucl._EK, 0.,M_PI, 0., twopi, 0., 0.);
   this->timedParticle(outPart, 1, _bbNucl._EK, _bbNucl._EK, 0.,M_PI, 0.,twopi, 0., 0.);
    return;
  }
  if(_modebb == 12) {
// fixed energies of two X-rays; no angular correlation
    this->timedParticle(outPart, 1, _bbNucl._EK, _bbNucl._EK, 0., M_PI, 0., twopi, 0., 0.);
    this->timedParticle(outPart, 1, _bbNucl._EK, _bbNucl._EK, 0., M_PI, 0., twopi, 0., 0.);
    return;
  }
  
// sampling the energies: first e-/e+ Acceptance/rejection method (Von Neumann), as far as I can tell. 
  double e2=0.;
  int numThrow = 0;
//  std::cerr << " ebb1 " << _ebb1  <<  " ebb2 " << _ebb2 << std::endl;
  while(true) {
     if (_modebb != 10) _e1 = _ebb2*G4UniformRand();
     else _e1 = _ebb1 + (_ebb2 - _ebb1)*G4UniformRand();
//     if ((_e0 - _e1) < 0.) continue; //not needed if energy range are set properly. 
     const size_t k = static_cast<size_t>(static_cast<int>(_e1*1000.) - 1);
     if (k >= _spthe1.size()) continue;
     if(_spmax*G4UniformRand() < _spthe1[k]) break;
     numThrow++;
     if (numThrow%1000 == 0) std::cerr << " Thrwing e1 ... " << numThrow << " times ... " << std::endl;
  }
//  second e-/e+ or X-ray
   if    ((_modebb == 1) || (_modebb == 2) || (_modebb == 3 ) ||
          (_modebb == 7) || (_modebb == 17) || (_modebb==18)) {
// modes with no emission of other particles beside of two e-/e+:
//  energy of second e-/e+ is calculated
      e2 =_e0 - _e1;
   } else if ((_modebb == 4) || (_modebb == 5) || (_modebb == 6) ||
            (_modebb == 8) || (_modebb == 13) || (_modebb == 14) ||
            (_modebb == 15) || (_modebb == 16))  {
// something else is emitted - energy of second e-/e+ is random
	double re2s = std::max(0., (_ebb1 - _e1));
	double re2f = _ebb2 - _e1;
	double f2max = -1;
	params[3] = _e1;
	int ke2s = std::max(0, static_cast<int>(re2s*1000.)-1);
	int ke2f= static_cast<int>(re2f*1000.) -1;
	for (int ke2 = ke2s; ke2 != ke2f; ke2++) {
	   e2 = 0.0005 + static_cast<double>(ke2)/1000.; // add 1/2 a bin... 
	   size_t kke2 = static_cast<size_t>(ke2) -1;
	   if (kke2 >= _spthe2.size()) continue; // Should not occur often..
	   switch(_modebb) {
	   case 4 :
	      _spthe2[kke2] = fe2_mod4(e2, p); break;
	   case 5 :   
	      _spthe2[kke2] = fe2_mod5(e2, p); break;
	   case 6 : 
	      _spthe2[kke2] = fe2_mod6(e2, p); break;
	   case  8 : 
	       _spthe2[kke2] = fe2_mod8(e2, p); break;
	   case 13 : 
	       _spthe2[kke2] = fe2_mod13(e2, p); break;
	   case 14 :
	     _spthe2[kke2] = fe2_mod14(e2, p); break;
	   case 15 : 
	     _spthe2[kke2] = fe2_mod15(e2, p); break;
	   case 16 : 
	     _spthe2[kke2] = fe2_mod16(e2, p); break;
	   default:
	     _spthe2[kke2] = 0.;
	   }
	   f2max = std::max(f2max, _spthe2[kke2]);
	}
//	std::cerr << " real f2max= "  << f2max << std::endl;

// c2	   e2=(e0-e1)*rnd1(d)
// sampling the energies: second e-/e+ Acceptance/rejection method (Von Neumann), again  
//
        while(true) {
	 e2 = re2s + (re2f-re2s)*G4UniformRand();
	 double fe2=0.;
	 switch(_modebb) {
	   case 4 :
	     fe2 = fe2_mod4(e2, p); break;
	   case 5 :
	     fe2 = fe2_mod5(e2, p); break;
	   case 6 :
	     fe2 = fe2_mod6(e2, p); break;
	   case 8 :
	     fe2 = fe2_mod8(e2, p); break;
	   case 13 :
	     fe2 = fe2_mod13(e2, p); break;
	   case 14 :
	     fe2 = fe2_mod14(e2, p); break;
	   case 15 :
	     fe2 = fe2_mod15(e2, p); break;
	   case 16 :
	     fe2 = fe2_mod16(e2, p); break;
	   default :
	     fe2 = 0.; break;
	 }
	 if ( f2max*G4UniformRand() < fe2) break;
        }
      } else if( _modebb == 10) {
// energy of X-ray is fixed; no angular correlation
           this->timedParticle(outPart, 2, _e1, _e1, 0., M_PI, 0., twopi, 0., 0.);
           this->timedParticle(outPart, 1, _bbNucl._EK, _bbNucl._EK, 0., M_PI, 0., twopi, 0., 0.);
	   return;
      }
      const double p1 = sqrt(_e1 * (_e1 + 2.*_emass));
      const double p2 = sqrt(e2 * ( e2 + 2.*_emass));
      const double b1 = p1/(_e1 + _emass);
      const double b2 = p2/(e2 + _emass);
//  sampling the angles with angular correlation
      double a = 1.;
      double b = -b1*b2;
      double c = 0.;
      if (_modebb == 2) {
	  b=b1*b2;
      } 
      if(_modebb == 3) {
	   const double w1 = _e1 + _emass;
	   const double w2 = e2 + _emass;
	   a = 3.*(w1*w2 + _emass*_emass)*(p1*p1 + p2*p2);
	   b = -p1*p2*((w1+w2)*(w1+w2) + 4.*(w1*w2+_emass*_emass));
	   c = 2.*p1*p1*p2*p2;
      }
      if(_modebb == 7) {
	   const double w1=_e1 +_emass;
	   const double w2=e2 +_emass;
	   a = 5. * (w1*w2 + _emass*_emass)*(p1*p1 + p2*p2)- p1*p1*p2*p2;
	   b = -p1*p2*(10.*(w1*w2+ _emass*_emass) + p1*p1 + p2*p2);
	   c = 3.*p1*p1*p2*p2;
      }
      if(_modebb == 8) b=b1*b2/3.;
      if(_modebb == 15) {
	   a = 9.*(_e0 - _e1 - e2)*(_e0 - _e1 - e2) + 21.*(e2-_e1)*(e2-_e1);
	   b = -b1*b2*(9.*(_e0 - _e1 - e2)*(_e0 - _e1 - e2) - 7.*(e2-_e1)*(e2-_e1));
      }
      if(_modebb == 16) b = b1*b2/3.;
      if(_modebb == 17) b = b1*b2;
	/*
	 Not yet implemented... 
	if(_modebb == 18) then
	   et0=e0/emass+1. ! total energies in the units of electron mass
	   et1=e1/emass+1.
	   et2=e2/emass+1.
	   a1=(et1*et2-1.)*(et1-et2)**2/(2.*et1*et2)
	   a2=-2.*(et1-et2)**2/(9.*et1*et2)
	   a3=2.*(et1*et2-1.)/(81.*et1*et2)
	   r=3.107526e-3*Adbb**(1./3.)
	   rksi=3./137.036*Zdbb+r*et0
	   a4=8.*(et1*et2+1.)/(r**2*et1*et2)
	   a5=-8.*(rksi*(et1*et2+1.)-2.*r*et0)/(3.*r**2*et1*et2)
	   a6=2.*((rksi**2+4.*r**2)*(et1*et2+1.)-4.*rksi*r*et0)/
     +      (9.*r**2*et1*et2)
	   chi_1plus =chip_GT+3.*chip_F-6.*chip_T
	   chi_1minus=chip_GT-3.*chip_F-6.*chip_T
	   chi_2plus =chi_GTw+chi_Fw-chi_1minus/9.
	   a_eta=a1*chi_2plus**2+a2*chi_2plus*chi_1minus+a3*chi_1minus**2
     +        +a4*chip_R**2+a5*chip_R*chip_P+a6*chip_P**2
	   b_eta=(et1-et2)**2*chi_2plus**2/2.-4.*chi_1minus**2/81.
     +        +8.*(rksi*chip_P/6.-chip_R)**2/r**2-8.*chip_P**2/9.
	   if(a_eta.ne.0.) then
	      b=b_eta/a_eta*b1*b2
	   else
	      print *,'a_eta=0'
	   endif
	endif
	*/
      const double romaxt = a + std::abs(b) + c;
      double phi1=0.; double phi2=0.; double ctet1 = 1.0; double stet1 = 0.0; 
      double ctet2 = 1.0; double stet2 = 0.0; 
      numThrow = 0;
      while(true) {
	  phi1 = twopi * G4UniformRand();
	  ctet1 = 1. - 2.* G4UniformRand();
	  stet1 = std::sqrt(1. - ctet1*ctet1);
	  phi2= twopi * G4UniformRand();
	  ctet2 = 1. - 2.*G4UniformRand();
	  stet2=std::sqrt(1. - ctet2*ctet2);
	  const double ctet = ctet1*ctet2 + stet1*stet2*std::cos(phi1-phi2);
	  if((romaxt*G4UniformRand()) < (a + b*ctet + c*ctet*ctet)) break;
	  numThrow++;
	  if (numThrow%10000 == 0) {
	     std::cerr << " Angular distribution Von Neumann accp/rej numThrow " << numThrow << std::endl;
	     std::cerr << " _e1 " << _e1 << " e2 " << e2 << " a " << a << " b  " << b << " c " << c  << std::endl;
	  }
	  if (numThrow > 1000000) {
	    outPart.clear();
	    return;
	  }
      }
      decay0Part aP;	
      if(_bbNucl._Zdbb > 0.) aP._pdgCode = 11;
      else aP._pdgCode = -11;
      aP._pmom[0] = p1*stet1*std::cos(phi1);
      aP._pmom[1] = p1*stet1*std::sin(phi1);
      aP._pmom[2] = p1*ctet1;
      aP._time = 0.;
      aP._energy = _e1;
      outPart.push_back(aP); // same particle id as above..
      aP._pmom[0] = p2*stet2*std::cos(phi2);
      aP._pmom[1] = p2*stet2*std::sin(phi2);
      aP._pmom[2] = p2*ctet2;
      aP._energy = e2;
      outPart.push_back(aP); // same particle id as above..
    // 
}
void decay0::Ba136low(std::vector<decay0Part> &outPart) const {   // Baryum 136 de-excitation.
// Subroutine describes the deexcitation process in Ba136 nucleus
// after 2b-decay of Xe136 or Ce136 to ground and excited 0+ and 2+ levels
// of Ba136 ("Table of Isotopes", 7th ed., 1978).
// Call  : call Ba136low(levelkeV)
// Input : levelkeV - energy of Ba136 level (integer in keV) occupied
//		    initially; following levels can be occupied:
//		    0+(gs) -	0 keV,
//		    2+(1)  -  819 keV,
//		    2+(2)  - 1551 keV,
//		    0+(1)  - 1579 keV,
//		    2+(3)  - 2080 keV,
//		    2+(4)  - 2129 keV,
//		    0+(2)  - 2141 keV,
//		    2+(5)  - 2223 keV,
//		    0+(3)  - 2315 keV,
//		    2+(6)  - 2400 keV.
// VIT, 28.06.1993, 22.10.1995.
// VIT, 10.12.2008: updated to NDS 95(2002)837 
// (E0 transitions from 2141 and 1579 keV levels are neglected); 
// levels 2080, 2129, 2141, 2223, 2315 and 2400 keV were added.
   const int levelkev = static_cast<int>(1000.*_fsFinalState._levelE + 0.01);
   double tclev=0.;
   double thlev=0.;
   double p = 0.;
   switch (levelkev) {
     case 0: 
        return;
     case 2400:
       	this->nucltransK(outPart, 1.581,0.037,1.0e-3,1.1e-4,tclev,thlev) ;
	thlev=1.93e-12;
	this->nucltransK(outPart, 0.819,0.037,2.9e-3,0.,tclev,thlev); 
	break;
     case 2315:
        this->nucltransK(outPart, 1.497,0.037,8.7e-4,7.7e-5,tclev,thlev) ; 
	thlev=1.93e-12;
	this->nucltransK(outPart, 0.819,0.037,2.9e-3,0.,tclev,thlev) ;
	break;
     case 2223:
        p = 100.0*G4UniformRand();
	if (p <= 4.3) {
	  this->nucltransK(outPart, 2.223,0.037,7.8e-4,4.0e-4,tclev,thlev) ;
	  return;
	} else if (p <= 51.9) { 
	  this->nucltransK(outPart, 1.404,0.037,9.6e-4,4.8e-5,tclev,thlev) ;
	  thlev=1.93e-12;
	  this->nucltransK(outPart, 0.819,0.037,2.9e-3,0.,tclev,thlev) ;
	} else { 
	  this->nucltransK(outPart, 0.672,0.037,6.5e-3,0.,tclev,thlev) ; 
	  thlev=1.01e-12; 
	  p = 100.0*G4UniformRand();
	  if (p < 52.1) this->nucltransK(outPart, 1.551,0.037,8.4e-4,9.7e-5,tclev,thlev) ;
	  else nucltransK(outPart, 0.733,0.037,4.5e-3,0.,tclev,thlev) ;
	}
	break;
     case 2141: 
       this->nucltransK(outPart, 1.323,0.037,1.0e-3,2.6e-5,tclev,thlev) ;
	thlev=1.93e-12;
	this->nucltransK(outPart, 0.819,0.037,2.9e-3,0.,tclev,thlev) ;
	break;
     case 2129:
       thlev=0.051e-12;
       p = 100.0*G4UniformRand();
       if (p < 33.3) this->nucltransK(outPart, 2.129,0.037,7.7e-4,3.6e-4,tclev,thlev) ;
       else {
          this->nucltransK(outPart, 1.310,0.037,1.4e-3,2.3e-5,tclev,thlev) ;
	  thlev=1.93e-12;
	  this->nucltransK(outPart, 0.819,0.037,2.9e-3,0.,tclev,thlev) ;
       }
       break;
     case 2080:
        thlev=0.6e-12;
        p = 100.0*G4UniformRand();
        if (p < 35.4) {
	  this->nucltransK(outPart, 2.080,0.037,7.6e-4,3.3e-4,tclev,thlev) ;
	  return;
	} else if (p < 93.8) {
	  this->nucltransK(outPart, 1.262,0.037,1.3e-3,1.4e-5,tclev,thlev) ;
	  thlev=1.93e-12;
	  this->nucltransK(outPart, 0.819,0.037,2.9e-3,0.,tclev,thlev) ;
	} else {
	  this->nucltransK(outPart, 0.529,0.037,1.0e-2,0.,tclev,thlev) ;
	   thlev=1.01e-12;
           p = 100.0*G4UniformRand();
	   if(p < 52.1) { 
	    this->nucltransK(outPart, 1.551,0.037,8.4e-4,9.7e-5,tclev,thlev) ;
	    return;
	   } else {
	     this->nucltransK(outPart, 0.733,0.037,4.5e-3,0.,tclev,thlev) ; 
	     thlev=1.93e-12;
	     this->nucltransK(outPart, 0.819,0.037,2.9e-3,0.,tclev,thlev) ;
	  }
	}
     case 1579:
        this->nucltransK(outPart, 0.761,0.037,3.4e-3,0.,tclev,thlev) ;
	 thlev=1.93e-12;
	 this->nucltransK(outPart, 0.819,0.037,2.9e-3,0.,tclev,thlev) ;
	 break;
     case 1551:
 	  thlev=1.01e-12;
          p = 100.0*G4UniformRand();
	   if(p < 52.1) { 
	    this->nucltransK(outPart, 1.551,0.037,8.4e-4,9.7e-5,tclev,thlev) ;
	    return;
	   } else {
	     this->nucltransK(outPart, 0.733,0.037,4.5e-3,0.,tclev,thlev) ; 
	     thlev=1.93e-12;
	     this->nucltransK(outPart, 0.819,0.037,2.9e-3,0.,tclev,thlev) ;
	  }
          break;
     case 819:
 	thlev=1.93e-12;
	this->nucltransK(outPart, 0.819,0.037,2.9e-3,0.,tclev,thlev) ;
        break;
     default:
       std::cerr << " decay0::Ba136low, Ba136: wrong level [keV] " << levelkev << std::endl;
       return;
   }

}
//void decay0::Xe130low(std::vector<decay0Part> &outPart) const {  // Xenon de-excitation.
//  std::cerr << " decay0::Xe130low, not yet coded .. " << std::endl;
  // perhaps we have to find other coding method.. 
//}
void decay0::nucltransK(std::vector<decay0Part> &outPart, const double Egamma, const double Ebinde, 
                    const double conve, const double convp, const double tclev, const double thlev) const {

// Subroutine nucltransK choise one of the three concurent processes
// by which the transition from one nuclear state to another is
// occured: gamma-ray emission, internal conversion and internal
// pair creation. Conversion electrons are emitted only with one fixed energy
// (usually with Egamma-E(K)_binding_energy).
//	 call nucltransK(Egamma,Ebinde,conve,convp,tclev,thlev,tdlev)
// Input : Egamma - gamma-ray energy (MeV) [=difference in state energies];
//	 Ebinde - binding energy of electron (MeV);
//	 conve  - internal electron conversion coefficient [=Nelectron/Ngamma];
//	 convp  - pair conversion coefficient [=Npair/Ngamma];
//	 tclev  - time of creation of level from which particle will be
//		  emitted (sec);
//	 thlev  - level halflife (sec).

   const double p=(1.+ conve + convp)*G4UniformRand();
   if (p < 1.) this->outGamma(outPart, Egamma,tclev,thlev);
   else if (p < (1.+conve) ) {
     this->outElectron(outPart, Egamma-Ebinde, tclev, thlev);
//     const double deltaT =outPart[outPart.size()-1]._time; 
//     this->outGamma(outPart, Ebinde, deltaT, 0.); // for real time  
     this->outGamma(outPart, Ebinde, 0., 0.); // for time shift ???   
   } else {
     this->outPair(outPart, Egamma - 2.*_emass,tclev,thlev);
   }
}    

void decay0::outGamma(std::vector<decay0Part> &outPart, const double e, const double tclev, const double thlev) const {

    this->timedParticle(outPart, 1, e, e, 0. ,M_PI, 0., 2.*M_PI, tclev, thlev);

}
void decay0::outElectron(std::vector<decay0Part> &outPart, const double e, const double tclev, const double thlev) const {

    this->timedParticle(outPart, 3, e, e, 0. ,M_PI, 0., 2.*M_PI, tclev, thlev);

}
void decay0::outPair(std::vector<decay0Part> &outPart, const double ePair, const double tclev, const double thlev) const {
// Generation of e+e- pair in zero-approximation to real subroutine for
//  INTERNAL pair creation:
//     1) energy of e+ is equal to the energy of e-;
//     2) e+ and e- are emitted in the same direction.
//  Call  : common/genevent/tevst,npfull,npgeant(100),pmoment(3,100),ptime(100)
// 	  call pair(Epair,tclev,thlev,tdlev)
//  Input:  Epair - kinetic energy of e+e- pair (MeV);
// 	  tclev - time of creation of level from which pair will be
//		  emitted (sec);
//
     const double phi =2.*M_PI*G4UniformRand();
     const double ctet = -1.+ 2.*G4UniformRand();
     const double teta=std::acos(ctet);
     const double e=0.5*ePair;
     this->timedParticle(outPart, 2,e,e,teta,teta,phi,phi,tclev, thlev);
//     const double deltaT =outPart[outPart.size()-1]._time; 
//     this->outParticle(outPar, 3, E,E,teta,teta,phi,phi, deltaT, 0.); // for real time  
     this->timedParticle(outPart, 3, e , e, teta, teta, phi, phi, 0., 0.); // for time shift ??  
}

// AddParticle. 
double decay0::timedParticle (std::vector<decay0Part> &outPart, int np, const double E1,
         const double E2, const double teta1, const double teta2, 
	 const double phi1, const double phi2, const double tclev, const double thlev) const {
// Input : np          - GEANT number for particle identification:
//                        1 - gamma         2 - positron     3 - electron
//                        4 - neutrino      5 - muon+        6 - muon-
// translation to G4 pdg..     
   decay0Part aP;
   double pMass=0.;
   switch (np) {
     case 1:
       aP._pdgCode = 21;
       break;
     case 2:
       aP._pdgCode = -11;
       pMass = _emass;
       break;
     case 3:   
       aP._pdgCode = 11;
       pMass = _emass;
       break;
     case 4:
       aP._pdgCode = 12;
       break;
     default : 
       std::cerr << " decay0::timedParticle Unrecognized particle, nothing stored.. " << std::endl;
       return 0.; 
   }
   const double phi = phi1 + (phi2 - phi1)*G4UniformRand();
   const double ctet1 = std::cos(teta1);
   const double ctet2 = std::cos(teta2);
   const double ctet =  ctet1 + (ctet2 - ctet1)*G4UniformRand();
   const double stet = std::sqrt(1.0 - ctet*ctet);
   aP._energy = E1;
   if (std::abs(E2 - E1) > 1.0e-10) aP._energy = E1 + (E2-E1)*G4UniformRand();   
   const double p = std::sqrt(aP._energy * (aP._energy + 2.*pMass));
   aP._pmom[0] = p*stet*std::cos(phi);
   aP._pmom[1] = p*stet*std::sin(phi);
   aP._pmom[2] = p*ctet;
   double t = tclev;
   if (thlev > 1.0e-100) t = tclev - thlev/std::log(2.0) * std::log(G4UniformRand());
   aP._time = t;
   outPart.push_back(aP);
   return t;	     
} 
// ***********************************************************************

double decay0::fe1_mod1(double e1, void *params) {
//  probability distribution for energy of first e-/e+ for modebb=1
//  (0nu2b with m(nu), 0+ -> 0+, 2n mechanism)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p;
//	std::cerr << " emass " << emass << " Zdbb " << Zdbb << " e0 " << e0 << " e1 " << e1  << std::endl;
	double a =0.;
	if (e1 > e0) return a;
	const double e2 = e0 - e1;
	const double p1=std::sqrt(e1*(e1+2.*emass));
	const double p2=std::sqrt(e2*(e2+2.*emass));
	a = (e1+emass) * p1 * decay0::fermi(Zdbb,e1) * (e2+emass) * p2 * decay0::fermi(Zdbb,e2);
//	std::cerr << " e1 " << e1 << " e2 " << e2 << std::endl;
//	std::cerr << " Fermi functions " << decay0::fermi(Zdbb,e1) << "  " << decay0::fermi(Zdbb,e2) << std::endl;
//	exit(2);
	return a;
}

// ***********************************************************************
double decay0::fe1_mod2(double e1, void *params) {
// probability distribution for energy of first e-/e+ for modebb=2
// (0nu2b with rhc, 0+ -> 0+, 2n mechanism)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p;
	double a=0.;
	if(e1 > e0) return a;
	const double e2 = e0 - e1;
	const double p1=std::sqrt(e1*(e1+2.*emass));
	const double p2=std::sqrt(e2*(e2+2.*emass));
	a=(e1+emass) * p1 * decay0::fermi(Zdbb,e1) * (e2+emass) * p2 * decay0::fermi(Zdbb,e2);
	a*= (e0-2.*e1)*(e0-2.*e1);
	return a;
}
// ***********************************************************************
double decay0::fe1_mod3(double e1, void *params) {
// probability distribution for energy of first e-/e+ for modebb=3
// (0nu2b with rhc, 0+ -> 0+ or 0+ -> 2+, N* mechanism)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p;
	double a=0.;
	if(e1 > e0) return a;
	const double e2 = e0 - e1;
	const double p1=std::sqrt(e1*(e1+2.*emass));
	const double p2=std::sqrt(e2*(e2+2.*emass));
	a = p1 * decay0::fermi(Zdbb,e1) * p2 * decay0::fermi(Zdbb,e2) *
                 (2.*p1*p1*p2*p2 + 9.*((e1+emass)*(e2+emass)+emass*emass) * (p1*p1+p2*p2));
	return a;
}
// ***********************************************************************
double decay0::fe12_mod4(double e2, void *params) {
// two-dimensional probability distribution for energies of e-/e+ for
// modebb=4 (as tsimpr needs)
// (2nu2b, 0+ -> 0+, 2n mechanism)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p; p++;
	const double e1 = *p; 
	double a=0.;
//	std::cerr << " fe12_mod4 ... emass " << emass << " e0 " << e0 << " e1 " << e1 << " e2 " << e2 <<  std::endl;
	if (e2 > (e0-e1)) return a;
	const double p1=std::sqrt(e1*(e1+2.*emass));
	const double p2=std::sqrt(e2*(e2+2.*emass));
	a = (e1+emass) * p1 * decay0::fermi(Zdbb,e1) * (e2+emass) * p2 * decay0::fermi(Zdbb,e2)*
                std::pow((e0-e1-e2),5.);
//	std::cerr << " fe12_mod4 ... e0 " << e0 << " e1 " << e1 << " e2 " << e2 << " a " << a <<  std::endl;
	return a;
}
// ***********************************************************************
double decay0::fe2_mod4(double e2, void *params) {
// probability distribution for energy of second e-/e+ for modebb=4
// (2nu2b, 0+ -> 0+, 2n mechanism)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p; p++;
	const double e1 = *p;
	double a=0.;
	if (e2 > (e0-e1)) return a;
	const double p2 = std::sqrt(e2*(e2+2.*emass));
	a= (e2+emass) * p2 * decay0::fermi(Zdbb,e2) * std::pow((e0-e1-e2),5.);
	return a;
}
// ***********************************************************************
double decay0::fe12_mod5(double e2, void *params) { 
// two-dimensional probability distribution for energies of e-/e+ for
// modebb=5 (as tsimpr needs)
// (0nu2b with Gelmini-Roncadelli Majoron (spectral index = 1),
// 0+ -> 0+, 2n mechanism)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p; p++;
	const double e1 = *p;
	double a=0.;
	if ( e2 > (e0-e1)) return a;
	const double p1=std::sqrt(e1*(e1+2.*emass));
	const double p2=std::sqrt(e2*(e2+2.*emass));
	a = (e1+emass) * p1 * decay0::fermi(Zdbb,e1) * (e2+emass)* p2 * fermi(Zdbb,e2) * (e0-e1-e2);
	return a;
}

// ***********************************************************************
double decay0::fe2_mod5(double e2, void *params) { 
// probability distribution for energy of second e-/e+ for modebb=5
// (0nu2b with Gelmini-Roncadelli Majoron (spectral index = 1),
//  0+ -> 0+, 2n mechanism)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p; p++;
	const double e1 = *p;
	double a=0.;
	if(e2 > (e0-e1)) return a;
	const double p2 = std::sqrt(e2*(e2+2.*emass));
	a= (e2+emass) * p2 * fermi(Zdbb,e2) * (e0-e1-e2);
	return a;
}
// ***********************************************************************
double decay0::fe12_mod6(double e2, void *params) { 
// two-dimensional probability distribution for energies of e-/e+ for
// modebb=6 (as tsimpr needs)
// (0nu2b with massive vector etc. Majoron (spectral index = 3),
// 0+ -> 0+, 2n mechanism)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p; p++;
	const double e1 = *p;
	double a=0.;
	if (e2 > (e0-e1)) return a;
	const double p1=std::sqrt(e1*(e1+2.*emass));
	const double p2=std::sqrt(e2*(e2+2.*emass));
	a = (e1+emass) * p1 * decay0::fermi(Zdbb,e1) * (e2+emass) *p2 * decay0::fermi(Zdbb,e2) *
            std::pow((e0-e1-e2), 3.);
	return a;
}
// ***********************************************************************
double decay0::fe2_mod6(double e2, void *params) {
// probability distribution for energy of second e-/e+ for modebb=6
// (0nu2b with massive vector etc. Majoron (spectral index = 3),
// 0+ -> 0+, 2n mechanism)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p; p++;
	const double e1 = *p;
	double a=0.;
	if(e2 > (e0-e1)) return a;
	const double p2=std::sqrt(e2*(e2+2.*emass));
	a=(e2+emass) *p2 * decay0::fermi(Zdbb,e2) * std::pow((e0-e1-e2),3.);
	return a;
}
// ***********************************************************************
double decay0::fe1_mod7(double e1, void *params) {
// probability distribution for energy of first e-/e+ for modebb=7
// (0nu2b with rhc, 0+ -> 2+, 2n mechanism)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p;
	double a=0.;
	if(e1 > e0) return a;
	const double e2 = e0-e1;
	const double p1=std::sqrt(e1*(e1+2.*emass));
	const double p2=std::sqrt(e2*(e2+2.*emass));
	a = p1 * decay0::fermi(Zdbb,e1) * p2 * decay0::fermi(Zdbb,e2) *
                ((e1+emass)*(e2+emass)+emass*emass)*(p1*p1+p2*p2);
	return a;
}
// ***********************************************************************
double decay0::fe12_mod8(double e2, void*params) {
// two-dimensional probability distribution for energies of e-/e+ for
// modebb=8 (as tsimpr needs)
// (2nu2b, 0+ -> 2+, 2n or N* mechanism)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p; p++;
	const double e1 = *p;
	double a=0.;
	if ( e2 > (e0-e1)) return a;
	const double p1=std::sqrt(e1*(e1+2.*emass));
	const double p2=std::sqrt(e2*(e2+2.*emass));
	a = (e1+emass) * p1 * decay0::fermi(Zdbb,e1) * (e2+emass) * p2 * decay0::fermi(Zdbb,e2) *
                 std::pow((e0-e1-e2),7.) * (e1-e2)*(e1-e2);
	return a;
}
// ***********************************************************************

double decay0::fe2_mod8(double e2, void *params) {
// probability distribution for energy of second e-/e+ for modebb=8
// (2nu2b, 0+ -> 2+, 2n or N* mechanism)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p; p++;
	const double e1 = *p;
	double a=0.;
	if ( e2 > (e0-e1)) return a;
	const double p2=std::sqrt(e2*(e2+2.*emass));
	a = (e2+emass) * p2 * fermi(Zdbb,e2) * std::pow((e0-e1-e2),7.) * (e1-e2)*(e1-e2);
	return a;
}
// ***********************************************************************
double decay0::fe1_mod10(double e1, void *params) {
// probability distribution for energy of e+ for modebb=10 (2nueb+)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p; p++;
	double a=0.;
	if(e1 > e0) return a;
	const double p1 = std::sqrt(e1*(e1+2.*emass));
	a = (e1+emass) * p1 * decay0::fermi(Zdbb,e1) * std::pow((e0-e1), 5.);
	return a;
}
// ***********************************************************************

double decay0::fe12_mod13(double e2, void *params) {
// two-dimensional probability distribution for energies of e-/e+ for
//  modebb=13 (as tsimpr needs)
//  (0nu2b with Majoron with spectral index = 7, 0+ -> 0+, 2n mechanism)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p; p++;
	const double e1 = *p;
	double a=0.;
	if(e2 > (e0-e1)) return a;
	const double p1 = std::sqrt(e1*(e1+2.*emass));
	const double p2 = std::sqrt(e2 * (e2 + 2.*emass));
	a = (e1+emass) * p1 * decay0::fermi(Zdbb,e1) * (e2+emass) * p2 * fermi(Zdbb,e2) * 
                  std::pow((e0-e1-e2), 7.);
	return a;
}
// ***********************************************************************

double decay0::fe2_mod13(double e2, void *params) {
// probability distribution for energy of second e-/e+ for modebb=13
// (0nu2b with Majoron with spectral index = 7, 0+ -> 0+, 2n mechanism)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p; p++;
	const double e1 = *p;
	double a=0.;
	if(e2 > (e0-e1)) return a;
	const double p2 = std::sqrt(e2 * (e2 + 2.*emass));
	a= (e2+emass) * p2 * decay0::fermi(Zdbb,e2) * std::pow((e0-e1-e2), 7.);
	return a;
}

//***********************************************************************
double decay0::fe12_mod14(double e2, void *params) {
// two-dimensional probability distribution for energies of e-/e+ for
// modebb=14 (as tsimpr needs)
// (0nu2b with Mohapatra Majoron with spectral index = 2, 0+ -> 0+, 2n mechanism)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p; p++;
	const double e1 = *p;
	double a=0.;
	if(e2 > (e0-e1)) return a;
	const double p1 = std::sqrt(e1*(e1+2.*emass));
	const double p2 = std::sqrt(e2 * (e2 + 2.*emass));
	a = (e1+emass) * p1 * decay0::fermi(Zdbb,e1) * (e2+emass)* p2 * decay0::fermi(Zdbb,e2) *
                 (e0-e1-e2)*(e0-e1-e2);
	return a;
}
double decay0::fe2_mod14(double e2, void *params) {
// probability distribution for energy of second e-/e+ for modebb=14
//  (0nu2b with Mohapatra Majoron with spectral index = 2, 0+ -> 0+, 2n mechanism)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p; p++;
	const double e1 = *p;
	double a=0.;
	if(e2 > (e0-e1)) return a;
	const double p2 = std::sqrt(e2 * (e2 + 2.*emass));
	a = (e2+emass) * p2 * decay0::fermi(Zdbb,e2) * (e0-e1-e2)*(e0-e1-e2);
	return a;
}

//***********************************************************************

double decay0::fe12_mod15(double e2, void *params) {
// two-dimensional probability distribution for energies of e-/e+ for
//  modebb=15 (as tsimpr needs)
//  (2nu2b, 0+ -> 0+, bosonic neutrinos, HSD: A.S.Barabash et al., NPB 783(2007)90,
//  eq. (27a) integrated in Enu1)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p; p++;
	const double e1 = *p;
	double a=0.;
	if(e2 > (e0-e1)) return a;
	const double p1 = std::sqrt(e1*(e1+2.*emass));
	const double p2 = std::sqrt(e2 * (e2 + 2.*emass));
	a= (e1+emass) * p1 * decay0::fermi(Zdbb,e1) * (e2+emass) * p2 * fermi(Zdbb,e2) *
                  std::pow((e0-e1-e2), 5.) * (9.*(e0-e1-e2)*(e0-e1-e2) + 21.* (e2-e1) * (e2-e1));
	return a;
}
double decay0::fe2_mod15(double e2, void *params) {
// probability distribution for energy of second e-/e+ for modebb=4
// (2nu2b, 0+ -> 0+, 2n mechanism)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p; p++;
	const double e1 = *p;
	double a=0.;
	if(e2 > (e0-e1)) return a;
	const double p2 = std::sqrt(e2 * (e2 + 2.*emass));
	a= (e2+emass) * p2 * decay0::fermi(Zdbb,e2) * std::pow((e0-e1-e2), 5.) *
                 (9.*(e0-e1-e2)*(e0-e1-e2) + 21.*(e2-e1)*(e2-e1));
	return a;
}
// ***********************************************************************

double decay0::fe12_mod16(double e2, void *params) {
// two-dimensional probability distribution for energies of e-/e+ for
// modebb=16 (as tsimpr needs)
// (2nu2b, 0+ -> 2+, bosonic neutrinos, HSD: A.S.Barabash et al., NPB 783(2007)90,
//  eq. (27b) integrated in Enu1)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p; p++;
	const double e1 = *p;
	double a=0.;
	if(e2 > (e0-e1)) return a;
	const double p1 = std::sqrt(e1*(e1+2.*emass));
	const double p2 = std::sqrt(e2 * (e2 + 2.*emass));
	a = (e1+emass) * p1 * decay0::fermi(Zdbb,e1) * (e2+emass)*p2*fermi(Zdbb,e2) * 
                std::pow((e0-e1-e2), 5.) * (e2-e1)*(e2-e1);
	return a;
}

double decay0::fe2_mod16(double e2, void *params) {
// probability distribution for energy of second e-/e+ for modebb=4
// (2nu2b, 0+ -> 0+, 2n mechanism)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p; p++;
	const double e1 = *p;
	double a=0.;
	if(e2 > (e0-e1)) return a;
	const double p2 = std::sqrt(e2 * (e2 + 2.*emass));
	a = (e2+emass) * p2 * decay0::fermi(Zdbb,e2) * std::pow((e0-e1-e2),5.) * (e2-e1)*(e2-e1);
	return a;
}
// ***********************************************************************

double decay0::fe1_mod17( double e1, void *params) {
// probability distribution for energy of first e-/e+ for modebb=17
// (0nu2b with rhc-eta, 0+ -> 0+, 2n mechanism, simplified expression)
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p;
	double a=0.;
	if(e1 > e0) return a;
	const double e2 = e0-e1;
	const double p1 = std::sqrt(e1*(e1+2.*emass));
	const double p2 = std::sqrt(e2 * (e2 + 2.*emass));
	a = (e1+emass) * p1 * decay0::fermi(Zdbb,e1) * (e2+emass) * p2 * decay0::fermi(Zdbb,e2);
	return a;
}

// ***********************************************************************
//double decay0::fe1_mod18(double e1, void *params, eta_nme &etaThing) {
// probability distribution for energy of the first e-/e+ for modebb=18
// (0nu2b with right-handed currents, eta term, 0+ -> 0+, 2n mechanism) 
//       std::cerr << " decay0::fe1_mod18 not yet translated... " << std::endl;
//       exit(2);
/*
        double *p = static_cast<double*>(params);
	const double emass = *p; p++;
	const double Zdbb = *p; p++;
	const double e0 = *p;
	double a=0.;
	common/eta_nme/chi_GTw,chi_Fw,chip_GT,chip_F,chip_T,
     +               chip_P,chip_R
	fe1_mod18=0.
	if(e1 > e0) return a;
	e2=e0-e1
	const double p1 = std::sqrt(e1*(e1+2.*emass));
	const double p2 = std::sqrt(e2 * (e2 + 2.*emass));
c total energies in the units of electron mass
	et0=e0/emass+1.
	et1=e1/emass+1.
	et2=e2/emass+1.
	a1=(et1*et2-1.)*(et1-et2)**2/(2.*et1*et2)
	a2=-2.*(et1-et2)**2/(9.*et1*et2)
	a3=2.*(et1*et2-1.)/(81.*et1*et2)
	r=3.107526e-3*Adbb**(1./3.)
	rksi=3./137.036*Zdbb+r*et0
	a4=8.*(et1*et2+1.)/(r**2*et1*et2)
	a5=-8.*(rksi*(et1*et2+1.)-2.*r*et0)/(3.*r**2*et1*et2)
	a6=2.*((rksi**2+4.*r**2)*(et1*et2+1.)-4.*rksi*r*et0)/
     +     (9.*r**2*et1*et2)
	chi_1plus =chip_GT+3.*chip_F-6.*chip_T
	chi_1minus=chip_GT-3.*chip_F-6.*chip_T
	chi_2plus =chi_GTw+chi_Fw-chi_1minus/9.
	a_eta=a1*chi_2plus**2+a2*chi_2plus*chi_1minus+a3*chi_1minus**2
     +     +a4*chip_R**2+a5*chip_R*chip_P+a6*chip_P**2
c	print *,'fe1_mod18:'
c	print *,'chi_GTw,chi_Fw,chip_GT,chip_F,chip_T,chip_P,chip_R'
c	print *,chi_GTw,chi_Fw,chip_GT,chip_F,chip_T,chip_P,chip_R
c	print *,'e1,r,rksi,a_eta=',e1,r,rksi,a_eta
c	if(e1.eq.e2) pause
	fe1_mod18=(e1+emass)*p1*fermi(Zdbb,e1)*
     +          (e2+emass)*p2*fermi(Zdbb,e2)*a_eta
	return
	end
*/
//      return 0.;
//}

double decay0::dshelp1(double e1, void *params) {
        const int numWordsScratch = 10000;
        gsl_integration_workspace *gwk = gsl_integration_workspace_alloc(numWordsScratch);       
        double *p = static_cast<double*>(params);
//	const double emass = *p; p++;
//	const double Zdbb = *p; p++;
//	const double e0 = *p; p++;
        p++; p++; p++;
	*p = e1; p++;
//	const int mode = static_cast<int>(*p); p++;
	p++;
//	const double dens = *p; p++; 
//	const double denf = *p; p++; 
	const double d_e1Low = *p; p++;
	const double d_e1High = *p;
        
	double a=0.;
	double relerr = 1.0e-4;
	double errAchieved = 0.;
        gsl_function dshelpXX;
	dshelpXX.function = &decay0::dshelp2;
        dshelpXX.params = params;
        int intSuccess = gsl_integration_qag(&dshelpXX, d_e1Low, d_e1High, 0., relerr, 1000, GSL_INTEG_GAUSS41,
	                                  gwk, &a,  &errAchieved );
	if (intSuccess != GSL_SUCCESS) {
	    std::cerr << " decay0::dshelp1, integration failure.. " << intSuccess 
	              << " e1 " << e1 << " d_e1Low " << d_e1Low << " d_e1High " << d_e1High << std::endl;
		      return 0.; 
	}
//	std::cerr << " decay0::dshelp1, integral result from " << d_e1Low << " and " << d_e1High << " is " << a << std::endl;
	gsl_integration_workspace_free (gwk);

	return a;
}
double decay0::dshelp2(double e2, void* params) {
        double *p = static_cast<double*>(params);
//	const double emass = *p; p++;
//	const double Zdbb = *p; p++;
//	const double e0 = *p; p++;
        p++; p++;
	const double e0 = *p; p++;
	const double e1 = *p; p++;
	const int mode = static_cast<int>(*p); p++;
	const double dens = *p; p++; 
	const double denf = *p; p++; 
	const double d_e1Low = *p; p++;
	const double d_e1High = *p;
//	std::cerr << " decay0::dshelp2   limits  " << d_e1Low << " / " <<  d_e1High 
//	          <<" dens/f  " << dens <<" / " << denf << std::endl;
//	std::cerr << " ..... e0 " << e0 << " e1 " <<  e1 << " e2 " << e2 << std::endl;
	double a=0.;
	if((e1 > 0. ) && (e2 > 0.) && ((e1+e2) < e0)) {
	   switch (mode) { 	     
	     case 4: 
	        a = decay0::fe12_mod4(e2, params);
		break;
	     case 5: 
	        a = decay0::fe12_mod5(e2, params);
		break;
	     case 6: 
	        a = decay0::fe12_mod6(e2, params);
		break;
	     case 8: 
	        a = decay0::fe12_mod8(e2, params);
		break;
	     case 13: 
	        a = decay0::fe12_mod13(e2, params);
		break;
	     case 14: 
	        a = decay0::fe12_mod14(e2, params);
		break;
	     default: 
	        break;
	   }
	   const double d_sum = d_e1Low + d_e1High;
//	   std::cerr << " .....  value e2 " << e2<< " mode  "  << mode << " is " << a  << " amd quit " << std::endl;
//	   exit(2);
	   if ((d_sum < dens) || (d_sum > denf)) a = 0.;
	} 
//	std::cerr << " .....  value e2, after zeroing  " << e2<< " mode  "  << mode << " is " << a  << std::endl;
	return a;
}

double decay0::fermi(double z, const double e) {

// Function fermi calculates the traditional function of Fermi in
// theory of beta decay to take into account the Coulomb correction
// to the shape of electron/positron energy spectrum.
// Call  : corr=fermi(Z,E)
// Input : Z - atomic number of daughter nuclei (>0 for e-, <0 for e+);
//         E - kinetic energy of particle (MeV; E>50 eV). Output: corr - value of correction factor (without normalization -
//                constant factors are removed - for MC simulation).
// V.I.Tretyak, 15.07.1992. Translated to C++ by Paul Lebrun. 
//	std::cerr << " From fermi, e " << e << std::endl;
	double E = e;
	if (E < 50.e-6) E=50.e-6;
	const double alfaz=z/137.036;
	const double w=E/0.511+1.; // rough value of emectron mass? 
	const double p=std::sqrt(w*w-1.);
	const double y=alfaz*w/p;
	const double g=std::sqrt(1.-alfaz*alfaz); // the real part 
	// gsl_sf_lngamma_complex_e (double zr, double zi, gsl_sf_result * lnr, gsl_sf_result * arg)
	gsl_sf_result lnr;
	lnr.val = 0.;
	gsl_sf_result arg;
	gsl_sf_lngamma_complex_e (g,y, &lnr, &arg);
	const double aa = lnr.val; // could check the error..
//	std::complex carg(g,y);
//	fermi=p**(2.*g-2.)*exp(3.1415927*y+2.*alog(cabs(cgamma(carg))))
        const double a = std::pow(p, (2.0*g - 2.)) * std::exp(M_PI*y + 2.0*aa);	
//	std::cerr << " g " << g << " y " << y << " lnr " << lnr.val << std::endl;
//	std::cerr << " exp term " << std::exp(M_PI*y + 2.0*aa) << " fermi "  << a <<  std::endl << std::endl; 
	
	return a;
}
