//
//  CloudsInputKinectOSC.cpp
//  YellowTail
//
//  Created by Elie Zananiri on 12/8/13.
//  
//

#include "CloudsInputKinectOSC.h"
#include "CloudsInputEvents.h"

int kListenPort          = 12345;
int kNumFramesForRemoval = 60;
int kPollThreshold       = 5;

//--------------------------------------------------------------
CloudsInputKinectOSC::CloudsInputKinectOSC(float activeThresholdY, float activeThresholdZ)
: activeThresholdY(activeThresholdY)
, activeThresholdZ(activeThresholdZ)
, primaryIdx(-1)
{

}

//--------------------------------------------------------------
void CloudsInputKinectOSC::enable()
{
	if (!enabled) {
        receiver.setup(kListenPort);
        ofAddListener(ofEvents().update, this, &CloudsInputKinectOSC::update);
		enabled = true;
	}
}

//--------------------------------------------------------------
void CloudsInputKinectOSC::disable()
{
	if (enabled) {
//        receiver.shutdown();
        ofRemoveListener(ofEvents().update, this, &CloudsInputKinectOSC::update);
		enabled = false;
	}
}

//--------------------------------------------------------------
void CloudsInputKinectOSC::update(ofEventArgs& args)
{
    // check for waiting messages
    while (receiver.hasWaitingMessages()) {
		// get the next message
		ofxOscMessage m;
		receiver.getNextMessage(&m);
        
        bool bRecognized = true;
        
		if (m.getAddress() == "/playerData") {
            // set up all the working vars
            k4w::HandState newHandState;
            lastOscFrame = ofGetFrameNum();

            float activeThresholdPosY = ofMap(activeThresholdY, 0, 1, 1, -1);
            bool bDoProximity = (activeThresholdZ != 1);
            float activeThresholdPosZ = ofMap(activeThresholdZ, 0, 1, 0, -1);

            int i = 0;
			int idx = m.getArgAsInt32(i++);
                        
            // process the body data
            
            // create a body if it doesn't exist yet
            if (bodies.find(idx) == bodies.end()) {
                bodies[idx] = new k4w::Body();
                bodies[idx]->idx = idx;
            }
            
            // update the head joint
            bodies[idx]->headJoint.type = (k4w::JointType)m.getArgAsInt32(i++);
            bodies[idx]->headJoint.trackingState = (k4w::TrackingState)m.getArgAsInt32(i++);
            bodies[idx]->headJoint.inputPosition.set(m.getArgAsFloat(i++), 
                                                     m.getArgAsFloat(i++), 
                                                     m.getArgAsFloat(i++));
            
            // update the spine neck joint
            bodies[idx]->spineNeckJoint.type = (k4w::JointType)m.getArgAsInt32(i++);
            bodies[idx]->spineNeckJoint.trackingState = (k4w::TrackingState)m.getArgAsInt32(i++);
            bodies[idx]->spineNeckJoint.inputPosition.set(m.getArgAsFloat(i++), 
                                                          m.getArgAsFloat(i++), 
                                                          m.getArgAsFloat(i++));
            
            // update the spine base joint
            bodies[idx]->spineBaseJoint.type = (k4w::JointType)m.getArgAsInt32(i++);
            bodies[idx]->spineBaseJoint.trackingState = (k4w::TrackingState)m.getArgAsInt32(i++);
            bodies[idx]->spineBaseJoint.inputPosition.set(m.getArgAsFloat(i++), 
                                                          m.getArgAsFloat(i++), 
                                                          m.getArgAsFloat(i++));
            
            // calculate the head to spine base length for mapping
            float mappingLength = bodies[idx]->headJoint.inputPosition.distance(bodies[idx]->spineBaseJoint.inputPosition);
            
            // map the spine points
            mapCoords(bodies[idx]->spineNeckJoint.inputPosition, mappingLength, bodies[idx]->spineNeckJoint);
            mapCoords(bodies[idx]->spineNeckJoint.inputPosition, mappingLength, bodies[idx]->headJoint);
            mapCoords(bodies[idx]->spineNeckJoint.inputPosition, mappingLength, bodies[idx]->spineBaseJoint);
            
            // refresh the update frame and age
            bodies[idx]->lastUpdateFrame = lastOscFrame;
            bodies[idx]->age++;
            
            // process the hand data
            for (int j = 0; j < 2; j++) {
                int handIdx = idx * 2 + j;
                
                // create a hand if it doesn't exist yet
                if (hands.find(handIdx) == hands.end()) {
                    hands[handIdx] = new k4w::Hand();
                    hands[handIdx]->idx = handIdx;
                    hands[handIdx]->bodyIdx = idx;
                }
                
                // update the hand joint
                hands[handIdx]->handJoint.type = (k4w::JointType)m.getArgAsInt32(i++);
                hands[handIdx]->handJoint.trackingState = (k4w::TrackingState)m.getArgAsInt32(i++);
                hands[handIdx]->handJoint.inputPosition.set(m.getArgAsFloat(i++), 
                                                            m.getArgAsFloat(i++), 
                                                            m.getArgAsFloat(i++));
                mapCoords(bodies[idx]->spineNeckJoint.inputPosition, mappingLength, hands[handIdx]->handJoint);
                
                // set the active state based on the local position
                if ((hands[handIdx]->handJoint.localPosition.y > activeThresholdPosY) && 
                    (hands[handIdx]->handJoint.localPosition.y > bodies[idx]->spineBaseJoint.localPosition.y)) {
                    hands[handIdx]->activeFrames++;
                }
                else {
                    hands[handIdx]->activeFrames = MAX(0, MIN(kPollThreshold, hands[handIdx]->activeFrames - 1));
                }
                
                // process the event
                if (bDoProximity) {
                    // ignore the incoming state and use proximity
                    if (hands[handIdx]->handJoint.localPosition.z < activeThresholdPosZ) {
                        newHandState = k4w::HandState_Lasso;
                    }
                    else {
                        newHandState = k4w::HandState_Open;
                    }
                    i++;  // bump the message index
                }
                else {
                    newHandState = (k4w::HandState)m.getArgAsInt32(i++);
                }
                hands[handIdx]->poll[newHandState]++;
                if (hands[handIdx]->poll[newHandState] >= kPollThreshold) {
                    // boom! new state achieved
                    processHandEvent(handIdx, hands[handIdx], newHandState);
                    hands[handIdx]->handJoint.handState = newHandState;
                        
                    for (int k = 0; k < k4w::HandState_Count; k++) {
                        if (k != newHandState) {
                            hands[handIdx]->poll[k] = 0;
                        }
                    }
                }
                else {
                    // carry on with the same state
                    processHandEvent(handIdx, hands[handIdx], hands[handIdx]->handJoint.handState);
                }
                
                // refresh the update frame
                hands[handIdx]->lastUpdateFrame = lastOscFrame;
            }
		}
		else {
            bRecognized = false;
        }
        
        if (!bRecognized) {
			// display in the console
			string msg_string;
			msg_string = m.getAddress();
			msg_string += ": ";
			for (int i = 0; i < m.getNumArgs(); i++) {
				// get the argument type
				msg_string += m.getArgTypeName(i);
				msg_string += ":";
				// display the argument - make sure we get the right type
				if (m.getArgType(i) == OFXOSC_TYPE_INT32) {
					msg_string += ofToString(m.getArgAsInt32(i));
				}
				else if (m.getArgType(i) == OFXOSC_TYPE_FLOAT) {
					msg_string += ofToString(m.getArgAsFloat(i));
				}
				else if (m.getArgType(i) == OFXOSC_TYPE_STRING) {
					msg_string += m.getArgAsString(i);
				}
				else {
					msg_string += "unknown";
				}
                msg_string += " ";
			}
            ofLogError("CloudsInputKinectOSC") << "Unrecognized message: " << msg_string;
		}
	}
    
    // remove any dead bodies
    vector<int> toRemove;
    for (map<int, k4w::Body *>::iterator it = bodies.begin(); it != bodies.end(); ++it) {
        if (ABS(it->second->lastUpdateFrame - lastOscFrame) > kNumFramesForRemoval) {
            toRemove.push_back(it->first);
        }
    }
    for (int i = 0; i < toRemove.size(); i++) {
        delete bodies[toRemove[i]];
        bodies.erase(toRemove[i]);
    }
    
    // remove any dead hands
    toRemove.clear();
    for (map<int, k4w::Hand *>::iterator it = hands.begin(); it != hands.end(); ++it) {
        if (ABS(it->second->lastUpdateFrame - lastOscFrame) > kNumFramesForRemoval) {
            // make sure the hand is not mid-action when getting removed
            processHandEvent(it->first, hands[it->first], k4w::HandState_Unknown);
            
            // if the hand was the primary cursor, unlink it 
            if (it->first == primaryIdx) {
                primaryIdx = -1;
            }
            
            toRemove.push_back(it->first);
        }
    }
    for (int i = 0; i < toRemove.size(); i++) {
        delete hands[toRemove[i]];
        hands.erase(toRemove[i]);
        
        if (inputPoints.find(toRemove[i]) != inputPoints.end()) {
            // also remove the matching input point
            inputPoints.erase(toRemove[i]);
        }
    }
    
    // unlink the primary cursor if it's been inactive for too long
    if (primaryIdx > -1 && hands[primaryIdx]->activeFrames <= 0) {
        primaryIdx = -1;
    }
    
    // look for a new designated cursor if necessary
    if (primaryIdx == -1) {
        int candidateIdx = -1;
        int candidateActiveFrames = 0;
        float candidateY = -1;
        for (map<int, k4w::Hand *>::iterator it = hands.begin(); it != hands.end(); ++it) {
            // select this hand if it is active AND either:
            //  1. there is no candidate yet
            //  2. it is older than the current candidate
            //  3. it is as old the current candidate but higher up
            if (it->second->activeFrames >= kPollThreshold && 
                    ((candidateIdx == -1) || 
                     (candidateActiveFrames < it->second->activeFrames) || 
                     (candidateActiveFrames == it->second->activeFrames && 
                         candidateY < it->second->handJoint.inputPosition.y))) {
                candidateIdx = it->first;
                candidateActiveFrames = it->second->activeFrames;
                candidateY = it->second->handJoint.inputPosition.y;
            }
        }
        primaryIdx = candidateIdx;
    }
    
    // set the current position to the designated hand
    if (primaryIdx == -1) {
        currentPosition.set(ofGetWidth() * 0.5, ofGetHeight() * 0.5);
    }
    else {
        currentPosition.set(hands[primaryIdx]->handJoint.mappedPosition);
    }
}

//--------------------------------------------------------------
void CloudsInputKinectOSC::mapCoords(ofVec3f& origin, float length, k4w::Joint& joint)
{
    // switch to a local coord system, centered at origin
    joint.localPosition = joint.inputPosition;
    joint.localPosition -= origin;
    
    // map the local position to the 2D viewport coord system
    float inMaxY = -(length - (2 * (1.0f - activeThresholdY) * length));
    joint.mappedPosition.set(ofMap(joint.localPosition.x, -length, length, 0, ofGetWidth()),
                             ofMap(joint.localPosition.y,  length, inMaxY, 0, ofGetHeight()),
                             ofMap(joint.localPosition.z, -length, length, 0, ofGetWidth()));
}

//--------------------------------------------------------------
void CloudsInputKinectOSC::processHandEvent(int handIdx, k4w::Hand * hand, k4w::HandState newState)
{    
    if (newState == k4w::HandState_Lasso) {
        if (hand->actionState == k4w::ActionState_Lasso) {
            // matching state: continue
            interactionDragged(hand->handJoint.mappedPosition, handIdx == primaryIdx, k4w::ActionState_Lasso, handIdx);
        }
        else if (hand->actionState == k4w::ActionState_Closed) {
            // state mismatch: end previous
            interactionEnded(hand->handJoint.mappedPosition, handIdx == primaryIdx, k4w::ActionState_Closed, handIdx);
            hand->actionState = k4w::ActionState_Idle;
        }
        else {
            // idle state: start
            interactionStarted(hand->handJoint.mappedPosition, handIdx == primaryIdx, k4w::ActionState_Lasso, handIdx);
            hand->actionState = k4w::ActionState_Lasso;
        }
    }  
    else if (newState == k4w::HandState_Closed) {
        if (hand->actionState == k4w::ActionState_Closed) {
            // matching state: continue
            interactionDragged(hand->handJoint.mappedPosition, handIdx == primaryIdx, k4w::ActionState_Closed, handIdx);
        }
        else if (hand->actionState == k4w::ActionState_Lasso) {
            // state mismatch: end previous
            interactionEnded(hand->handJoint.mappedPosition, handIdx == primaryIdx, k4w::ActionState_Lasso, handIdx);
            hand->actionState = k4w::ActionState_Idle;
        }
        else {
            // idle state: start
            interactionStarted(hand->handJoint.mappedPosition, handIdx == primaryIdx, k4w::ActionState_Closed, handIdx);
            hand->actionState = k4w::ActionState_Closed;
        }
    }
    else if (newState <= k4w::HandState_Open) {
        if (hand->actionState == k4w::ActionState_Idle) {
            // matching state: continue
            interactionMoved(hand->handJoint.mappedPosition, handIdx == primaryIdx, k4w::ActionState_Idle, handIdx);
        }
        else {
            // state mismatch: end previous
            interactionEnded(hand->handJoint.mappedPosition, handIdx == primaryIdx, hand->actionState, handIdx);
            hand->actionState = k4w::ActionState_Idle;
        }
    }
}

//--------------------------------------------------------------
void SetCloudsInputKinect(float activeThresholdY, float activeThresholdZ)
{
    SetCloudsInput(ofPtr<CloudsInput>(new CloudsInputKinectOSC(activeThresholdY, activeThresholdZ)));
}