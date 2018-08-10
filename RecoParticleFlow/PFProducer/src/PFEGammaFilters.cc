// 
// Original Authors: Nicholas Wardle, Florian Beaudette
//
#include "RecoParticleFlow/PFProducer/interface/PFEGammaFilters.h"
#include "RecoParticleFlow/PFTracking/interface/PFTrackAlgoTools.h"
#include "DataFormats/GsfTrackReco/interface/GsfTrack.h"
#include "DataFormats/ParticleFlowReco/interface/PFBlock.h"
#include "DataFormats/ParticleFlowReco/interface/PFBlockElement.h"

using namespace std;
using namespace reco;

PFEGammaFilters::PFEGammaFilters(float ph_Et,
				 float ph_combIso,
				 float ph_loose_hoe,
				 float ph_sietaieta_eb,
				 float ph_sietaieta_ee,
				 const edm::ParameterSet& ph_protectionsForJetMET,
				 const edm::ParameterSet& ph_protectionsForBadHcal,
				 float ele_iso_pt,
				 float ele_iso_mva_eb,
				 float ele_iso_mva_ee,
				 float ele_iso_combIso_eb,
				 float ele_iso_combIso_ee,
				 float ele_noniso_mva,
				 unsigned int ele_missinghits,
				 const string& ele_iso_path_mvaWeightFile,
				 const edm::ParameterSet& ele_protectionsForJetMET,
		                 const edm::ParameterSet& ele_protectionsForBadHcal
				 ):
  ph_Et_(ph_Et),
  ph_combIso_(ph_combIso),
  ph_loose_hoe_(ph_loose_hoe),
  ph_sietaieta_eb_(ph_sietaieta_eb),
  ph_sietaieta_ee_(ph_sietaieta_ee),
  pho_sumPtTrackIso(ph_protectionsForJetMET.getParameter<double>("sumPtTrackIso")), 
  pho_sumPtTrackIsoSlope(ph_protectionsForJetMET.getParameter<double>("sumPtTrackIsoSlope")),
  ele_iso_pt_(ele_iso_pt),
  ele_iso_mva_eb_(ele_iso_mva_eb),
  ele_iso_mva_ee_(ele_iso_mva_ee),
  ele_iso_combIso_eb_(ele_iso_combIso_eb),
  ele_iso_combIso_ee_(ele_iso_combIso_ee),
  ele_noniso_mva_(ele_noniso_mva),
  ele_missinghits_(ele_missinghits),
  ele_maxNtracks(ele_protectionsForJetMET.getParameter<double>("maxNtracks")), 
  ele_maxHcalE(ele_protectionsForJetMET.getParameter<double>("maxHcalE")), 
  ele_maxTrackPOverEele(ele_protectionsForJetMET.getParameter<double>("maxTrackPOverEele")), 
  ele_maxE(ele_protectionsForJetMET.getParameter<double>("maxE")),
  ele_maxEleHcalEOverEcalE(ele_protectionsForJetMET.getParameter<double>("maxEleHcalEOverEcalE")),
  ele_maxEcalEOverPRes(ele_protectionsForJetMET.getParameter<double>("maxEcalEOverPRes")), 
  ele_maxEeleOverPoutRes(ele_protectionsForJetMET.getParameter<double>("maxEeleOverPoutRes")),
  ele_maxHcalEOverP(ele_protectionsForJetMET.getParameter<double>("maxHcalEOverP")), 
  ele_maxHcalEOverEcalE(ele_protectionsForJetMET.getParameter<double>("maxHcalEOverEcalE")), 
  ele_maxEcalEOverP_1(ele_protectionsForJetMET.getParameter<double>("maxEcalEOverP_1")),
  ele_maxEcalEOverP_2(ele_protectionsForJetMET.getParameter<double>("maxEcalEOverP_2")), 
  ele_maxEeleOverPout(ele_protectionsForJetMET.getParameter<double>("maxEeleOverPout")), 
  ele_maxDPhiIN(ele_protectionsForJetMET.getParameter<double>("maxDPhiIN")),
  badHcal_eleEnable_(ele_protectionsForBadHcal.getParameter<bool>("enableProtections")),
  badHcal_phoTrkSolidConeIso_offs_(ph_protectionsForBadHcal.getParameter<double>("solidConeTrkIsoOffset")),
  badHcal_phoTrkSolidConeIso_slope_(ph_protectionsForBadHcal.getParameter<double>("solidConeTrkIsoSlope")),
  badHcal_phoEnable_(ph_protectionsForBadHcal.getParameter<bool>("enableProtections")),
  debug_(false)
{
    readEBEEParams_(ele_protectionsForBadHcal, "full5x5_sigmaIetaIeta", badHcal_full5x5_sigmaIetaIeta_);
    readEBEEParams_(ele_protectionsForBadHcal, "eInvPInv", badHcal_eInvPInv_);
    readEBEEParams_(ele_protectionsForBadHcal, "dEta", badHcal_dEta_);
    readEBEEParams_(ele_protectionsForBadHcal, "dPhi", badHcal_dPhi_);
}

void PFEGammaFilters::readEBEEParams_(const edm::ParameterSet &pset, const std::string &name, std::array<float,2> & out) {
    const auto & vals = pset.getParameter<std::vector<double>>(name);
    if (vals.size() != 2) throw cms::Exception("Configuration") << "Parameter " << name << " does not contain exactly 2 values (EB, EE)\n";
    out[0] = vals[0]; 
    out[1] = vals[1]; 
}
 

bool PFEGammaFilters::passPhotonSelection(const reco::Photon & photon) {
  // First simple selection, same as the Run1 to be improved in CMSSW_710


  // Photon ET
  if(photon.pt()  < ph_Et_ ) return false;
  bool validHoverE = photon.hadTowOverEmValid();
  if (debug_) std::cout<< "PFEGammaFilters:: photon pt " << photon.pt()
		     << "   eta, phi " << photon.eta() << ", " << photon.phi() 
		     << "   isoDr03 " << (photon.trkSumPtHollowConeDR03()+photon.ecalRecHitSumEtConeDR03()+photon.hcalTowerSumEtConeDR03())  << " (cut: " << ph_combIso_ << ")"
		     << "   H/E " << photon.hadTowOverEm() << " (valid? " << validHoverE << ", cut: " << ph_loose_hoe_ << ")"
		     << "   s(ieie) " << photon.sigmaIetaIeta()  << " (cut: " << (photon.isEB() ? ph_sietaieta_eb_ : ph_sietaieta_ee_) << ")"
		     << "   isoTrkDr03Solid " << (photon.trkSumPtSolidConeDR03())  << " (cut: " << (validHoverE || !badHcal_phoEnable_ ? -1 : badHcal_phoTrkSolidConeIso_offs_ + badHcal_phoTrkSolidConeIso_slope_*photon.pt()) << ")"
 << std::endl;

  if (photon.hadTowOverEm() >ph_loose_hoe_ ) return false;
  //Isolation variables in 0.3 cone combined
  if(photon.trkSumPtHollowConeDR03()+photon.ecalRecHitSumEtConeDR03()+photon.hcalTowerSumEtConeDR03() > ph_combIso_)
    return false;

  //patch for bad hcal
  if (!validHoverE && badHcal_phoEnable_ && photon.trkSumPtSolidConeDR03() > badHcal_phoTrkSolidConeIso_offs_ + badHcal_phoTrkSolidConeIso_slope_*photon.pt()) {
    return false;
  }
  
  if(photon.isEB()) {
    if(photon.sigmaIetaIeta() > ph_sietaieta_eb_) 
      return false;
  }
  else {
    if(photon.sigmaIetaIeta() > ph_sietaieta_ee_) 
      return false;
  }


  return true;
}

bool PFEGammaFilters::passElectronSelection(const reco::GsfElectron & electron,
					    const reco::PFCandidate & pfcand, 
					    const int & nVtx) {
  // First simple selection, same as the Run1 to be improved in CMSSW_710
 
  bool validHoverE = electron.hcalOverEcalValid();
  if (debug_) std::cout << "PFEGammaFilters:: Electron pt " << electron.pt()
		     << " eta, phi " << electron.eta() << ", " << electron.phi() 
		     << " charge " << electron.charge() 
		     << " isoDr03 " << (electron.dr03TkSumPt() + electron.dr03EcalRecHitSumEt() + electron.dr03HcalTowerSumEt()) 
		     << " mva_isolated " << electron.mva_Isolated() 
		     << " mva_e_pi " << electron.mva_e_pi() 
		     << " H/E_valid " << validHoverE 
		     << " s(ieie) " << electron.full5x5_sigmaIetaIeta()
		     << " H/E " << electron.hcalOverEcal()
		     << " 1/e-1/p " << (1.0-electron.eSuperClusterOverP())/electron.ecalEnergy()
		     << " deta " << std::abs(electron.deltaEtaSeedClusterTrackAtVtx())
		     << " dphi " << std::abs(electron.deltaPhiSuperClusterTrackAtVtx()) 
		     << endl;

  bool passEleSelection = false;
  
  // Electron ET
  float electronPt = electron.pt();
  
  if( electronPt > ele_iso_pt_) {

    double isoDr03 = electron.dr03TkSumPt() + electron.dr03EcalRecHitSumEt() + electron.dr03HcalTowerSumEt();
    double eleEta = fabs(electron.eta());
    if (eleEta <= 1.485 && isoDr03 < ele_iso_combIso_eb_) {
      if( electron.mva_Isolated() > ele_iso_mva_eb_ ) 
	passEleSelection = true;
    }
    else if (eleEta > 1.485  && isoDr03 < ele_iso_combIso_ee_) {
      if( electron.mva_Isolated() > ele_iso_mva_ee_ ) 
	passEleSelection = true;
    }

  }

  //  cout << " My OLD MVA " << pfcand.mva_e_pi() << " MyNEW MVA " << electron.mva() << endl;
  if(electron.mva_e_pi() > ele_noniso_mva_) {
    if (validHoverE || !badHcal_eleEnable_) {
        passEleSelection = true; 
    } else {
        bool EE = (std::abs(electron.eta()) > 1.485); // for prefer consistency with above than with E/gamma for now
        if ((electron.full5x5_sigmaIetaIeta() < badHcal_full5x5_sigmaIetaIeta_[EE]) &&
            (std::abs(1.0-electron.eSuperClusterOverP())/electron.ecalEnergy()  < badHcal_eInvPInv_[EE]) && 
            (std::abs(electron.deltaEtaSeedClusterTrackAtVtx())  < badHcal_dEta_[EE]) && // looser in case of misalignment
            (std::abs(electron.deltaPhiSuperClusterTrackAtVtx())  < badHcal_dPhi_[EE])) {
            passEleSelection = true; 
        } 
    }
  }
  
  return passEleSelection;
}

bool PFEGammaFilters::isElectron(const reco::GsfElectron & electron) {
 
  unsigned int nmisshits = electron.gsfTrack()->hitPattern().numberOfLostHits(reco::HitPattern::MISSING_INNER_HITS);
  if(nmisshits > ele_missinghits_)
    return false;

  return true;
}


bool PFEGammaFilters::isElectronSafeForJetMET(const reco::GsfElectron & electron, 
					      const reco::PFCandidate & pfcand,
					      const reco::Vertex & primaryVertex,
					      bool& lockTracks) {

  bool debugSafeForJetMET = false;
  bool isSafeForJetMET = true;

//   cout << " ele_maxNtracks " <<  ele_maxNtracks << endl
//        << " ele_maxHcalE " << ele_maxHcalE << endl
//        << " ele_maxTrackPOverEele " << ele_maxTrackPOverEele << endl
//        << " ele_maxE " << ele_maxE << endl
//        << " ele_maxEleHcalEOverEcalE "<< ele_maxEleHcalEOverEcalE << endl
//        << " ele_maxEcalEOverPRes " << ele_maxEcalEOverPRes << endl
//        << " ele_maxEeleOverPoutRes "  << ele_maxEeleOverPoutRes << endl
//        << " ele_maxHcalEOverP " << ele_maxHcalEOverP << endl
//        << " ele_maxHcalEOverEcalE " << ele_maxHcalEOverEcalE << endl
//        << " ele_maxEcalEOverP_1 " << ele_maxEcalEOverP_1 << endl
//        << " ele_maxEcalEOverP_2 " << ele_maxEcalEOverP_2 << endl
//        << " ele_maxEeleOverPout "  << ele_maxEeleOverPout << endl
//        << " ele_maxDPhiIN " << ele_maxDPhiIN << endl;
    
  // loop on the extra-tracks associated to the electron
  PFCandidateEGammaExtraRef pfcandextra = pfcand.egammaExtraRef();
  
  unsigned int iextratrack = 0;
  unsigned int itrackHcalLinked = 0;
  float SumExtraKfP = 0.;
  //float Ene_ecalgsf = 0.;


  // problems here: for now get the electron cluster from the gsf electron
  //  const PFCandidate::ElementsInBlocks& eleCluster = pfcandextra->gsfElectronClusterRef();
  // PFCandidate::ElementsInBlocks::const_iterator iegfirst = eleCluster.begin(); 
  //  float Ene_hcalgsf = pfcandextra->


  float ETtotal = electron.ecalEnergy();

  //NOTE take this from EGammaExtra
  float Ene_ecalgsf = electron.electronCluster()->energy();
  float Ene_hcalgsf = pfcandextra->hadEnergy();
  float HOverHE = Ene_hcalgsf/(Ene_hcalgsf + Ene_ecalgsf);
  float EtotPinMode = electron.eSuperClusterOverP();

  //NOTE take this from EGammaExtra
  float EGsfPoutMode = electron.eEleClusterOverPout();
  float HOverPin = Ene_hcalgsf / electron.gsfTrack()->pMode();
  float dphi_normalsc = electron.deltaPhiSuperClusterTrackAtVtx();


  const PFCandidate::ElementsInBlocks& extraTracks = pfcandextra->extraNonConvTracks();
  for (PFCandidate::ElementsInBlocks::const_iterator itrk = extraTracks.begin(); 
       itrk<extraTracks.end(); ++itrk) {
    const PFBlock& block = *(itrk->first);
    const PFBlock::LinkData& linkData =  block.linkData();
    const PFBlockElement& pfele = block.elements()[itrk->second];

    if(debugSafeForJetMET) 
      cout << " My track element number " <<  itrk->second << endl;
    if(pfele.type()==reco::PFBlockElement::TRACK) {
      const reco::TrackRef& trackref = pfele.trackRef();

      bool goodTrack = PFTrackAlgoTools::isGoodForEGM(trackref->algo());
      // iter0, iter1, iter2, iter3 = Algo < 3
      // algo 4,5,6,7
      int nexhits = trackref->hitPattern().numberOfLostHits(reco::HitPattern::MISSING_INNER_HITS); 
      
      bool trackIsFromPrimaryVertex = false;
      for (Vertex::trackRef_iterator trackIt = primaryVertex.tracks_begin(); trackIt != primaryVertex.tracks_end(); ++trackIt) {
	if ( (*trackIt).castTo<TrackRef>() == trackref ) {
	  trackIsFromPrimaryVertex = true;
	  break;
	}
      }
      
      // probably we could now remove the algo request?? 
      if(goodTrack && nexhits == 0 && trackIsFromPrimaryVertex) {
	float p_trk = trackref->p();
	SumExtraKfP += p_trk;
	iextratrack++;
	// Check if these extra tracks are HCAL linked
	std::multimap<double, unsigned int> hcalKfElems;
	block.associatedElements( itrk->second,
				  linkData,
				  hcalKfElems,
				  reco::PFBlockElement::HCAL,
				  reco::PFBlock::LINKTEST_ALL );
	if(!hcalKfElems.empty()) {
	  itrackHcalLinked++;
	}
	if(debugSafeForJetMET) 
	  cout << " The ecalGsf cluster is not isolated: >0 KF extra with algo < 3" 
	       << " Algo " << trackref->algo()
	       << " nexhits " << nexhits
	       << " trackIsFromPrimaryVertex " << trackIsFromPrimaryVertex << endl;
	if(debugSafeForJetMET) 
	  cout << " My track PT " << trackref->pt() << endl;

      }
      else {
	if(debugSafeForJetMET) 
	  cout << " Tracks from PU " 
	       << " Algo " << trackref->algo()
	       << " nexhits " << nexhits
	       << " trackIsFromPrimaryVertex " << trackIsFromPrimaryVertex << endl;
	if(debugSafeForJetMET) 
	  cout << " My track PT " << trackref->pt() << endl;
      }
    }
  }
  if( iextratrack > 0) {
    if(iextratrack > ele_maxNtracks || Ene_hcalgsf > ele_maxHcalE || (SumExtraKfP/Ene_ecalgsf) > ele_maxTrackPOverEele 
       || (ETtotal > ele_maxE && iextratrack > 1 && (Ene_hcalgsf/Ene_ecalgsf) > ele_maxEleHcalEOverEcalE) ) {
      if(debugSafeForJetMET) 
	cout << " *****This electron candidate is discarded: Non isolated  # tracks "		
	     << iextratrack << " HOverHE " << HOverHE 
	     << " SumExtraKfP/Ene_ecalgsf " << SumExtraKfP/Ene_ecalgsf 
	     << " SumExtraKfP " << SumExtraKfP 
	     << " Ene_ecalgsf " << Ene_ecalgsf
	     << " ETtotal " << ETtotal
	     << " Ene_hcalgsf/Ene_ecalgsf " << Ene_hcalgsf/Ene_ecalgsf
	     << endl;
      
      isSafeForJetMET = false;
    }
    // the electron is retained and the kf tracks are not locked
    if( (fabs(1.-EtotPinMode) < ele_maxEcalEOverPRes && (fabs(electron.eta()) < 1.0 || fabs(electron.eta()) > 2.0)) ||
	((EtotPinMode < 1.1 && EtotPinMode > 0.6) && (fabs(electron.eta()) >= 1.0 && fabs(electron.eta()) <= 2.0))) {
      if( fabs(1.-EGsfPoutMode) < ele_maxEeleOverPoutRes && 
	  (itrackHcalLinked == iextratrack)) {

	lockTracks = false;
	//	lockExtraKf = false;
	if(debugSafeForJetMET) 
	  cout << " *****This electron is reactivated  # tracks "		
	       << iextratrack << " #tracks hcal linked " << itrackHcalLinked 
	       << " SumExtraKfP/Ene_ecalgsf " << SumExtraKfP/Ene_ecalgsf  
	       << " EtotPinMode " << EtotPinMode << " EGsfPoutMode " << EGsfPoutMode 
	       << " eta gsf " << electron.eta()  <<endl;
      }
    }
  }

  if (HOverPin > ele_maxHcalEOverP && HOverHE > ele_maxHcalEOverEcalE && EtotPinMode < ele_maxEcalEOverP_1) {
    if(debugSafeForJetMET) 
      cout << " *****This electron candidate is discarded  HCAL ENERGY "	
	   << " HOverPin " << HOverPin << " HOverHE " << HOverHE  << " EtotPinMode" << EtotPinMode << endl;
    isSafeForJetMET = false;
  }
  
  // Reject Crazy E/p values... to be understood in the future how to train a 
  // BDT in order to avoid to select this bad electron candidates. 
  
  if( EtotPinMode < ele_maxEcalEOverP_2 && EGsfPoutMode < ele_maxEeleOverPout ) {
    if(debugSafeForJetMET) 
      cout << " *****This electron candidate is discarded  Low ETOTPIN "
	   << " EtotPinMode " << EtotPinMode << " EGsfPoutMode " << EGsfPoutMode << endl;
    isSafeForJetMET = false;
  }
  
  // For not-preselected Gsf Tracks ET > 50 GeV, apply dphi preselection
  if(ETtotal > ele_maxE && fabs(dphi_normalsc) > ele_maxDPhiIN ) {
    if(debugSafeForJetMET) 
      cout << " *****This electron candidate is discarded  Large ANGLE "
	   << " ETtotal " << ETtotal << " EGsfPoutMode " << dphi_normalsc << endl;
    isSafeForJetMET = false;
  }



  return isSafeForJetMET;
}
bool PFEGammaFilters::isPhotonSafeForJetMET(const reco::Photon & photon, const reco::PFCandidate & pfcand) {

  bool isSafeForJetMET = true;
  bool debugSafeForJetMET = false;

//   cout << " pho_sumPtTrackIsoForPhoton " << pho_sumPtTrackIso
//        << " pho_sumPtTrackIsoSlopeForPhoton " << pho_sumPtTrackIsoSlope <<  endl;

  float sum_track_pt = 0.;

  PFCandidateEGammaExtraRef pfcandextra = pfcand.egammaExtraRef();
  const PFCandidate::ElementsInBlocks& extraTracks = pfcandextra->extraNonConvTracks();
  for (PFCandidate::ElementsInBlocks::const_iterator itrk = extraTracks.begin(); 
       itrk<extraTracks.end(); ++itrk) {
    const PFBlock& block = *(itrk->first);
    const PFBlockElement& pfele = block.elements()[itrk->second];
 
    
    if(pfele.type()==reco::PFBlockElement::TRACK) {

     
      const reco::TrackRef& trackref = pfele.trackRef();
      
      if(debugSafeForJetMET)
	cout << "PFEGammaFilters::isPhotonSafeForJetMET photon track:pt " << trackref->pt() << " SingleLegSize " << pfcandextra->singleLegConvTrackRefMva().size() << endl;
   
      
      //const std::vector<reco::TrackRef>&  mySingleLeg = 
      bool singleLegConv = false;
      for(unsigned int iconv =0; iconv<pfcandextra->singleLegConvTrackRefMva().size(); iconv++) {
	if(debugSafeForJetMET)
	  cout << "PFEGammaFilters::SingleLeg track:pt " << (pfcandextra->singleLegConvTrackRefMva()[iconv].first)->pt() << endl;
	
	if(pfcandextra->singleLegConvTrackRefMva()[iconv].first == trackref) {
	  singleLegConv = true;
	  if(debugSafeForJetMET)
	    cout << "PFEGammaFilters::isPhotonSafeForJetMET: SingleLeg conv track " << endl;
	  break;

	}
      }
      if(singleLegConv)
	continue;
      
      sum_track_pt += trackref->pt();

    }

  }

  if(debugSafeForJetMET)
    cout << " PFEGammaFilters::isPhotonSafeForJetMET: SumPt " << sum_track_pt << endl;

  if(sum_track_pt>(pho_sumPtTrackIso + pho_sumPtTrackIsoSlope * photon.pt())) {
    isSafeForJetMET = false;
    if(debugSafeForJetMET)
      cout << "************************************!!!! PFEGammaFilters::isPhotonSafeForJetMET: Photon Discaded !!! " << endl;
  }

  return isSafeForJetMET;
}

