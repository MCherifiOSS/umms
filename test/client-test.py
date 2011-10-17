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
    "GetPlayerState",
    "SetProxy",
    "Suspend",
    "Restore",
    "GetPlayingContentMetadata",
    "SetGlobalVolume",
    "GetGlobalVolume",
    "DisableAudioOuput",
    "EnableAudioOuput",
    "GetAudioOutputState",#31
    "GetCurrentVideo",
    "GetCurrentAudio",
    "SetCurrentVideo",
    "SetCurrentAudio",
    "GetVideoNum",
    "GetAudioNum",
    "SetSubtitleUri",
    "SetCurrentSubtitle",
    "GetCurrentSubtitle",#40
    "GetVideoCodec",
    "GetPat",
    "GetPmt",
    "Record",
    "ChangeProgram"
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
    GetPlayerState,
    SetProxy,
    Suspend,
    Restore,
    GetPlayingContentMetadata,
    SetGlobalVolume,
    GetGlobalVolume,
    DisableAudioOuput,
    EnableAudioOuput,
    GetAudioOutputState,
    GetCurrentVideo,
    GetCurrentAudio,
    SetCurrentVideo,
    SetCurrentAudio,
    GetVideoNum,
    GetAudioNum,
    SetSubtitleUri,
    SetCurrentSubtitle,
    GetCurrentSubtitle,
    GetVideoCodec,
    GetPat,
    GetPmt,
    Record,
    ChangeProgram
) = range (len(method_name))

(
    XWindow,
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

def print_pmt (pmt):
    print "program-number: %4u" % pmt[0]
    print "pcr-pid:        %4u" % pmt[1]
    info = pmt[2]
    print "stream info:"
    for i in range (0, len(info)):
        print "     stream-type : %4u, pid: %4u" % (info[i]['stream-type'], info[i]['pid'])

def print_pat (pat):
    for i in range (0, len(pat)):
        print "program-number : %4u, pid : %4u" % (pat[i]['program-number'], pat[i]['pid'])

def print_metadata_list(metadata):
    for i in range (0, len(metadata)):
        print "URI : '%s'" %  metadata[i]['URI']
        print "Title : '%s'" %  metadata[i]['Title']
        print "Artist : '%s'" % metadata[i]['Artist']


def metadata_updated_cb(metadata):
    print "viewer: metadata_updated_cb"
    print_metadata_list(metadata)

def initialized_cb():
	print "MeidaPlayer initialized"

def player_state_changed_cb(old_state, new_state):
	print "State changed from '%s' to '%s'" % (state_name[old_state], state_name[new_state])

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
    print "Error Domain:'%d', msg='%s'" % (err_id, msg)

def request_window_cb():
    print "Player engine request a X window"

def target_ready_cb(target_infos):
    print "target's rectangle = '%s'" % target_infos["rectangle"]#Assume we are using gdl plane target
    print "target's plane-id = '%d'" % target_infos["plane-id"]

def suspended_cb():
    print "player suspended"

def restored_cb():
    print "player restored"

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
     proxy.connect_to_signal("Suspended", suspended_cb)
     proxy.connect_to_signal("Restored", restored_cb)
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
        if mid == SetUri:
            uri = raw_input("Input the uri to playback, 'd' for default: '%s'\n" % (default_uri))
            print "uri to playback :'%s'" % (uri)
            if uri == "d":
                self.player.SetUri(default_uri)
            else :
                self.player.SetUri(uri)
        elif mid == SetTarget:
            print "SetTarget"
            type = raw_input("Input target type: 'd' for DataCopy, 'r' for ReservedType0(plane), 'x' for XWindow\n")
            if type == "d":
                self.player.SetTarget(DataCopy, {})
            elif type == "r": 
                rectangle = raw_input ("Input the rectangle: 'd' for fullscreen\n")
                if rectangle == "d":
		  rectangle = "0,0,0,0"
                self.player.SetTarget(ReservedType0, {"rectangle":rectangle, "plane-id":UPP_A})
            elif type == "x":
                wid_str = raw_input("Input window id: \n")
            	if cmp(wid_str[0:2],"0x"[0:2]) == 0:
	            wid = int (wid_str, 16)
                else:
                    wid = int (wid_str)
                print "wid: %d" % wid
                self.player.SetTarget(XWindow, {"window-id":wid})
            else:
                print "Not supported target type:'%s'\n" % (type)
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
        elif mid == SetProxy:
        	state = self.player.SetProxy({"proxy-uri":"http://proxy01.pd.intel.com:911"})
        elif mid == Suspend:
        	state = self.player.Suspend();
        elif mid == Restore:
        	state = self.player.Restore();

        elif mid == GetCurrentVideo:
            print self.player.GetCurrentVideo();

        elif mid == GetCurrentAudio:
            print self.player.GetCurrentAudio();

        elif mid == SetCurrentVideo:
            num = int(raw_input ("Input video number to set:\n"))
            self.player.SetCurrentVideo(num);
       
        elif mid == SetCurrentAudio:
            num = int(raw_input ("Input audio number to set:\n"))
            self.player.SetCurrentAudio(num);

        elif mid == GetVideoNum:
            print self.player.GetVideoNum();
        elif mid == GetAudioNum:
            print self.player.GetAudioNum();
	
        elif mid == SetSubtitleUri:
            uri = raw_input("Input the Subtitle, 'd' for default: '%s'\n" % (default_sub))
            print "uri for subtitle :'%s'" % (uri)
            if uri == "d":
                self.player.SetSubtitleUri(default_sub)
            else :
                self.player.SetSubtitleUri(uri)
	
        elif mid == SetCurrentSubtitle:
            sub = int(raw_input("Input your expect subtitle: \n"))
            self.player.SetCurrentSubtitle(sub);

        elif mid == GetCurrentSubtitle:
        	print self.player.GetCurrentSubtitle();

        elif mid == GetVideoCodec:
        	print self.player.GetVideoCodec(0);
	
        elif mid == GetPlayingContentMetadata:
            metadata = metadata_viewer.GetPlayingContentMetadata();
            print_metadata_list(metadata)
        elif mid == SetGlobalVolume:
            volume = int(raw_input ("Input volume : "))
            audio_manager.SetVolume(0, volume);
        elif mid == GetGlobalVolume:
            volume = audio_manager.GetVolume(0);
            print "Global volume = %f" % (volume)
        elif mid == DisableAudioOuput:
            type = self.input_audio_type()
            audio_manager.SetState(type, 0);
        elif mid == EnableAudioOuput:
            type = self.input_audio_type()
            audio_manager.SetState(type, 1);
        elif mid == GetAudioOutputState:
            type = self.input_audio_type()
            state = audio_manager.GetState(type)
            print "audio output(%d) state = %d" % (type, state)
        elif mid == GetPat:
            pat = self.player.GetPat()
            print_pat (pat)
        elif mid == GetPmt:
            #(program_num, pcr_pid, streams) = self.player.GetPmt()
            pmt = self.player.GetPmt()
            print_pmt (pmt)
	          #print "program_num: %4u, pcr_pid: %4u" % (program_num, pcr_pid)
	          #print "stream info:"
	          #for i in range (0, len(streams)):
		        #    print "pid : %4u, stream-type: %4u" % (streams[i]['pid'], streams[i]['stream-type'])
        elif mid == Record:
            to_record = int (raw_input ("0: stop recording, 1: start recording "))
            self.player.Record (to_record)
        elif mid == ChangeProgram:
            num = raw_input("Input program number: ")
            dvburi = "dvb://?program-number="+num+"&type=0&modulation=1&trans-mod=0&bandwidth=0&frequency=578000000&code-rate-lp=0&code-rate-hp=3&guard=0&hierarchy=0"
            self.player.SetUri (dvburi)
            self.player.Play ()
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
default_uri = "dvb://?program-number=-1&type=0&modulation=1&trans-mod=0&bandwidth=0&frequency=578000000&code-rate-lp=0&code-rate-hp=3&guard=0&hierarchy=0"
#default_uri = "file:///root/p.mkv"
default_sub = "file:///root/video/subtest/text-subtitle.srt"
#"file://root/text-subtitle.srt"
#default_uri = "file:///root/multi.mkv"
player_name = ""
metadata_viewer = None
audio_manager = None

if __name__ == '__main__':

    libummsclient.init()
    (remote_proxy, name)  = libummsclient.request_player(True, 0)
    metadata_viewer  = libummsclient.get_metadata_viewer()
    metadata_viewer.connect_to_signal("MetadataUpdated", metadata_updated_cb)
    audio_manager  = libummsclient.get_audio_manager()
    
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

