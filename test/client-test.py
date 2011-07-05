#!/usr/bin/env python

import sys
import gobject
import dbus
import dbus.mainloop.glib
import dbus.glib
import threading
import string
sys.path.append("../libummsclient")
import libummsclient

method_name = (
	"SetUri",
  "SetTarget",
	"Play",
	"Pause",
	"Stop",
	"SetPosition",
	"GetPosition",#5
	"SetPlaybackRate",
	"GetPlaybackRate",
	"SetVolume",
	"GetVolume",
	"SetWindowId",#10
	"SetVideoSize",
	"GetVideoSize",
	"GetBufferedTime",
	"GetBufferedBytes",
	"GetMediaSizeTime",#15
	"GetMediaSizeBytes",
	"HasVideo",
	"HasAudio",
	"IsStreaming",
	"IsSeekable",#20
	"SupportFullscreen",
	"GetPlayerState"
	)

(
	SetUri,
  SetTarget,
	Play,
	Pause,
	Stop,
	SetPosition,
	GetPosition,
	SetPlaybackRate,
	GetPlaybackRate,
	SetVolume,
	GetVolume,
	SetWindowId,
	SetVideoSize,
	GetVideoSize,
	GetBufferedTime,
	GetBufferedBytes,
	GetMediaSizeTime,
	GetMediaSizeBytes,
	HasVideo,
	HasAudio,
	IsStreaming,
	IsSeekable,
	SupportFullscreen,
	GetPlayerState
) = range (len(method_name))

(
    XWINDOW,
    DataCopy,
    Socket,
    ReservedType0,#On CE4100 platform, indicate using gdl plane directly to render video data 
    ReservedType1,
    ReservedType2,
    ReservedType3
) = range (7)

#CE4100 specific
(
  UPP_A,
  UPP_B,
  UPP_C
) = range (4, 7)

#define TARGET_PARAM_KEY_RECTANGLE "rectangle"
#define TARGET_PARAM_KEY_PlANE_ID "plane-id"


def initialized_cb():
	print "MeidaPlayer initialized"

def player_state_changed_cb(state):
	print "State changed to '%s'" % state_name[state]

def eof_cb ():
	print "EOF...."

def begin_buffering_cb():
    print "Begin buffering"

def buffered_cb():
    print "Buffering completed"

def seeked_cb():
    print "Seeking completed"

def stopped_cb():
    print "Player stopped"

def error_cb(err_id, msg):
    print "Error Domain:'%s', msg='%s'" % (error_type_name[err_id], msg)

def request_window_cb():
    print "Player engine request a X window"

def target_ready_cb(target_infos):
    print "target's rectangle = '%s'" % target_infos["rectangle"]#Assume we are using gdl plane target
    print "target's plane-id = '%d'" % target_infos["plane-id"]

def connect_sigs (proxy):
     print "connect signals"
     proxy.connect_to_signal("Initialized", initialized_cb)
     proxy.connect_to_signal("Eof", eof_cb)
     proxy.connect_to_signal("Buffering", begin_buffering_cb)
     proxy.connect_to_signal("Buffered", buffered_cb)
     proxy.connect_to_signal("RequestWindow", request_window_cb)
     proxy.connect_to_signal("Seeked", seeked_cb)
     proxy.connect_to_signal("Stopped", stopped_cb)
     proxy.connect_to_signal("Error", error_cb)
     proxy.connect_to_signal("PlayerStateChanged", player_state_changed_cb)
     proxy.connect_to_signal("TargetReady", target_ready_cb)
     return


class CmdHandler(threading.Thread):
    def __init__(self, player):
        threading.Thread.__init__(self)
        self.player = player
	
    def run(self):
        quit = False
        while quit == False:
            cmd = raw_input ("Input cmd:\n")
            quit = (self.handle_cmd(cmd) == False)
        else :
            loop.quit()
	
    def handle_cmd(self, cmd):
        ret = True
        if cmd.isalpha():
            if cmd == "q":
                ret = False
            else:
                self.print_help()

        elif cmd.isdigit():
            method_id = int(cmd)
            ret = self.remote_call(method_id)
        else:
            ret = False;
        return ret

    def print_help(self):
        print '  ===========Usage============'
        for i in range(0, len(method_name)):
            print "  %d:\t%s" % (i, method_name[i])
        print "  q:\tto quit this program"
        print "  h:\tprint this usage"


    		
    def remote_call(self, mid):
        print "Begin remote_call, method_id = '%d'" % (mid)
        global player_name
        if mid == SetUri:
            uri = raw_input("Input the uri to playback, 'd' for default: '%s'\n" % (default_uri))
            print "uri to playback :'%s'" % (uri)
            if uri == "d":
                self.player.SetUri(default_uri)
            else :
                self.player.SetUri(uri)
        elif mid == SetTarget:
            print "SetTarget"
	    self.player.SetTarget(ReservedType0, {"rectangle":"0,0,352,288", "plane-id":UPP_A})
        elif mid == Play:
            print "Play"
            self.player.Play()
        elif mid == Pause:
            print "Pause"
            self.player.Pause()
        elif mid == Stop:
            print "Stop"
            self.player.Stop()
        elif mid == SetPosition:
            pos = 1000*int(raw_input ("Input pos to seek: (seconds)\n"))
            print "SetPosition to '%d' ms" % pos
            self.player.SetPosition(pos)
        elif mid == GetPosition:
            pos = self.player.GetPosition()
            print "Current Position : '%d' ms" % (pos)
        elif mid == SetPlaybackRate:
        	rate = float(raw_input("Input playback rate: "))
        	print "Set rate to '%f'" % rate
        	self.player.SetPlaybackRate(rate)
        elif mid == GetPlaybackRate:
        	rate = self.player.GetPlaybackRate()
        	print "Current rate is '%f'" % rate
        elif mid == SetVolume:
            vol = int(raw_input ("Input volume to set: [0,100]\n"))
            print "SetVolume to %d" % vol
            self.player.SetVolume(vol)
        elif mid == GetVolume:
        	vol = self.player.GetVolume()
        	print "Current vol is '%d'" % vol
        elif mid == SetWindowId:
        	print "TODO:"
        elif mid == SetVideoSize:
            rectangle = raw_input ("Input dest reactangle, Example: 0,0,352,288\n")
            rectangle = rectangle.split(',')
            x = int(rectangle[0])
            y = int(rectangle[1])
            w = int(rectangle[2])
            h = int(rectangle[3])
            print "SetVideoSize (%d,%d,%d,%d)" % (x,y,w,h) 
            self.player.SetVideoSize(x,y,w,h)
        elif mid == GetVideoSize:
        	(width, height) = self.player.GetVideoSize()
        	print "width=%d, height=%d" % (width, height)
        elif mid == GetBufferedTime:
        	buffered_time = self.player.GetBufferedTime()
        	print "Buffered time = %d" % buffered_time
        elif mid == GetBufferedBytes:
        	buffered_bytes = self.player.GetBufferedBytes()
        	print "Buffered bytes = %d" % buffered_bytes
        elif mid == GetMediaSizeTime:
        	duration = self.player.GetMediaSizeTime()
        	print "Duration is '%d'" % duration
        elif mid == GetMediaSizeBytes:
        	size = self.player.GetMediaSizeBytes()
        	print "Media file size = %d" % size
        elif mid == HasVideo:
        	ret = self.player.HasVideo()
        	print "Has video is %d" % ret
        elif mid == HasAudio:
        	ret = self.player.HasAudio()
        	print "Has audio is %d" % ret
        elif mid == IsStreaming:
        	ret = self.player.IsStreaming()
        	print "IsStreaming = %d" % ret
        elif mid == IsSeekable:
        	ret = self.player.IsSeekable()
        	print "IsSeekable = %d" % ret
        elif mid == SupportFullscreen:
        	ret = self.player.SupportFullscreen()
        	print "SupportFullscreen = %d" % ret
        elif mid == GetPlayerState:
        	state = self.player.GetPlayerState()
        	print "Current state = '%s'" % state_name[state]
        else:
        	print "Unsupported method id '%d'" % (mid)
        	self.print_help(self);
        return True

error_type_name = (
    	"ErrorTypeEngine",
    	"NumOfErrorType"
	)

state_name = (
	"PlayerStateNull",
	"PlayerStatePaused",
	"PlayerStatePlaying",
	"PlayerStateStopped"
)


default_uri = "file:///root/720p.m4v"
player_name = ""

if __name__ == '__main__':

    libummsclient.init()
    (remote_proxy, name)  = libummsclient.request_player(True, 0)
    
    #Request a unattended execution
    #(remote_proxy, name)  = libummsclient.request_player(False, 10)
    print "player name = '%s'" % name
    connect_sigs(remote_proxy)

    gobject.threads_init()
    dbus.glib.init_threads()

    cmd_handler = CmdHandler(remote_proxy)
    cmd_handler.start()
	
    loop = gobject.MainLoop()
    loop.run()
    print "Client exit...."
