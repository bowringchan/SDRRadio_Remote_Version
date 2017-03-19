#!/usr/bin/python 
#coding=utf-8
from flask import Flask,Response,redirect
from xml.dom.minidom import parse 
import os.path

# set the project root directory as the static folder, you can set others.
app = Flask(__name__)
app.config["CACHE_TYPE"] = "null"

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

if __name__ == '__main__':
    app.run(host='0.0.0.0',port=80)