#include <limits>

#include "DataFormats/BTauReco/interface/SoftLeptonTagInfo.h"
#include "RecoBTag/SoftLepton/interface/LeptonSelector.h"
#include "RecoBTag/SoftLepton/interface/MuonTagger.h"


MuonTagger::MuonTagger(const edm::ParameterSet& conf): m_selector(conf) {
  uses("smTagInfos");
  weightFile="RecoBTag/SoftLepton/data/MVA_SM_BDT_weight.weights.xml";
  methodName="SoftMuon";
  
  TMVAReader = new TMVA::Reader("!Color:!Silent:Error");
  TMVAReader->SetVerbose(kTRUE);
  TMVAReader->AddVariable("sip3d", &mva_sip3d);
  TMVAReader->AddVariable("sip2d", &mva_sip2d);
  TMVAReader->AddVariable("ptRel", &mva_ptRel);
  TMVAReader->AddVariable("dR", &mva_dR);
  TMVAReader->BookMVA(methodName, weightFile);
  
  random=new TRandom3();
}

MuonTagger::~MuonTagger() {
  delete random;
  delete TMVAReader;
}


// b-tag a jet based on track-to-jet parameters in the extened info collection
float MuonTagger::discriminator(const TagInfoHelper& tagInfo) {

  float bestTag = - std::numeric_limits<float>::infinity(); // default value, used if there are no leptons associated to this jet
  const reco::SoftLeptonTagInfo& info = tagInfo.get<reco::SoftLeptonTagInfo>();
  
  // If there are multiple leptons, look for the highest tag result
  for (unsigned int i=0; i<info.leptons(); i++) {
    const reco::SoftLeptonProperties& properties = info.properties(i);
    if(m_selector(properties)) {
      int seed=1+round(10000.0*fabs(properties.deltaR));
      random->SetSeed(seed);
      float rndm = random->Uniform(0,1);
      // For negative tagger, flip 50% of the negative signs to positive value
      mva_sip3d = (m_selector.isNegative() && rndm<0.5) ? -properties.sip3d : properties.sip3d;
      mva_sip2d = (m_selector.isNegative() && rndm<0.5) ? -properties.sip2d : properties.sip2d;
      mva_ptRel = properties.ptRel;
      mva_dR    = properties.deltaR;
      // Evaluate tagger
      float tag = TMVAReader->EvaluateMVA(methodName);
      if(tag>bestTag) bestTag = tag;
    }
  }
  return bestTag;
}
