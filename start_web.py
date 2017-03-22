#!/usr/bin/python 
#coding=utf-8
from flask import Flask,Response,redirect,request
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

def shutdown_server():
    func = request.environ.get('werkzeug.server.shutdown')
    if func is None:
        raise RuntimeError('Not running with the Werkzeug Server')
    func()

def close_running_threads(_signo, _stack_frame):
    asc.thread_running = False
    print 'signal Audio stream controller to exit\n'
    sleep(5)
    subprocess.call("sh clean.sh",shell = True)
    try:
        shutdown_server()
    except RuntimeError:
        exit(0)


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

@app.route('/frequency',methods=['GET'])
def web_set_frequency():
    freq = float(request.args.get('freq'))
    cutoff = float(request.args.get('bandwidth'))/2
    print 'Request for new frequency: ' + str(freq) + ' '+str(cutoff)
    asc.tb.set_FM_freq(freq)
    asc.tb.set_cutoff_freq(cutoff)
    return ('',200)

if __name__ == '__main__':
    #atexit.register(close_running_threads)
    signal.signal(signal.SIGINT, close_running_threads)
    asc_thread = threading.Thread(target=asc.main)
    asc_thread.start()
    print 'asc_thread started'
    app.run(host='0.0.0.0',port=80)