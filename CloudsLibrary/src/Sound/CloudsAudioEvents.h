//
//  CloudsAudioEvents.h
//  CLOUDS
//
//  Created by Gal Sasson on 12/19/13.
//
//


#pragma once

#include "ofMain.h"
#include "ofEvents.h"

class CloudsAudioEvents {
public:
    ofEvent<ofAudioEventArgs>   musicAudioRequested;
    ofEvent<ofAudioEventArgs>   diageticAudioRequested;
};

ofPtr<CloudsAudioEvents> GetCloudsAudioEvents();
