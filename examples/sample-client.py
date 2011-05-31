#!/usr/bin/python

import gobject
import libummsclient

if __name__ == '__main__':

    libummsclient.init()
    (player, name) = libummsclient.request_player(True, 0)

    player.SetUri("file:///root/720p.m4v")
    player.Play()

    loop = gobject.MainLoop()
    loop.run()
    print "Client exit...."

