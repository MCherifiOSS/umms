#!/usr/bin/env python

import gobject
import dbus
import dbus.mainloop.glib
import dbus.glib
import threading
import string



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


   

class CmdHandler(threading.Thread):
    def __init__(self, obj_mngr):
        threading.Thread.__init__(self)
        self.mngr = obj_mngr
	
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
        print "  q:\tto quit this program"
        print "  h:\tprint this usage"
        for i in range(0, len(method_name)):
            print "  %d:\t%s" % (i, method_name[i])

    def get_player(self, name):
        print "Create proxy for remote player '%s'" % (name)
        bus=dbus.SessionBus()
        bus_obj=bus.get_object("com.meego.UMMS", name)
        proxy=dbus.Interface(bus_obj, 'com.meego.UMMS.MediaPlayer')
        proxy.connect_to_signal("Initialized", initialized_cb)
        proxy.connect_to_signal("Eof", eof_cb)
        proxy.connect_to_signal("Buffering", begin_buffering_cb)
        proxy.connect_to_signal("Buffered", buffered_cb)
        proxy.connect_to_signal("RequestWindow", request_window_cb)
        proxy.connect_to_signal("Seeked", seeked_cb)
        proxy.connect_to_signal("Stopped", stopped_cb)
        proxy.connect_to_signal("Error", error_cb)
        proxy.connect_to_signal("PlayerStateChanged", player_state_changed_cb)
        return proxy
		
    def remote_call(self, mid):
        print "Begin remote_call, method_id = '%d'" % (mid)
        global player_name
        if mid == 0:
            print "RequestMediaPlayer"
            player_name = self.mngr.RequestMediaPlayer()
            player.append(self.get_player (player_name)) 
        elif mid == 1:
            time_to_execute = float(raw_input("Input the MediaPlayer execution duration (in second):\n")) 
            print "RequestMediaPlayerUnattended(%f s)" % time_to_execute
            (token, player_name) = self.mngr.RequestMediaPlayerUnattended(time_to_execute)
            print "token = '%s', player_name = '%s'" % (token, player_name)
            player.append(self.get_player (player_name)) 
        elif mid == 2:
            print "Remove '%s'" % (player_name)
            self.mngr.RemoveMediaPlayer(player_name)
        elif mid == 3:
            uri = raw_input("Input the uri to playback, default: '%s'\n" % (default_uri))
            print "uri to playback :'%s'" % (uri)
            if uri == "":
                player[0].SetUri(default_uri)
            else :
                player[0].SetUri(uri)
        elif mid == 4:
            print "Play"
            player[0].Play()
        elif mid == 5:
            print "Pause"
            player[0].Pause()
        elif mid == 6:
            print "Stop"
            player[0].Stop()
        elif mid == 7:
            pos = 1000*int(raw_input ("Input pos to seek: (seconds)\n"))
            print "SetPosition to '%d' ms" % pos
            player[0].SetPosition(pos)
        elif mid == 8:
            pos = player[0].GetPosition()
            print "Current Position : '%d' ms" % (pos)
        elif mid == 9:
        	rate = float(raw_input("Input playback rate: "))
        	print "Set rate to '%f'" % rate
        	player[0].SetPlaybackRate(rate)
        elif mid == 10:
        	rate = player[0].GetPlaybackRate()
        	print "Current rate is '%f'" % rate
        elif mid == 11:
            vol = int(raw_input ("Input volume to set: [0,100]\n"))
            print "SetVolume to %d" % vol
            player[0].SetVolume(vol)
        elif mid == 12:
        	vol = player[0].GetVolume()
        	print "Current vol is '%d'" % vol
        elif mid == 13:
        	print "TODO:"
        elif mid == 14:
            rectangle = raw_input ("Input dest reactangle, Example: 0,0,352,288\n")
            rectangle = rectangle.split(',')
            x = int(rectangle[0])
            y = int(rectangle[1])
            w = int(rectangle[2])
            h = int(rectangle[3])
            print "SetVideoSize (%d,%d,%d,%d)" % (x,y,w,h) 
            player[0].SetVideoSize(x,y,w,h)
        elif mid == 15:
        	(width, height) = player[0].GetVideoSize()
        	print "width=%d, height=%d" % (width, height)
        elif mid == 16:
        	buffered_time = player[0].GetBufferedTime()
        	print "Buffered time = %d" % buffered_time
        elif mid == 17:
        	buffered_bytes = player[0].GetBufferedBytes()
        	print "Buffered bytes = %d" % buffered_bytes
        elif mid == 18:
        	duration = player[0].GetMediaSizeTime()
        	print "Duration is '%d'" % duration
        elif mid == 19:
        	size = player[0].GetMediaSizeBytes()
        	print "Media file size = %d" % size
        elif mid == 20:
        	ret = player[0].HasVideo()
        	print "Has video is %d" % ret
        elif mid == 21:
        	ret = player[0].HasAudio()
        	print "Has audio is %d" % ret
        elif mid == 22:
        	ret = player[0].IsStreaming()
        	print "IsStreaming = %d" % ret
        elif mid == 23:
        	ret = player[0].IsSeekable()
        	print "IsSeekable = %d" % ret
        elif mid == 24:
        	ret = player[0].SupportFullscreen()
        	print "SupportFullscreen = %d" % ret
        elif mid == 25:
        	state = player[0].GetPlayerState()
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
	"RequestMediaPlayer",
	"RequestMediaPlayerUnattended",
	"RemoveMediaPlayer",
	
	"SetUri",#3
	"Play",
	"Pause",
	"Stop",
	
	"SetPosition",#7
	"GetPosition",
	"SetPlaybackRate",
	"GetPlaybackRate",
	"SetVolume",
	"GetVolume",
	"SetWindowId",
	"SetVideoSize",
	"GetVideoSize",
	"GetBufferedTime",
	"GetBufferedBytes",
	"GetMediaSizeTime",
	"GetMediaSizeBytes",
	"HasVideo",#20
	"HasAudio",
	"IsStreaming",
	"IsSeekable",
	"SupportFullscreen",
	"GetPlayerState"
	)

default_uri = "file:///root/720p.m4v"
player = []
player_name = ""

if __name__ == '__main__':


    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)


    bus=dbus.SessionBus()
    bus_obj=bus.get_object('com.meego.UMMS', '/com/meego/UMMS/ObjectManager')
    iface=dbus.Interface(bus_obj, 'com.meego.UMMS.ObjectManager.iface')
	
    gobject.threads_init()
    dbus.glib.init_threads()

    cmd_handler = CmdHandler(iface)
    cmd_handler.start()
	
    loop = gobject.MainLoop()
    loop.run()
    print "Client exit...."

