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
import re
from functools import wraps
from flask import request, Response

# set the project root directory as the static folder, you can set others.
app = Flask(__name__)
app.config["CACHE_TYPE"] = "null"
# turn on audio stream controller
asc = None
config_storage = {}

def shutdown_server():
    func = request.environ.get('werkzeug.server.shutdown')
    if func is None:
        raise RuntimeError('Not running with the Werkzeug Server')
    func()

def close_running_threads(_signo, _stack_frame):
    asc.thread_running = False
    print 'signal Audio stream controller to exit\n'
    sleep(3)
    subprocess.call("sudo sh clean.sh",shell = True)
    subprocess.call(["stty","echo"])#Flask BUG?stty echo not recovery...
    try:
        shutdown_server()
    except RuntimeError:
        exit(0)

def compare_freq_str(x, y):
    pat = r'<freq>(.*)</freq>'
    intx = int(re.match(pat,x).group(1))
    inty = int(re.match(pat,y).group(1))
    return 1 if intx>inty else -1

def check_auth(username, password):
    """This function is called to check if a username /
    password combination is valid.
    """
    return username == 'admin' and password == 'secret'

def authenticate():
    """Sends a 401 response that enables basic auth"""
    return Response(
    'Could not verify your access level for that URL.\n'
    'You have to login with proper credentials', 401,
    {'WWW-Authenticate': 'Basic realm="Login Required"'})

def requires_auth(f):
    @wraps(f)
    def decorated(*args, **kwargs):
        auth = request.authorization
        if not auth or not check_auth(auth.username, auth.password):
            return authenticate()
        return f(*args, **kwargs)
    return decorated

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
@requires_auth
def web_root():
	return redirect("static/index.html")

@app.route('/autosrch/')
@requires_auth
def web_search():
    return redirect("static/search_config.html")

@app.route('/ajax/program_list/')
def web_get_program_list():
    xml_file = open('static/program_list.xml', 'r')
    return Response(xml_file, mimetype='text/xml')

@app.route('/ajax/search_result/')
def web_get_search_result():
    xml_file = open('static/search_result.xml', 'r')
    return Response(xml_file, mimetype='text/xml')

@app.route('/ajax/power2display/')
def web_get_power2display():
    csv_file = open('static/power2display.csv', 'r')
    return Response(csv_file, mimetype='text/plain')

@app.route('/frequency',methods=['GET'])
def web_set_frequency():
    freq = float(request.args.get('freq'))
    cutoff = float(request.args.get('bandwidth'))/2
    if cutoff<1000.0:
        cutoff = float(75000.0)
    asc.set_dsp_command({"freq":[freq,cutoff]})
    return ('',200)

@app.route('/ajax/noise_line/',methods=['GET'])
def web_get_noise_line():
    return Response(config_storage['noise_line'],'text/plain')

@app.route('/startsrch',methods=['POST'])
def web_start_search():
    search_config_form = request.form
    noise_line = search_config_form['noise_line']
    start_freq = search_config_form['start_freq']
    stop_freq = search_config_form['stop_freq']
    config_storage['noise_line'] = noise_line;
    #release RTL dongle
    asc.thread_running = False
    print 'signal Audio stream controller to exit\n'
    sleep(2)
    subprocess.call(["sudo","sh","clean.sh"])
    #call gr-scan to search
    subprocess.call(["sudo","./gr-scan/gr-scan","-x",start_freq,"-y",stop_freq,"-n",noise_line,"-o","static/search_result.xml"])
    #restarting
    asc.thread_running = True
    asc_thread = threading.Thread(target=asc.main)
    asc_thread.start()
    sleep(2)#buffering live stream
    return redirect('static/search_result.html',303)
    
@app.route('/updtsrch',methods=['POST'])
def web_update_search_result():
    search_result = (request.form).values()
    search_result.sort(compare_freq_str)
    f = open('static/program_list.xml','w')
    f.write('<?xml version="1.0" encoding="UTF-8"?>\n<program_list>\n')
    for line in search_result:
        f.write('<program>'+line+'<mcs>FM</mcs>'+'</program>\n')
    f.write('</program_list>')
    f.flush()
    f.close()
    #print search_result
    return ('',200)

if __name__ == '__main__':
    #atexit.register(close_running_threads)
    signal.signal(signal.SIGINT, close_running_threads)
    asc = audio_stream.audio_stream_controller()
    asc_thread = threading.Thread(target=asc.main)
    asc_thread.start()
    print 'asc_thread started'
    app.run(host='0.0.0.0',port=80)