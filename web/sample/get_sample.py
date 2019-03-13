# encoding: utf-8
import os
import cgi, cgitb 

form = cgi.FieldStorage() 

left_num = int(form.getvalue('left'))
right_num  = int(form.getvalue('right'))

# 正文
context = "<!DOCTYPE html>"
context += "<html>"
context += "<head>"
context += "<meta charset=\"utf-8\">"
context += "<title>Get Result</title>"
context += "</head>"
context += "<body style=\"text-align: center\">"
context += "<h1>Result</h1>"
context += "<hr>"
context += "<h2>" + str(left_num * right_num) + "</h2>"
context += "</body>"

print("Content-type:text/html")
print("Content-length:" + str(len(context)))
print("")
print(context)
