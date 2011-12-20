#!/usr/bin/python
# Filename: libummsclient.py

import dbus
import dbus.mainloop.glib
import dbus.glib

obj_mngr = None
metadata_viewer = None
audio_manager = None
video_output = None

def init():
    print "Init libclient"
    global obj_mngr
    global metadata_viewer
    global audio_manager
    global video_output 
    
    if (obj_mngr != None):
        print "UMMS client lib already initialized"
        return

    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    bus=dbus.SystemBus()
    bus_obj=bus.get_object('com.UMMS', '/com/UMMS/ObjectManager')
    obj_mngr=dbus.Interface(bus_obj, 'com.UMMS.ObjectManager.iface')
    print "New obj_mngr"

    bus_obj=bus.get_object('com.UMMS', '/com/UMMS/PlayingContentMetadataViewer')
    metadata_viewer=dbus.Interface(bus_obj, 'com.UMMS.PlayingContentMetadataViewer')
    print "New metadata_viewer"

    bus_obj=bus.get_object('com.UMMS', '/com/UMMS/AudioManager')
    audio_manager =dbus.Interface(bus_obj, 'com.UMMS.AudioManager')
    print "New audio_manager"

    bus_obj=bus.get_object('com.UMMS', '/com/UMMS/VideoOutput')
    video_output=dbus.Interface(bus_obj, 'com.UMMS.VideoOutput')
    print "New video_output"


def need_reply_cb (obj_path):
    client_monitor = get_iface (obj_path, 'com.UMMS.MediaPlayer')
    client_monitor.Reply()
    #print "Reply to object '%s'" % obj_path

def get_iface(obj_path, iface_name):
    #print "Getting interface '%s' from '%s'" % (iface_name, obj_path)
    proxy = None
    bus=dbus.SystemBus()
    bus_obj=bus.get_object("com.UMMS", obj_path)
    if iface_name == 'com.UMMS.MediaPlayer':
        proxy=dbus.Interface(bus_obj, 'com.UMMS.MediaPlayer')
    else :
        print "Unknown interface '%s'" % iface_name
    return proxy

def connect_to_need_reply_sig(player):
    player.connect_to_signal("NeedReply", need_reply_cb, path_keyword="obj_path")
    print "connect_to_signal NeedReply"

def request_player(attended, time_to_execution):
    print "Request media player, attended = %d, time_to_execution = %f" % (attended, time_to_execution) 
    global obj_mngr

    if (attended):
        player_name = obj_mngr.RequestMediaPlayer()
    else:
        (token, player_name) = obj_mngr.RequestMediaPlayerUnattended(time_to_execution)

    player = get_iface (player_name, 'com.UMMS.MediaPlayer') 

    if (attended) :
        connect_to_need_reply_sig(player)

    return (player, player_name)

def remove_player(proxy):
    global obj_mngr
    obj_path = proxy.object_path
    print "UMMS client lib: Removing media player '%s'" % obj_path
    obj_mngr.RemoveMediaPlayer(obj_path)

def get_metadata_viewer():
    global metadata_viewer;
    print "Return global metadata_viewer"
    return metadata_viewer;

def get_audio_manager():
    global audio_manager;
    print "Return global audio_manager"
    return audio_manager;

def get_video_output():
    global video_output;
    print "Return global video_output"
    return video_output;
