#ifndef RecoBTag_SoftLepton_MuonTagger_h
#define RecoBTag_SoftLepton_MuonTagger_h

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "RecoBTau/JetTagComputer/interface/JetTagComputer.h"
#include "RecoBTag/SoftLepton/interface/LeptonSelector.h"
#include "RecoBTag/SoftLepton/src/MuonTaggerMLP.h"

#include <TROOT.h>
#include "TRandom3.h"
#include "TMVA/Factory.h"
#include "TMVA/Tools.h"
#include "TMVA/Reader.h"

class MuonTagger : public JetTagComputer {

  public:
  
    MuonTagger(const edm::ParameterSet&);
    ~MuonTagger();
    
    virtual float discriminator(const TagInfoHelper& tagInfo);
    
  private:

    btag::LeptonSelector m_selector;
    
    TMVA::Reader* TMVAReader;
    TRandom3* random;
    
    std::string weightFile, methodName;
    float mva_sip3d, mva_sip2d, mva_ptRel, mva_dR;

};

#endif

