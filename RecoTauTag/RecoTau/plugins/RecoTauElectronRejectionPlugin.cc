/*
 * ===========================================================================
 *
 *       Filename:  RecoTauElectronRejectionPlugin.cc
 *
 *    Description:  Add electron rejection information to PFTau
 *
 *         Authors:  Chi Nhan Nguyen, Simone Gennai, Evan Friis
 *
 * ===========================================================================
 */

#include "RecoTauTag/RecoTau/interface/RecoTauBuilderPlugins.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/ParticleFlowReco/interface/PFBlockElement.h"
#include "DataFormats/ParticleFlowReco/interface/PFBlock.h"
#include "DataFormats/ParticleFlowReco/interface/PFCluster.h"
#include <Math/VectorUtil.h>
#include <algorithm>

namespace reco { namespace tau {

class RecoTauElectronRejectionPlugin : public RecoTauModifierPlugin {
  public:
  explicit RecoTauElectronRejectionPlugin(const edm::ParameterSet& pset, edm::ConsumesCollector && iC);
    ~RecoTauElectronRejectionPlugin() override {}
    void operator()(PFTau&) const override;
  private:
    double ElecPreIDLeadTkMatch_maxDR_;
    double EcalStripSumE_minClusEnergy_;
    double EcalStripSumE_deltaEta_;
    double EcalStripSumE_deltaPhiOverQ_minValue_;
    double EcalStripSumE_deltaPhiOverQ_maxValue_;
    double maximumForElectrionPreIDOutput_;
    std::string DataType_;
};

RecoTauElectronRejectionPlugin::RecoTauElectronRejectionPlugin(
  const edm::ParameterSet& pset, edm::ConsumesCollector && iC):RecoTauModifierPlugin(pset,std::move(iC)) {
  // Load parameters
  ElecPreIDLeadTkMatch_maxDR_ =
    pset.getParameter<double>("ElecPreIDLeadTkMatch_maxDR");
  EcalStripSumE_minClusEnergy_ =
    pset.getParameter<double>("EcalStripSumE_minClusEnergy");
  EcalStripSumE_deltaEta_ =
    pset.getParameter<double>("EcalStripSumE_deltaEta");
  EcalStripSumE_deltaPhiOverQ_minValue_ =
    pset.getParameter<double>("EcalStripSumE_deltaPhiOverQ_minValue");
  EcalStripSumE_deltaPhiOverQ_maxValue_ =
    pset.getParameter<double>("EcalStripSumE_deltaPhiOverQ_maxValue");
  maximumForElectrionPreIDOutput_ =
    pset.getParameter<double>("maximumForElectrionPreIDOutput");
  DataType_ = pset.getParameter<std::string>("DataType");
}

namespace {
bool checkPos(std::vector<math::XYZPoint> CalPos,math::XYZPoint CandPos) {
  bool flag = false;
  for (unsigned int i=0;i<CalPos.size();i++) {
    if (CalPos[i] == CandPos) {
      flag = true;
      break;
    }
  }
  return flag;
}
}

void RecoTauElectronRejectionPlugin::operator()(PFTau& tau) const {
  // copy pasted from PFRecoTauAlgorithm...
  double myECALenergy             =  0.;
  double myHCALenergy             =  0.;
  double myHCALenergy3x3          =  0.;
  double myMaximumHCALPFClusterE  =  0.;
  double myMaximumHCALPFClusterEt =  0.;
  double myStripClusterE          =  0.;
  double myEmfrac                 = -1.;
  double myElectronPreIDOutput    = -1111.;
  bool   myElecPreid              =  false;
  reco::TrackRef myElecTrk;

  typedef std::pair<reco::PFBlockRef, unsigned> ElementInBlock;
  typedef std::vector< ElementInBlock > ElementsInBlocks;

  CandidatePtr myleadChargedCand = tau.leadPFChargedHadrCand();
  // Build list of PFCands in tau
  std::vector<CandidatePtr> myPFCands;
  myPFCands.reserve(tau.isolationPFCands().size()+tau.signalPFCands().size());

  std::copy(tau.isolationPFCands().begin(), tau.isolationPFCands().end(),
      std::back_inserter(myPFCands));
  std::copy(tau.signalPFCands().begin(), tau.signalPFCands().end(),
      std::back_inserter(myPFCands));

  //Use the electron rejection only in case there is a charged leading pion
  if(myleadChargedCand.isNonnull()){
    const reco::PFCandidate* myleadPFChargedCand = dynamic_cast<const reco::PFCandidate*>(myleadChargedCand.get());
    if (myleadPFChargedCand == nullptr)
    	throw cms::Exception("Type Mismatch") << "The PFTau was not made from PFCandidates, and this outdated algorithm was not updated to cope with PFTaus made from other Candidates.\n";
    myElectronPreIDOutput = myleadPFChargedCand->mva_e_pi();

    math::XYZPointF myElecTrkEcalPos = myleadPFChargedCand->positionAtECALEntrance();
    myElecTrk = myleadPFChargedCand->trackRef();//Electron candidate

    if(myElecTrk.isNonnull()) {
      //FROM AOD
      if(DataType_ == "AOD"){
        // Corrected Cluster energies
        for(const auto& cand : myPFCands){
          const reco::PFCandidate* pfcand = dynamic_cast<const reco::PFCandidate*>(cand.get());
          if (pfcand == nullptr) {
            throw cms::Exception("Type Mismatch") << "The PFTau was not made from PFCandidates, and this outdated algorithm was not updated to cope with PFTaus made from other Candidates.\n";
          }
          myHCALenergy += pfcand->hcalEnergy();
          myECALenergy += pfcand->ecalEnergy();

          math::XYZPointF candPos;
          if (pfcand->particleId()==1 || pfcand->particleId()==2)//if charged hadron or electron
            candPos = pfcand->positionAtECALEntrance();
          else
            candPos = math::XYZPointF(pfcand->px(),pfcand->py(),pfcand->pz());

          double deltaR   = ROOT::Math::VectorUtil::DeltaR(myElecTrkEcalPos,candPos);
          double deltaPhi = ROOT::Math::VectorUtil::DeltaPhi(myElecTrkEcalPos,candPos);
          double deltaEta = std::abs(myElecTrkEcalPos.eta()-candPos.eta());
          double deltaPhiOverQ = deltaPhi/(double)myElecTrk->charge();

          if (pfcand->ecalEnergy() >= EcalStripSumE_minClusEnergy_ && deltaEta < EcalStripSumE_deltaEta_ &&
              deltaPhiOverQ > EcalStripSumE_deltaPhiOverQ_minValue_  && deltaPhiOverQ < EcalStripSumE_deltaPhiOverQ_maxValue_) {
            myStripClusterE += pfcand->ecalEnergy();
          }
          if (deltaR<0.184) {
            myHCALenergy3x3 += pfcand->hcalEnergy();
          }
          if (pfcand->hcalEnergy()>myMaximumHCALPFClusterE) {
            myMaximumHCALPFClusterE = pfcand->hcalEnergy();
          }
          if ((pfcand->hcalEnergy()*fabs(sin(candPos.Theta())))>myMaximumHCALPFClusterEt) {
            myMaximumHCALPFClusterEt = (pfcand->hcalEnergy()*fabs(sin(candPos.Theta())));
          }
        }

      } else if(DataType_ == "RECO"){ //From RECO
        // Against double counting of clusters
        std::vector<math::XYZPoint> hcalPosV; hcalPosV.clear();
        std::vector<math::XYZPoint> ecalPosV; ecalPosV.clear();
        for(const auto& cand : myPFCands){
          const reco::PFCandidate* pfcand = dynamic_cast<const reco::PFCandidate*>(cand.get());
          if (pfcand == nullptr) {
            throw cms::Exception("Type Mismatch") << "The PFTau was not made from PFCandidates, and this outdated algorithm was not updated to cope with PFTaus made from other Candidates.\n";
          }
          const ElementsInBlocks& elts = pfcand->elementsInBlocks();
          for(ElementsInBlocks::const_iterator it=elts.begin(); it!=elts.end(); ++it) {
            const reco::PFBlock& block = *(it->first);
            unsigned indexOfElementInBlock = it->second;
            const edm::OwnVector< reco::PFBlockElement >& elements = block.elements();
            assert(indexOfElementInBlock<elements.size());

            const reco::PFBlockElement& element = elements[indexOfElementInBlock];

            if(element.type()==reco::PFBlockElement::HCAL) {
              math::XYZPoint clusPos = element.clusterRef()->position();
              double en = (double)element.clusterRef()->energy();
              double et = (double)element.clusterRef()->energy()*fabs(sin(clusPos.Theta()));
              if (en>myMaximumHCALPFClusterE) {
                myMaximumHCALPFClusterE = en;
              }
              if (et>myMaximumHCALPFClusterEt) {
                myMaximumHCALPFClusterEt = et;
              }
              if (!checkPos(hcalPosV,clusPos)) {
                hcalPosV.push_back(clusPos);
                myHCALenergy += en;
                double deltaR = ROOT::Math::VectorUtil::DeltaR(myElecTrkEcalPos,clusPos);
                if (deltaR<0.184) {
                  myHCALenergy3x3 += en;
                }
              }
            } else if(element.type()==reco::PFBlockElement::ECAL) {
              double en = (double)element.clusterRef()->energy();
              math::XYZPoint clusPos = element.clusterRef()->position();
              if (!checkPos(ecalPosV,clusPos)) {
                ecalPosV.push_back(clusPos);
                myECALenergy += en;
                double deltaPhi = ROOT::Math::VectorUtil::DeltaPhi(myElecTrkEcalPos,clusPos);
                double deltaEta = std::abs(myElecTrkEcalPos.eta()-clusPos.eta());
                double deltaPhiOverQ = deltaPhi/(double)myElecTrk->charge();
                if (en >= EcalStripSumE_minClusEnergy_ && deltaEta<EcalStripSumE_deltaEta_ && deltaPhiOverQ > EcalStripSumE_deltaPhiOverQ_minValue_ && deltaPhiOverQ < EcalStripSumE_deltaPhiOverQ_maxValue_) {
                  myStripClusterE += en;
                }
              }
            }
          } //end elements in blocks
        } //end loop over PFcands
      } //end RECO case
    } // end check for null electrk
  } // end check for null pfChargedHadrCand

  if ((myHCALenergy+myECALenergy)>0.)
    myEmfrac = myECALenergy/(myHCALenergy+myECALenergy);
  tau.setemFraction((float)myEmfrac);

  // scale the appropriate quantities by the momentum of the electron if it exists
  if (myElecTrk.isNonnull())
  {
    float myElectronMomentum = (float)myElecTrk->p();
    if (myElectronMomentum > 0.)
    {
      myHCALenergy            /= myElectronMomentum;
      myMaximumHCALPFClusterE /= myElectronMomentum;
      myHCALenergy3x3         /= myElectronMomentum;
      myStripClusterE         /= myElectronMomentum;
    }
  }
  tau.sethcalTotOverPLead((float)myHCALenergy);
  tau.sethcalMaxOverPLead((float)myMaximumHCALPFClusterE);
  tau.sethcal3x3OverPLead((float)myHCALenergy3x3);
  tau.setecalStripSumEOverPLead((float)myStripClusterE);
  tau.setmaximumHCALPFClusterEt(myMaximumHCALPFClusterEt);
  tau.setelectronPreIDOutput(myElectronPreIDOutput);
  if (myElecTrk.isNonnull())
    tau.setelectronPreIDTrack(myElecTrk);
  if (myElectronPreIDOutput > maximumForElectrionPreIDOutput_)
    myElecPreid = true;
  tau.setelectronPreIDDecision(myElecPreid);

  // These need to be filled!
  //tau.setbremsRecoveryEOverPLead(my...);

  /* End elecron rejection */
}
}} // end namespace reco::tau
#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_EDM_PLUGIN(RecoTauModifierPluginFactory,
    reco::tau::RecoTauElectronRejectionPlugin,
    "RecoTauElectronRejectionPlugin");
