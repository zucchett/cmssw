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
  
  // Declare and open Vertex collection
  edm::Handle<reco::VertexCollection> theVertexCollection;
  iEvent.getByToken(vertexToken, theVertexCollection);
  if(!theVertexCollection.isValid() || theVertexCollection->empty()) return;
  const reco::Vertex* vertex=&theVertexCollection->front();
  
  // Biult TransientTrackBuilder
  edm::ESHandle<TransientTrackBuilder> theTrackBuilder;
  iSetup.get<TransientTrackRecord>().get("TransientTrackBuilder", theTrackBuilder);
  const TransientTrackBuilder* transientTrackBuilder=theTrackBuilder.product();
  
  
  // Loop on jets
  for(unsigned int i=0; i<theJetCollection->size(); i++) { // --- Begin loop on jets
    edm::RefToBase<reco::Jet> jetRef = theJetCollection->refAt(i);
    // Build TagInfo object
    reco::SoftLeptonTagInfo tagInfo;
    tagInfo.setJetRef(jetRef);
    
    for(unsigned int id=0, nd=jetRef->numberOfDaughters(); id<nd; ++id) { // --- Begin loop on daughters
      const reco::Candidate* lep = jetRef->daughter(id);
      const reco::Track* trk(NULL);
      reco::TrackBaseRef trkRef;
      
      // Part 1: Identification
      if(useMINIAOD) {
        const pat::PackedCandidate* pkcand = dynamic_cast<const pat::PackedCandidate*>(lep);
        // Identification
        if(!pkcand || abs(pkcand->pdgId())!=13) continue; // Identification
        if(!(pkcand->isGlobalMuon() /*|| pkcand->isTrackerMuon()*/) ) continue; // Quality FIXME
        trk=&pkcand->pseudoTrack(); // Track FIXME
      }
      else {
        const reco::PFCandidate* pfcand = dynamic_cast<const reco::PFCandidate*>(lep);
        if(!pfcand || pfcand->particleId()!=3) continue; // Identification
        const reco::Muon* muon=pfcand->muonRef().get();
        if( !muon::isLooseMuon(*muon) ) continue; // Quality
        trk=(muon->globalTrack().isNonnull() ? muon->globalTrack() : muon->innerTrack()).get(); // Track
        trkRef=reco::TrackBaseRef(muon->globalTrack().isNonnull() ? muon->globalTrack() : muon->innerTrack()); // TrackRef
      }
      // Additional quality cuts
      if(lep->pt()<pTcut) continue;
      
      // Build Transient Track
      reco::TransientTrack transientTrack=transientTrackBuilder->build(trk);
      // Calculate variables
      reco::SoftLeptonProperties properties;
      //float ip2d     = IPTools::signedTransverseImpactParameter(transientTrack, GlobalVector(jetRef->px(), jetRef->py(), jetRef->pz()), *vertex).second.value();
      //float ip3d     = IPTools::signedImpactParameter3D(transientTrack, GlobalVector(jetRef->px(), jetRef->py(), jetRef->pz()), *vertex).second.value();
      properties.sip2d    = IPTools::signedTransverseImpactParameter(transientTrack, GlobalVector(jetRef->px(), jetRef->py(), jetRef->pz()), *vertex).second.significance();
      properties.sip3d    = IPTools::signedImpactParameter3D(transientTrack, GlobalVector(jetRef->px(), jetRef->py(), jetRef->pz()), *vertex).second.significance();
      properties.deltaR   = deltaR(*jetRef, *lep);
      properties.ptRel    = ( (jetRef->p4().Vect()-lep->p4().Vect()).Cross(lep->p4().Vect()) ).R() / jetRef->p4().Vect().R(); // | (Pj-Pu) X Pu | / | Pj |
      float mag = lep->p4().Vect().R()*jetRef->p4().Vect().R();
      float dot = lep->p4().Dot(jetRef->p4());
      properties.etaRel   = -log((mag - dot)/(mag + dot)) / 2.;
      properties.ratio    = lep->pt() / jetRef->pt();
      properties.ratioRel = lep->p4().Dot(jetRef->p4()) / jetRef->p4().Vect().Mag2();

      // Filter leptons from W, Z decays
      if(useFilter && ((fabs(properties.sip3d)<IPcut && properties.ratio>ratio1cut) || properties.ratio>ratio2cut)) continue;
      
      // Fill Tag Info
      tagInfo.insert(trkRef, properties);
      
    } // --- End loop on daughters
    
  } // --- End loop on jets
  std::auto_ptr<reco::SoftLeptonTagInfoCollection> MuonTagInfoCollection(theMuonTagInfo);
  iEvent.put(MuonTagInfoCollection);
}

