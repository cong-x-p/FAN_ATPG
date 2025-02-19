// **************************************************************************
// File       [ atpg.cpp ]
// Author     [ littleshamoo ]
// Synopsis   [ This files include most of the method of class Atpg ]
// Date       [ 2011/11/01 created ]
// **************************************************************************

#include "atpg.h"
#include <algorithm>

#include <mutex>
#include "unistd.h"

using namespace CoreNs;
std::mutex mtx;

// test
void Atpg::parallelBalanceStuckAtFaultATPG(FaultPtrList* faultPtrListForGen, PatternProcessor* pPatternProcessor, int& numOfAtpgUntestableFaults) {
    Fault* pCurrentFault = NULL;
    FaultPtrList faultPtrListForGenTemp;
    for (Fault* fault : *faultPtrListForGen) {
        faultPtrListForGenTemp.push_back(fault);
    }
    while (!faultPtrListForGenTemp.empty()) {
        if (faultPtrListForGenTemp.front()->faultState_ == Fault::AB) {
            break;
        }

        if (pCurrentFault == faultPtrListForGenTemp.front()) {
            faultPtrListForGenTemp.front()->faultState_ = Fault::DT;
            faultPtrListForGenTemp.pop_front();
            continue;
        }

        pCurrentFault = faultPtrListForGenTemp.front();

        SINGLE_PATTERN_GENERATION_STATUS result = generateSinglePatternOnTargetFault(*(faultPtrListForGenTemp.front()), false);
        if (result == PATTERN_FOUND) {
            Pattern pattern(pCircuit_);
            pPatternProcessor->patternVector_.push_back(pattern);

            resetPrevAtpgValStored();
            clearAllFaultEffectByEvaluation();
            storeCurrentAtpgVal();
            writeAtpgValToPatternPI(pPatternProcessor->patternVector_.back());

            if (pPatternProcessor->dynamicCompression_ == PatternProcessor::ON) {
                FaultPtrList faultListTemp = faultPtrListForGenTemp;
                pSimulator_->parallelFaultFaultSimWithOnePattern(pPatternProcessor->patternVector_.back(), faultPtrListForGenTemp);
                pSimulator_->goodSim();
                writeGoodSimValToPatternPO(pPatternProcessor->patternVector_.back());

                for (Fault* pFault : faultListTemp) {
                    // skip detected faults
                    if (pFault->faultState_ == Fault::DT) {
                        continue;
                    }

                    Gate* pGateForActivation = getGateForFaultActivation(*pFault);
                    if (((pGateForActivation->atpgVal_ == L) && (pFault->faultType_ == Fault::SA0)) ||
                        ((pGateForActivation->atpgVal_ == H) && (pFault->faultType_ == Fault::SA1))) {
                        continue;
                    }

                    // Activation check
                    if (pGateForActivation->atpgVal_ != X) {
                        if ((pFault->faultType_ == Fault::SA0) || (pFault->faultType_ == Fault::SA1)) {
                            setGateAtpgValAndRunImplication((*pGateForActivation), X);
                        } else {
                            continue;
                        }
                    }

                    if (xPathExists(pGateForActivation)) {
                        // TO-DO homework 05 implement DTC here end of TO-DO
                        if (generateSinglePatternOnTargetFault(*pFault, true) == PATTERN_FOUND) {
                            resetPrevAtpgValStored();
                            clearAllFaultEffectByEvaluation();
                            storeCurrentAtpgVal();
                            writeAtpgValToPatternPI(pPatternProcessor->patternVector_.back());
                        } else {
                            for (Gate& gate : pCircuit_->circuitGates_) {
                                gate.atpgVal_ = gate.prevAtpgValStored_;
                            }
                        }
                    } else {
                        setGateAtpgValAndRunImplication((*pGateForActivation), pGateForActivation->prevAtpgValStored_);
                    }
                }
            }

            clearAllFaultEffectByEvaluation();
            storeCurrentAtpgVal();
            writeAtpgValToPatternPI(pPatternProcessor->patternVector_.back());

            if (pPatternProcessor->XFill_ == PatternProcessor::ON) {
                // Randomly fill the pats_.back().
                // Note that the v_, gh_, gl_, fh_ and fl_ do not be changed.
                randomFill(pPatternProcessor->patternVector_.back());
            }

            //  This function will assign pi/ppi stored in pats_.back() to
            //  the gh_ and gl_ in each gate, and then it will run fault
            //  simulation to drop fault.

            pSimulator_->parallelFaultFaultSimWithOnePattern(pPatternProcessor->patternVector_.back(), faultPtrListForGenTemp);

            // After pSimulator_->parallelFaultFaultSimWithOnePattern(pPatternProcessor->patternVector_.back(),faultListToGen) , the pi/ppi
            // values have been passed to gh_ and gl_ of each gate.  Therefore, we can
            // directly use "writeGoodSimValToPatternPO" to perform goodSim to get the PoValue.
            pSimulator_->goodSim();
            writeGoodSimValToPatternPO(pPatternProcessor->patternVector_.back());
            // faultPtrListForGen->pop_front();
        } else if (result == FAULT_UNTESTABLE) {
            faultPtrListForGenTemp.front()->faultState_ = Fault::AU;
            // numOfAtpgUntestableFaults += faultListTemp.front()->equivalent_;
            faultPtrListForGenTemp.pop_front();
        } else {
            faultPtrListForGenTemp.front()->faultState_ = Fault::AB;
            faultPtrListForGenTemp.push_back(faultPtrListForGenTemp.front());
            faultPtrListForGenTemp.pop_front();
        }
    }
    // std::cout << "finish" << std::endl;
}

// cxp
void Atpg::parallelStuckAtFaultATPG(FaultPtrList* faultPtrListForGen, PatternProcessor* pPatternProcessor, int& numOfAtpgUntestableFaults) {
    SINGLE_PATTERN_GENERATION_STATUS result = generateSinglePatternOnTargetFault(*(faultPtrListForGen->front()), false);
    std::cout << "result: " << result << std::endl;
    if (result == PATTERN_FOUND) {
        Pattern pattern(pCircuit_);
        pPatternProcessor->patternVector_.push_back(pattern);

        resetPrevAtpgValStored();
        clearAllFaultEffectByEvaluation();
        storeCurrentAtpgVal();
        writeAtpgValToPatternPI(pPatternProcessor->patternVector_.back());

        // if (pPatternProcessor->dynamicCompression_ == PatternProcessor::ON) {
        //     FaultPtrList faultListTemp = faultPtrListForGen;
        //     pSimulator_->parallelFaultFaultSimWithOnePattern(pPatternProcessor->patternVector_.back(), faultPtrListForGen);
        //     pSimulator_->goodSim();
        //     writeGoodSimValToPatternPO(pPatternProcessor->patternVector_.back());

        //     for (Fault* pFault : faultListTemp) {
        //         // skip detected faults
        //         if (pFault->faultState_ == Fault::DT) {
        //             continue;
        //         }

        //         Gate* pGateForActivation = getGateForFaultActivation(*pFault);
        //         if (((pGateForActivation->atpgVal_ == L) && (pFault->faultType_ == Fault::SA0)) ||
        //             ((pGateForActivation->atpgVal_ == H) && (pFault->faultType_ == Fault::SA1))) {
        //             continue;
        //         }

        //         // Activation check
        //         if (pGateForActivation->atpgVal_ != X) {
        //             if ((pFault->faultType_ == Fault::SA0) || (pFault->faultType_ == Fault::SA1)) {
        //                 setGateAtpgValAndRunImplication((*pGateForActivation), X);
        //             } else {
        //                 continue;
        //             }
        //         }

        //         if (xPathExists(pGateForActivation)) {
        //             // TO-DO homework 05 implement DTC here end of TO-DO
        //             if (generateSinglePatternOnTargetFault(*pFault, true) == PATTERN_FOUND) {
        //                 resetPrevAtpgValStored();
        //                 clearAllFaultEffectByEvaluation();
        //                 storeCurrentAtpgVal();
        //                 writeAtpgValToPatternPI(pPatternProcessor->patternVector_.back());
        //             } else {
        //                 for (Gate& gate : pCircuit_->circuitGates_) {
        //                     gate.atpgVal_ = gate.prevAtpgValStored_;
        //                 }
        //             }
        //         } else {
        //             setGateAtpgValAndRunImplication((*pGateForActivation), pGateForActivation->prevAtpgValStored_);
        //         }
        //     }
        // }

        // clearAllFaultEffectByEvaluation();
        // storeCurrentAtpgVal();
        // writeAtpgValToPatternPI(pPatternProcessor->patternVector_.back());

        if (pPatternProcessor->XFill_ == PatternProcessor::ON) {
            // Randomly fill the pats_.back().
            // Note that the v_, gh_, gl_, fh_ and fl_ do not be changed.
            randomFill(pPatternProcessor->patternVector_.back());
        }

        //  This function will assign pi/ppi stored in pats_.back() to
        //  the gh_ and gl_ in each gate, and then it will run fault
        //  simulation to drop fault.

        pSimulator_->parallelFaultFaultSimWithOnePattern(pPatternProcessor->patternVector_.back(), *faultPtrListForGen);

        // After pSimulator_->parallelFaultFaultSimWithOnePattern(pPatternProcessor->patternVector_.back(),faultListToGen) , the pi/ppi
        // values have been passed to gh_ and gl_ of each gate.  Therefore, we can
        // directly use "writeGoodSimValToPatternPO" to perform goodSim to get the PoValue.
        pSimulator_->goodSim();
        writeGoodSimValToPatternPO(pPatternProcessor->patternVector_.back());
        // faultPtrListForGen->pop_front();
    }
    std::cout << "size: " << pPatternProcessor->patternVector_.size() << std::endl;
}

// **************************************************************************
// Function   [ Atpg::generatePatternSet ]
// Commenter  [ CAL WWS ]
// Synopsis   [ usage:	The main function of class Atpg.
//
// 							description:
// 								This function generates a test pattern set based on a
// 								extracted fault list extracted from the target circuit.
// 								Activate STC/DTC depending on the pPatternProcessor's flag
// 								which is set previously in atpg_cmd.cpp based on user's
// 								script.
//
// 							arguments:
// 								[in, out] pPatternProcessor : A pointer to an empty pattern
// 								processor. It will contain the final test pattern set
// 								generated by ATPG after this function call. The test
// 								pattern set is generated based on the faults extracted from
// 								the target circuit.
//
// 								[in] pFaultListExtractor : A pointer to a fault list
// 								extractor containing the fault list extracted from the
// 								target circuit.
//
// 								[in] isMFO : A flag specifying whether the MFO mode is
// 								activated. MFO stands for multiple fault order, which is a
// 								heuristic with Multiple Fault Orderings.
// 						]
// Date       [ Ver. 1.0 started 2013/08/13	last modified 2023/01/05 ]
// **************************************************************************
void Atpg::generatePatternSet(PatternProcessor* pPatternProcessor, FaultListExtract* pFaultListExtractor, bool isMFO) {
    Fault* pCurrentFault = NULL;
    FaultPtrList originalFaultPtrList, faultPtrListForSTC;
    setupCircuitParameter();
    pPatternProcessor->init(pCircuit_);

    // setting faults for running ATPG
    for (Fault* pFault : pFaultListExtractor->faultsInCircuit_) {
        const bool faultIsQualified = (pFault->faultState_ != Fault::DT && pFault->faultState_ != Fault::RE && pFault->faultyLine_ >= 0);
        if (faultIsQualified) {
            originalFaultPtrList.push_back(pFault);
            faultPtrListForSTC.push_back(pFault);
        }
    }

    // testClearFaultEffect(originalFaultPtrList); // only used for debug

    const double faultPtrListSize = (double)(originalFaultPtrList.size());
    int numOfAtpgUntestableFaults = 0;
    // record pattern set when lower undetected fault/ lower test length with same undetected fault
    numOfAtpgUntestableFaults = 0;

    pPatternProcessor->patternVector_.clear();
    pPatternProcessor->patternVector_.reserve(MAX_LIST_SIZE);

    // start ATPG
    while (!originalFaultPtrList.empty()) {
        // meaning the originalFaultPtrList is already left with aborted fault
        if (originalFaultPtrList.front()->faultState_ == Fault::AB) {
            break;
        }

        // the fault is not popped in previous call of StuckAtFaultATPG()
        // => the fault is neither aborted nor untestable => a pattern was found => detected fault
        if (pCurrentFault == originalFaultPtrList.front()) {
            originalFaultPtrList.front()->faultState_ = Fault::DT;
            originalFaultPtrList.pop_front();
            continue;
        }

        pCurrentFault = originalFaultPtrList.front();
        const bool isTransitionDelayFault = (pCurrentFault->faultType_ == Fault::STR || pCurrentFault->faultType_ == Fault::STF);
        if (isTransitionDelayFault) {
            TransitionDelayFaultATPG(originalFaultPtrList, pPatternProcessor, numOfAtpgUntestableFaults);
        } else {
            StuckAtFaultATPG(originalFaultPtrList, pPatternProcessor, numOfAtpgUntestableFaults);
        }
    }
    if (pPatternProcessor->staticCompression_ == PatternProcessor::ON) {
        staticTestCompressionByReverseFaultSimulation(pPatternProcessor, faultPtrListForSTC);
    }

    // finsh calculation equivalent faults left
    for (Fault* pFault : faultPtrListForSTC) {
        numOfAtpgUntestableFaults += pFault->equivalent_;
    }
}

// **************************************************************************
// Function   [ Atpg::setupCircuitParameter ]
// Commenter  [ KOREAL WWS ]
// Synopsis   [ usage:	Initialize the target circuit's parameters.
//
// 							description:
// 								This function set up all the circuits' parameters and gates'
// 								parameters. Including circuitLevel_to_eventStack.
// 						]
// Date       [ KOREAL Ver. 1.0 started 2013/08/10 last modified 2023/01/05 ]
// **************************************************************************
void Atpg::setupCircuitParameter() {
    // set depthFromPo_
    calculateGateDepthFromPO();

    // Determine the lineType of a gate is FREE_LINE, BOUND_LINE or HEAD_LINE.
    identifyGateLineType();

    // see identifyGateDominator()
    identifyGateDominator();

    // see identifyGateUniquePath()
    identifyGateUniquePath();
}

// **************************************************************************
// Function   [ Atpg::calculateGateDepthFromPO ]
// Commenter  [ CAL WWS ]
// Synopsis   [ usage:	Calculate the depthFromPo_ of each gate.
//
//              description:
// 								This functions calculates the depth (how many gates) from
// 								PO/PPO of every gates.
//
// 								This function also initializes the
// 								this->gateID_to_valModified_,
// 								but should be moved to other places (TODO) for readability.
//            ]
// Date       [ started 2020/07/06 last modified 2023/01/05 ]
// **************************************************************************
void Atpg::calculateGateDepthFromPO() {
    for (int gateID = pCircuit_->totalGate_ - 1; gateID >= 0; --gateID) {
        Gate& gate = pCircuit_->circuitGates_[gateID];
        gateID_to_valModified_[gateID] = 0;  // sneak the initialization assignment in here

        gate.depthFromPo_ = INFINITE;
        if ((gate.gateType_ == Gate::PO) || (gate.gateType_ == Gate::PPO)) {
            gate.depthFromPo_ = 0;
        } else if (gate.numFO_ > 0) {
            for (const int& fanOutGateID : gate.fanoutVector_) {
                const Gate& fanOutGate = pCircuit_->circuitGates_[fanOutGateID];
                if (fanOutGate.depthFromPo_ < gate.depthFromPo_) {
                    gate.depthFromPo_ = fanOutGate.depthFromPo_ + 1;
                }
            }
        }
        // else exist no path to output, so default assignment large number
    }
}

// **************************************************************************
// Function   [ Atpg::identifyGateLineType ]
// Commenter  [ CKY WWS ]
// Synopsis   [ usage:	Identify and sets the gates' lineType
//
// 							description:
// 								This functions sets this->gateID_to_lineType_ (FREE_LINE,
// 								HEAD_LINE, BOUND_LINE). Records number of headline gates
// 								to this->numOfHeadLines_. Records all the headline gates'
// 								gateID into this->headLineGateIDs_.
//            ]
// Date       [ CKY Ver. 1.0 started 2013/08/17 last modified 2023/01/05 ]
// **************************************************************************
void Atpg::identifyGateLineType() {
    numOfheadLines_ = 0;

    for (const Gate& gate : pCircuit_->circuitGates_) {
        const int& gateID = gate.gateId_;

        gateID_to_lineType_[gateID] = FREE_LINE;  // initialize to FREE_LINE

        if (gate.gateType_ != Gate::PI && gate.gateType_ != Gate::PPI) {
            for (const int& fanInGateID : gate.faninVector_) {
                if (gateID_to_lineType_[fanInGateID] != FREE_LINE) {
                    gateID_to_lineType_[gateID] = BOUND_LINE;
                    break;
                }
            }
        }

        // check it is HEAD_LINE or not(rule 1)
        if ((gateID_to_lineType_[gateID] == FREE_LINE) && (gate.numFO_ != 1)) {
            gateID_to_lineType_[gateID] = HEAD_LINE;
            ++numOfheadLines_;
        }

        // check it is HEAD_LINE or not(rule 2)
        if (gateID_to_lineType_[gate.gateId_] == BOUND_LINE) {
            for (const int& fanInGateID : gate.faninVector_) {
                if (gateID_to_lineType_[fanInGateID] == FREE_LINE) {
                    gateID_to_lineType_[fanInGateID] = HEAD_LINE;
                    ++numOfheadLines_;
                }
            }
        }
    }

    // store all head lines to array headLineGateIDs_
    headLineGateIDs_.reserve(numOfheadLines_);

    int count = 0;
    for (const Gate& gate : pCircuit_->circuitGates_) {
        if (gateID_to_lineType_[gate.gateId_] == HEAD_LINE) {
            headLineGateIDs_.push_back(gate.gateId_);
            ++count;
        }
        if (count == numOfheadLines_) {
            break;
        }
    }
}

// **************************************************************************
// Function   [ Atpg::identifyGateDominator ]
// Commenter  [ CAL WWS ]
// Synopsis   [ usage:
// 								Identify Dominator of every gate for unique sensitization.
//
// 							description:
// 								Traverse every gate and try to find each gates’ Dominator.
// 								For each gate, if it has 1 or 0 fanout gate, we can skip it
// 								because a fanout free gate's Dominator is always the same.
// 								Push its fanout gates into circuitLevel_to_eventStack. We
// 								check the event stack for levels bigger than the gate level.
// 								In the process of finding the Dominator, we keep adding
// 								the fanout gates of gates into the event stack to traverse
// 								all paths the gate would pass.(For fanout of the gate, we
// 								just need to add its Dominator.) Once the event stack has
// 								only one gate left, we say that all paths will pass this
// 								gate, so this gate is the Dominator and we push this gate
// 								in this->gateID_to_uniquePath. We also check the existence
// 								of the Dominator in the process.
// 								The dominator doesn’t exist when:
// 									1.	Event stack isn’t empty but we find the PO/PPO in the
// 											event stack(numFO_ == 0). This implies more than one
// 										path to PO/PPO.
// 									2.	Event stack contains a fanout which has no dominator.
// 								Notice that the gateCount is equal to the number of events
// 								in the the whole event stack during this function call.
// 								If we have finished finding the Dominator of the gate,
// 								or the Dominator doesn't exist, gateCount will be 0 or
// 								set to 0. Then, we remove all the remaining events in
// 								event stack and go to next iteration(next gate).
// 								In addition, we check this->gateID_to_valModified_ to avoid
// 								repeated assignments.
//
// 								After this function, each gate has one or zero
// 								Dominator recorded in this->gateID_to_uniquePath_.
// 								A Dominator of a gate is the wire that must be passed
// 								for the dominated gate to reach PO/PPO.
//
// Date       [ Ver. 1.0 started 2013/08/13  last modified 2023/01/05 ]
// **************************************************************************
void Atpg::identifyGateDominator() {
    for (int i = pCircuit_->totalGate_ - 1; i >= 0; --i) {
        Gate& gate = pCircuit_->circuitGates_[i];
        if (gate.numFO_ <= 1)  // if gate has only 1 or 0 output skip this gate
        {
            continue;
        }

        int gateCount = pushGateFanoutsToEventStack(i);
        for (int j = gate.numLevel_ + 1; j < pCircuit_->totalLvl_; ++j) {
            // if next level's output isn't empty
            while (!circuitLevel_to_EventStack_[j].empty()) {
                Gate& gDom = pCircuit_->circuitGates_[circuitLevel_to_EventStack_[j].top()];
                circuitLevel_to_EventStack_[j].pop();
                gateID_to_valModified_[gDom.gateId_] = 0;  // set the gDom to not handle
                // Because the gateCount is zero while the circuitLevel_to_EventStack_ is not zero, we
                // only need to pop without other operations. That is, continue operation.
                if (gateCount <= 0) {
                    continue;
                }

                --gateCount;

                if (gateCount == 0) {
                    if ((int)gateID_to_uniquePath_.capacity() < pCircuit_->totalLvl_) {
                        gateID_to_uniquePath_.reserve(pCircuit_->totalLvl_);
                    }
                    // when all the fanout gate has been calculated
                    gateID_to_uniquePath_[gate.gateId_].push_back(gDom.gateId_);  // push the last calculate fanout gate into gateID_to_uniquePath_
                    break;
                }
                // If there is a gate without gates on its output, then we suppose that this
                // gate is PO/PPO.  Because there are other gates still inside the
                // event queue (gateCount larger than 1), it means that gate gate does
                // not have any dominator.  Hence, we will set gateCount to zero as a
                // signal to clear the gates left in circuitLevel_to_EventStack_.  While the circuitLevel_to_EventStack_ is
                // not empty but gateCount is zero, we will continue.
                if (gDom.numFO_ == 0) {
                    gateCount = 0;
                } else if (gDom.numFO_ > 1) {
                    if ((int)gateID_to_uniquePath_[gDom.gateId_].size() == 0) {
                        gateCount = 0;
                    } else {
                        // Because the first gate in gateID_to_uniquePath_ is the closest Dominator, we just
                        // push it to the circuitLevel_to_EventStack_. Then, we can skip the operation we have done
                        // for gates with higher circuitLvl_ than gate gate.
                        Gate& gTmp = pCircuit_->circuitGates_[gateID_to_uniquePath_[gDom.gateId_][0]];
                        if (!gateID_to_valModified_[gTmp.gateId_]) {
                            circuitLevel_to_EventStack_[gTmp.numLevel_].push(gTmp.gateId_);
                            gateID_to_valModified_[gTmp.gateId_] = 1;
                            ++gateCount;
                        }
                    }
                } else if (!gateID_to_valModified_[gDom.fanoutVector_[0]]) {
                    Gate& gTmp = pCircuit_->circuitGates_[gDom.fanoutVector_[0]];
                    circuitLevel_to_EventStack_[gTmp.numLevel_].push(gTmp.gateId_);
                    gateID_to_valModified_[gTmp.gateId_] = 1;
                    ++gateCount;
                }
            }
        }
    }
}

// **************************************************************************
// Function   [ Atpg::identifyGateUniquePath ]
// Commenter  [ CAL WWS ]
// Synopsis   [ usage:
// 								Compute this->gateID_to_uniquePath_(2D vector).
//                In unique path sensitization phase, we will need to know
//                if the inputs of a gate is fault reachable. Then, we can
//                prevent assigning non-controlling value to them.
//
//                We find the Dominator, then we push_back the input gate
//                which is fault reachable from the current gate.
//
//                After identifying the unique path, if a gate has Dominator,
// 								this->gateID_to_uniquePath_ of this gate will contains the
//                following gate id:
//
//                	[dominatorID fRIG1ID fRIG2ID ......]
//
// 								fRIG is faultReachableInputGate1ID for the above example
// 								Do NOT use fRIG in actual code for the sake of readability.
//
// 							description:
// 								We traverse all gates. For each gate, if it has no
// 								Dominator, we skip the gate. Now we push its fanout gates
// 								into the event stack.
// 								Notice that "count" is equal to the number of events in
// 								the whole event stack. We check the event stack for levels
// 								higher than the gate level. In this function, we keep
// 								adding the fanout of the current gate into the event stack
// 								to traverse all paths the gate would have to pass to reach
// 								PO/PPO. Simultaneously we adjust "count" and set the
// 								reachableByDominator of the fanout to current gate.
// 								Once "count" is 0 (the event stack has only one gate left),
// 								we should get the Dominator.
// 								Then we check reachableByDominator of the fanin of the
// 								Dominator. If it is the current gate, then we push the
// 								fanin into this->gateID_to_uniquePath.
// 								Finally, we go to the next iteration (the next gate).
//
//            ]
// Date       [ Ver. 1.0 started 2013/08/13  last modified 2023/01/05 ]
// **************************************************************************
void Atpg::identifyGateUniquePath() {
    static std::vector<int> reachableByDominator(pCircuit_->totalGate_);
    for (int i = pCircuit_->totalGate_ - 1; i >= 0; --i) {
        Gate& gate = pCircuit_->circuitGates_[i];
        // Because we will call identifyGateDominator before entering this function,
        // a gate with gateID_to_uniquePath_ will contain one Dominator.  Hence, we can
        // skip the gates while the sizes of their gateID_to_uniquePath_ is zero.
        if (gateID_to_uniquePath_[gate.gateId_].size() == 0) {
            continue;
        }

        reachableByDominator[gate.gateId_] = i;
        int count = pushGateFanoutsToEventStack(i);
        for (int j = gate.numLevel_ + 1; j < pCircuit_->totalLvl_; ++j) {
            while (!circuitLevel_to_EventStack_[j].empty()) {  // if fanout gate was not empty
                Gate& gTmp = pCircuit_->circuitGates_[circuitLevel_to_EventStack_[j].top()];
                circuitLevel_to_EventStack_[j].pop();
                gateID_to_valModified_[gTmp.gateId_] = 0;
                reachableByDominator[gTmp.gateId_] = i;
                --count;
                if (count == 0) {
                    for (int gReachID : gTmp.faninVector_) {
                        if (reachableByDominator[gReachID] == i)  // if it is UniquePath
                        {
                            // save gate to gateID_to_uniquePath_ list
                            gateID_to_uniquePath_[gate.gateId_].push_back(gReachID);
                        }
                    }
                    break;
                }
                count += pushGateFanoutsToEventStack(gTmp.gateId_);
            }
        }
    }
}

// **************************************************************************
// Function   [ Atpg::TransitionDelayFaultATPG ]
// Commenter  [ HKY CYW WWS ]
// Synopsis   [ usage: Do transition delay fault model ATPG
//
// 							description:
// 								This function is implemented very similar to the
// 								StuckAtFaultATPG() except for the following differences.
// 								1.	The fault model used is transition delay fault instead
// 										of stuck at fault.
// 								2.	Dynamic test compression is not implemented for
// 										transition delay fault.
//
// 							arguments:
// 								Please see the documentation of Atpg::StuckAtFaultATPG().
//            ]
// Date       [ HKY Ver. 1.0 started 2014/09/01 last modified 2023/01/05 ]
// **************************************************************************
void Atpg::TransitionDelayFaultATPG(FaultPtrList& faultPtrListForGen, PatternProcessor* pPatternProcessor, int& numOfAtpgUntestableFaults) {
    const Fault& fTDF = *faultPtrListForGen.front();

    SINGLE_PATTERN_GENERATION_STATUS result = generateSinglePatternOnTargetFault(Fault(fTDF.gateID_ + pCircuit_->numGate_, fTDF.faultType_, fTDF.faultyLine_, fTDF.equivalent_, fTDF.faultState_), false);
    if (result == PATTERN_FOUND) {
        Pattern pattern(pCircuit_);
        pattern.initForTransitionDelayFault(pCircuit_);
        pPatternProcessor->patternVector_.push_back(pattern);
        writeAtpgValToPatternPI(pPatternProcessor->patternVector_.back());

        if ((pPatternProcessor->staticCompression_ == PatternProcessor::OFF) && (pPatternProcessor->XFill_ == PatternProcessor::ON)) {
            randomFill(pPatternProcessor->patternVector_.back());
        }

        pSimulator_->parallelFaultFaultSimWithOnePattern(pPatternProcessor->patternVector_.back(), faultPtrListForGen);
        pSimulator_->goodSim();
        writeGoodSimValToPatternPO(pPatternProcessor->patternVector_.back());
    } else if (result == FAULT_UNTESTABLE) {
        faultPtrListForGen.front()->faultState_ = Fault::AU;
        numOfAtpgUntestableFaults += faultPtrListForGen.front()->equivalent_;
        faultPtrListForGen.pop_front();
    } else {
        faultPtrListForGen.front()->faultState_ = Fault::AB;
        faultPtrListForGen.push_back(faultPtrListForGen.front());
        faultPtrListForGen.pop_front();
    }
}

// **************************************************************************
// Function   [ Atpg::StuckAtFaultATPG ]
// Commenter  [ CAL WWS ]
// Synopsis   [ usage:
// 								Do stuck at fault model ATPG on one fault and do DTC
// 								on the pattern generated to the single fault if the DTC
// 								flag is set to ON.
//
// 							description:
// 								The first fault pointed to by the first pointer in
// 								faultPtrListForGen will be selected as the first target
// 								fault for single pattern generation in this function.
// 								There will be three possible scenario after the
// 								single pattern generation on the first selected fault.
// 								1.	PATTERN_FOUND
// 										If a pattern is found for the first selected fault.
// 										A pattern will be allocated and push back to
// 										pPatternProcessor->patternVector and will be updated
// 										immediately. If the DTC is set to ON for the pattern
// 										processor. The first selected fault will immediately
// 										be dropped by fault simulation.
// 										Then the DTC stage will start officially and select
// 										other undetected faults one by one for DTC. If the
// 										latter selected fault can be detected by filling
// 										some of the X(s) to 1 or 0. The fault state of the
// 										fault will be set to DT(detected) and the pattern will
// 										be updated to the original pattern with specific X(s)
// 										assigned.
// 										If the latter selected undetected fault is not detected,
// 										atpgVals will have to be restored to previous atpgVals
// 										because the single pattern generation will change the
//										gate atpg values. If there is no more faults for DTC,
// 										the loop of DTC will be ended. After the loop, we will
// 										randomly XFill the pattern and perform fault simulation
// 										with the most recently updated pattern to drop the
// 										additional faults detected during the DTC phase.
// 								2.	FAULT_UNTESTABLE
// 										If the fault is not detected even after all the
// 										backtracks is done in the single pattern generation
// 										the fault is then declared as fault untestable.
// 								3.	ABORT
// 										If the Atpg is aborted due to the time of backtracks
// 										exceeding the BACKTRACK_LIMIT 500 (can be changed
// 										manually in namespace atpg.h::CoreNs).
//
// 							arguments:
// 								[in, out] faultPtrListForGen : Current list of fault
// 								pointers that are pointed to undetected faults. If detected
// 								when seen as the first selected target fault, it will be
// 								dropped immediately by fault simulation. If detected during
// 								DTC stage the faults will be dropped altogether after DTC.
//
// 								[in, out] pPatternProcessor : A pointer to pattern
// 								processor that contains a pattern vector recording the
// 								whole pattern set. In this function, the pattern processor
// 								should already possess the patterns generated for the
// 								faults before the current fault. A new Pattern will be
// 								pushed back to the the pPatternProcessor->patternVector_
// 								if the fault first selected in this function is detected.
// 								It will become pPatternProcessor->patternVector_.back().
// 								Then it will be determined and random XFilled at end of
// 								the function.
//
// 								[in, out] numOfAtpgUntestableFaults : It is a reference
// 								variable for recording the number of equivalent faults
// 								untestable. Here untestable faults means this function call
// 								has ended without abortion. If the function is aborted due
// 								to backtrack time exceeding limit, it is called aborted
// 								fault which is different to untestable fault.
//            ]
// Date       [ started 2020/07/07    last modified 2023/01/05 ]
// **************************************************************************
void Atpg::StuckAtFaultATPG(FaultPtrList& faultPtrListForGen, PatternProcessor* pPatternProcessor, int& numOfAtpgUntestableFaults) {
    SINGLE_PATTERN_GENERATION_STATUS result = generateSinglePatternOnTargetFault(*faultPtrListForGen.front(), false);
    if (result == PATTERN_FOUND) {
        Pattern pattern(pCircuit_);
        pPatternProcessor->patternVector_.push_back(pattern);

        resetPrevAtpgValStored();
        clearAllFaultEffectByEvaluation();
        storeCurrentAtpgVal();
        writeAtpgValToPatternPI(pPatternProcessor->patternVector_.back());

        if (pPatternProcessor->dynamicCompression_ == PatternProcessor::ON) {
            FaultPtrList faultListTemp = faultPtrListForGen;
            pSimulator_->parallelFaultFaultSimWithOnePattern(pPatternProcessor->patternVector_.back(), faultPtrListForGen);
            pSimulator_->goodSim();
            writeGoodSimValToPatternPO(pPatternProcessor->patternVector_.back());

            for (Fault* pFault : faultListTemp) {
                // skip detected faults
                if (pFault->faultState_ == Fault::DT) {
                    continue;
                }

                Gate* pGateForActivation = getGateForFaultActivation(*pFault);
                if (((pGateForActivation->atpgVal_ == L) && (pFault->faultType_ == Fault::SA0)) ||
                    ((pGateForActivation->atpgVal_ == H) && (pFault->faultType_ == Fault::SA1))) {
                    continue;
                }

                // Activation check
                if (pGateForActivation->atpgVal_ != X) {
                    if ((pFault->faultType_ == Fault::SA0) || (pFault->faultType_ == Fault::SA1)) {
                        setGateAtpgValAndRunImplication((*pGateForActivation), X);
                    } else {
                        continue;
                    }
                }

                if (xPathExists(pGateForActivation)) {
                    // TO-DO homework 05 implement DTC here end of TO-DO
                    if (generateSinglePatternOnTargetFault(*pFault, true) == PATTERN_FOUND) {
                        resetPrevAtpgValStored();
                        clearAllFaultEffectByEvaluation();
                        storeCurrentAtpgVal();
                        writeAtpgValToPatternPI(pPatternProcessor->patternVector_.back());
                    } else {
                        for (Gate& gate : pCircuit_->circuitGates_) {
                            gate.atpgVal_ = gate.prevAtpgValStored_;
                        }
                    }
                } else {
                    setGateAtpgValAndRunImplication((*pGateForActivation), pGateForActivation->prevAtpgValStored_);
                }
            }
        }

        clearAllFaultEffectByEvaluation();
        storeCurrentAtpgVal();
        writeAtpgValToPatternPI(pPatternProcessor->patternVector_.back());

        if (pPatternProcessor->XFill_ == PatternProcessor::ON) {
            // Randomly fill the pats_.back().
            // Note that the v_, gh_, gl_, fh_ and fl_ do not be changed.
            randomFill(pPatternProcessor->patternVector_.back());
        }

        //  This function will assign pi/ppi stored in pats_.back() to
        //  the gh_ and gl_ in each gate, and then it will run fault
        //  simulation to drop fault.

        pSimulator_->parallelFaultFaultSimWithOnePattern(pPatternProcessor->patternVector_.back(), faultPtrListForGen);

        // After pSimulator_->parallelFaultFaultSimWithOnePattern(pPatternProcessor->patternVector_.back(),faultListToGen) , the pi/ppi
        // values have been passed to gh_ and gl_ of each gate.  Therefore, we can
        // directly use "writeGoodSimValToPatternPO" to perform goodSim to get the PoValue.
        pSimulator_->goodSim();
        writeGoodSimValToPatternPO(pPatternProcessor->patternVector_.back());
    } else if (result == FAULT_UNTESTABLE) {
        faultPtrListForGen.front()->faultState_ = Fault::AU;
        numOfAtpgUntestableFaults += faultPtrListForGen.front()->equivalent_;
        faultPtrListForGen.pop_front();
    } else {
        faultPtrListForGen.front()->faultState_ = Fault::AB;
        faultPtrListForGen.push_back(faultPtrListForGen.front());
        faultPtrListForGen.pop_front();
    }
}

// **************************************************************************
// Function   [ Atpg::getGateForFaultActivation ]
// Commenter  [ CAL WWS ]
// Synopsis   [ usage:
// 								This function is used in DTC stage.
//                Find and return the gate needed for fault activation。
//
// 							arguments:
//              	[in] faultToActivate: The latter fault selected to be
// 								activated in DTC stage.
//
// 							output:
// 								A gate pointer pointing to the gate needed to activate
// 								"faultToActivate".
//            ]
// Date       [ started 2020/07/07    last modified 2023/01/05 ]
// **************************************************************************
Gate* Atpg::getGateForFaultActivation(const Fault& faultToActivate) {
    bool isOutputFault = (faultToActivate.faultyLine_ == 0);
    Gate* pGateForActivation = NULL;
    Gate* pFaultyGate = &pCircuit_->circuitGates_[faultToActivate.gateID_];
    if (!isOutputFault) {
        pGateForActivation = &pCircuit_->circuitGates_[pFaultyGate->faninVector_[faultToActivate.faultyLine_ - 1]];
    } else {
        pGateForActivation = pFaultyGate;
    }
    return pGateForActivation;
}

// **************************************************************************
// Function   [ Atpg::setGateAtpgValAndEventDrivenEvaluation ]
// Commenter  [ CAL WWS ]
// Synopsis   [ usage:
//                Directly set the output of "gate" to "value" and run
// 								evaluations by event driven.
//
//              description:
// 								1.	Call clearEventStack() and set gate.atpgVal_ to "val"
// 								2.	For each fanout gate of gate, push the gateID into the
// 										event stack if not in the event stack.
// 								3.	Do event driven evaluation to update all the gates in
//										the event stack.
//
// 							arguments:
// 								[in, out] gate : The gate to set "val" to.
// 								[in] val : The "val" to assign to gate.atpgVal_.
//            ]
// Date       [ started 2020/07/07    last modified 2023/01/05 ]
// **************************************************************************
void Atpg::setGateAtpgValAndRunImplication(Gate& gate, const Value& val) {
    clearEventStack(false);
    gate.atpgVal_ = val;
    for (const int& fanoutID : gate.fanoutVector_) {
        Gate& og = pCircuit_->circuitGates_[fanoutID];
        if (isInEventStack_[og.gateId_] == 0) {
            circuitLevel_to_EventStack_[og.numLevel_].push(og.gateId_);
            isInEventStack_[og.gateId_] = 1;
        }
    }

    // event-driven simulation
    for (int i = gate.numLevel_; i < pCircuit_->totalLvl_; ++i) {
        while (!circuitLevel_to_EventStack_[i].empty()) {
            int gateID = circuitLevel_to_EventStack_[i].top();
            circuitLevel_to_EventStack_[i].pop();
            isInEventStack_[gateID] = 0;
            Gate& currGate = pCircuit_->circuitGates_[gateID];
            Value newValue = evaluateGoodVal(currGate);
            if (currGate.atpgVal_ != newValue) {
                currGate.atpgVal_ = newValue;
                for (int j = 0; j < currGate.numFO_; ++j) {
                    Gate& og = pCircuit_->circuitGates_[currGate.fanoutVector_[j]];
                    if (isInEventStack_[og.gateId_] == 0) {
                        circuitLevel_to_EventStack_[og.numLevel_].push(og.gateId_);
                        isInEventStack_[og.gateId_] = 1;
                    }
                }
            }
        }
    }
}

// **************************************************************************
// Function   [ Atpg::resetPrevAtpgValStoredToX ]
// Commenter  [ CAL WWS ]
// Synopsis   [ usage: Reset the prevAtpgValStored_ of each gate to X. ]
// Date       [ started 2020/07/07    last modified 2023/01/05 ]
// **************************************************************************
void Atpg::resetPrevAtpgValStored() {
    for (Gate& gate : pCircuit_->circuitGates_) {
        gate.prevAtpgValStored_ = X;
    }
}

// **************************************************************************
// Function   [ Atpg::storeCurrentAtpgVal ]
// Commenter  [ CAL WWS ]
// Synopsis   [ usage:
//                Store all the gates' atpgVal_ to prevAtpgValStored_ in
// 								the circuit.
//
//              output:
// 								Count of values which change from H/L to the value which is
// 								not the same as prevAtpgValStored_.
//            ]
// Date       [ started 2020/07/04    last modified 2023/01/05 ]
// **************************************************************************
int Atpg::storeCurrentAtpgVal() {
    int numAssignedValueChanged = 0;
    for (Gate& gate : pCircuit_->circuitGates_) {
        if ((gate.prevAtpgValStored_ != X) && (gate.prevAtpgValStored_ != gate.atpgVal_)) {
            ++numAssignedValueChanged;
        }
        gate.prevAtpgValStored_ = gate.atpgVal_;
    }

    if (numAssignedValueChanged != 0) {
        std::cerr << "Bug: storeCurrentAtpgVal detects the numAssignedValueChanged is not 0\n";
        std::cin.get();
    }
    return numAssignedValueChanged;
}

// **************************************************************************
// Function   [ Atpg::clearAllFaultEffectByEvaluation ]
// Commenter  [ CAL WWS ]
// Synopsis   [ usage:
//                Clear all the fault effects before test generation for next
//                target fault.
//            ]
// Date       [ started 2020/07/04    last modified 2023/01/05 ]
// **************************************************************************
void Atpg::clearAllFaultEffectByEvaluation() {
    int numOfInputGate = pCircuit_->numPI_ + pCircuit_->numPPI_;
    for (Gate& gate : pCircuit_->circuitGates_) {
        if (gate.gateId_ < numOfInputGate) {
            // Remove fault effects in input gates
            clearFaultEffectOnGateAtpgVal(gate);
        } else {
            // Simulate the whole circuit ( gates were sorted by circuitLvl_ in "pCircuit_->circuitGates_" )
            gate.atpgVal_ = evaluateGoodVal(gate);
        }
    }
}

// **************************************************************************
// Function   [ Atpg::clearFaultEffectOnGateAtpgVal ]
// Commenter  [ CAL WWS ]
// Synopsis   [ usage:
//                This function replace value of a gate from D/B to H/L.
//
//              arguments:
// 								[in] gate : The gate to have atpgVal_ cleared.
//            ]
// Date       [ started 2020/07/04    last modified 2023/01/05 ]
// **************************************************************************
void Atpg::clearFaultEffectOnGateAtpgVal(Gate& gate) {
    if (gate.atpgVal_ == D) {
        gate.atpgVal_ = H;
    } else if (gate.atpgVal_ == B) {
        gate.atpgVal_ = L;
    }
}

// **************************************************************************
// Function   [ Atpg::generateSinglePatternOnTargetFault ]
// Commenter  [ KOREAL WWS ]
// Synopsis   [ usage:
// 								Given a target fault, generate a pattern.
//
// 							description:
// 								First, call initialize:
// 									Set the pFaultyLineGate as the gate whose output is
// 									the target fault.
// 								Then, set the backtraceFlag to INITIAL.
// 								Keep calling doImplication() in a while loop, set the
// 								genStatus for latter return if PATTERN_FOUND,
//								FAULT_UNTESTABLE, ABORT corresponding to the scenario of
// 								their literal meaning.
// 								Loop content(while):
// 									IF number of backtracks exceeds BACKTRACK_LIMIT, ABORT
// 									IF doImplication() return false(conflicts):
// 										Clear the event stack and set
// 										this->gateID_to_valModified to all false.
// 										Call backtrack()
// 										If backtrack successful:
// 											Reset backtraceFlag to INITIAL for the latter
// 											findFinalObjectives(), set implicationStatus
// 											according to the BackImpLevel and reset
// 											pLastDFrontier to NULL
// 										Else IF backtrack failed meaning all backtracks
// 											have been finished but there is still no pattern
// 											found.
// 											=> FAULT_UNTESTABLE
// 									Else IF doImplication() return true:
// 										IF continuationMeaningful() false:
// 											Then reset the backtraceFlag to INITIAL
// 										IF fault is propagated to any PO/PPO:
// 											IF there are any unjustified bound lines in circuit
// 												call findFinalObjective() and
// 												assignAtpgValToFinalObjectiveGates() and set
// 												implyStatus to FORWARD
// 											ELSE
// 												Justify all the free lines
// 												=> PATTERN_FOUND
// 										ELSE:
// 											IF the number of d-frontiers is 0:
// 												backtrack()
// 												IF backtrack successful:
// 													reset backtraceFlag to INITIAL
// 													...
// 												ELSE:
// 													=> FAULT_UNTESTABLE
// 											ELSE IF number of d-frontiers is 1:
// 												do unique sensitization:
// 													If UNIQUE_PATH_SENSITIZATION_FAIL:
// 														continue back to the while loop
// 														(will backtrack, no more dFrontier)
// 													ELSE IF sensitization successful
// 														implyStatus = BACKWARD and continue
// 													ELSE IF back implication level == 0
// 														continue;
// 													ELSE IF nothing happened
// 														call
// 														findFinalObjective() and
// 														assignAtpgValToFinalObjectiveGates()
// 														and set implyStatus to FORWARD
//
// 								There are four main atpgStatus for backtrace while
// 								generating the pattern:
//
//									IMPLY_AND_CHECK: 	Determine as many signal values as
// 																		possible then check if the backtrace
//																		is meaningful or not.
//									DECISION:					Using the multiple backtrace procedure
// 																		to determine a final objective.
//									BACKTRACK:				If the values are incompatible during
// 																		propagation or implications,
// 																		backtracking is necessary.
//									JUSTIFY_FREE:			At the end of the process. Finding
// 																		values on the primary inputs which
// 																		justify all the values on the head
// 																		lines.
//
// 							arguments:
// 								[in] targetFault : The target fault for this function to
//  							generate pattern on.
//
// 								[in] isAtStageDTC : The flag is true if this function is
// 								called during the DTC stage.
// 								See Atpg::initializeForSinglePatternGeneration() for more
// 								of how this flag affect the behavior of this function.
//
//              output:
// 								SINGLE_PATTERN_GENERATION_STATUS,
// 								PATTERN_FOUND:		Single pattern generation successful.
// 																	A pattern is found for target fault.
// 								FAULT_UNTESTABLE:	The target fault is not detected
// 																	after all backtracks have ended.
// 								ABORT:	The single pattern generation is aborted due to
//												the time of backtracks exceeding the
// 												BACKTRACK_LIMIT(500).
//            ]
// Date       [ KOREAL Ver. 1.0 started 2013/08/10 last modified 2023/01/05 ]
// **************************************************************************
Atpg::SINGLE_PATTERN_GENERATION_STATUS Atpg::generateSinglePatternOnTargetFault(Fault targetFault, bool isAtStageDTC) {
    int backwardImplicationLevel = 0;                            // backward imply level
    int numOfBacktrack = 0;                                      // backtrack times
    bool Finish = false;                                         // Finish is true when whole pattern generation process is done
    bool faultHasPropagatedToPO = false;                         // faultHasPropagatedToPO is true when the fault is propagate to the PO
    Gate* pFaultyLine = NULL;                                    // the gate pointer, whose fanOut is the target fault
    Gate* pLastDFrontier = NULL;                                 // the D-frontier gate which has the highest level of the D-frontier
    IMPLICATION_STATUS implicationStatus;                        // decide implication to go forward or backward
    BACKTRACE_STATUS backtraceFlag;                              // backtrace flag including    { INITIAL, CHECK_AND_SELECT, CURRENT_OBJ_DETERMINE, FAN_OBJ_DETERMINE }
    SINGLE_PATTERN_GENERATION_STATUS genStatus = PATTERN_FOUND;  // the atpgStatus that will be return, including		{ PATTERN_FOUND, FAULT_UNTESTABLE, ABORT }

    // Get the gate whose output is fault line and set the backwardImplicationLevel
    // SET A FAULT SIGNAL
    pFaultyLine = initializeForSinglePatternGeneration(targetFault, backwardImplicationLevel, implicationStatus, isAtStageDTC);

    // If there's no such gate, return FAULT_UNTESTABLE
    if (!pFaultyLine) {
        return FAULT_UNTESTABLE;
    }
    // SET BACKTRACE FLAG
    backtraceFlag = INITIAL;

    while (!Finish) {
        if (!doImplication(implicationStatus, backwardImplicationLevel)) {
            // implication INCONSISTENCY
            // record the number of backtrack
            // 通过检查backtrackDecisionTree_是否为空来标记是否完成了一次backtrack，完成了则numOfBacktrack+1
            if (backtrackDecisionTree_.lastNodeMarked()) {
                ++numOfBacktrack;
            }
            // Abort if numOfBacktrack reaching the BACKTRACK_LIMIT
            // 如果numOfBacktrack比限制值大了，中止生成过程
            if (numOfBacktrack > BACKTRACK_LIMIT) {
                genStatus = ABORT;
                Finish = true;
            }

            clearAllEvents();

            // IS THERE AN UNTRIED COMBINATION OF VALUES ON ASSIGNED HEAD LINES OR FANOUT POINTS?
            // If yes, SET UNTRIED COMBINATION OF VALUES in the function backtrack
            // 因为该情况被触发是因为蕴含后产生了冲突，如果还有未尝试的则继续根据算出的backwardImplicationLevel和implicationStatus回去调用doImplication函数进行蕴含，如果没有剩余的未尝试的组合了，则说明发生了无法解决的冲突，这个错误无法被检测到。backtrack函数的返回值用于判断是否还有未尝试的组合，即backtrackDecisionTree_是否为空
            if (backtrack(backwardImplicationLevel)) {
                // backtrack success and initialize the data
                // SET BACKTRACE FLAG
                backtraceFlag = INITIAL;
                implicationStatus = (backwardImplicationLevel > 0) ? BACKWARD : FORWARD;
                pLastDFrontier = NULL;
            } else {
                // backtrack fail
                // EXIT: FAULT_UNTESTABLE FAULT
                genStatus = FAULT_UNTESTABLE;
                Finish = true;
            }
            continue;
        }

        // IS CONTINUATION OF BACKTRACE MEANINGFUL?
        // 如果initialObjectives_为空（在遍历其中所有元素并检查其是否已经被回溯或者蕴含，若是则移除）或者pLastDFrontier的atpgVal_是否为X，两者存在一种则为false，即不需要继续backtrace
        if (!continuationMeaningful(pLastDFrontier)) {
            backtraceFlag = INITIAL;
        }

        // FAULT SIGNAL PROPAGATED TO A PRIMARY OUTPUT?
        // 判断是否把fault传到PO或者PPO上了（就是看PO或者PPO上有没有值为D/B的）
        if (checkIfFaultHasPropagatedToPO(faultHasPropagatedToPO)) {
            // IS THERE ANY UNJUSTIFIED BOUND LINE?
            // 检查是否有unjustified的bound line
            if (checkForUnjustifiedBoundLines()) {
                // DETERMINE A FINAL OBJECTIVE TO ASSIGN A VALUE
                // 后续会再看这个函数具体的方式，这个只是按照文章中的算法步骤来的
                findFinalObjective(backtraceFlag, faultHasPropagatedToPO, pLastDFrontier);
                // ASSIGN A VALUE TO THE FINAL OBJECTIVE LINE
                // 把finalObjectives_中的所有门按照n0和n1的数量赋值为对应的H/L，然后把当前赋值的决策放入决策树中
                assignAtpgValToFinalObjectiveGates();
                // 因为给一个finalobjective赋值了，所以下一个循环应该直接从这里开始往后进行蕴含，检查是否存在冲突
                implicationStatus = FORWARD;
                continue;
            } else {
                // Finding values on the primary inputs which justify all the values on the head lines
                // LINE JUSTIFICATION OF FREE LINES
                justifyFreeLines(targetFault);
                // EXIT: TEST GENERATED
                genStatus = PATTERN_FOUND;
                Finish = true;
            }
        } else {
            // not propagate to PO
            // THE NUMBER OF GATES IN D-FRONTIER?
            // 该函数可能后续还要再看，关于为什么需要重置pFaultyLine之后的门的gateID_to_xPathStatus_为UNKNOWN，实质为更新DFrontiers后获取目前有用的DFrontier个数
            int numGatesInDFrontier = countEffectiveDFrontiers(pFaultyLine);

            // ZERO
            if (numGatesInDFrontier == 0) {
                // no frontier can propagate to the PO
                // record the number of backtrack
                if (backtrackDecisionTree_.lastNodeMarked()) {
                    ++numOfBacktrack;
                }
                // Abort if numOfBacktrack reaching the BACKTRACK_LIMIT
                if (numOfBacktrack > BACKTRACK_LIMIT) {
                    genStatus = ABORT;
                    Finish = true;
                }

                clearAllEvents();

                // IS THERE AN UNTRIED COMBINATION OF VALUES ON ASSIGNED HEAD LINES OR FANOUT POINTS?
                // If yes, SET UNTRIED COMBINATION OF VALUES in the function backtrack
                if (backtrack(backwardImplicationLevel)) {
                    // backtrack success and initializeForSinglePatternGeneration the data
                    // SET BACKTRACE FLAG
                    backtraceFlag = INITIAL;
                    implicationStatus = (backwardImplicationLevel > 0) ? BACKWARD : FORWARD;
                    pLastDFrontier = NULL;
                } else {
                    // backtrack fail
                    // EXIT: FAULT_UNTESTABLE FAULT
                    genStatus = FAULT_UNTESTABLE;
                    Finish = true;
                }
            } else if (numGatesInDFrontier == 1) {
                // There exist just one path to the PO
                // UNIQUE SENSITIZATION
                backwardImplicationLevel = doUniquePathSensitization(pCircuit_->circuitGates_[dFrontiers_[0]]);
                // Unique Sensitization fail
                if (backwardImplicationLevel == UNIQUE_PATH_SENSITIZE_FAIL) {
                    // If UNIQUE_PATH_SENSITIZE_FAIL, the number of gates in d-frontier in the next while loop
                    // and will backtrack
                    continue;
                }
                // Unique Sensitization success
                if (backwardImplicationLevel > 0) {
                    implicationStatus = BACKWARD;
                    continue;
                } else if (backwardImplicationLevel == 0) {
                    continue;
                } else {
                    // backwardImplicationLevel < 0, find an objective and set backtraceFlag and pLastDFrontier
                    findFinalObjective(backtraceFlag, faultHasPropagatedToPO, pLastDFrontier);
                    assignAtpgValToFinalObjectiveGates();
                    implicationStatus = FORWARD;
                    continue;
                }
            } else {  // more than one
                // DETERMINE A FINAL OBJECTIVE TO ASSIGN A VALUE
                findFinalObjective(backtraceFlag, faultHasPropagatedToPO, pLastDFrontier);
                // ASSIGN A VALUE TO THE FINAL OBJECTIVE LINE
                assignAtpgValToFinalObjectiveGates();
                implicationStatus = FORWARD;
                continue;
            }
        }
    }
    return genStatus;
}

// **************************************************************************
// Function   [ Atpg::initializeForSinglePatternGeneration ]
// Commenter  [ WWS ]
// Synopsis   [ usage:
//                This function replace value of a gate from D/B to H/L.
//
// 							description:
// 								First, assign fault to this->currentTargetFault_ for the
//								future use of other functions.
// 								Then, assign the faulty gate to pFaultyLineGate. Initialize
// 								all the objectives and d-frontiers in Atpg.
// 								Initialize the circuit according to the faulty gate.
// 								IF gFaultyLine is free line,
// 									Set the value according to Fault.type_.
// 									SetFreeFaultyGate() to get the equivalent HEADLINE fault.
// 									Assign this->currentFault_ to the new fault.
// 									Set BackImpLevel to 0, implyStatus to FORWARD,
// 									faultyGateID to the new fault.gateID.
// 								ELSE
// 									setFaultyGate() to assign the BackImpLevel and assign
// 									the value of fanin gates of pFaultyLineGate and itself.
// 									Add the faultyGateID to the this->dFrontier_.
//									Do unique sensitization to pre assign some values and
// 									then set implyStatus to BACKWARD.
// 								Last, If fault.type_ is STR or STF, setup time frames for
// 								transition delay faults.
//
//              arguments:
// 								[in] targetFault : The target fault for single pattern
// 								generation, the faultyLine_ can be at input or output.
//
// 								[in, out] backwardImplicationLevel : The variable reference
// 								of backward implication level in single pattern generation,
// 								will be initialized according to the targetFault, and will
// 								be assigned to 0 if the implicationStatus is FORWARD.
//
// 								[in, out] implicationStatus : The variable reference of
// 								implication status in single pattern generation which
// 								indicates whether to do implication FORWARD or BACKWARD
// 								according to the targetFault.
//
// 								[in] isAtStageDTC : Specifying whether this function is
// 								called in the single pattern generation in DTC stage or
// 								not.
//
// 							output:
// 								The faulty gate corresponding to the fault.
//            ]
// Date       [ last modified 2023/01/05 ]
// **************************************************************************
Gate* Atpg::initializeForSinglePatternGeneration(Fault& targetFault, int& backwardImplicationLevel, IMPLICATION_STATUS& implicationStatus, const bool& isAtStageDTC) {
    Gate* gFaultyLine = &pCircuit_->circuitGates_[targetFault.gateID_];
    currentTargetFault_ = targetFault;

    // if targetFault at gate's input, change the gFaultyLine to the input gate
    if (targetFault.faultyLine_ != 0) {
        gFaultyLine = &pCircuit_->circuitGates_[gFaultyLine->faninVector_[targetFault.faultyLine_ - 1]];  // gFaultyLine是当前targetFault真正的有故障的线（门）
    }
    initializeObjectivesAndFrontiers();
    initializeCircuitWithFaultyGate(*gFaultyLine, isAtStageDTC);
    int fGate_id = targetFault.gateID_;

    // currentTargetHeadLineFault_.gateID_ = -1; //bug report
    if (gateID_to_lineType_[gFaultyLine->gateId_] == FREE_LINE) {
        // 按targetFault的fault类型设置gFaultyLine的atpgVal_
        if ((targetFault.faultType_ == Fault::SA0 || targetFault.faultType_ == Fault::STR) && gFaultyLine->atpgVal_ != L) {
            gFaultyLine->atpgVal_ = D;
        }
        if ((targetFault.faultType_ == Fault::SA1 || targetFault.faultType_ == Fault::STF) && gFaultyLine->atpgVal_ != H) {
            gFaultyLine->atpgVal_ = B;
        }
        backtrackImplicatedGateIDs_.push_back(gFaultyLine->gateId_);

        // 对gFaultyLine调用setFreeLineFaultyGate函数找到gFaultyLine扇出中的headline，并在路径上将遍历到的线设置为非控制值（即找到gFaultyLine等效到headline上的故障）
        currentTargetHeadLineFault_ = setFreeLineFaultyGate(*gFaultyLine);
        currentTargetFault_ = currentTargetHeadLineFault_;
        backwardImplicationLevel = 0;
        implicationStatus = FORWARD;  // 等效到headline后不需要往回implication
        fGate_id = currentTargetHeadLineFault_.gateID_;
    } else {
        backwardImplicationLevel = setFaultyGate(targetFault);  // backwardImplicationLevel为targetFault指向的门的扇入中的最大level
    }

    if (backwardImplicationLevel < 0) {
        return NULL;
    }

    dFrontiers_.push_back(fGate_id);

    // 对fGate_id指向的门进行通路敏化，应该是用于保证fGate_id上的fault可以传到output，Level为fGate的UniquePath中最后一个门的扇入的最大level
    int Level = doUniquePathSensitization(pCircuit_->circuitGates_[fGate_id]);
    if (Level == UNIQUE_PATH_SENSITIZE_FAIL) {
        return NULL;
    }

    if (Level > backwardImplicationLevel) {
        backwardImplicationLevel = Level;
        implicationStatus = BACKWARD;
    }

    if (targetFault.faultType_ == Fault::STR || targetFault.faultType_ == Fault::STF) {
        Level = setUpFirstTimeFrame(targetFault);
        if (Level < 0) {
            return NULL;
        }

        if (Level > backwardImplicationLevel) {
            backwardImplicationLevel = Level;
            implicationStatus = BACKWARD;
        }
    }
    return &pCircuit_->circuitGates_[fGate_id];
}

// **************************************************************************
// Function   [ Atpg::initializeObjectivesAndFrontiers ]
// Commenter  [ WWS ]
// Synopsis   [ usage:
//                This function clear all the objectives, and most of the
// 								attributes of the circuit.
//            ]
// Date       [ last modified 2023/01/06 ]
// **************************************************************************
void Atpg::initializeObjectivesAndFrontiers() {
    initialObjectives_.clear();
    currentObjectives_.clear();
    fanoutObjectives_.clear();
    headLineObjectives_.clear();
    finalObjectives_.clear();
    initialObjectives_.reserve(MAX_LIST_SIZE);
    currentObjectives_.reserve(MAX_LIST_SIZE);
    fanoutObjectives_.reserve(MAX_LIST_SIZE);
    headLineObjectives_.reserve(MAX_LIST_SIZE);
    finalObjectives_.reserve(MAX_LIST_SIZE);

    unjustifiedGateIDs_.clear();
    dFrontiers_.clear();
    backtrackImplicatedGateIDs_.clear();
    backtrackDecisionTree_.clear();
    currentTargetHeadLineFault_ = Fault();  // NE
    firstTimeFrameHeadLine_ = NULL;
    unjustifiedGateIDs_.reserve(MAX_LIST_SIZE);
    dFrontiers_.reserve(MAX_LIST_SIZE);
    backtrackImplicatedGateIDs_.reserve(pCircuit_->totalGate_);
}

// **************************************************************************
// Function   [ Atpg::initializeCircuitWithFaultyGate ]
// Commenter  [ WWS ]
// Synopsis   [ usage:
//                Initialize the gates' atpgVal_ and vectors in Atpg (Ex:
// 								gateID_to_...)
//
// 							description:
// 								Traverse through all the gates in the circuit.
// 								If the gate is free line,
// 									Set gateID_to_valModified_ to true.
// 									(free line doesn't need to be implicated/backtraced)
// 								else
// 									Set gateID_to_valModified_ to false.
// 								Initialize this->gateID_to_reachableByTargetFault_ to all
// 								 false.(All gate not reachable as default)
// 								Assign all gates' atpgVal_ to X if isAtStageDTC is false.
// 								(Keep the atpgVal_ from previous single pattern generation
// 								on first selected target fault or updated atpgVal_ during
// 								previous iteration in DTC)
// 								Initialize whole this->xPathStatus_ to UNKNOWN for
// 								future xPathTracing().
// 								Set this->gateID_to_reachableByTargetFault_ to 1 and
// 								this->gateID_to_valModified_[gate.gateId_] to 0 for all
// 								the reachable fanout gate from the faultyGate.
//
// 							arguments:
// 								[in] faultyGate: The gate whose output is faulty.
//
// 								[in] isAtStageDTC: Specifies if the single pattern
// 								generation is at DTC stage.
//            ]
// Date       [ last modified 2023/01/06 ]
// **************************************************************************
void Atpg::initializeCircuitWithFaultyGate(Gate& gFaultyLine, bool isAtStageDTC) {
    for (Gate& gate : pCircuit_->circuitGates_) {
        if (gateID_to_lineType_[gate.gateId_] == FREE_LINE) {
            gateID_to_valModified_[gate.gateId_] = 1;
        } else {
            gateID_to_valModified_[gate.gateId_] = 0;
        }
        gateID_to_reachableByTargetFault_[gate.gateId_] = 0;

        // assign value outside the generateSinglePatternOnTargetFault for DTC, so
        // only need to initializeForSinglePatternGeneration it for primary fault.
        if (!isAtStageDTC) {
            gate.atpgVal_ = X;
        }
        gateID_to_xPathStatus_[gate.gateId_] = UNKNOWN;
    }

    pushGateToEventStack(gFaultyLine.gateId_);

    for (int i = gFaultyLine.numLevel_; i < pCircuit_->totalLvl_; ++i) {
        while (!circuitLevel_to_EventStack_[i].empty()) {
            int gateID = popEventStack(i);
            const Gate& rCurrentGate = pCircuit_->circuitGates_[gateID];
            gateID_to_valModified_[gateID] = 0;
            gateID_to_reachableByTargetFault_[gateID] = 1;

            for (int fanoutID : rCurrentGate.fanoutVector_) {
                pushGateToEventStack(fanoutID);
            }
        }
    }
}

// **************************************************************************
// Function   [ Atpg::clearEventStack ]
// Commenter  [ CAL WWS ]
// Synopsis   [ usage:
//                Clear this->circuitLevel_to_eventStack_.
// 								Set this->gateID_to_valModified_, this->isInEventStack_
// 								to 0.
//
// 							arguments:
// 								[in] isDebug: Check this->isInEventStack_ correctness if
// 								the flag is true.
//            ]
// Date       [ started 2020/07/07    last modified 2023/01/06 ]
// **************************************************************************
void Atpg::clearEventStack(bool isDebug) {
    // pop and remove mark
    for (std::stack<int>& eventStack : circuitLevel_to_EventStack_) {
        while (!eventStack.empty()) {
            int gateID = eventStack.top();
            eventStack.pop();
            gateID_to_valModified_[gateID] = 0;
            isInEventStack_[gateID] = 0;
        }
    }

    // expect all gates in circuit must be unmarked
    // after the above for-loop.
    if (isDebug) {
        for (int i = 0; i < pCircuit_->totalGate_; ++i) {
            if (isInEventStack_[i]) {
                std::cerr << "Bug: Warning clearEventStack found unexpected behavior\n";
                isInEventStack_[i] = 0;
                std::cin.get();
            }
        }
    }
}

// **************************************************************************
// Function   [ Atpg::doImplication ]
// Commenter  [ CLT WWS ]
// Synopsis   [ usage:
// 								Do BACKWARD and FORWARD implications to gates in
// 								this->circuitLevel_to_eventStack_.
//
// 							description:
// 								Enter a do while (backward) loop
// 								Loop content :
// 									IF the status is backward:
// 										Do evaluation backward starting from
//										this->circuitLevel_to_eventStack_[implicationStartLevel]
// 										to this->circuitLevel_to_eventStack_[0].
// 									Then, do evaluation() forward from
// 									this->circuitLevel_to_eventStack_[0..totalLevel]
// 								evaluateAndSetGateAtpgVal() will return
// 									FORWARD : do nothing
// 									BACKWARD :
// 										Do nothing if doing evaluations backward.
// 										If doing evaluations forward, immediately break current
// 										loop and go back to the loop doing backward evaluations
// 										in the event stack.
// 									CONFLICT : any failed evaluations
//
//              arguments:
// 								[in] atpgStatus: Indicating the current atpg implication
// 								direction (FORWARD or BACKWARD)
//
// 								[in] implicationStartLevel: The starting circuit level to do
// 								implications in this function.
//
//              output:
// 								A boolean,
// 								Return false if conflict after evaluateAndSetGateAtpgVal()
// 								Return true if no conflicts for all implications
//            ]
// Date       [ Ver. 1.0 started 2013/08/13 2023/01/06 ]
// **************************************************************************
bool Atpg::doImplication(IMPLICATION_STATUS atpgStatus, int startLevel) {
    IMPLICATION_STATUS impRet;

    if (atpgStatus != BACKWARD) {
        startLevel = 0;
    }

    do {
        if (atpgStatus == BACKWARD) {
            // BACKWARD loop: Do evaluateAndSetGateAtpgVal() to gates in circuitLevel_to_EventStack_ in BACKWARD order from startLevel.
            // If one of them returns CONFLICT, doImplication() returns false.
            for (int i = startLevel; i >= 0; --i) {
                while (!circuitLevel_to_EventStack_[i].empty()) {
                    Gate* pGate = &pCircuit_->circuitGates_[popEventStack(i)];
                    impRet = evaluateAndSetGateAtpgVal(pGate);
                    if (impRet == CONFLICT) {
                        return false;
                    }
                }
            }
        }

        atpgStatus = FORWARD;
        for (int i = 0; i < pCircuit_->totalLvl_; ++i) {
            // FORWARD loop: Do evaluateAndSetGateAtpgVal() to gates in circuitLevel_to_EventStack_ in FORWARD order till it gets to MaxLevel.
            // If one of them returns CONFLICT, doImplication() returns false.
            // If one of them returns BACKWARD, set startLevel to current level - 1, break for loop
            while (!circuitLevel_to_EventStack_[i].empty()) {
                Gate* pGate = &pCircuit_->circuitGates_[popEventStack(i)];
                impRet = evaluateAndSetGateAtpgVal(pGate);
                if (impRet == CONFLICT) {
                    return false;
                } else if (impRet == BACKWARD) {
                    startLevel = i - 1;
                    atpgStatus = BACKWARD;
                    break;
                }
            }

            if (atpgStatus == BACKWARD) {
                break;
            }
        }
    } while (atpgStatus == BACKWARD);

    return true;
}

// **************************************************************************
// Function   [ Atpg::doOneGateBackwardImplication ]
// Commenter  [ WWS ]
// Synopsis   [ usage:
// 								Do backward implication on one gate.
//
// 							descriptions:
// 								This function is specific designed for
//								evaluateAndSetGateAtpgVal() to call when pGate's
// 								atpgVal_ can’t be evaluated due to the lack of determined
// 								gate inputs’ values.
// 								This function is aimed to keep doing implication backward
// 								starting from pGate.
// 								It will return FORWARD when reach PI/PPI or is unable to
// 								justify atpgVal_, otherwise it will return BACKWARD.
// 								Note that this function will never return CONFLICT.
//
// 							arguments:
// 								[in] pGate: The pointer to the gate to do backward
// 								implication on.
//
// 							output:
// 								IMPLICATION_STATUS,
// 								Whether to implicate forward or backward
//            ]
// Date       [ last modified 2023/01/06 ]
// **************************************************************************
Atpg::IMPLICATION_STATUS Atpg::doOneGateBackwardImplication(Gate* pGate) {
    IMPLICATION_STATUS implicationStatus = FORWARD;
    if (pGate->gateType_ == Gate::PI || pGate->gateType_ == Gate::PPI) {
        return FORWARD;
    }

    if (pGate->gateType_ == Gate::BUF || pGate->gateType_ == Gate::INV || pGate->gateType_ == Gate::PO || pGate->gateType_ == Gate::PPO) {
        Gate* pImpGate = &pCircuit_->circuitGates_[pGate->faninVector_[0]];
        gateID_to_valModified_[pGate->gateId_] = 1;

        Value isINV = pGate->gateType_ == Gate::INV ? H : L;
        pImpGate->atpgVal_ = cXOR2(pGate->atpgVal_, isINV);

        backtrackImplicatedGateIDs_.push_back(pImpGate->gateId_);
        pushGateToEventStack(pGate->faninVector_[0]);
        pushGateFanoutsToEventStack(pGate->faninVector_[0]);
        implicationStatus = BACKWARD;
    } else if (pGate->gateType_ == Gate::XOR2 || pGate->gateType_ == Gate::XNOR2) {
        Gate* pInputGate0 = &pCircuit_->circuitGates_[pGate->faninVector_[0]];
        Gate* pInputGate1 = &pCircuit_->circuitGates_[pGate->faninVector_[1]];

        implicationStatus = BACKWARD;

        if (pInputGate0->atpgVal_ == X && pInputGate1->atpgVal_ != X) {
            if (pGate->gateType_ == Gate::XOR2) {
                pInputGate0->atpgVal_ = cXOR2(pGate->atpgVal_, pInputGate1->atpgVal_);
            } else {
                pInputGate0->atpgVal_ = cXNOR2(pGate->atpgVal_, pInputGate1->atpgVal_);
            }
            gateID_to_valModified_[pGate->gateId_] = 1;
            backtrackImplicatedGateIDs_.push_back(pInputGate0->gateId_);
            pushGateToEventStack(pGate->faninVector_[0]);
            pushGateFanoutsToEventStack(pGate->faninVector_[0]);
        } else if (pInputGate1->atpgVal_ == X && pInputGate0->atpgVal_ != X) {
            if (pGate->gateType_ == Gate::XOR2) {
                pInputGate1->atpgVal_ = cXOR2(pGate->atpgVal_, pInputGate0->atpgVal_);
            } else {
                pInputGate1->atpgVal_ = cXNOR2(pGate->atpgVal_, pInputGate0->atpgVal_);
            }
            gateID_to_valModified_[pGate->gateId_] = 1;
            backtrackImplicatedGateIDs_.push_back(pInputGate1->gateId_);
            pushGateToEventStack(pGate->faninVector_[1]);
            pushGateFanoutsToEventStack(pGate->faninVector_[1]);
        } else {
            implicationStatus = FORWARD;
            unjustifiedGateIDs_.push_back(pGate->gateId_);
        }
    } else if (pGate->gateType_ == Gate::XOR3 || pGate->gateType_ == Gate::XNOR3) {
        Gate* pInputGate0 = &pCircuit_->circuitGates_[pGate->faninVector_[0]];
        Gate* pInputGate1 = &pCircuit_->circuitGates_[pGate->faninVector_[1]];
        Gate* pInputGate2 = &pCircuit_->circuitGates_[pGate->faninVector_[2]];
        unsigned NumOfX = 0;
        unsigned ImpPtr = 0;
        if (pInputGate0->atpgVal_ == X) {
            ++NumOfX;
            ImpPtr = 0;
        }
        if (pInputGate1->atpgVal_ == X) {
            ++NumOfX;
            ImpPtr = 1;
        }
        if (pInputGate2->atpgVal_ == X) {
            ++NumOfX;
            ImpPtr = 2;
        }
        if (NumOfX == 1) {
            Gate* pImpGate = &pCircuit_->circuitGates_[pGate->faninVector_[ImpPtr]];
            Value temp;
            if (ImpPtr == 0) {
                temp = cXOR3(pGate->atpgVal_, pInputGate1->atpgVal_, pInputGate2->atpgVal_);
            } else if (ImpPtr == 1) {
                temp = cXOR3(pGate->atpgVal_, pInputGate0->atpgVal_, pInputGate2->atpgVal_);
            } else {
                temp = cXOR3(pGate->atpgVal_, pInputGate1->atpgVal_, pInputGate0->atpgVal_);
            }

            if (pGate->gateType_ == Gate::XNOR3) {
                temp = cINV(temp);
            }
            pImpGate->atpgVal_ = temp;
            gateID_to_valModified_[pGate->gateId_] = 1;
            backtrackImplicatedGateIDs_.push_back(pImpGate->gateId_);
            pushGateToEventStack(pGate->faninVector_[ImpPtr]);
            pushGateFanoutsToEventStack(pGate->faninVector_[ImpPtr]);
            implicationStatus = BACKWARD;
        } else {
            unjustifiedGateIDs_.push_back(pGate->gateId_);
            implicationStatus = FORWARD;
        }
    } else {
        Value OutputControlVal = pGate->getOutputCtrlValue();
        Value InputControlVal = pGate->getInputCtrlValue();

        if (pGate->atpgVal_ == OutputControlVal) {
            gateID_to_valModified_[pGate->gateId_] = 1;
            Value InputNonControlVal = pGate->getInputNonCtrlValue();

            for (int i = 0; i < pGate->numFI_; ++i) {
                Gate* pFaninGate = &pCircuit_->circuitGates_[pGate->faninVector_[i]];
                if (pFaninGate->atpgVal_ == X) {
                    pFaninGate->atpgVal_ = InputNonControlVal;
                    backtrackImplicatedGateIDs_.push_back(pFaninGate->gateId_);
                    pushGateToEventStack(pGate->faninVector_[i]);
                    pushGateFanoutsToEventStack(pGate->faninVector_[i]);
                }
            }
            implicationStatus = BACKWARD;
        } else {
            int NumOfX = 0;
            int ImpPtr = 0;
            for (int i = 0; i < pGate->numFI_; ++i) {
                Gate* pFaninGate = &pCircuit_->circuitGates_[pGate->faninVector_[i]];
                if (pFaninGate->atpgVal_ == X) {
                    ++NumOfX;
                    ImpPtr = i;
                }
            }

            if (NumOfX == 1) {
                Gate* pImpGate = &pCircuit_->circuitGates_[pGate->faninVector_[ImpPtr]];
                pImpGate->atpgVal_ = InputControlVal;
                gateID_to_valModified_[pGate->gateId_] = 1;
                backtrackImplicatedGateIDs_.push_back(pImpGate->gateId_);
                pushGateToEventStack(pGate->faninVector_[ImpPtr]);
                pushGateFanoutsToEventStack(pGate->faninVector_[ImpPtr]);
                implicationStatus = BACKWARD;
            } else {
                unjustifiedGateIDs_.push_back(pGate->gateId_);
                implicationStatus = FORWARD;
            }
        }
    }
    return implicationStatus;
}

// **************************************************************************
// Function   [ Atpg::backtrack ]
// Commenter  [ CLT WWS ]
// Synopsis   [ usage:
// 								When we backtrack a single gate in the decision tree,
// 								we need to recover all the associated implications
// 								starting from the startPoint of its DecisionTreeNode as
// 								a index in this->backtrackImplicatedGateIDs_.
//
// 							description:
// 								Check if the decisionTree_.get(...) is true
// 								If true : DecisionTreeNode already marked,
// 									Pop it from decision tree and check next bottom node.
// 								Else :
// 									Update the unjustified lines.
// 								Backtrack the gate.atpgVal_ from previous decisionTree_.get(),
// 								reset all the gate in this->backtrackImplicatedGateIDs_ to
// 								not modified and the value to unknown. Recalculate the
// 								backward implication level, reconstruct the event stack,
// 								this->dFrontiers_, this->unjustifiedGateIDs_,
// 								this->xPathStatus.
// 								return true, indicating backtrack successful.
// 								If no more gate(node) in decision tree to backtrack than
// 								return false, indicating backtrack failed, fault untestable
//
// 							arguments:
// 								[in, out] backwardImplicationLevel: The backward implication level
// 								of current single pattern generation, will be updated in
// 								this function if any backtrack happened.
//
//              output:
// 								A boolean indicating whether backtrack has failed, if
// 								failed the whole current target fault is determined as
// 								untestable in this atpg algorithm.
//            ]
// Date       [ Ver. 1.0 started 2013/08/13 last modified 2023/01/06 ]
// **************************************************************************
//
// The decision gates are put in the decisionTree.
// The associated implications of corresponding decision gates are put in
// this->backtrackImplicatedGateIDs_.
//
// When we backtrack a single gate in decision tree, we need to recover all
// the associated implications starting from the startPoint in
// this->backtrackImplicatedGateIDs_.
//
//                       +------+------+
//  DecisionTreeNodes  : | G0=1 | G1=0 |...
//                       +------+------+
//       startPoint of G0=1 |        \ startPoint of G1=0
//                          V         \						.
//                       +----+----++----+----+----+
//  backtrackList:       |G4=0|G7=1||G6=1|G9=0|G8=0|...
//                       +----+----++----+----+----+
//                       \____ ____/\______ _______/
//                            V            V
// associated implications of G0=1         associated implications of G1=0
//
// **************************************************************************
bool Atpg::backtrack(int& backwardImplicationLevel) {
    int backtrackPoint = 0;
    int mDecisionGateID;
    Value Val;
    Gate* pDecisionGate = NULL;
    // backtrackImplicatedGateIDs_ is for backtrack

    while (!backtrackDecisionTree_.empty()) {
        // get the last node in backtrackDecisionTree_ as backtrackPoint
        // 找到没有被标记过的决策树的节点，该节点的gateID赋值给mDecisionGateID，该节点在backtrackList中的位置赋值给backtrackPoint
        if (backtrackDecisionTree_.get(mDecisionGateID, backtrackPoint)) {
            continue;
        }
        // pDecisionGate is the bottom node of backtrackDecisionTree_

        updateUnjustifiedGateIDs();
        pDecisionGate = &pCircuit_->circuitGates_[mDecisionGateID];  // mDecisionGateID指向的门
        Val = cINV(pDecisionGate->atpgVal_);

        // 从backtrackPoint开始往后遍历backtrackImplicatedGateIDs_，重置所有门的atpgVal_，将所有门的状态重置为没有被回溯且蕴含
        for (int i = backtrackPoint; i < (int)backtrackImplicatedGateIDs_.size(); ++i) {
            // Reset gates and their ouput in backtrackImplicatedGateIDs_, starts from its backtrack point.
            Gate* pGate = &pCircuit_->circuitGates_[backtrackImplicatedGateIDs_[i]];

            pGate->atpgVal_ = X;
            gateID_to_valModified_[pGate->gateId_] = 0;

            for (int j = 0; j < pGate->numFO_; ++j) {
                Gate* pFanoutGate = &pCircuit_->circuitGates_[pGate->fanoutVector_[j]];
                gateID_to_valModified_[pFanoutGate->gateId_] = 0;
            }
        }

        backwardImplicationLevel = 0;

        // 从backtrackPoint后一个点开始往后遍历backtrackImplicatedGateIDs_，遍历每个backtrackPoint的扇出门，unjustifiedGateIDs_中放入atpgVal_值确定但还没有被回溯或者蕴含的门，backwardImplicationLevel赋值为最大扇出level
        for (int i = backtrackPoint + 1; i < (int)backtrackImplicatedGateIDs_.size(); ++i) {
            // Find MAX level output in backtrackImplicatedGateIDs_ and save it to backwardImplicationLevel
            Gate* pGate = &pCircuit_->circuitGates_[backtrackImplicatedGateIDs_[i]];

            for (int j = 0; j < pGate->numFO_; ++j) {
                Gate* pFanoutGate = &pCircuit_->circuitGates_[pGate->fanoutVector_[j]];

                if (pFanoutGate->atpgVal_ != X) {
                    if (!gateID_to_valModified_[pFanoutGate->gateId_]) {
                        unjustifiedGateIDs_.push_back(pFanoutGate->gateId_);
                    }
                    pushGateToEventStack(pFanoutGate->gateId_);
                }

                if (pFanoutGate->numLevel_ > backwardImplicationLevel) {
                    backwardImplicationLevel = pFanoutGate->numLevel_;
                }
            }
        }

        backtrackImplicatedGateIDs_.resize(backtrackPoint + 1);  // cut the last backtracked point and its associated gates
        pDecisionGate->atpgVal_ = Val;                           // toggle its value, do backtrack, ex: 1=>0, 0=>1（这个是决策树，上层里面的那个门）

        if (gateID_to_lineType_[pDecisionGate->gateId_] == HEAD_LINE) {
            gateID_to_valModified_[pDecisionGate->gateId_] = 0;
        } else {
            pushGateToEventStack(pDecisionGate->gateId_);
        }

        pushGateFanoutsToEventStack(pDecisionGate->gateId_);

        Gate* pFaultyGate = &pCircuit_->circuitGates_[currentTargetFault_.gateID_];

        dFrontiers_.clear();
        dFrontiers_.push_back(pFaultyGate->gateId_);
        updateDFrontiers();

        // Update unjustifiedGateIDs_ list
        for (int k = (int)unjustifiedGateIDs_.size() - 1; k >= 0; --k) {
            if (pCircuit_->circuitGates_[unjustifiedGateIDs_[k]].atpgVal_ == X) {
                vecDelete(unjustifiedGateIDs_, k);
            }
        }
        // Reset xPathStatus
        std::fill(gateID_to_xPathStatus_.begin(), gateID_to_xPathStatus_.end(), UNKNOWN);
        return true;
    }
    return false;
}

// **************************************************************************
// Function   [ Atpg::continuationMeaningful ]
// Commenter  [ WWS ]
// Synopsis   [ usage:
// 								Used in single pattern generation to see if it is
// 								meaningful to continue.
//
// 							description
// 								First call updateUnjustifiedLines().
// 								If any gate in this->initialObjectives_ is modified,
// 								pop it from this->initialObjectives_
// 								If the atpgVal_ of last D-Frontier has changed
// 								or all initial objectives modified(initIObjectives_.empty()),
// 								there is no need to continue doing the backtrace based on
// 								the current status.
//
// 							arguments:
// 								[in] pLastDFrontier: The last d-frontier in this->dFrontiers
//
// 							output:
// 								A boolean indicating if continuation in atpg is meaningful.
//            ]
// Date       [ last modified 2023/01/06 ]
// **************************************************************************
bool Atpg::continuationMeaningful(Gate* pLastDFrontier) {
    bool fDFrontierChanged;  // fDFrontierChanged is true when D-frontier must change
    // update unjustified lines
    updateUnjustifiedGateIDs();

    // if the initial object is modified, delete it from initialObjectives_ list
    for (int k = (int)initialObjectives_.size() - 1; k >= 0; --k) {
        if (gateID_to_valModified_[initialObjectives_[k]]) {
            vecDelete(initialObjectives_, k);
        }
    }

    // determine the pLastDFrontier should be changed or not
    if (pLastDFrontier != NULL) {
        if (pLastDFrontier->atpgVal_ == X) {
            fDFrontierChanged = false;
        } else {
            fDFrontierChanged = true;
        }
    } else {
        fDFrontierChanged = true;
    }
    // If all init. objectives have been implied or the last D-frontier has changed, reset backtrace atpgStatus
    return !(initialObjectives_.empty() || fDFrontierChanged);
}

// **************************************************************************
// Function   [ Atpg::updateUnjustifiedGateIDs ]
// Commenter  [ CAL WWS ]
// Synopsis   [ usage: Update this->unjustifiedGateIDs_.
//
// 							description:
// 								Traverse all gates in this->unjustifiedGateIDs_, if any
// 								gate was put into unjustified list but was implied
// 								afterwards by other gates, remove those gates from
// 								this->unjustifiedGateIDs_.
// 								IF modifiedGate is modified,
// 									Delete it from this->unjustifiedGateIDs_
// 								Else
// 									Push modifiedGate into this->finalObjectives_
//            ]
// Date       [ Ver. 1.0 started 2013/08/13 last modified 2023/01/06 ]
// **************************************************************************
void Atpg::updateUnjustifiedGateIDs() {
    // scan all gates in unjustifiedGateIDs_ List, if some gates were put into unjustified list but were implied afterwards by other gates, remove those gates from the unjustified list.
    // if gateID_to_valModified_[mGate.gateId_]  == true, delete it from unjustifiedGateIDs_ List
    // else push mGate into finalObjectives_ List
    for (int i = unjustifiedGateIDs_.size() - 1; i >= 0; --i) {
        Gate& mGate = pCircuit_->circuitGates_[unjustifiedGateIDs_[i]];
        if (gateID_to_valModified_[mGate.gateId_]) {
            vecDelete(unjustifiedGateIDs_, i);
        } else {
            gateID_to_valModified_[mGate.gateId_] = 0;
            finalObjectives_.push_back(mGate.gateId_);
        }
    }

    // pop all element from finalObjectives_ and set it's gateID_to_valModified_ to false till finalObjectives_ is empty
    int gateID;
    while (!finalObjectives_.empty()) {
        gateID = vecPop(finalObjectives_);
        gateID_to_valModified_[gateID] = 0;
    }
}

// **************************************************************************
// Function   [ Atpg::updateDFrontiers ]
// Commenter  [ CAL WWS ]
// Synopsis   [ usage: Update this->dFrontiers_.
// 							description:
// 								Remove determined d-frontiers and add new propagated
// 								d-frontiers into this->dFrontiers_.
//            ]
// Date       [ Ver. 1.0 started 2013/08/13 last modified 2023/01/06 ]
// **************************************************************************
void Atpg::updateDFrontiers() {
    for (int i = 0; i < dFrontiers_.size();) {
        Gate& mGate = pCircuit_->circuitGates_[dFrontiers_[i]];
        if (mGate.atpgVal_ == D || mGate.atpgVal_ == B) {
            for (int j = 0; j < mGate.numFO_; ++j) {
                dFrontiers_.push_back(mGate.fanoutVector_[j]);
            }
            vecDelete(dFrontiers_, i);
        } else if (mGate.atpgVal_ == X) {
            ++i;
        } else {
            vecDelete(dFrontiers_, i);
        }
    }
}

// **************************************************************************
// Function   [ Atpg::checkIfFaultHasPropagatedToPO ]
// Commenter  [ WWS ]
// Synopsis   [ usage: Check if fault has propagated to PO/PPO.
//
// 							description:
// 								If there is any D or B at PO/PPO, assign
// 								faultHasPropagatedToPO to true and return true.
// 								Otherwise assign false and return false.
//
// 							arguments:
// 								[in, out] faultHasPropagatedToPO: Will be assigned to true
//								if the fault has propagated to PO/PPO.
//
// 							output:
// 								A boolean value same to faultHasPropagatedToPO
//            ]
// Date       [ last modified 2023/01/06 ]
// **************************************************************************
bool Atpg::checkIfFaultHasPropagatedToPO(bool& faultHasPropagatedToPO) {
    // see if there is any D or B at PO/PPO?
    // i.e. The fault has propagated to the PO/PPO
    for (int i = 0; i < pCircuit_->numPO_ + pCircuit_->numPPI_; ++i) {
        const Value& v = pCircuit_->circuitGates_[pCircuit_->totalGate_ - i - 1].atpgVal_;
        if (v == D || v == B) {
            faultHasPropagatedToPO = true;
            return true;
        }
    }
    faultHasPropagatedToPO = false;
    return false;
}

// **************************************************************************
// Function   [ Atpg::checkForUnjustifiedBoundLines ]
// Commenter  [ WWS ]
// Synopsis   [ usage: Check for any left unjustified bound lines.
//
// 							output:
// 								A boolean indicating if any unjustified bound lines are
// 								left in current single pattern generation.
//            ]
// Date       [ last modified 2023/01/06 ]
// **************************************************************************
bool Atpg::checkForUnjustifiedBoundLines() {
    // Find if there exists any unjustified bound line
    for (int i = 0; i < unjustifiedGateIDs_.size(); ++i) {
        Gate* pGate = &pCircuit_->circuitGates_[unjustifiedGateIDs_[i]];
        if (pGate->atpgVal_ != X && !gateID_to_valModified_[pGate->gateId_] && gateID_to_lineType_[pGate->gateId_] == BOUND_LINE) {  // unjustified bound line
            return true;
        }
    }
    return false;
}

// **************************************************************************
// Function   [ Atpg::findFinalObjective ]
// Commenter  [ WYH WWS ]
// Synopsis   [ usage: Determination of final objectives.
//
// 							description:
// 								Choose a value and a line such that if the chosen value is
// 								assigned to the chosen line the initial objectives will be
// 								satisfied.
//
//              arguments:
// 								[in, out] backtraceFlag: It indicates the backtrace
// 								atpgStatus.
//
//                [in] faultCanPropToPO: It indicates whether the
//                 fault signal can propagate to PO/PPO or not.
//
//                [in, out] pLastDFrontier: A pointer reference of pointer
// 								of the last d-frontier in single pattern generation.
//            ]
// Date       [ WYH Ver. 1.0 started 2013/08/15 last modified 2023/01/06 ]
// **************************************************************************
void Atpg::findFinalObjective(BACKTRACE_STATUS& backtraceFlag, const bool& faultCanPropToPO, Gate*& pLastDFrontier) {
    int index;
    Gate* pGate = NULL;
    BACKTRACE_RESULT result;
    int finalObjectiveId = -1;

    while (true) {
        // IS BACKTRACE FLAG ON?
        if (backtraceFlag == INITIAL) {  // YES
            // RESET BACKTRACE FLAG
            backtraceFlag = FAN_OBJ_DETERMINE;
            // set the times of objective 0 and objective 1 of the gate to be zero
            // AND LET ALL THE SETS OF OBJECTIVES BE EMPTY
            for (const int& gateID : gateIDsToResetAfterBackTrace_) {
                setGaten0n1(gateID, 0, 0);
            }
            gateIDsToResetAfterBackTrace_.clear();
            clearAllObjectives();

            // IS THERE ANY UNJUSTIFIED LINE?
            if (!unjustifiedGateIDs_.empty()) {  // YES
                // LET ALL THE UNJUSTIFIED LINES BE THE SET OF INITIAL OBJECTIVES
                initialObjectives_ = unjustifiedGateIDs_;

                // FAULT SIGNAL PROPAGATED TO A PRIMARY OUTPUT?
                if (faultCanPropToPO)  // YES
                {                      // do not add any gates
                    pLastDFrontier = NULL;
                } else {  // NO
                    // ADD A GATE IN D-FRONTIER TO THE SET OF INITIAL OBJECTIVES
                    pLastDFrontier = findClosestToPO(dFrontiers_, index);
                    initialObjectives_.push_back(pLastDFrontier->gateId_);
                }
            } else {  // NO
                // ADD A GATE IN D-FRONTIER TO THE SET OF INITIAL OBJECTIVES
                pLastDFrontier = findClosestToPO(dFrontiers_, index);
                initialObjectives_.push_back(pLastDFrontier->gateId_);
            }

            // A
            // MULTIPLE BACKTRACE FROM THE SET OF INITIAL OBJECTIVES
            result = multipleBacktrace(INITIAL, finalObjectiveId);
            // CONTRADICTORY REQUIREMENT AT A FANOUT-POINT OCCURRED?
            if (result == CONTRADICTORY) {  // YES
                // LET THE FANOUT-POINT OBJECTIVE BE FINAL OBJECTIVE TO ASSIGN VALUE
                finalObjectives_.push_back(finalObjectiveId);
                // EXIT
                return;
            }
        } else {  // NO
            // IS THE SET OF FANOUT-POINT OBJECTIVE EMPTY?
            if (!fanoutObjectives_.empty()) {  // NO
                // B
                // MULTIPLE BACKTRACE FROM A FANOUT-POINT OBJECTIVE
                result = multipleBacktrace(FAN_OBJ_DETERMINE, finalObjectiveId);
                // CONTRADICTORY REQUIREMENT AT A FANOUT-POINT OCCURRED?
                if (result == CONTRADICTORY) {  // YES
                    // LET THE FANOUT-POINT OBJECTIVE BE FINAL OBJECTIVE TO ASSIGN VALUE
                    finalObjectives_.push_back(finalObjectiveId);
                    // EXIT
                    return;
                }
            }
        }

        while (true) {
            // IS THE SET OF HEAD OBJECTIVES EMPTY?
            if (headLineObjectives_.empty()) {  // YES
                backtraceFlag = INITIAL;
                break;
            } else {  // NO
                // TAKE OUT A HEAD OBJECTIVE
                pGate = &pCircuit_->circuitGates_[vecPop(headLineObjectives_)];
                // IS THE HEAD LINE UNSPECIFIED?
                if (pGate->atpgVal_ == X) {  // YES
                    // LET THE HEAD OBJECTIVE BE FINAL OBJECTIVE
                    finalObjectives_.push_back(pGate->gateId_);
                    // EXIT
                    return;
                }
            }
        }
    }
}

// **************************************************************************
// Function   [ Atpg::clearAllObjectives ]
// Commenter  [ WWS ]
// Synopsis   [ usage:
// 								Clear and reinitialize all the objectives.
//            ]
// Date       [ last modified 2023/01/06 ]
// **************************************************************************
void Atpg::clearAllObjectives() {
    initialObjectives_.clear();
    currentObjectives_.clear();
    fanoutObjectives_.clear();
    headLineObjectives_.clear();
    finalObjectives_.clear();
    initialObjectives_.reserve(MAX_LIST_SIZE);
    currentObjectives_.reserve(MAX_LIST_SIZE);
    fanoutObjectives_.reserve(MAX_LIST_SIZE);
    headLineObjectives_.reserve(MAX_LIST_SIZE);
    finalObjectives_.reserve(MAX_LIST_SIZE);
}

// **************************************************************************
// Function   [ Atpg::assignAtpgValToFinalObjectiveGates]
// Commenter  [ WWS ]
// Synopsis   [ usage:
// 								Literal meaning of this function name.
//
// 							description:
// 								Decide the atpgVal_ of final objective gates by n0 and n1
// 								calculated by previous multiple backtrace.
//            ]
// Date       [ last modified 2023/01/06 ]
// **************************************************************************
void Atpg::assignAtpgValToFinalObjectiveGates() {
    while (!finalObjectives_.empty()) {  // while exist any finalObject
        Gate* pGate = &pCircuit_->circuitGates_[vecPop(finalObjectives_)];

        // judge the value by numOfZero and numOfOne
        if (gateID_to_n0_[pGate->gateId_] > gateID_to_n1_[pGate->gateId_]) {
            pGate->atpgVal_ = L;
        } else {
            pGate->atpgVal_ = H;
        }

        // put decision of the finalObjective into decisionTree
        backtrackDecisionTree_.put(pGate->gateId_, (int)backtrackImplicatedGateIDs_.size());
        // record this gate and backtrace later
        backtrackImplicatedGateIDs_.push_back(pGate->gateId_);

        if (gateID_to_lineType_[pGate->gateId_] == HEAD_LINE) {
            gateID_to_valModified_[pGate->gateId_] = 1;
        } else {
            pushGateToEventStack(pGate->gateId_);
        }
        pushGateFanoutsToEventStack(pGate->gateId_);
    }
}

// **************************************************************************
// Function   [ Atpg::justifyFreeLines ]
// Commenter  [ CLT WWS ]
// Synopsis   [ usage:
// 								Justify free lines before terminating current
// 								single pattern generation.
//
//              arguments:
// 								[in] originalFault: The original target fault for
// 								single pattern generation.
//            ]
// Date       [ Ver. 1.0 started 2013/08/13 last modified 2023/01/06 ]
// **************************************************************************
void Atpg::justifyFreeLines(Fault& originalFault) {
    Gate* pHeadLineFaultGate = NULL;

    int gateID = currentTargetHeadLineFault_.gateID_;

    if (gateID != -1) {
        pHeadLineFaultGate = &pCircuit_->circuitGates_[gateID];
    }

    // scan each HEADLINE
    for (int i = 0; i < numOfheadLines_; ++i) {
        Gate* pGate = &pCircuit_->circuitGates_[headLineGateIDs_[i]];
        if (pGate->prevAtpgValStored_ == pGate->atpgVal_) {
            continue;
        }
        if (pHeadLineFaultGate == pGate) {  // if the HEADLINE scanned now(pGate) is the new faulty gate
            // If the Original Fault is on a FREE LINE, map it back
            restoreFault(originalFault);
            continue;
        }
        // for other HEADLINE, set D or D' to H or L respectively,
        if (pGate->atpgVal_ == D) {
            pGate->atpgVal_ = H;
        } else if (pGate->atpgVal_ == B) {
            pGate->atpgVal_ = L;
        }

        if (!(pGate->gateType_ == Gate::PI || pGate->gateType_ == Gate::PPI || pGate->atpgVal_ == X)) {
            fanoutFreeBacktrace(pGate);
        }
    }
}

// **************************************************************************
// Function   [ Atpg::restoreFault ]
// Commenter  [ CLT WWS ]
// Synopsis   [ usage: Restore the faulty gate to the original position.
//
// 							description:
// 								This function is called because when the original target
// 								fault is injected at an gate input, it will then be
// 								modified to equivalent headline fault and set to the
//								corresponding gate.atpgVal_. We need to the revert the
// 								previously mentioned operation for latter algorithm in atpg.
//
//              arguments:
// 								[in] originalFault: The original target fault.
//            ]
// Date       [ Ver. 1.0 started 2013/08/13 last modified 2023/01/06 ]
// **************************************************************************
void Atpg::restoreFault(Fault& originalFault) {
    Gate* pFaultPropGate = NULL;

    fanoutObjectives_.clear();
    pFaultPropGate = &pCircuit_->circuitGates_[originalFault.gateID_];

    if (originalFault.faultyLine_ == 0) {
        fanoutObjectives_.push_back(pFaultPropGate->gateId_);
    }

    // To fix bugs of free line fault which is at gate input
    for (int i = 0; i < pFaultPropGate->numFI_; ++i) {
        Gate* pFaninGate = &pCircuit_->circuitGates_[pFaultPropGate->faninVector_[i]];
        if (pFaninGate->atpgVal_ == D) {
            pFaninGate->atpgVal_ = H;
        } else if (pFaninGate->atpgVal_ == B) {
            pFaninGate->atpgVal_ = L;
        }

        if (pFaninGate->atpgVal_ == L || pFaninGate->atpgVal_ == H) {
            fanoutObjectives_.push_back(pFaninGate->gateId_);
        }
    }

    // push original fault gate into fanoutObjectives_ list

    // for each loop, scan all fanin gates of pFaultPropGate (initial to be original fault gate)
    // if fanin gates' value == 0 or 1, add it into fanoutObjectives_ list iteratively
    // let pFaultPropGate's output gate (FREE_LINE only have one output gate) be new pFaultPropGate
    // and perform the procedure of each loop till pFaultPropGate is HEADLINE
    if (gateID_to_lineType_[pFaultPropGate->gateId_] == FREE_LINE) {
        while (pFaultPropGate->numFO_ > 0) {
            pFaultPropGate = &pCircuit_->circuitGates_[pFaultPropGate->fanoutVector_[0]];
            for (int i = 0; i < pFaultPropGate->numFI_; ++i) {
                Gate* pFaninGate = &pCircuit_->circuitGates_[pFaultPropGate->faninVector_[i]];
                if (pFaninGate->atpgVal_ == L || pFaninGate->atpgVal_ == H) {
                    fanoutObjectives_.push_back(pFaninGate->gateId_);
                }
            }
            if (gateID_to_lineType_[pFaultPropGate->gateId_] == HEAD_LINE) {
                break;
            }
        }
    }
    // each loop pop out an element in the fanoutObjectives_ list
    while (!fanoutObjectives_.empty()) {
        Gate* pGate = &pCircuit_->circuitGates_[vecPop(fanoutObjectives_)];
        // if the gate's value is D set to H, D' set to L
        if (pGate->atpgVal_ == D) {
            pGate->atpgVal_ = H;
        } else if (pGate->atpgVal_ == B) {
            pGate->atpgVal_ = L;
        }

        if (!(pGate->gateType_ == Gate::PI || pGate->gateType_ == Gate::PPI || pGate->atpgVal_ == X))  // if the gate's value not unknown and the gates type not PI or PPI
        {
            fanoutFreeBacktrace(pGate);
        }
    }
}

// **************************************************************************
// Function   [ Atpg::countEffectiveDFrontiers ]
// Commenter  [ WWS ]
// Synopsis   [ usage:
//								Update the this->dFrontiers to make sure the d-frontiers
// 								in it are all effective.
//
// 							Description:
// 								By effective we mean if a d-frontier is able to propagate
// 								to PO/PPO.
//
//              arguments:
// 								[in] pFaultyLineGate: The original target fault for single
// 								pattern generation.
//
// 							output:
//								The updated this->dFrontier.size().
//            ]
// Date       [ last modified 2023/01/06 ]
// **************************************************************************
int Atpg::countEffectiveDFrontiers(Gate* pFaultyLineGate) {
    // update the frontier
    updateDFrontiers();

    // Change the xPathStatus from XPATH_EXIST to UNKNOWN of a gate
    // which has equal or higher level than the faulty gate
    //(the gate array has been given its level already)
    // This is to clear the xPathStatus of previous xPathTracing
    for (int i = pFaultyLineGate->gateId_; i < pCircuit_->totalGate_; ++i) {
        Gate* pGate = &pCircuit_->circuitGates_[i];
        if (gateID_to_xPathStatus_[pGate->gateId_] == XPATH_EXIST) {
            gateID_to_xPathStatus_[pGate->gateId_] = UNKNOWN;
        }
    }

    // if D-frontier can't propagate to the PO, erase it
    for (int k = dFrontiers_.size() - 1; k >= 0; --k) {
        if (!xPathTracing(&pCircuit_->circuitGates_[dFrontiers_[k]])) {
            vecDelete(dFrontiers_, k);
        }
    }
    return dFrontiers_.size();
}

// **************************************************************************
// Function   [ Atpg::doUniquePathSensitization ]
// Commenter  [ CLT WWS ]
// Synopsis   [ usage:	Finds the last gate(pNextGate) in the uniquePath
// 											starting from pGate, return backwardImplicationLevel,
// 											which is the max of the pNextGate's input level.
// 											backwardImplicationLevel is -1 if no uniquePath.
//
// 							description:
// 								First check whether "gate" is the current target fault's
// 								gate. If not, we check the values of its fanin gates.
// 								If the value is unknown, set it to non-control value,
// 								push it into the this->backtrackImplicatedGateIDs,
// 								and call pushInputEvent function for this fanin gate.
// 								backwardImplicationLevel is set to the max of fanin level.
// 								If the value is control value, return
// 								UNIQUE_PATH_SENSITIZE_FAIL(-2).
// 								Now set the current gate to the input gate and enter the
// 								while loop.
// 								If the current gate is PO/PPO, or it has no
// 								dominators(excluded fanout free), leave the loop.
// 								Then set the next gate to the fanout gate
// 								(for fanout free gate) or its dominator.
// 								Now check whether the non-control value is not unknown
// 								and the gate is not unary.
// 								If false, do nothing, else we have two cases:
// 								fanout-free :
// 									Check the fanin gates of the next gate that is not the
// 									current gate. If its value is the control value,
// 									then return UNIQUE_PATH_SENSITIZE_FAIL. Otherwise if its
// 									value is unknown, set it to non-control value, push it
// 									into this->backtrackImplicatedGateIDs_.
// 									BackImpLevel is set to the maximum of fanin level.
// 								not fanout-free :
// 									Check the fanin gates of the next gate.
// 									If it is fault reachable, do nothing
// 									else check its value.
// 										If its value is the control value,
// 											return UNIQUE_PATH_SENSITIZE_FAIL.
// 										Otherwise if its value is unknown,
// 											set it to non-control value,
// 											push it into this->backtrackImplicatedGateIDs_,
// 											BackImpLevel is set to the maximum of fanin level.
// 								Finally, set the current gate to the next gate and start a
// 								new loop.
// 								Return BackImpLevel at last.
//
//              arguments:
// 								[in] gate : The gate to do unique sensitization on.
//
// 							output:
// 								int(backwardImplicationLevel)
//            ]
// Date       [ Ver. 1.0 started 2013/08/13 last modified 2023/01/06 ]
// **************************************************************************
int Atpg::doUniquePathSensitization(Gate& gate) {
    int backwardImplicationLevel = NO_UNIQUE_PATH;
    Gate* pFaultyGate = &pCircuit_->circuitGates_[currentTargetFault_.gateID_];

    // if gate is not the FaultyGate
    if (pFaultyGate->gateId_ != gate.gateId_) {
        Value NonControlVal = gate.getInputNonCtrlValue();

        // if gate has an NonControlVal (1 or 0)
        if (NonControlVal != X) {
            for (int i = 0; i < gate.numFI_; ++i) {
                Gate* pFaninGate = &pCircuit_->circuitGates_[gate.faninVector_[i]];
                if (pFaninGate->atpgVal_ == X) {
                    pFaninGate->atpgVal_ = NonControlVal;
                    if (backwardImplicationLevel < pFaninGate->numLevel_)  // backwardImplicationLevel becomes MAX of fan in level
                    {
                        backwardImplicationLevel = pFaninGate->numLevel_;
                    }

                    backtrackImplicatedGateIDs_.push_back(pFaninGate->gateId_);
                    pushGateToEventStack(gate.faninVector_[i]);
                    pushGateFanoutsToEventStack(gate.faninVector_[i]);
                } else if (pFaninGate->atpgVal_ == gate.getInputCtrlValue()) {
                    return UNIQUE_PATH_SENSITIZE_FAIL;
                }
            }
        }
    }

    Gate* pCurrGate = &gate;
    Gate* pNextGate = NULL;

    while (true) {
        std::vector<int>& UniquePathList = gateID_to_uniquePath_[pCurrGate->gateId_];

        // If pCurrGate is PO or PPO, break.
        if (pCurrGate->gateType_ == Gate::PO || pCurrGate->gateType_ == Gate::PPO) {
            break;
        } else if (pCurrGate->numFO_ == 1)  // If pCurrGate is fanout free, set pNextGate to its output gate.
        {
            pNextGate = &pCircuit_->circuitGates_[pCurrGate->fanoutVector_[0]];
        } else if (UniquePathList.size() == 0)  // If pCurrGate no UniquePath, break.
        {
            break;
        } else {
            pNextGate = &pCircuit_->circuitGates_[UniquePathList[0]];  // set pNextGate to UniquePathList[0].
        }

        Value NonControlVal = pNextGate->getInputNonCtrlValue();

        if (NonControlVal != X && !pNextGate->isUnary()) {  // If pNextGate is Unary and its NonControlVal is not unknown.
            if (pCurrGate->numFO_ == 1) {
                // If gCurrGate(pGate) is fanout free, pNextGate is gCurrGate's output gate.
                // Go through all pNextGate's input.
                for (int i = 0; i < pNextGate->numFI_; ++i) {
                    Gate* pFaninGate = &pCircuit_->circuitGates_[pNextGate->faninVector_[i]];

                    if (pFaninGate != pCurrGate && pNextGate->getInputCtrlValue() != X && pFaninGate->atpgVal_ == pNextGate->getInputCtrlValue()) {
                        return UNIQUE_PATH_SENSITIZE_FAIL;
                    }

                    if (pFaninGate != pCurrGate && pFaninGate->atpgVal_ == X) {
                        pFaninGate->atpgVal_ = NonControlVal;  // Set input gate of pNextGate to pNextGate's NonControlVal
                        if (backwardImplicationLevel < pFaninGate->numLevel_) {
                            backwardImplicationLevel = pFaninGate->numLevel_;
                        }
                        // backwardImplicationLevel becomes MAX of pNextGate's fanin level
                        backtrackImplicatedGateIDs_.push_back(pFaninGate->gateId_);
                        pushGateToEventStack(pNextGate->faninVector_[i]);
                        pushGateFanoutsToEventStack(pNextGate->faninVector_[i]);
                    }
                }
            } else {
                // gCurrGate(pGate) is not fanout free, pNextGate is UniquePathList[0].
                bool DependOnCurrent;
                for (int i = 0; i < pNextGate->numFI_; ++i) {
                    // Go through all pNextGate's input.
                    Gate* pFaninGate = &pCircuit_->circuitGates_[pNextGate->faninVector_[i]];
                    DependOnCurrent = false;

                    for (int j = 1; j < UniquePathList.size(); ++j) {
                        if (UniquePathList[j] == pFaninGate->gateId_) {
                            DependOnCurrent = true;
                            break;
                        }
                    }

                    if (!DependOnCurrent) {
                        if (pFaninGate->atpgVal_ != X && pFaninGate->atpgVal_ == pNextGate->getInputCtrlValue() && pNextGate->getInputCtrlValue() != X) {
                            return UNIQUE_PATH_SENSITIZE_FAIL;
                        }

                        if (pFaninGate->atpgVal_ != X) {
                            continue;
                        }

                        pFaninGate->atpgVal_ = NonControlVal;  // set to NonControlVal

                        if (backwardImplicationLevel < pFaninGate->numLevel_) {
                            backwardImplicationLevel = pFaninGate->numLevel_;  // backwardImplicationLevel becomes MAX of pNextGate's fanin level
                        }
                        backtrackImplicatedGateIDs_.push_back(pFaninGate->gateId_);
                        pushGateToEventStack(pNextGate->faninVector_[i]);
                        pushGateFanoutsToEventStack(pNextGate->faninVector_[i]);
                    }
                }
            }
        }
        pCurrGate = pNextGate;  // move to last gate in gateID_to_uniquePath_
    }
    return backwardImplicationLevel;
}

// **************************************************************************
// Function   [ Atpg::xPathExists ]
// Commenter  [ CAL WWS ]
// Synopsis   [ usage:
// 								Determine if xpath exist for "gate".
//
// 							description:
//                Used before generateSinglePatternOnTargetFault
//                Return true if there is X-path. Otherwise return false.
//
//              arguments:
// 								[in] gate: The gate to see if xpath exists.
//
//              output:
// 								A boolean indicating if the x path exists.
//            ]
// Date       [ started 2020/07/07    last modified 2023/01/06 ]
// **************************************************************************
bool Atpg::xPathExists(Gate* pGate) {
    // Clear the gateID_to_xPathStatus_ from target gate to PO/PPO
    // TODO: This part can be implemented by event-driven method
    for (int i = pGate->gateId_; i < pCircuit_->totalGate_; ++i) {
        gateID_to_xPathStatus_[i] = UNKNOWN;
    }
    return xPathTracing(pGate);
}

// **************************************************************************
// Function   [ Atpg::xPathTracing ]
// Commenter  [ CLT WWS ]
// Synopsis   [ usage:
// 								Determine if xpath exist for "gate".
//
// 							description:
// 								Recursive call the function itself with the fanout of
//								original pGate until PO/PPO is reached and check if pGate
// 								has a X path.
//
//              arguments:
// 								[in] gate: The gate to see if xpath exists.
//
//              output:
// 								A boolean indicating if the x path exists.
//            ]
// Date       [ Ver. 1.0 started 2013/08/13 last modified 2023/01/06 ]
// **************************************************************************
bool Atpg::xPathTracing(Gate* pGate) {
    if (pGate->atpgVal_ != X || gateID_to_xPathStatus_[pGate->gateId_] == NO_XPATH_EXIST) {
        gateID_to_xPathStatus_[pGate->gateId_] = NO_XPATH_EXIST;
        return false;
    }

    if (gateID_to_xPathStatus_[pGate->gateId_] == XPATH_EXIST) {
        return true;
    }

    if (pGate->gateType_ == Gate::PO || pGate->gateType_ == Gate::PPO) {
        gateID_to_xPathStatus_[pGate->gateId_] = XPATH_EXIST;
        return true;
    }

    for (int i = 0; i < pGate->numFO_; ++i) {
        // TO-DO homework 03
        if (xPathTracing(&(pCircuit_->circuitGates_[pGate->fanoutVector_[i]]))) {
            gateID_to_xPathStatus_[pGate->gateId_] = XPATH_EXIST;
            return true;
        }
        // end of TO-DO
    }
    gateID_to_xPathStatus_[pGate->gateId_] = NO_XPATH_EXIST;
    return false;
}

// **************************************************************************
// Function   [ Atpg::setFaultyGate ]
// Commenter  [ WYH WWS ]
// Synopsis   [ usage: Initial assignment of fault signal.
//
// 							description:
//                There are two situations :
//                1. Fault is on the input line of pFaultyGate, and
//                   pFaultyLineGate is the fanin gate of pFaultyGate
//                  (1) Activate the fault, and set value of pFaultyLineGate
// 			  			 				according to fault type.
//                  (2) According to the type of pFaultyGate, set other
//                      fanin gate of pFaultyGate to NoneControl value
//                      of pFaultyGate, and set value of pFaultyGate.
//                  (3) Schedule all fanout gates of fanin gates of
// 			  			 				pFaultyGate, and schedule fanout gates of pFaultyGate.
//                  (4) Update backwardImplicationLevel to be max level
//                      of fanin gates of pFaultyGate.
//                2. Fault is on the ouput line of pFaultyGate, and
//                   pFaultyLineGate is pFaultyGate.
//                  (1) Activate the fault, and set value of pFaultyLineGate
// 			  			  			according to fault type.
//                  (2) Schedule fanout gates of pFaultyGate.
//                  (3) If pFaultyGate is a HEADLINE, all it's fanin
//                      gates are FREE_LINE, no need to set value.
//                      Else, set the value of it's fanin gates, and
// 			  			 				schedule all fanout gates of fanin gates
// 											of pFaultyGate.
//                  (4) Update backwardImplicationLevel to be max level
//                      of fanin gates of pFaultyGate.
//              arguments:
//								[in] fault: The fault for setting value to gate.
//
//              output:
// 								The backwardImplicationLevel which indicates the backward
//                imply level,
// 								return -1 when fault FAULT_UNTESTABLE
//            ]
// Date       [ WYH Ver. 1.0 started 2013/08/16 last modified 2023/01/06 ]
// **************************************************************************
int Atpg::setFaultyGate(Fault& fault) {
    Value valueTemp;
    int backwardImplicationLevel = 0;
    Gate* pFaultyGate = NULL;
    Gate* pFaultyLine = NULL;                       // pFaultyLine is the gate before the fault (fanin gate of pFaultyGate)
    bool isOutputFault = (fault.faultyLine_ == 0);  // if the fault is SA0 or STR, then set faulty value to D (1/0)
    Value FaultyValue = (fault.faultType_ == Fault::SA0 || fault.faultType_ == Fault::STR) ? D : B;

    pFaultyGate = &pCircuit_->circuitGates_[fault.gateID_];
    // if the fault is input fault, the pFaultyLine is decided by fault.faultyLine_
    // else pFaultyLine is pFaultyGate
    if (!isOutputFault) {
        pFaultyLine = &pCircuit_->circuitGates_[pFaultyGate->faninVector_[fault.faultyLine_ - 1]];
    } else {
        pFaultyLine = pFaultyGate;
    }

    if (!isOutputFault) {
        if (FaultyValue == D && pFaultyLine->atpgVal_ != L) {
            pFaultyLine->atpgVal_ = H;
        } else if (FaultyValue == B && pFaultyLine->atpgVal_ != H) {
            pFaultyLine->atpgVal_ = L;
        } else {  // The fault can not propagate to PO
            return -1;
        }

        backtrackImplicatedGateIDs_.push_back(pFaultyLine->gateId_);

        if (pFaultyGate->gateType_ == Gate::AND2 || pFaultyGate->gateType_ == Gate::AND3 || pFaultyGate->gateType_ == Gate::AND4 || pFaultyGate->gateType_ == Gate::NAND2 || pFaultyGate->gateType_ == Gate::NAND3 || pFaultyGate->gateType_ == Gate::NAND4 || pFaultyGate->gateType_ == Gate::NOR2 || pFaultyGate->gateType_ == Gate::NOR3 || pFaultyGate->gateType_ == Gate::NOR4 || pFaultyGate->gateType_ == Gate::OR2 || pFaultyGate->gateType_ == Gate::OR3 || pFaultyGate->gateType_ == Gate::OR4) {
            // scan all fanin gate of pFaultyGate
            bool isFaultyGateScanned = false;
            for (int i = 0; i < pFaultyGate->numFI_; ++i) {
                Gate* pFaninGate = &pCircuit_->circuitGates_[pFaultyGate->faninVector_[i]];
                if (pFaninGate != pFaultyLine) {
                    if (pFaninGate->atpgVal_ == X) {
                        pFaninGate->atpgVal_ = pFaultyGate->getInputNonCtrlValue();
                        backtrackImplicatedGateIDs_.push_back(pFaninGate->gateId_);
                    } else if (pFaninGate->atpgVal_ != pFaultyGate->getInputNonCtrlValue()) {
                        // If the value has already been set, it should be
                        // non-control value, otherwise the fault can't propagate
                        return -1;
                    }
                } else {
                    if (isFaultyGateScanned == false) {
                        isFaultyGateScanned = true;
                    } else {
                        return -1;  // FAULT_UNTESTABLE FAULT
                    }
                }
            }
            valueTemp = pFaultyGate->isInverse();
            // find the pFaultyGate output value
            pFaultyGate->atpgVal_ = cXOR2(valueTemp, FaultyValue);
            backtrackImplicatedGateIDs_.push_back(pFaultyGate->gateId_);
        } else if (pFaultyGate->gateType_ == Gate::INV || pFaultyGate->gateType_ == Gate::BUF || pFaultyGate->gateType_ == Gate::PO || pFaultyGate->gateType_ == Gate::PPO) {
            valueTemp = pFaultyGate->isInverse();
            pFaultyGate->atpgVal_ = cXOR2(valueTemp, FaultyValue);
            backtrackImplicatedGateIDs_.push_back(pFaultyGate->gateId_);
        }

        if (pFaultyGate->atpgVal_ != X) {
            // schedule all the fanout gate of pFaultyGate
            pushGateFanoutsToEventStack(pFaultyGate->gateId_);
            gateID_to_valModified_[pFaultyGate->gateId_] = 1;
        }

        for (int i = 0; i < pFaultyGate->numFI_; ++i) {
            Gate* pFaninGate = &pCircuit_->circuitGates_[pFaultyGate->faninVector_[i]];
            if (pFaninGate->atpgVal_ != X) {
                // set the backwardImplicationLevel to be maximum of fanin gate's level
                if (backwardImplicationLevel < pFaninGate->numLevel_) {
                    backwardImplicationLevel = pFaninGate->numLevel_;
                }
                // schedule all fanout gates of the fanin gate
                // pushInputEvents(pFaultyGate->gateId_, i); replaced by following, by wang
                pushGateToEventStack(pFaultyGate->faninVector_[i]);
                pushGateFanoutsToEventStack(pFaultyGate->faninVector_[i]);
            }
        }
    } else {  // output fault
        if ((FaultyValue == D && pFaultyGate->atpgVal_ == L) || (FaultyValue == B && pFaultyGate->atpgVal_ == H)) {
            return -1;
        }
        pFaultyGate->atpgVal_ = FaultyValue;
        backtrackImplicatedGateIDs_.push_back(pFaultyGate->gateId_);
        // schedule all of fanout gate of the pFaultyGate
        pushGateFanoutsToEventStack(pFaultyGate->gateId_);
        // backtrace stops at HEAD LINE
        if (gateID_to_lineType_[pFaultyGate->gateId_] == HEAD_LINE) {
            gateID_to_valModified_[pFaultyGate->gateId_] = 1;
        } else if (pFaultyGate->gateType_ == Gate::INV || pFaultyGate->gateType_ == Gate::BUF || pFaultyGate->gateType_ == Gate::PO || pFaultyGate->gateType_ == Gate::PPO) {
            Gate* pFaninGate = &pCircuit_->circuitGates_[pFaultyGate->faninVector_[0]];
            gateID_to_valModified_[pFaultyGate->gateId_] = 1;

            Value Val = (FaultyValue == D) ? H : L;
            valueTemp = pFaultyGate->isInverse();
            pFaninGate->atpgVal_ = cXOR2(valueTemp, Val);
            backtrackImplicatedGateIDs_.push_back(pFaninGate->gateId_);
            pushGateToEventStack(pFaultyGate->faninVector_[0]);
            pushGateFanoutsToEventStack(pFaultyGate->faninVector_[0]);

            if (backwardImplicationLevel < pFaninGate->numLevel_) {
                backwardImplicationLevel = pFaninGate->numLevel_;
            }
        } else if ((FaultyValue == D && pFaultyGate->getOutputCtrlValue() == H) || (FaultyValue == B && pFaultyGate->getOutputCtrlValue() == L)) {
            gateID_to_valModified_[pFaultyGate->gateId_] = 1;
            // scan all fanin gate of pFaultyGate
            for (int i = 0; i < pFaultyGate->numFI_; ++i) {
                Gate* pFaninGate = &pCircuit_->circuitGates_[pFaultyGate->faninVector_[i]];
                if (pFaninGate->atpgVal_ == X) {
                    pFaninGate->atpgVal_ = pFaultyGate->getInputNonCtrlValue();
                    backtrackImplicatedGateIDs_.push_back(pFaninGate->gateId_);
                    // schedule all fanout gate of the pFaninGate
                    pushGateToEventStack(pFaultyGate->faninVector_[i]);
                    pushGateFanoutsToEventStack(pFaultyGate->faninVector_[i]);
                    if (backwardImplicationLevel < pFaninGate->numLevel_) {
                        backwardImplicationLevel = pFaninGate->numLevel_;
                    }
                }
            }
        } else {
            pushGateToEventStack(pFaultyGate->gateId_);
            if (backwardImplicationLevel < pFaultyGate->numLevel_) {
                backwardImplicationLevel = pFaultyGate->numLevel_;
            }
        }
    }
    return backwardImplicationLevel;
}

// **************************************************************************
// Function   [ Atpg::setFreeLineFaultyGate ]
// Commenter  [ WYH WWS ]
// Synopsis   [ usage:
//								Set equivalent fault according to the faulty gate.
//
// 							description:
// 								This function is called when gate is FREE_LINE.
//                That means it has only one output gate.
//                The returned fault must be on the output line of its gateID.
//                In the while loop, sets unknown fanin gate of pCurrentGate
//                to non-control value of pCurrentGate and sets the value of
//                pCurrentGate.
//                The loop breaks when pCurrentGate becomes a HEADLINE.
//                When pCurrentGate is a HEADLINE, this function schedules
//                all fanout gate of pCurrentGate, and decides the new
//                fault type according to the value of pCurrentGate
//                and returns the new fault.
//
//              arguments:
// 								[in] gate: The faulty gate.
//
//              output:
// 								The new head line fault that is equivalent to the original
// 								free line fault.
//            ]
// Date       [ WYH Ver. 1.0 started 2013/08/17 2023/01/06 ]
// **************************************************************************
Fault Atpg::setFreeLineFaultyGate(Gate& gate) {
    int gateID = 0;
    Gate* pCurrentGate = NULL;
    std::vector<int> sStack;
    sStack.push_back(gate.fanoutVector_[0]);

    while (!sStack.empty()) {
        gateID = vecPop(sStack);
        pCurrentGate = &pCircuit_->circuitGates_[gateID];
        Value Val = pCurrentGate->getInputNonCtrlValue();
        if (Val == X) {
            Val = L;
        }

        for (int i = 0; i < pCurrentGate->numFI_; ++i) {
            Gate& gFanin = pCircuit_->circuitGates_[pCurrentGate->faninVector_[i]];
            // set pCurrentGate unknown fanin gate to non-control value
            if (gFanin.atpgVal_ == X) {
                gFanin.atpgVal_ = Val;
                backtrackImplicatedGateIDs_.push_back(gFanin.gateId_);
            }
        }

        if (pCurrentGate->atpgVal_ == X) {
            // set the value of pCurrentGate by evaluateGoodVal
            pCurrentGate->atpgVal_ = evaluateGoodVal(*pCurrentGate);
            backtrackImplicatedGateIDs_.push_back(pCurrentGate->gateId_);
        }
        // if the pCurrentGate is FREE LINE, pCurrentGate output gate becomes a new pCurrentGate
        // until pCurrentGate becomes HEAD LINE
        if (gateID_to_lineType_[gateID] == FREE_LINE) {
            sStack.push_back(pCurrentGate->fanoutVector_[0]);
        }
    }
    gateID_to_valModified_[gateID] = 1;
    pushGateFanoutsToEventStack(gateID);
    // decide the new fault type according to pCurrentGate value
    return Fault(gateID, (pCurrentGate->atpgVal_ == D) ? Fault::SA0 : Fault::SA1, 0);
}

// **************************************************************************
// Function   [ Atpg::fanoutFreeBacktrace ]
// Commenter  [ CLT WWS ]
// Synopsis   [ usage: Backtrace in fanout free situation.
//
//              arguments:
//								[in] pGate: The gate to start fanout free backtrace.
//            ]
// Date       [ Ver. 1.0 started 2013/08/13 last modified 2023/01/06]
// **************************************************************************
void Atpg::fanoutFreeBacktrace(Gate* pGate) {
    currentObjectives_.clear();
    currentObjectives_.reserve(MAX_LIST_SIZE);
    currentObjectives_.push_back(pGate->gateId_);

    while (!currentObjectives_.empty()) {
        Gate* pGate = &pCircuit_->circuitGates_[vecPop(currentObjectives_)];

        if (pGate->gateType_ == Gate::PI || pGate->gateType_ == Gate::PPI || pGate == firstTimeFrameHeadLine_) {
            continue;
        }

        Value vInv = pGate->isInverse();

        if (pGate->gateType_ == Gate::XOR2 || pGate->gateType_ == Gate::XNOR2) {
            if (&pCircuit_->circuitGates_[pGate->faninVector_[1]] != firstTimeFrameHeadLine_) {
                if (pCircuit_->circuitGates_[pGate->faninVector_[0]].atpgVal_ == X) {
                    pCircuit_->circuitGates_[pGate->faninVector_[0]].atpgVal_ = L;
                }
                pCircuit_->circuitGates_[pGate->faninVector_[1]].atpgVal_ = cXOR3(pGate->atpgVal_, pGate->isInverse(), pCircuit_->circuitGates_[pGate->faninVector_[0]].atpgVal_);
            } else {
                pCircuit_->circuitGates_[pGate->faninVector_[0]].atpgVal_ = cXOR3(pGate->atpgVal_, pGate->isInverse(), pCircuit_->circuitGates_[pGate->faninVector_[1]].atpgVal_);
            }
            currentObjectives_.push_back(pGate->faninVector_[0]);
            currentObjectives_.push_back(pGate->faninVector_[1]);                      // push both input gates into currentObjectives_ list
        } else if (pGate->gateType_ == Gate::XOR3 || pGate->gateType_ == Gate::XNOR3)  // debugged by wang
        {
            if (&pCircuit_->circuitGates_[pGate->faninVector_[1]] != firstTimeFrameHeadLine_) {
                if (pCircuit_->circuitGates_[pGate->faninVector_[0]].atpgVal_ == X) {
                    pCircuit_->circuitGates_[pGate->faninVector_[0]].atpgVal_ = L;
                }
                if (pCircuit_->circuitGates_[pGate->faninVector_[2]].atpgVal_ == X) {
                    pCircuit_->circuitGates_[pGate->faninVector_[2]].atpgVal_ = L;
                }
                pCircuit_->circuitGates_[pGate->faninVector_[1]].atpgVal_ = cXOR3(cXOR2(pGate->atpgVal_, pGate->isInverse()), pCircuit_->circuitGates_[pGate->faninVector_[0]].atpgVal_, pCircuit_->circuitGates_[pGate->faninVector_[2]].atpgVal_);
            } else {
                pCircuit_->circuitGates_[pGate->faninVector_[0]].atpgVal_ = L;
                pCircuit_->circuitGates_[pGate->faninVector_[2]].atpgVal_ = cXOR3(cXOR2(pGate->atpgVal_, pGate->isInverse()), pCircuit_->circuitGates_[pGate->faninVector_[0]].atpgVal_, pCircuit_->circuitGates_[pGate->faninVector_[1]].atpgVal_);
            }
            currentObjectives_.push_back(pGate->faninVector_[0]);
            currentObjectives_.push_back(pGate->faninVector_[1]);
            currentObjectives_.push_back(pGate->faninVector_[2]);
        } else if (pGate->isUnary()) {  // if pGate only have one input gate
            if (&pCircuit_->circuitGates_[pGate->faninVector_[0]] != firstTimeFrameHeadLine_) {
                pCircuit_->circuitGates_[pGate->faninVector_[0]].atpgVal_ = cXOR2(pGate->atpgVal_, vInv);
            }
            currentObjectives_.push_back(pGate->faninVector_[0]);  // add input gate into currentObjectives_ list
        } else {
            Value Val = cXOR2(pGate->atpgVal_, vInv);
            if (Val == pGate->getInputCtrlValue()) {
                Gate* pMinLevelGate = &pCircuit_->circuitGates_[pGate->minLevelOfFanins_];
                if (pMinLevelGate != firstTimeFrameHeadLine_) {
                    pMinLevelGate->atpgVal_ = Val;
                    currentObjectives_.push_back(pMinLevelGate->gateId_);
                } else {
                    Gate* pFaninGate = NULL;
                    for (int i = 0; i < pGate->numFI_; ++i) {
                        pFaninGate = &pCircuit_->circuitGates_[pGate->faninVector_[i]];
                        if (pFaninGate != firstTimeFrameHeadLine_) {
                            break;
                        }
                    }
                    pFaninGate->atpgVal_ = Val;
                    currentObjectives_.push_back(pFaninGate->gateId_);
                }
            } else {
                Gate* pFaninGate = NULL;
                for (int i = 0; i < pGate->numFI_; ++i) {
                    pFaninGate = &pCircuit_->circuitGates_[pGate->faninVector_[i]];
                    if (pFaninGate->atpgVal_ == X) {
                        pFaninGate->atpgVal_ = Val;
                    }
                    currentObjectives_.push_back(pFaninGate->gateId_);
                }
            }
        }
    }
}

// **************************************************************************
// Function   [ Atpg::multipleBacktrace ]
// Commenter  [ CKY WWS ]
// Synopsis   [ usage:
// 								return NO_CONTRADICTORY or CONTRADICTORY after backtrace
//                see paper P.4 P.5 and Fig.8 for detail information
//
// 							arguments:
// 								[in] atpgStatus:
//										it have 2 possibilities,
//                    atpgStatus == INITIAL means Multiple Backtrace from the
//                    set of initial objectives
//                    atpgStatus == FAN_OBJ_DETERMINE means Multiple Backtrace
//                    from the set of Fanout-Point Objectives
// 								[in, out] possibleFinalObjectiveID:
// 										Reference of possible fanout objective in
// 										single pattern generation.
//
//              output:	BACKTRACE_RESULT
//											return CONTRADICTORY when we find a
//                      Fanout-Point Objective that is not reachable from
//                      the fault line and n0, n1 of it are both not zero;
//                      otherwise, return NO_CONTRADICTORY
//                      n0 is the number of times objective 0 is required,
//                      n1 is the number of times objective 1 is required
//            ]
// Date       [ CKY Ver. 1.0 commented 2013/08/17 last modified 2023/01/06 ]
// **************************************************************************
Atpg::BACKTRACE_RESULT Atpg::multipleBacktrace(BACKTRACE_STATUS atpgStatus, int& possibleFinalObjectiveID) {
    int index;
    int n0, n1, nn1, nn0;
    Gate* pCurrentObj = NULL;

    // atpgStatus may be INITIAL, CHECK_AND_SELECT, CURRENT_OBJ_DETERMINE, FAN_OBJ_DETERMINE
    if (atpgStatus == INITIAL) {
        // LET THE SET OF INITIAL OBJECTIVES BE THE SET OF CURRENT OBJECTIVES
        initializeForMultipleBacktrace();
        atpgStatus = CHECK_AND_SELECT;  // ready to get into while loop
    }

    while (true) {
        Gate* pEasiestInput = NULL;
        switch (atpgStatus) {
            case CHECK_AND_SELECT:
                // IS THE SET OF CURRENT OBJECTIVES EMPTY?
                if (currentObjectives_.empty()) {  // YES
                    // IS THE SET OF FANOUT-POINT OBJECTIVES EMPTY?
                    if (fanoutObjectives_.empty())  // YES
                    {                               // C, NO_CONTRADICTORY
                        return NO_CONTRADICTORY;
                    } else  // NO
                    {
                        atpgStatus = FAN_OBJ_DETERMINE;
                    }
                } else {  // NO
                    // TAKE OUT A CURRENT OBJECTIVE
                    pCurrentObj = &pCircuit_->circuitGates_[vecPop(currentObjectives_)];
                    atpgStatus = CURRENT_OBJ_DETERMINE;
                }
                break;  // switch break
            case CURRENT_OBJ_DETERMINE:
                // IS OBJECTIVE LINE A HEAD LINE?
                if (gateID_to_lineType_[pCurrentObj->gateId_] == HEAD_LINE) {  // YES
                    // ADD THE CURRENT OBJECTIVE TO THE SET OF HEAD OBJECTIVES
                    headLineObjectives_.push_back(pCurrentObj->gateId_);
                } else {  // NO
                    // store m_NumOfZero, m_NumOfOne in n0 or n1, depend on
                    // different Gate Type
                    // different Gate return different value, the value used in FindEasiestInput()
                    Value Val = assignBacktraceValue(n0, n1, *pCurrentObj);  // get Val,no and n1 of pCurrentObj

                    // FindEasiestInput() in <atpg.cpp>,
                    // return Gate* of the pCurrentObj's fanin gate that is
                    // the easiest gate to control value to Val
                    pEasiestInput = findEasiestInput(pCurrentObj, Val);

                    // add next objectives to current objectives and
                    // calculate n0, n1 by rule1~rule6
                    // rule1~rule6:[Reference paper p4, p5 Fig6
                    for (int i = 0; i < pCurrentObj->numFI_; ++i) {
                        // go through all fanin gate of pCurrentObj
                        Gate* pFaninGate = &pCircuit_->circuitGates_[pCurrentObj->faninVector_[i]];

                        // ignore the fanin gate that already set value
                        // (not unknown)
                        if (pFaninGate->atpgVal_ != X) {
                            continue;
                        }

                        // DOES OBJECTIVE LINE FOLLOW A FANOUT-POINT?
                        if (pFaninGate->numFO_ > 1) {  // YES
                            // rule 1 ~ rule 5
                            if (pFaninGate == pEasiestInput) {
                                nn0 = n0;
                                nn1 = n1;
                            } else if (Val == L) {
                                nn0 = 0;
                                nn1 = n1;
                            } else if (Val == H) {
                                nn0 = n0;
                                nn1 = 0;
                            } else {
                                if (n0 > n1) {
                                    nn0 = n0;
                                    nn1 = n1;
                                } else {
                                    nn0 = n1;
                                    nn1 = n0;
                                }
                            }

                            if (nn0 > 0 || nn1 > 0) {
                                // first find this fanout point,  add to
                                // Fanout-Point Objectives set
                                if (gateID_to_n0_[pFaninGate->gateId_] == 0 && gateID_to_n1_[pFaninGate->gateId_] == 0) {
                                    fanoutObjectives_.push_back(pFaninGate->gateId_);
                                }

                                // new value = old value(gateID_to_n0_(),gateID_to_n1_()) + this branch's value (nn0, nn1)
                                // rule6: fanout point's n0, n1 = sum of it's branch's no, n1
                                // int NewZero = gateID_to_n1_[pFaninGate->gateId_] + nn0; removed by wang
                                // int NewOne = gateID_to_n1_[pFaninGate->gateId_] + nn1; removed by wang
                                // ADD n0 AND n1 TO THE CORRESPONDING
                                // modified to safe
                                setGaten0n1(pFaninGate->gateId_, gateID_to_n0_[pFaninGate->gateId_] + nn0, gateID_to_n1_[pFaninGate->gateId_] + nn1);
                                gateIDsToResetAfterBackTrace_.push_back(pFaninGate->gateId_);
                            }
                        } else {  // NO
                            // rule 1 ~ rule 5
                            if (pFaninGate == pEasiestInput) {
                                nn0 = n0;
                                nn1 = n1;
                            } else if (Val == L) {
                                nn0 = 0;
                                nn1 = n1;
                            } else if (Val == H) {
                                nn0 = n0;
                                nn1 = 0;
                            } else {
                                if (n0 > n1) {
                                    nn0 = n0;
                                    nn1 = n1;
                                } else {
                                    nn0 = n1;
                                    nn1 = n0;
                                }
                            }

                            if (nn0 > 0 || nn1 > 0) {
                                // add gate into Current Objective set
                                // BY THE RULES(1)-(5) DETERMINE NEXT OBJECTIVES
                                // AND ADD THEM TO THE SET OF CURRENT OBJECTIVES
                                setGaten0n1(pFaninGate->gateId_, nn0, nn1);
                                gateIDsToResetAfterBackTrace_.push_back(pFaninGate->gateId_);
                                currentObjectives_.push_back(pFaninGate->gateId_);
                            }
                        }
                    }
                }
                atpgStatus = CHECK_AND_SELECT;
                break;  // switch break

            case FAN_OBJ_DETERMINE:
                // TAKE OUT A FANOUT-POINT OBJECTIVE p CLOSEST TO A PRIMARY OUTPUT
                pCurrentObj = findClosestToPO(fanoutObjectives_, index);

                // specified by the index from FanObject
                vecDelete(fanoutObjectives_, index);

                // if value of pCurrent is not X
                // ignore the Fanout-Point Objective that already set value
                // (not unknown), back to CHECK_AND_SELECT state
                if (pCurrentObj->atpgVal_ != X) {
                    atpgStatus = CHECK_AND_SELECT;
                    break;  // switch break
                }

                // if fault reach of pCurrent is equal to 1 => reachable
                // IS p REACHABLE FROM THE FAULT LINE?
                if (gateID_to_reachableByTargetFault_[pCurrentObj->gateId_] == 1) {  // YES
                    atpgStatus = CURRENT_OBJ_DETERMINE;
                    break;  // switch break
                }

                // if one of numOfZero or numOfOne is equal to 0
                if (!(gateID_to_n0_[pCurrentObj->gateId_] != 0 && gateID_to_n1_[pCurrentObj->gateId_] != 0)) {
                    atpgStatus = CURRENT_OBJ_DETERMINE;
                    break;  // switch break
                }

                // if three conditions are not set up, then push back pCurrentObj to finalObject
                // then terminate Multiple Backtrace procedure
                // when "a Fanout-Point objective is not reachable from the
                // fault line" and "both no, n1 of it are nonzero "
                // return CONTRADICTORY in this situation
                // Let the Fanout-Point objective to be a Final Objective
                possibleFinalObjectiveID = pCurrentObj->gateId_;
                return CONTRADICTORY;

            default:
                break;
        }
    }
    // after breaking while loop
    return NO_CONTRADICTORY;
}

// **************************************************************************
// Function   [ Atpg::assignBacktraceValue ]
// Commenter  [ CKY WWS ]
// Synopsis   [ usage:
// 								Get n0, n1 and Value depending on Gate's controlling value.
//
//              arguments:
// 								[in, out] n0:	n0 (int reference) to be set
// 								[in, out] n1: n1 (int reference) to be set
// 								[in] gate:
// 									gate to assign backtrace value to but the gate.atpgVal_
// 									is assigned outside this function
//
//              output:
// 								The decided value to assign to gate.
//            ]
// Date       [ CKY Ver. 1.0 commented 2013/08/17 last modified 2023/01/06 ]
// **************************************************************************
Value Atpg::assignBacktraceValue(int& n0, int& n1, const Gate& gate) {
    int v1;
    Value val;
    switch (gate.gateType_) {
        // when gate is AND type,n0 = numOfZero,n1 = numOfOne
        case Gate::AND2:
        case Gate::AND3:
        case Gate::AND4:
            n0 = gateID_to_n0_[gate.gateId_];
            n1 = gateID_to_n1_[gate.gateId_];
            return L;

            // when gate is OR type,n0 = numOfZero,n1 = numOfOne
        case Gate::OR2:
        case Gate::OR3:
        case Gate::OR4:
            // TO-DO homework 04
            n0 = gateID_to_n0_[gate.gateId_];
            n1 = gateID_to_n1_[gate.gateId_];
            return H;
            // end of TO-DO

            // when gate is NAND type,n0 = numOfOne,n1 = numOfZero
        case Gate::NAND2:
        case Gate::NAND3:
        case Gate::NAND4:
            n0 = gateID_to_n1_[gate.gateId_];
            n1 = gateID_to_n0_[gate.gateId_];
            return L;

            // when gate is NOR type,n0 = numOfOne,n1 = numOfZero
        case Gate::NOR2:
        case Gate::NOR3:
        case Gate::NOR4:
            // TO-DO homework 04
            n0 = gateID_to_n1_[gate.gateId_];
            n1 = gateID_to_n0_[gate.gateId_];
            return H;
            // end of TO-DO

            // when gate is inverter,n0 = numOfOne,n1 = numOfZero
        case Gate::INV:
            n0 = gateID_to_n1_[gate.gateId_];
            n1 = gateID_to_n0_[gate.gateId_];
            return X;

            // when gate is XOR2 or XNOR2
        case Gate::XOR2:
        case Gate::XNOR2:
            val = pCircuit_->circuitGates_[gate.faninVector_[0]].atpgVal_;
            if (val == X) {
                val = pCircuit_->circuitGates_[gate.faninVector_[1]].atpgVal_;
            }

            if (val == H) {
                n0 = gateID_to_n1_[gate.gateId_];
                n1 = gateID_to_n0_[gate.gateId_];
            } else {
                n0 = gateID_to_n0_[gate.gateId_];
                n1 = gateID_to_n1_[gate.gateId_];
            }

            if (gate.gateType_ == Gate::XNOR2) {
                int temp = n0;
                n0 = n1;
                n1 = temp;
            }
            return X;

            // when gate is XOR3 or XNOR3
        case Gate::XOR3:
        case Gate::XNOR3:
            v1 = 0;
            for (int i = 0; i < gate.numFI_; ++i) {
                if (pCircuit_->circuitGates_[gate.faninVector_[0]].atpgVal_ == H) {
                    ++v1;
                }
            }
            if (v1 == 2) {
                n0 = gateID_to_n1_[gate.gateId_];
                n1 = gateID_to_n0_[gate.gateId_];
            } else {
                n0 = gateID_to_n0_[gate.gateId_];
                n1 = gateID_to_n1_[gate.gateId_];
            }

            if (gate.gateType_ == Gate::XNOR3) {
                int temp = n0;
                n0 = n1;
                n1 = temp;
            }
            return X;
        default:
            n0 = gateID_to_n0_[gate.gateId_];
            n1 = gateID_to_n1_[gate.gateId_];
            return X;
    }
}

// **************************************************************************
// Function   [ Atpg::initializeForMultipleBacktrace ]
// Commenter  [ CKY WWS ]
// Synopsis   [ usage:
// 								Initialize all this->gateID_to_n0_, this->gateID_to_n1_.
//
// 							description:
// 								Copy the initial objectives into current objectives.
// 								Traverse all gate in current objectives.
// 								If gate's atpgVal_ is L or B
// 									n0 = 1, n1 = 0
// 								Else if gates's atpgVal_ is H pr D
// 									n1 = 0, n0 = 1
// 								Else if X, Z, I
// 									Set line number depend on gate type
//            ]
// Date       [ CKY Ver. 1.0 commented 2013/08/17 last modified 2023/01/06 ]
// **************************************************************************
void Atpg::initializeForMultipleBacktrace() {
    currentObjectives_ = initialObjectives_;  // vector assignment

    // go through all current Object size
    for (const int& currentObjectGateID : currentObjectives_) {
        // get currentObject gate in pCircuit_ to pGate
        Gate* pGate = &pCircuit_->circuitGates_[currentObjectGateID];

        // if single value of the gate is Low or D', numOfZero=1, numOfOne=0
        if (pGate->atpgVal_ == L || pGate->atpgVal_ == B) {
            setGaten0n1(pGate->gateId_, 1, 0);
        } else if (pGate->atpgVal_ == H || pGate->atpgVal_ == D) {  // if single value of the gate is High or D, numOfZero=0, numOfOne=1
            setGaten0n1(pGate->gateId_, 0, 1);
        } else {  // if single value of the gate is X or Z or I, set line number depend on gate type
            switch (pGate->gateType_) {
                case Gate::AND2:
                case Gate::AND3:
                case Gate::AND4:
                case Gate::NOR2:
                case Gate::NOR3:
                case Gate::NOR4:
                case Gate::XNOR2:
                case Gate::XNOR3:
                    setGaten0n1(pGate->gateId_, 0, 1);
                    break;
                case Gate::OR2:
                case Gate::OR3:
                case Gate::OR4:
                case Gate::NAND2:
                case Gate::NAND3:
                case Gate::NAND4:
                case Gate::XOR2:
                case Gate::XOR3:
                    setGaten0n1(pGate->gateId_, 1, 0);
                    break;
                default:
                    break;
            }
        }
        // record reset list
        gateIDsToResetAfterBackTrace_.push_back(pGate->gateId_);
    }
}

// **************************************************************************
// Function   [ Atpg::findEasiestFaninGate ]
// Commenter  [ KOREAL WWS ]
// Synopsis   [ usage: Find the easiest fanin by gate::cc0_ or gate::cc1_.
//
// 							description:
// 								Utilize SCOAP heuristic if addSCOAP is called in
// 								setupCircuitParameter().
// 								Otherwise cc0_ and cc1_ is 0.
// 								SCOAP heuristic is finished and can be found in atpg.cpp
// 								but is not included in the algorithm because the result
// 								of SCOAP is even worse.
//
// 							arguments:
// 								[in] pGate:
// 									The gate to find easiest fanin gate.
//								[in, out] atpgValOfpGate:
// 									The value to of the pGate.
//
//              output: The easiest fanin gate to assign value.
//            ]
// Date       [ CPJ Ver. 1.0 started 2013/08/10 last modified 2023/01/06 ]
// **************************************************************************
Gate* Atpg::findEasiestInput(Gate* pGate, Value atpgValOfpGate) {
    // declaration of the return gate pointer
    Gate* pRetGate = NULL;
    // easiest input gate's scope(non-select yet)
    int easyControlVal = INFINITE;

    // if the fanIn amount is 1, just return the only fanIn
    if (pGate->gateType_ == Gate::PO || pGate->gateType_ == Gate::PPO ||
        pGate->gateType_ == Gate::BUF || pGate->gateType_ == Gate::INV) {
        return &pCircuit_->circuitGates_[pGate->faninVector_[0]];
    }

    if (atpgValOfpGate == L) {
        // choose the value-undetermined faninGate which has smallest cc0_
        for (int i = 0; i < pGate->numFI_; ++i) {
            Gate* pFaninGate = &pCircuit_->circuitGates_[pGate->faninVector_[i]];
            if (pFaninGate->atpgVal_ != X) {
                continue;
            }

            if (pFaninGate->cc0_ < easyControlVal) {
                easyControlVal = pFaninGate->cc0_;
                pRetGate = pFaninGate;
            }
        }
    } else {
        // choose the value-undetermined faninGate which has smallest cc1_
        for (int i = 0; i < pGate->numFI_; ++i) {
            Gate* pFaninGate = &pCircuit_->circuitGates_[pGate->faninVector_[i]];
            if (pFaninGate->atpgVal_ != X) {
                continue;
            }

            if (pFaninGate->cc1_ < easyControlVal) {
                easyControlVal = pFaninGate->cc1_;
                pRetGate = pFaninGate;
            }
        }
    }
    return pRetGate;
}

// **************************************************************************
// Function   [ Atpg::findClosestToPO ]
// Commenter  [ CLT WWS ]
// Synopsis   [ usage:
// 								Find the gate which is the closest to output.
//
// 							arguments:
// 								[in] gateVec: The gate vector to search.
// 								[in, out] index: The index of the gate closest to PO/PPO.
//
//              output:
// 								The gate which is closest to PO/PPO.
//            ]
// Date       [ Ver. 1.0 started 2013/08/13 last modified 2023/01/06 ]
// *************************************************************************
Gate* Atpg::findClosestToPO(std::vector<int>& gateVec, int& index) {
    Gate* pCloseGate = NULL;

    if (gateVec.empty()) {
        return NULL;
    }

    pCloseGate = &pCircuit_->circuitGates_[gateVec.back()];
    index = gateVec.size() - 1;
    for (int i = gateVec.size() - 2; i >= 0; --i) {
        if (pCircuit_->circuitGates_[gateVec[i]].depthFromPo_ < pCloseGate->depthFromPo_) {
            index = i;
            pCloseGate = &pCircuit_->circuitGates_[gateVec[i]];
        }
    }
    return pCloseGate;
}

// **************************************************************************
// Function   [ Atpg::evaluateAndSetGateAtpgVal ]
// Commenter  [ KOREAL WWS ]
// Synopsis   [ usage:
// 								The literal meaning of function name.
//
//							description:
//              	If pGate is the faulty gate, return FaultEvaluation(pGate)
//              	else check the relationships between pGate's evaluated
// 								value and current value.
//
//              	If they are the same, set pGate to be modified,
//              	 return FORWARD,
//              	else if current value is unknown,
//              		set it to the evaluated value and return FORWARD,
//              	else if the evaluated value is different from current value
//              		return CONFLICT,
//              	else (only know current value)
//              		return BackwardImplication(pGate).
//
//							arguments:
// 								[in] pGate: The gate to do evaluation on.
//
// 							output:
// 								The implication status after this function call.
//            ]
// Date       [ KOREAL Ver. 1.0 started 2013/08/15 last modifed 2023/01/06 ]
// **************************************************************************
Atpg::IMPLICATION_STATUS Atpg::evaluateAndSetGateAtpgVal(Gate* pGate) {
    gateID_to_valModified_[pGate->gateId_] = 0;
    if (gateID_to_lineType_[pGate->gateId_] == HEAD_LINE) {
        // pGate is head line, set modify and return FORWARD
        gateID_to_valModified_[pGate->gateId_] = 1;
        return FORWARD;
    }

    // pGate is the faulty gate, see FaultEvaluation();
    if (pGate->gateId_ == currentTargetFault_.gateID_) {
        return evaluateAndSetFaultyGateAtpgVal(pGate);
    }

    // pGate is not the faulty gate, see evaluateGoodVal(*pGate)
    Value Val = evaluateGoodVal(*pGate);

    if (pGate->atpgVal_ == Val) {
        if (Val != X) {  // Good value is equal to the gate output, return FORWARD
            gateID_to_valModified_[pGate->gateId_] = 1;
        }
        return FORWARD;
    } else if (pGate->atpgVal_ == X) {
        // set it to the evaluated value.
        pGate->atpgVal_ = Val;
        backtrackImplicatedGateIDs_.push_back(pGate->gateId_);
        gateID_to_valModified_[pGate->gateId_] = 1;
        pushGateFanoutsToEventStack(pGate->gateId_);
        return FORWARD;
    } else if (Val != X) {  // Good value is different to the gate output, return CONFLICT
        return CONFLICT;
    }
    return doOneGateBackwardImplication(pGate);  // atpgVal != X && Val == X
}

// **************************************************************************
// Function   [ Atpg::evaluateAndSetFaultyGateAtpgVal ]
// Commenter  [ KOREAL WWS ]
// Synopsis   [ usage:
// 								The literal meaning of function name.
//
//							description:
//              	Check the relationships between pGate's current value and
//              	the evaluated value of pGate.
//
//              	If evaluated value is unknown
// 									if pGate has current value,
// 										if only one input has ONE unknown value
// 											set the input to proper value and return BACKWARD
// 										else
// 											push pGate into unjustified_ list
// 								If they are the same
// 									set pGate to be modified, return FORWARD
// 								If the evaluated value is different from current value
// 									return CONFLICT
//
//							arguments:
// 								[in] pGate: The gate to do evaluation on.
//
// 							output:
// 								The implication status after this function call.
//            ]
// Date       [ KOREAL Ver. 1.0 started 2013/08/15 last modified 2023/01/06 ]
// **************************************************************************
Atpg::IMPLICATION_STATUS Atpg::evaluateAndSetFaultyGateAtpgVal(Gate* pGate) {
    // get the evaluated value of pGate.
    Value Val = evaluateFaultyVal(*pGate);
    int ImpPtr = 0;
    if (Val == X) {  // The evaluated value is X, means the init faulty objective has not achieved yet.
        if (pGate->atpgVal_ != X) {
            int NumOfX = 0;

            // Count NumOfX, the amount of fanIn whose value is X.
            // set ImpPtr to be one of the fanIn whose value is X.
            for (int i = 0; i < pGate->numFI_; ++i) {
                Gate* pFaninGate = &pCircuit_->circuitGates_[pGate->faninVector_[i]];
                if (pFaninGate->atpgVal_ == X) {
                    ++NumOfX;
                    ImpPtr = i;
                }
            }
            if (NumOfX == 1) {  // The fanin has X value can be set to impl. value

                // ImpVal is the pGate pImpGate value
                Value ImpVal;

                // pImpGate is the pGate's fanIn whose value is X.
                Gate* pImpGate = &pCircuit_->circuitGates_[pGate->faninVector_[ImpPtr]];

                // set ImpVal if pGate is not XOR or XNOR
                if (pGate->atpgVal_ == D) {
                    ImpVal = H;
                } else if (pGate->atpgVal_ == B) {
                    ImpVal = L;
                } else {
                    ImpVal = pGate->atpgVal_;
                }

                // set ImpVal if pGate is XOR2 or XNOR2
                if (pGate->gateType_ == Gate::XOR2 || pGate->gateType_ == Gate::XNOR2) {
                    Value temp = (ImpPtr == 0) ? pCircuit_->circuitGates_[pGate->faninVector_[1]].atpgVal_ : pCircuit_->circuitGates_[pGate->faninVector_[0]].atpgVal_;
                    ImpVal = cXOR2(ImpVal, temp);
                }
                // set ImpVal if pGate is XOR3 or XNOR3
                if (pGate->gateType_ == Gate::XOR3 || pGate->gateType_ == Gate::XNOR3) {
                    Value temp;
                    if (ImpPtr == 0) {
                        temp = cXOR2(pCircuit_->circuitGates_[pGate->faninVector_[1]].atpgVal_, pCircuit_->circuitGates_[pGate->faninVector_[2]].atpgVal_);
                    } else if (ImpPtr == 1) {
                        temp = cXOR2(pCircuit_->circuitGates_[pGate->faninVector_[0]].atpgVal_, pCircuit_->circuitGates_[pGate->faninVector_[2]].atpgVal_);
                    } else {
                        temp = cXOR2(pCircuit_->circuitGates_[pGate->faninVector_[1]].atpgVal_, pCircuit_->circuitGates_[pGate->faninVector_[0]].atpgVal_);
                    }
                    ImpVal = cXOR2(ImpVal, temp);
                }

                // if pGate is an inverse function, ImpVal = NOT(ImpVal)
                Value isInv = pGate->isInverse();
                ImpVal = cXOR2(ImpVal, isInv);

                // set modify and the final value of pImpGate
                gateID_to_valModified_[pGate->gateId_] = 1;
                pImpGate->atpgVal_ = ImpVal;

                // backward setting
                // pushInputEvents(pGate->gateId_, ImpPtr);
                pushGateToEventStack(pGate->faninVector_[ImpPtr]);
                pushGateFanoutsToEventStack(pGate->faninVector_[ImpPtr]);
                backtrackImplicatedGateIDs_.push_back(pImpGate->gateId_);

                return BACKWARD;
            } else {
                unjustifiedGateIDs_.push_back(pGate->gateId_);
            }
        }
    } else if (pGate->atpgVal_ == Val) {  // The initial faulty objective has already been achieved
        gateID_to_valModified_[pGate->gateId_] = 1;
    } else if (pGate->atpgVal_ == X) {
        // if pGate's value is unknown, set pGate's value
        gateID_to_valModified_[pGate->gateId_] = 1;
        pGate->atpgVal_ = Val;
        // forward setting
        pushGateFanoutsToEventStack(pGate->gateId_);
        backtrackImplicatedGateIDs_.push_back(pGate->gateId_);
    } else {
        return CONFLICT;  // If the evaluated value is different from current value, return CONFLICT.
    }
    return FORWARD;
}

// **************************************************************************
// Function   [ Atpg::staticTestCompressionByReverseFaultSimulation ]
// Commenter  [ CAL WWS ]
// Synopsis   [ usage:
//                Perform reverse fault simulation to do static test
// 								compression.
//
//              arguments:
// 								[in, out] pPatterProcessor:
// 									The pattern processor contains
// 									the complete test pattern set before STC.
// 									It will then be reassigned to static compressed
// 									test pattern set.
// 								[in, out] originalFaultList:
// 									List of faults to be detected.
// 									Would be modified after this function call.
//            ]
// Date       [ started 2020/07/08    last modified 2023/01/06 ]
// **************************************************************************
void Atpg::staticTestCompressionByReverseFaultSimulation(PatternProcessor* pPatternProcessor, FaultPtrList& originalFaultList) {
    for (Fault* pFault : originalFaultList) {
        pFault->detection_ = 0;
        if (pFault->faultState_ == Fault::DT) {
            pFault->faultState_ = Fault::UD;
        }
    }

    std::vector<Pattern> tmp = pPatternProcessor->patternVector_;
    pPatternProcessor->patternVector_.clear();

    // Perform reverse fault simulation
    int leftFaultCount = originalFaultList.size();
    for (std::vector<Pattern>::reverse_iterator rit = tmp.rbegin(); rit != tmp.rend(); ++rit) {
        pSimulator_->parallelFaultFaultSimWithOnePattern((*rit), originalFaultList);
        if (leftFaultCount > originalFaultList.size()) {
            leftFaultCount = originalFaultList.size();
            pPatternProcessor->patternVector_.push_back((*rit));
        } else if (leftFaultCount < originalFaultList.size()) {
            std::cerr << "Bug: staticTestCompressionByReverseFaultSimulation() unexpected behavior\n";
            std::cin.get();
        }
        // else
        // {
        // 	delete (*rit);
        // }
    }
}

// **************************************************************************
// Function   [ Atpg::setUpFirstTimeFrame ]
// Commenter  [ WYH ]
// Synopsis   [ usage: Initial assignment of fault signal, and set the first time
//                     meet HEAD LINE gate.
//                     There are two situations :
//                     1.Faulty gate is FREE LINE
//                      (1)Activate the fault, and set value of pFaultyLine
//                         according to fault type.
//                      (2)Do while loop until pFaultyLine becomes HEAD LINE
//                      (3)Schedule fanout gates of pFaultyLine.
//                     2.Fault is HEAD LINE or BOUND LINE
//                      (1)Activate the fault, and set value of pFaultyLine
//                         according to fault type.
//                      (2)Schedule fanout gates of pFaultyLine.
//                      (3) Update backwardImplicationLevel to be maximum level of
//                          fanin gates of pFaultyLine.
//              in:    Fault& fault
//              out:   int backwardImplicationLevel: The backwardImplicationLevel indicates the backward
//                     imply level return -1 when fault FAULT_UNTESTABLE
//            ]
// Date       [ WYH Ver. 1.0 started 2013/08/17 ]
// **************************************************************************
int Atpg::setUpFirstTimeFrame(Fault& fault) {  // NE
    int backwardImplicationLevel = 0;
    Gate* pFaultyGate = NULL;
    Gate* pFaultyLine = NULL;
    bool isOutputFault = (fault.faultyLine_ == 0);
    Value FaultyValue = (fault.faultType_ == Fault::STR) ? L : H;
    Value valueTemp;

    pFaultyGate = &pCircuit_->circuitGates_[fault.gateID_ - pCircuit_->numGate_];

    std::cout << pCircuit_->pNetlist_->getTop()->getCell(pCircuit_->circuitGates_[fault.gateID_].cellId_)->name_ << " " << pCircuit_->pNetlist_->getTop()->getCell(pFaultyGate->cellId_)->name_ << std::endl;

    if (!isOutputFault) {
        pFaultyLine = &pCircuit_->circuitGates_[pFaultyGate->faninVector_[fault.faultyLine_ - 1]];
    } else {
        pFaultyLine = pFaultyGate;
    }

    if (gateID_to_lineType_[pFaultyLine->gateId_] == FREE_LINE) {
        if ((FaultyValue == H && pFaultyLine->atpgVal_ == L) || (FaultyValue == L && pFaultyLine->atpgVal_ == H)) {
            return -1;
        }

        pFaultyLine->atpgVal_ = FaultyValue;
        backtrackImplicatedGateIDs_.push_back(pFaultyLine->gateId_);
        fanoutFreeBacktrace(pFaultyLine);
        Gate* gTemp = pFaultyLine;
        Gate* gNext = NULL;
        // propagate until meet the other LINE
        do {
            gNext = &pCircuit_->circuitGates_[gTemp->fanoutVector_[0]];
            if (!gNext->isUnary() && (gNext->getOutputCtrlValue() == X || gTemp->atpgVal_ != gNext->getInputCtrlValue())) {
                break;
            }
            gNext->atpgVal_ = cXOR2(gNext->isInverse(), gTemp->atpgVal_);
            gTemp = gNext;
        } while (gateID_to_lineType_[gTemp->gateId_] == FREE_LINE);

        if (gateID_to_lineType_[gTemp->gateId_] == HEAD_LINE) {
            gateID_to_valModified_[gTemp->gateId_] = 1;
            backtrackImplicatedGateIDs_.push_back(gTemp->gateId_);
            pushGateFanoutsToEventStack(gTemp->gateId_);
        }
        firstTimeFrameHeadLine_ = gTemp;
    } else {
        if ((FaultyValue == H && pFaultyLine->atpgVal_ == L) || (FaultyValue == L && pFaultyLine->atpgVal_ == H)) {
            return -1;
        }

        pFaultyLine->atpgVal_ = FaultyValue;
        backtrackImplicatedGateIDs_.push_back(pFaultyLine->gateId_);
        pushGateFanoutsToEventStack(pFaultyLine->gateId_);

        if (gateID_to_lineType_[pFaultyLine->gateId_] == HEAD_LINE) {
            gateID_to_valModified_[pFaultyLine->gateId_] = 1;
        } else if (pFaultyLine->gateType_ == Gate::INV || pFaultyLine->gateType_ == Gate::BUF || pFaultyLine->gateType_ == Gate::PO || pFaultyLine->gateType_ == Gate::PPO) {
            Gate* pFaninGate = &pCircuit_->circuitGates_[pFaultyLine->faninVector_[0]];
            gateID_to_valModified_[pFaultyLine->gateId_] = 1;

            Value Val = FaultyValue == H ? H : L;
            valueTemp = pFaultyLine->isInverse();
            pFaninGate->atpgVal_ = cXOR2(valueTemp, Val);
            backtrackImplicatedGateIDs_.push_back(pFaninGate->gateId_);
            pushGateToEventStack(pFaultyLine->faninVector_[0]);
            pushGateFanoutsToEventStack(pFaultyLine->faninVector_[0]);
            if (backwardImplicationLevel < pFaninGate->numLevel_) {
                backwardImplicationLevel = pFaninGate->numLevel_;
            }
        } else if ((FaultyValue == H && pFaultyLine->getOutputCtrlValue() == H) || (FaultyValue == L && pFaultyLine->getOutputCtrlValue() == L)) {
            gateID_to_valModified_[pFaultyLine->gateId_] = 1;
            for (int i = 0; i < pFaultyLine->numFI_; ++i) {
                Gate* pFaninGate = &pCircuit_->circuitGates_[pFaultyLine->faninVector_[i]];
                // if the value has not been set, then set it to non-control value
                if (pFaninGate->atpgVal_ == X) {
                    pFaninGate->atpgVal_ = pFaultyLine->getInputNonCtrlValue();
                    backtrackImplicatedGateIDs_.push_back(pFaninGate->gateId_);
                    pushGateToEventStack(pFaultyLine->faninVector_[i]);
                    pushGateFanoutsToEventStack(pFaultyLine->faninVector_[i]);
                    // set the backwardImplicationLevel to be maximum of fanin gate's level
                    if (backwardImplicationLevel < pFaninGate->numLevel_) {
                        backwardImplicationLevel = pFaninGate->numLevel_;
                    }
                }
            }
        } else {
            pushGateToEventStack(pFaultyLine->gateId_);
            if (backwardImplicationLevel < pFaultyLine->numLevel_) {
                backwardImplicationLevel = pFaultyLine->numLevel_;
            }
        }
    }
    return backwardImplicationLevel;
}

// **************************************************************************
// Function   [ Atpg::checkLevelInfo ]
// Commenter  [ CAL ]
// Synopsis   [ usage:
//                To check if the circuitLvl_ of all the gates does not exceed pCircuit_->totalLvl_
//              in:    void
//              out:   void
//            ]
// Date       [ started 2020/07/07    last modified 2020/07/07 ]
// **************************************************************************
void Atpg::checkLevelInfo() {
    for (int i = 0; i < pCircuit_->totalGate_; ++i) {
        Gate& gate = pCircuit_->circuitGates_[i];
        if (gate.numLevel_ >= pCircuit_->totalLvl_) {
            std::cerr << "checkLevelInfo found that at least one gate.numLevel_ is greater than pCircuit_->totalLvl_\n";
            std::cin.get();
        }
    }
}

// **************************************************************************
// Function   [ Atpg::getValStr ]
// Commenter  [ CAL ]
// Synopsis   [ usage: return string type of Value
//              in:    Value
//              out:   string of Value
//            ]
// Date       [ started 2020/07/04    last modified 2020/07/04 ]
// **************************************************************************
std::string Atpg::getValStr(Value val) {
    std::string valStr;
    switch (val) {
        case H:
            valStr = "H";
            break;
        case L:
            valStr = "L";
            break;
        case D:
            valStr = "D";
            break;
        case B:
            valStr = "B";
            break;
        case X:
            valStr = "X";
            break;
        default:
            valStr = "Error";
            break;
    }
    return valStr;
}

// TODO comment by wang
void Atpg::calSCOAP() {
    // cc0, cc1 and co default is 0, check if is changed before
    for (int gateID = 0; gateID < pCircuit_->totalGate_; ++gateID) {
        Gate& gate = pCircuit_->circuitGates_[gateID];
        if (gate.cc0_ != 0) {
            std::cerr << "cc0_ is not -1\n";
            std::cin.get();
        }
        if (gate.cc1_ != 0) {
            std::cerr << "cc1_ is not -1\n";
            std::cin.get();
        }
        if (gate.co_ != 0) {
            std::cerr << "co_ is not -1\n";
            std::cin.get();
        }
    }

    // array for xor2, xor3, xnor2, xnor3
    // xor2 xnor2 : {00,01,10,11}
    // xor3 xnor3 : {000, 001, 010, 011, 100, 101, 110, 111}
    int xorcc[8] = {0};
    Gate gateInputs[3];

    // calculate cc0 and cc1 starting from PI and PPI
    for (int gateID = 0; gateID < pCircuit_->totalGate_; ++gateID) {
        Gate& gate = pCircuit_->circuitGates_[gateID];
        switch (gate.gateType_) {
            case Gate::PPI:
            case Gate::PI:
                gate.cc0_ = 1;
                gate.cc1_ = 1;
                break;
            case Gate::PO:
            case Gate::PPO:
            case Gate::BUF:
                gate.cc0_ = pCircuit_->circuitGates_[gate.faninVector_[0]].cc0_;
                gate.cc1_ = pCircuit_->circuitGates_[gate.faninVector_[0]].cc1_;
                break;
            case Gate::INV:
                gate.cc0_ = pCircuit_->circuitGates_[gate.faninVector_[0]].cc1_ + 1;
                gate.cc1_ = pCircuit_->circuitGates_[gate.faninVector_[0]].cc0_ + 1;
                break;
            case Gate::AND2:
            case Gate::AND3:
            case Gate::AND4:
                for (int j = 0; j < gate.numFI_; ++j) {
                    Gate& gateInput = pCircuit_->circuitGates_[gate.faninVector_[j]];
                    if (j == 0 || (gateInput.cc0_ < gate.cc0_)) {
                        gate.cc0_ = gateInput.cc0_;
                    }
                    gate.cc1_ += gateInput.cc1_;
                }
                ++gate.cc1_;
                ++gate.cc0_;
                break;
            case Gate::NAND2:
            case Gate::NAND3:
            case Gate::NAND4:
                for (int j = 0; j < gate.numFI_; ++j) {
                    Gate& gateInput = pCircuit_->circuitGates_[gate.faninVector_[j]];
                    if (j == 0 || (gateInput.cc0_ < gate.cc1_)) {
                        gate.cc1_ = gateInput.cc0_;
                    }
                    gate.cc0_ += gateInput.cc1_;
                }
                ++gate.cc0_;
                ++gate.cc1_;
                break;
            case Gate::OR2:
            case Gate::OR3:
            case Gate::OR4:
                for (int j = 0; j < gate.numFI_; ++j) {
                    Gate& gateInput = pCircuit_->circuitGates_[gate.faninVector_[j]];
                    if (j == 0 || (gateInput.cc1_ < gate.cc1_)) {
                        gate.cc1_ = gateInput.cc1_;
                    }
                    gate.cc0_ += gateInput.cc0_;
                }
                ++gate.cc0_;
                ++gate.cc1_;
                break;
            case Gate::NOR2:
            case Gate::NOR3:
            case Gate::NOR4:
                for (int j = 0; j < gate.numFI_; ++j) {
                    Gate& gateInput = pCircuit_->circuitGates_[gate.faninVector_[j]];
                    if (j == 0 || (gateInput.cc1_ < gate.cc0_)) {
                        gate.cc0_ = gateInput.cc1_;
                    }
                    gate.cc1_ += gateInput.cc0_;
                }
                ++gate.cc0_;
                ++gate.cc1_;
                break;
            case Gate::XOR2:
                gateInputs[0] = pCircuit_->circuitGates_[gate.faninVector_[0]];
                gateInputs[1] = pCircuit_->circuitGates_[gate.faninVector_[1]];
                xorcc[0] = gateInputs[0].cc0_ + gateInputs[1].cc0_;
                xorcc[1] = gateInputs[0].cc0_ + gateInputs[1].cc1_;
                xorcc[2] = gateInputs[0].cc1_ + gateInputs[1].cc0_;
                xorcc[3] = gateInputs[0].cc1_ + gateInputs[1].cc1_;
                gate.cc0_ = std::min(xorcc[0], xorcc[3]);
                gate.cc1_ = std::min(xorcc[1], xorcc[2]);
                ++gate.cc0_;
                ++gate.cc1_;
                break;
            case Gate::XOR3:
                gateInputs[0] = pCircuit_->circuitGates_[gate.faninVector_[0]];
                gateInputs[1] = pCircuit_->circuitGates_[gate.faninVector_[1]];
                gateInputs[2] = pCircuit_->circuitGates_[gate.faninVector_[2]];
                xorcc[0] = gateInputs[0].cc0_ + gateInputs[1].cc0_ + gateInputs[2].cc0_;
                xorcc[1] = gateInputs[0].cc0_ + gateInputs[1].cc0_ + gateInputs[2].cc1_;
                xorcc[2] = gateInputs[0].cc0_ + gateInputs[1].cc1_ + gateInputs[2].cc0_;
                xorcc[3] = gateInputs[0].cc0_ + gateInputs[1].cc1_ + gateInputs[2].cc1_;
                xorcc[4] = gateInputs[0].cc1_ + gateInputs[1].cc0_ + gateInputs[2].cc0_;
                xorcc[5] = gateInputs[0].cc1_ + gateInputs[1].cc0_ + gateInputs[2].cc1_;
                xorcc[6] = gateInputs[0].cc1_ + gateInputs[1].cc1_ + gateInputs[2].cc0_;
                xorcc[7] = gateInputs[0].cc1_ + gateInputs[1].cc1_ + gateInputs[2].cc1_;
                gate.cc0_ = std::min(xorcc[0], xorcc[7]);
                for (int j = 1; j < 7; ++j) {
                    if (j == 1 || xorcc[j] < gate.cc1_) {
                        gate.cc1_ = xorcc[j];
                    }
                }
                ++gate.cc0_;
                ++gate.cc1_;
                break;
            case Gate::XNOR2:
                gateInputs[0] = pCircuit_->circuitGates_[gate.faninVector_[0]];
                gateInputs[1] = pCircuit_->circuitGates_[gate.faninVector_[1]];
                xorcc[0] = gateInputs[0].cc0_ + gateInputs[1].cc0_;
                xorcc[1] = gateInputs[0].cc0_ + gateInputs[1].cc1_;
                xorcc[2] = gateInputs[0].cc1_ + gateInputs[1].cc0_;
                xorcc[3] = gateInputs[0].cc1_ + gateInputs[1].cc1_;
                gate.cc0_ = std::min(xorcc[1], xorcc[2]);
                gate.cc1_ = std::min(xorcc[0], xorcc[3]);
                ++gate.cc0_;
                ++gate.cc1_;
                break;
            case Gate::XNOR3:
                gateInputs[0] = pCircuit_->circuitGates_[gate.faninVector_[0]];
                gateInputs[1] = pCircuit_->circuitGates_[gate.faninVector_[1]];
                gateInputs[2] = pCircuit_->circuitGates_[gate.faninVector_[2]];
                xorcc[0] = gateInputs[0].cc0_ + gateInputs[1].cc0_ + gateInputs[2].cc0_;
                xorcc[1] = gateInputs[0].cc0_ + gateInputs[1].cc0_ + gateInputs[2].cc1_;
                xorcc[2] = gateInputs[0].cc0_ + gateInputs[1].cc1_ + gateInputs[2].cc0_;
                xorcc[3] = gateInputs[0].cc0_ + gateInputs[1].cc1_ + gateInputs[2].cc1_;
                xorcc[4] = gateInputs[0].cc1_ + gateInputs[1].cc0_ + gateInputs[2].cc0_;
                xorcc[5] = gateInputs[0].cc1_ + gateInputs[1].cc0_ + gateInputs[2].cc1_;
                xorcc[6] = gateInputs[0].cc1_ + gateInputs[1].cc1_ + gateInputs[2].cc0_;
                xorcc[7] = gateInputs[0].cc1_ + gateInputs[1].cc1_ + gateInputs[2].cc1_;
                gate.cc1_ = std::min(xorcc[0], xorcc[7]);
                for (int j = 1; j < 7; ++j) {
                    if (j == 1 || xorcc[j] < gate.cc1_) {
                        gate.cc0_ = xorcc[j];
                    }
                }
                ++gate.cc0_;
                ++gate.cc1_;
                break;
            default:
                std::cerr << "Bug: reach switch case default while calculating cc0_, cc1_";
                std::cin.get();
                break;
        }
    }

    // calculate co_ starting from PO and PP
    for (int gateID = 0; gateID < pCircuit_->totalGate_; ++gateID) {
        Gate& gate = pCircuit_->circuitGates_[gateID];
        switch (gate.gateType_) {
            case Gate::PO:
            case Gate::PPO:
                gate.co_ = 0;
                break;
            case Gate::PPI:
            case Gate::PI:
            case Gate::BUF:
                for (int j = 0; j < gate.numFO_; ++j) {
                    if (j == 0 || pCircuit_->circuitGates_[gate.fanoutVector_[j]].co_ < gate.co_) {
                        gate.co_ = pCircuit_->circuitGates_[gate.fanoutVector_[j]].co_;
                    }
                }
                break;
            case Gate::INV:
                gate.co_ = pCircuit_->circuitGates_[gate.fanoutVector_[0]].co_ + 1;
                break;
            case Gate::AND2:
            case Gate::AND3:
            case Gate::AND4:
            case Gate::NAND2:
            case Gate::NAND3:
            case Gate::NAND4:
                gate.co_ = pCircuit_->circuitGates_[gate.fanoutVector_[0]].co_ + 1;
                for (int j = 0; j < pCircuit_->circuitGates_[gate.fanoutVector_[0]].numFI_; ++j) {
                    if (pCircuit_->circuitGates_[gate.fanoutVector_[0]].faninVector_[j] != gateID) {
                        Gate& gateSibling = pCircuit_->circuitGates_[pCircuit_->circuitGates_[gate.fanoutVector_[0]].faninVector_[j]];
                        gate.co_ += gateSibling.cc1_;
                    }
                }
                break;
            case Gate::OR2:
            case Gate::OR3:
            case Gate::OR4:
            case Gate::NOR2:
            case Gate::NOR3:
            case Gate::NOR4:
                gate.co_ = pCircuit_->circuitGates_[gate.fanoutVector_[0]].co_ + 1;
                for (int j = 0; j < pCircuit_->circuitGates_[gate.fanoutVector_[0]].numFI_; ++j) {
                    if (pCircuit_->circuitGates_[gate.fanoutVector_[0]].faninVector_[j] != gateID) {
                        Gate& gateSibling = pCircuit_->circuitGates_[pCircuit_->circuitGates_[gate.fanoutVector_[0]].faninVector_[j]];
                        gate.co_ += gateSibling.cc0_;
                    }
                }
                break;
            case Gate::XOR2:
            case Gate::XNOR2:
            case Gate::XOR3:
            case Gate::XNOR3:
                gate.co_ = pCircuit_->circuitGates_[gate.fanoutVector_[0]].co_ + 1;
                for (int j = 0; j < pCircuit_->circuitGates_[gate.fanoutVector_[0]].numFI_; ++j) {
                    Gate& gateSibling = pCircuit_->circuitGates_[pCircuit_->circuitGates_[gate.fanoutVector_[0]].faninVector_[j]];
                    if (pCircuit_->circuitGates_[gate.fanoutVector_[0]].faninVector_[j] != gateID) {
                        gate.co_ += std::min(gateSibling.cc0_, gateSibling.cc1_);
                    }
                }
                break;
            default:
                std::cerr << "Bug: reach switch case default while calculating co_";
                std::cin.get();
                break;
        }
    }

    return;
}

// **************************************************************************
// Function   [ Atpg::testClearFaultEffect ]
// Commenter  [ CAL ]
// Synopsis   [ usage:
//                Test clearAllFaultEffectByEvaluation for all faults
//              in:    void
//              out:   void
//            ]
// Date       [ started 2020/07/04    last modified 2020/07/04 ]
// **************************************************************************
void Atpg::testClearFaultEffect(FaultPtrList& faultListToTest) {
    for (Fault* pFault : faultListToTest) {
        generateSinglePatternOnTargetFault(*pFault, false);
        clearAllFaultEffectByEvaluation();

        for (int i = 0; i < pCircuit_->totalGate_; ++i) {
            Gate& gate = pCircuit_->circuitGates_[i];
            if ((gate.atpgVal_ == D) || (gate.atpgVal_ == B)) {
                std::cerr << "testClearFaultEffect found bug" << std::endl;
                std::cin.get();
            }
        }
    }
}

// **************************************************************************
// Function   [ Atpg::resetIsInEventStack ]
// Commenter  [ CAL ]
// Synopsis   [ usage:
//                Set all element in isInEventStack_ to false
//              in:    void
//              out:   void
//            ]
// Date       [ started 2020/07/07    last modified 2020/07/07 ]
// **************************************************************************
void Atpg::resetIsInEventStack() {
    std::fill(isInEventStack_.begin(), isInEventStack_.end(), 0);
}

// **************************************************************************
// Function   [ Atpg::XFill ]
// Commenter  [ HKY CYW ]
// Synopsis   [ usage: do X-Fill on generated pattern
//              in:    Pattern list
//              out:   void //TODO
//            ]
// Date       [ HKY Ver. 1.0 started 2014/09/01 ]
// **************************************************************************
void Atpg::XFill(PatternProcessor* pPatternProcessor) {
    for (int i = 0; i < (int)pPatternProcessor->patternVector_.size(); ++i) {
        randomFill(pPatternProcessor->patternVector_[i]);
        pSimulator_->assignPatternToCircuitInputs(pPatternProcessor->patternVector_.at(i));
        pSimulator_->goodSim();
        writeGoodSimValToPatternPO(pPatternProcessor->patternVector_.at(i));
    }
}