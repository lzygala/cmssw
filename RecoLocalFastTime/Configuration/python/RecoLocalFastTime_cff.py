import FWCore.ParameterSet.Config as cms

from RecoLocalFastTime.FTLRecProducers.mtdUncalibratedRecHits_cfi import mtdUncalibratedRecHits
from RecoLocalFastTime.FTLRecProducers.mtdRecHits_cfi import mtdRecHits
from RecoLocalFastTime.FTLRecProducers.mtdTrackingRecHits_cfi import mtdTrackingRecHits
from RecoLocalFastTime.FTLClusterizer.mtdClusters_cfi import mtdClusters

from RecoLocalFastTime.FTLClusterizer.MTDCPEESProducers_cff import *
from RecoLocalFastTime.FTLRecProducers.MTDTimeCalibESProducers_cff import *

fastTimingLocalRecoTask = cms.Task(mtdUncalibratedRecHits,mtdRecHits,mtdClusters,mtdTrackingRecHits)
fastTimingLocalReco = cms.Sequence(fastTimingLocalRecoTask)
