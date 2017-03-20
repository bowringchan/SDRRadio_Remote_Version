#!/usr/bin/python 
#coding=utf-8
from flask import Flask,Response,redirect
from xml.dom.minidom import parse 
import threading
import audio_stream
import atexit
import os
import signal
from time import sleep
import subprocess

# set the project root directory as the static folder, you can set others.
app = Flask(__name__)
app.config["CACHE_TYPE"] = "null"
# turn on audio stream controller
asc = audio_stream.audio_stream_controller()


def close_running_threads(_signo, _stack_frame):
    asc.thread_running = False
    print 'signal Audio stream controller to exit\n'
    sleep(5)
    subprocess.call("sh clean.sh",shell = True)

@app.after_request
def add_header(r):
    """
    Add headers to both force latest IE rendering engine or Chrome Frame,
    and also to cache the rendered page for 10 minutes.
    """
    r.headers["Cache-Control"] = "no-cache, no-store, must-revalidate"
    r.headers["Pragma"] = "no-cache"
    r.headers["Expires"] = "0"
    r.headers['Cache-Control'] = 'public, max-age=0'
    return r

@app.route('/')
def web_root():
	return redirect("static/index.html")

@app.route('/ajax/program_list/')
def web_get_program_list():
    xml_file = open('static/program_list.xml', 'r')
    return Response(xml_file, mimetype='text/xml')

@app.route('/frequency/<float:f>')
def web_set_frequency(f):
	asc.tb.set_FM_freq(f)

@app.route('/cutoff_freq/<float:cutoff_freq>')
def web_set_cutoff_freq(cutoff_freq):
	asc.tb.set_cutoff_freq(cutoff_freq)


if __name__ == '__main__':
    #atexit.register(close_running_threads)
    signal.signal(signal.SIGINT, close_running_threads)
    asc_thread = threading.Thread(target=asc.main)
    asc_thread.start()
    print 'asc_thread started'
    app.run(host='0.0.0.0',port=80)