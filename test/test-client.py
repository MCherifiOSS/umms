#!/usr/bin/env python

import gobject
import dbus
import dbus.mainloop.glib
import dbus.glib
import threading
import string

import libummsclient

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
        global remote_proxy
        ret = True
        if cmd.isalpha():
            if cmd == "q":
                ret = False
            elif cmd == "r":
                print "request media player"
                (remote_proxy, name)  = libummsclient.request_player(True, 0)
            elif cmd == "u":
                print "request unattended media player"
                (remote_proxy, name)  = libummsclient.request_player(False, 5)
            elif cmd == "d":
                print "remove current media player"
                libummsclient.remove_player(remote_proxy);
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
        print "  q:\tto quit this program"
        print "  r:\trequest a media player"
        print "  u:\trequest an unattended media player"
        print "  d:\tremove current media player"
        print "  h:\tprint this usage"
        for i in range(0, len(method_name)):
            print "  %d:\t%s" % (i, method_name[i])

    		
    def remote_call(self, mid):
        print "Begin remote_call, method_id = '%d'" % (mid)
        global player_name
        if mid == 0:
            uri = raw_input("Input the uri to playback, default: '%s'\n" % (default_uri))
            print "uri to playback :'%s'" % (uri)
            if uri == "":
                self.player.SetUri(default_uri)
            else :
                self.player.SetUri(uri)
        elif mid == 1:
            print "Play"
            self.player.Play()
        elif mid == 2:
            print "Pause"
            self.player.Pause()
        elif mid == 3:
            print "Stop"
            self.player.Stop()
        elif mid == 4:
            pos = 1000*int(raw_input ("Input pos to seek: (seconds)\n"))
            print "SetPosition to '%d' ms" % pos
            self.player.SetPosition(pos)
        elif mid == 5:
            pos = self.player.GetPosition()
            print "Current Position : '%d' ms" % (pos)
        elif mid == 6:
        	rate = float(raw_input("Input playback rate: "))
        	print "Set rate to '%f'" % rate
        	self.player.SetPlaybackRate(rate)
        elif mid == 7:
        	rate = self.player.GetPlaybackRate()
        	print "Current rate is '%f'" % rate
        elif mid == 8:
            vol = int(raw_input ("Input volume to set: [0,100]\n"))
            print "SetVolume to %d" % vol
            self.player.SetVolume(vol)
        elif mid == 9:
        	vol = self.player.GetVolume()
        	print "Current vol is '%d'" % vol
        elif mid == 10:
        	print "TODO:"
        elif mid == 11:
            rectangle = raw_input ("Input dest reactangle, Example: 0,0,352,288\n")
            rectangle = rectangle.split(',')
            x = int(rectangle[0])
            y = int(rectangle[1])
            w = int(rectangle[2])
            h = int(rectangle[3])
            print "SetVideoSize (%d,%d,%d,%d)" % (x,y,w,h) 
            self.player.SetVideoSize(x,y,w,h)
        elif mid == 12:
        	(width, height) = self.player.GetVideoSize()
        	print "width=%d, height=%d" % (width, height)
        elif mid == 13:
        	buffered_time = self.player.GetBufferedTime()
        	print "Buffered time = %d" % buffered_time
        elif mid == 14:
        	buffered_bytes = self.player.GetBufferedBytes()
        	print "Buffered bytes = %d" % buffered_bytes
        elif mid == 15:
        	duration = self.player.GetMediaSizeTime()
        	print "Duration is '%d'" % duration
        elif mid == 16:
        	size = self.player.GetMediaSizeBytes()
        	print "Media file size = %d" % size
        elif mid == 17:
        	ret = self.player.HasVideo()
        	print "Has video is %d" % ret
        elif mid == 18:
        	ret = self.player.HasAudio()
        	print "Has audio is %d" % ret
        elif mid == 19:
        	ret = self.player.IsStreaming()
        	print "IsStreaming = %d" % ret
        elif mid == 20:
        	ret = self.player.IsSeekable()
        	print "IsSeekable = %d" % ret
        elif mid == 21:
        	ret = self.player.SupportFullscreen()
        	print "SupportFullscreen = %d" % ret
        elif mid == 22:
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

method_name = (
	"SetUri",
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

default_uri = "file:///root/720p.m4v"
remote_proxy = None
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

