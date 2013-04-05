#pragma once

#include "ofMain.h"
#include "ofxCocoaGLView.h"
#include "CloudsFCPParser.h"
#include "CloudsFCPVisualizer.h"
#include "CloudsD3Exporter.h"
#include "ofxTimeline.h"
#include "ofxUI.h"
#include "CloudsStoryEngine.h"

@class ViewerApp;
@interface testView : ofxCocoaGLView <NSTableViewDataSource, NSTableViewDelegate, NSTokenFieldDelegate> {
    IBOutlet NSTableView* keywordTable;
    IBOutlet NSTableView* clipTable;
    IBOutlet NSTableView* linkTable;
	IBOutlet NSTableView* playlistTable;

    IBOutlet NSTextField* currentClipLabel;
    
    IBOutlet NSTokenField* currentKeywords;
    IBOutlet NSTextField* linkText;
	
	IBOutlet NSTextField* seedKeyword;
	
	IBOutlet ViewerApp* viewerApp;
	
    CloudsFCPParser parser;
    CloudsFCPVisualizer visualizer;
    CloudsStoryEngine storyEngine;
	
    bool updatePhysics;
    vector<string> selectedKeywords;
    ClipMarker currentPlayingClip;
    BOOL clipLoaded;
	
    vector<ClipMarker> selectedClips;
    vector<CloudsLink> currentClipLinks;

	bool playingPlaylist;
	int currentPlaylistIndex;
	
    int clipEndFrame;
    ofVideoPlayer preview;
    ofxTimeline inpoint;

	ofxUICanvas* gui;
	bool autoProgressStory;
	float timeOfNextStory;
}

@property(nonatomic,readonly) int clipEndFrame;
@property(nonatomic,readonly) ofVideoPlayer& preview;
@property(nonatomic,readonly) bool playingPlaylist;

- (void)setup;
- (void)update;
- (void)draw;
- (void)exit;

- (IBAction) setXMLFolder:(id)sender;
- (IBAction) refreshXML:(id)sender;
- (IBAction) createLink:(id)sender;
- (IBAction) playDoubleClickedRow:(id)sender;
- (IBAction) saveLinks:(id)sender;
- (IBAction) deleteLink:(id)sender;
- (IBAction) regenerateGraph:(id)sender;
- (IBAction) unloadVideo:(id)sender;

- (IBAction) nextOnPlaylist:(id)sender;
- (IBAction) prevOnPlaylist:(id)sender;
- (IBAction) playCurrentPlaylist:(id)sender;

- (void)keyPressed:(int)key;
- (void)keyReleased:(int)key;
- (void)mouseMoved:(NSPoint)p;
- (void)mouseDragged:(NSPoint)p button:(int)button;
- (void)mousePressed:(NSPoint)p button:(int)button;
- (void)mouseReleased:(NSPoint)p button:(int)button;
- (void)windowResized:(NSSize)size;

- (BOOL)tableView:(NSTableView *)aTableView shouldEditTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex;

- (ClipMarker&) selectedClip;
- (ClipMarker&) selectedClipFromPlaylist;

- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView;
- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex;
- (void)tableViewSelectionDidChange:(NSNotification *)aNotification;
- (void)tableView:(NSTableView *)tableView sortDescriptorsDidChange: (NSArray *)oldDescriptors;
- (NSArray *)tokenField:(NSTokenField *)tokenField completionsForSubstring:(NSString *)substring indexOfToken:(NSInteger)tokenIndex indexOfSelectedItem:(NSInteger *)selectedIndex;


@end
