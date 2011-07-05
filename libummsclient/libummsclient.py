#!/usr/bin/python
# Filename: libummsclient.py

import dbus
import dbus.mainloop.glib
import dbus.glib

obj_mngr = None

def init():
    print "Init libclient"
    global obj_mngr
    if (obj_mngr != None):
        print "UMMS client lib already initialized"
        return

    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    bus=dbus.SystemBus()
    bus_obj=bus.get_object('com.meego.UMMS', '/com/meego/UMMS/ObjectManager')
    obj_mngr=dbus.Interface(bus_obj, 'com.meego.UMMS.ObjectManager.iface')
    print "New obj_mngr"


def need_reply_cb (obj_path):
    client_monitor = get_iface (obj_path, 'com.meego.UMMS.MediaPlayer')
    client_monitor.Reply()
    #print "Reply to object '%s'" % obj_path

def get_iface(obj_path, iface_name):
    #print "Getting interface '%s' from '%s'" % (iface_name, obj_path)
    proxy = None
    bus=dbus.SystemBus()
    bus_obj=bus.get_object("com.meego.UMMS", obj_path)
    if iface_name == 'com.meego.UMMS.MediaPlayer':
        proxy=dbus.Interface(bus_obj, 'com.meego.UMMS.MediaPlayer')
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

    player = get_iface (player_name, 'com.meego.UMMS.MediaPlayer') 

    if (attended) :
        connect_to_need_reply_sig(player)

    return (player, player_name)

def remove_player(proxy):
    global obj_mngr
    obj_path = proxy.object_path
    print "UMMS client lib: Removing media player '%s'" % obj_path
    obj_mngr.RemoveMediaPlayer(obj_path)