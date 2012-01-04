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
    "Stop"
)

(
    Stop
) = range (len(method_name))

def record_start_cb():
	print "Received RecordStart signal"

def record_stop_cb():
	print "Received RecordStop signal"

def error_cb(err_id, msg):
    print "Error Domain:'%d', msg='%s'" % (err_id, msg)

def connect_sigs (proxy):
     print "connect signals"
     proxy.connect_to_signal("RecordStart", record_start_cb)
     proxy.connect_to_signal("RecordStop", record_stop_cb)
     proxy.connect_to_signal("Error", error_cb)
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

    def input_audio_type(self):
        return int(raw_input ("Chose audio output type: 0:hdmi, 1:spdif, 2:i2s0, 3:i2s1 : "))
    		
    def remote_call(self, mid):
        print "Begin remote_call, method_id = '%d'" % (mid)
        global player_name
        if mid == Stop:
            print "Stop"
            self.player.Stop()
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
    "PlayerStateStopped",
    "PlayerStatePaused",
    "PlayerStatePlaying"
)


#default_uri = "file:///root/video/720p.m4v"
#default_uri = "file:///root/rpmbuild/BUILD/dvbsub.ts"
default_uri = "dvb://?program-number=-1&type=0&modulation=1&trans-mod=0&bandwidth=0&frequency=578000000&code-rate-lp=0&code-rate-hp=3&guard=0&hierarchy=0"
#default_uri = "file:///root/p.mkv"
default_sub = "file:///root/video/subtest/text-subtitle.srt"
#"file://root/text-subtitle.srt"
#default_uri = "file:///root/multi.mkv"
player_name = ""
metadata_viewer = None
audio_manager = None
video_output = None

if __name__ == '__main__':

    libummsclient.init()
    (remote_proxy, name)  = libummsclient.request_scheduled_recorder(5, 5, default_uri, "/tmp/dvb.ts")
    
    print "player name = '%s'" % name
    connect_sigs(remote_proxy)

    gobject.threads_init()
    dbus.glib.init_threads()

    cmd_handler = CmdHandler(remote_proxy)
    cmd_handler.start()
	
    loop = gobject.MainLoop()
    loop.run()
    print "Client exit...."

