import FWCore.ParameterSet.Config as cms

softPFMuonsTagInfos = cms.EDProducer("SoftPFMuonTagInfoProducer",
  jets              = cms.InputTag("ak4PFJetsCHS"),
  muons             = cms.InputTag("muons"),
  vertex            = cms.InputTag("offlinePrimaryVertices"),
  muonPt            = cms.double(2.),
	filterIp          = cms.double(3.),
	filterRatio1      = cms.double(0.4),
	filterRatio2      = cms.double(0.7),
	dRcut             = cms.double(0.4), # Effective only for MINIAOD
  filterPromptMuons = cms.bool(True),
  runMINIAOD        = cms.bool(True)
)
