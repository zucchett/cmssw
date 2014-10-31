#include "RecoBTag/SoftLepton/plugins/SoftPFMuonTagInfoProducer.h"
#include "DataFormats/GsfTrackReco/interface/GsfTrack.h"
#include "DataFormats/GsfTrackReco/interface/GsfTrackFwd.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/Math/interface/Vector3D.h"
#include "DataFormats/Math/interface/Point3D.h"
#include "FWCore/Utilities/interface/EDMException.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "RecoParticleFlow/PFProducer/interface/Utils.h"
#include "DataFormats/JetReco/interface/PFJetCollection.h"
#include "DataFormats/JetReco/interface/Jet.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
// Muons
#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/MuonReco/interface/MuonFwd.h"
#include "DataFormats/MuonReco/interface/MuonSelectors.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidateFwd.h"

#include "DataFormats/BTauReco/interface/SoftLeptonTagInfo.h"

// Transient Track and IP
#include "TrackingTools/TransientTrack/interface/TransientTrack.h"
#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "TrackingTools/IPTools/interface/IPTools.h"
#include "DataFormats/GeometryVector/interface/GlobalVector.h"
#include <cmath>

SoftPFMuonTagInfoProducer::SoftPFMuonTagInfoProducer (const edm::ParameterSet& conf) {
	jetToken    = consumes<edm::View<reco::Jet> >(conf.getParameter<edm::InputTag>("jets"));
	muonToken   = consumes<edm::View<reco::Muon> >(conf.getParameter<edm::InputTag>("muons"));
	vertexToken = consumes<reco::VertexCollection>(conf.getParameter<edm::InputTag>("vertex"));
	pTcut       = conf.getParameter<double>("muonPt");
	IPcut       = conf.getParameter<double>("filterIp");
	ratio1cut   = conf.getParameter<double>("filterRatio1");
	ratio2cut   = conf.getParameter<double>("filterRatio2");
	dRcut       = conf.getParameter<double>("dRcut");
	useFilter   = conf.getParameter<bool>("filterPromptMuons");
	useMINIAOD  = conf.getParameter<bool>("runMINIAOD");
	produces<reco::SoftLeptonTagInfoCollection>();
}

SoftPFMuonTagInfoProducer::~SoftPFMuonTagInfoProducer() {}

void SoftPFMuonTagInfoProducer::produce(edm::Event& iEvent, const edm::EventSetup& iSetup) {
  // Declare produced collection
  reco::SoftLeptonTagInfoCollection* theMuonTagInfo = new reco::SoftLeptonTagInfoCollection;		
  
  // Declare and open Jet collection
  edm::Handle<edm::View<reco::Jet> > theJetCollection;
  iEvent.getByToken(jetToken, theJetCollection);
  
  // Declare Muon collection (open only with MINIAOD)
  edm::Handle<edm::View<reco::Muon> > theMuonCollection;
  if(useMINIAOD) iEvent.getByToken(muonToken, theMuonCollection);
  
  // Declare and open Vertex collection
  edm::Handle<reco::VertexCollection> theVertexCollection;
  iEvent.getByToken(vertexToken, theVertexCollection);
  if(!theVertexCollection.isValid() || theVertexCollection->empty()) return;
  const reco::Vertex* theVertex=&theVertexCollection->front();
  
  // Biult TransientTrackBuilder
  edm::ESHandle<TransientTrackBuilder> theTrackBuilder;
  iSetup.get<TransientTrackRecord>().get("TransientTrackBuilder", theTrackBuilder);
  const TransientTrackBuilder* transientTrackBuilder=theTrackBuilder.product();
  
  
  // Loop on jets
  for(unsigned int i=0; i<theJetCollection->size(); i++) {
    edm::RefToBase<reco::Jet> jetRef = theJetCollection->refAt(i);
    // Build TagInfo object
    reco::SoftLeptonTagInfo tagInfo;
    tagInfo.setJetRef(jetRef);
    
    if(useMINIAOD) { // Loop on muon collection
      for(unsigned int im=0, nm=theMuonCollection->size(); im<nm; im++) {
        const reco::Muon* muon=theMuonCollection->refAt(im).get();
        if(deltaR(*jetRef, *muon)>dRcut) continue;
        fillSoftMuonTagInfo(muon, &*jetRef, theVertex, transientTrackBuilder, &tagInfo);
      }
//      for(std::vector<reco::Muon>::const_iterator it=theMuonCollection->begin(); it!=theMuonCollection->end(); ++it) {
//        if(deltaR(*jetRef, *it)>dRcut) continue;
//        fillSoftMuonTagInfo(&*it, &*jetRef, theVertex, transientTrackBuilder, &tagInfo);
//      }
    }
    else { // Loop on constituent list
      const std::vector<reco::CandidatePtr> jetConst = jetRef->getJetConstituents();
      for(unsigned int ic=0, nc=jetConst.size(); ic<nc; ic++) {
        const reco::PFCandidate* pfc = dynamic_cast <const reco::PFCandidate*>(jetConst[ic].get());
        if(pfc==NULL || pfc->particleId()!=3) continue;
        fillSoftMuonTagInfo(pfc->muonRef().get(), &*jetRef, theVertex, transientTrackBuilder, &tagInfo);
      }
    }
    theMuonTagInfo->push_back(tagInfo);
  }
  std::auto_ptr<reco::SoftLeptonTagInfoCollection> MuonTagInfoCollection(theMuonTagInfo);
  iEvent.put(MuonTagInfoCollection);
}


void SoftPFMuonTagInfoProducer::fillSoftMuonTagInfo(const reco::Muon* muon, const reco::Jet* jet, const reco::Vertex* vertex, const TransientTrackBuilder* trackBuilder, reco::SoftLeptonTagInfo* softMuonTagInfo) {
  // Muon quality cuts
  if(!muon::isLooseMuon(*muon) || muon->pt()<pTcut) return;
  // Get Track BaseRef
  reco::TrackBaseRef trkRef(muon->globalTrack().isNonnull() ? muon->globalTrack() : muon->innerTrack());
  // Build Transient Track
  reco::TransientTrack transientTrack=trackBuilder->build(muon->track());
  // Calculate variables
  //float ip2d     = IPTools::signedTransverseImpactParameter(transientTrack, GlobalVector(jet->px(), jet->py(), jet->pz()), *vertex).second.value();
  //float ip3d     = IPTools::signedImpactParameter3D(transientTrack, GlobalVector(jet->px(), jet->py(), jet->pz()), *vertex).second.value();
  float sip2d    = IPTools::signedTransverseImpactParameter(transientTrack, GlobalVector(jet->px(), jet->py(), jet->pz()), *vertex).second.significance();
  float sip3d    = IPTools::signedImpactParameter3D(transientTrack, GlobalVector(jet->px(), jet->py(), jet->pz()), *vertex).second.significance();
  float dR       = deltaR(*jet, *muon);
  float ptRel    = ( (jet->p4().Vect()-muon->p4().Vect()).Cross(muon->p4().Vect()) ).R() / jet->p4().Vect().R(); // | (Pj-Pu) X Pu | / | Pj |
  float mag      = muon->p4().Vect().R()*jet->p4().Vect().R();
  float dot      = muon->p4().Dot(jet->p4());
  float etaRel   = -log((mag - dot)/(mag + dot)) / 2.;
  float ratio    = muon->pt() / jet->pt();
  float ratioRel = muon->p4().Dot(jet->p4()) / jet->p4().Vect().Mag2();
  
  // Fiter leptons from W, Z decays
  if(useFilter && promptLeptonFilter(sip3d, ratio)) return;
  
  // Insert lepton properties and track ref to TagInfo
  reco::SoftLeptonProperties properties;
  properties.sip2d    = sip2d;
  properties.sip3d    = sip3d;
  properties.deltaR   = dR;
  properties.ptRel    = ptRel;
  properties.etaRel   = etaRel;
  properties.ratio    = ratio;
  properties.ratioRel = ratioRel;
  
  // Fill Tag Info
	softMuonTagInfo->insert(trkRef, properties);
}

bool SoftPFMuonTagInfoProducer::promptLeptonFilter(float ip, float r) {
  return (fabs(ip)<IPcut && r>ratio1cut) || r>ratio2cut;
}
