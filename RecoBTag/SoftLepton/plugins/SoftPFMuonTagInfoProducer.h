#ifndef RecoBTag_SoftLepton_SoftPFMuonTagInfoProducer_h
#define RecoBTag_SoftLepton_SoftPFMuonTagInfoProducer_h


#include <vector>

#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/Common/interface/ValueMap.h"
#include <DataFormats/ParticleFlowCandidate/interface/PFCandidate.h>
#include "DataFormats/JetReco/interface/PFJetCollection.h"
// Muons
#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/MuonReco/interface/MuonFwd.h"
#include "DataFormats/MuonReco/interface/MuonSelectors.h"

#include "DataFormats/JetReco/interface/PFJetCollection.h"

#include "TrackingTools/TransientTrack/interface/TransientTrack.h"
#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"

#include "RecoEgamma/EgammaTools/interface/ConversionTools.h" 

// Vertex
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/BTauReco/interface/SoftLeptonTagInfo.h"
// SoftPFMuonTagInfoProducer:  the SoftPFMuonTagInfoProducer takes
// a PFCandidateCollection as input and produces a RefVector
// to the likely soft muons in this collection.

class SoftPFMuonTagInfoProducer : public edm::stream::EDProducer<>
{

  public:

    SoftPFMuonTagInfoProducer (const edm::ParameterSet& conf);
    ~SoftPFMuonTagInfoProducer();
    reco::SoftLeptonTagInfo tagMuon (const edm::RefToBase<reco::Jet> &, reco::PFCandidateCollection &) ;
  private:

    virtual void produce(edm::Event&, const edm::EventSetup&);
    virtual void fillSoftMuonTagInfo(const reco::Muon*, const reco::Jet*, const reco::Vertex*, const TransientTrackBuilder*, reco::SoftLeptonTagInfo*);
    virtual bool promptLeptonFilter(float, float);

    edm::EDGetTokenT<edm::View<reco::Jet> > jetToken;
    edm::EDGetTokenT<edm::View<reco::Muon> > muonToken;
    edm::EDGetTokenT<reco::VertexCollection> vertexToken;
    float pTcut, IPcut, ratio1cut, ratio2cut, dRcut;
    bool useFilter, useMINIAOD;
};


#endif
