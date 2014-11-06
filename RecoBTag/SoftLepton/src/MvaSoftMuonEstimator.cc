// * Author: Alberto Zucchetta
// * Mail: a.zucchetta@cern.ch
// * November 6, 2014

#include <limits>

#include "DataFormats/BTauReco/interface/SoftLeptonTagInfo.h"
#include "RecoBTag/SoftLepton/interface/LeptonSelector.h"
#include "RecoBTag/SoftLepton/interface/MvaSoftMuonEstimator.h"


MvaSoftMuonEstimator::MvaSoftMuonEstimator() {
  weightFile="RecoBTag/SoftLepton/softPFMuon.weights.xml";
  methodName="SoftPFMuon";
  
  TMVAReader = new TMVA::Reader("!Color:!Silent:Error");
  TMVAReader->SetVerbose(kTRUE);
  TMVAReader->AddVariable("TagInfo1.sip3d", &mva_sip3d);
  TMVAReader->AddVariable("TagInfo1.sip2d", &mva_sip2d);
  TMVAReader->AddVariable("TagInfo1.ptRel", &mva_ptRel);
  TMVAReader->AddVariable("TagInfo1.ratioRel", &mva_ratioRel);
  TMVAReader->BookMVA(methodName, weightFile);
}

MvaSoftMuonEstimator::~MvaSoftMuonEstimator() {
  delete TMVAReader;
}


// b-tag a jet based on track-to-jet parameters in the extened info collection
float MvaSoftMuonEstimator::mvaValue(float sip3d, float sip2d, float ptRel, float ratioRel) {
  mva_sip3d = sip3d;
  mva_sip2d = sip2d;
  mva_ptRel = ptRel;
  mva_ratioRel = ratioRel;
  // Evaluate tagger
  float tag = TMVAReader->EvaluateMVA(methodName) + 0.5;
  return tag;
}

